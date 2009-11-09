/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ExternalAI/IGlobalAICallback.h"
#include "ExternalAI/IAICallback.h"
#include "ExternalAI/IAICheats.h"
#include "ExternalAI/AICheats.h" // only for: CAICheats::IsPassive()
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/SSkirmishAICallbackImpl.h"
#include "ExternalAI/SkirmishAILibraryInfo.h"
#include "ExternalAI/SAIInterfaceCallbackImpl.h"
#include "ExternalAI/SkirmishAIHandler.h"
//#include "ExternalAI/EngineOutHandler.h"
#include "ExternalAI/Interface/AISCommands.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Units/Groups/GroupHandler.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Misc/Resource.h"
#include "Sim/Misc/ResourceHandler.h"
#include "Sim/Misc/ResourceMapAnalyzer.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/QuadField.h" // for qf->GetFeaturesExact(pos, radius)
#include "Map/ReadMap.h"
#include "Map/MetalMap.h"
#include "Game/SelectedUnits.h"
#include "Game/UI/GuiHandler.h" //TODO: fix some switch for new gui
#include "Game/GameVersion.h"
#include "Game/GameSetup.h"
#include "GlobalUnsynced.h" // for myTeam
#include "LogOutput.h"


static const char* SKIRMISH_AIS_VERSION_COMMON = "common";

static std::map<int, IGlobalAICallback*> team_globalCallback;
static std::map<int, IAICallback*> team_callback;
static std::map<int, SSkirmishAICallback*> team_cCallback;
static std::map<int, bool> team_cheatingEnabled;
static std::map<int, IAICheats*> team_cheatCallback;

static const size_t TMP_ARR_SIZE = 16384;
// FIXME: the following lines are relatively memory intensive (~1MB per line)
// this memory is only freed at exit of the application
// There is quite some CPU and Memory performance waste present
// in them and their use.
static int tmpIntArr[MAX_SKIRMISH_AIS][TMP_ARR_SIZE];
static PointMarker tmpPointMarkerArr[MAX_SKIRMISH_AIS][TMP_ARR_SIZE];
static LineMarker tmpLineMarkerArr[MAX_SKIRMISH_AIS][TMP_ARR_SIZE];

static void checkTeamId(int teamId) {

	if ((teamId < 0) || (team_cCallback.find(static_cast<size_t>(teamId)) == team_cCallback.end())) {
		const static size_t teamIdError_maxSize = 512;
		char teamIdError[teamIdError_maxSize];
		SNPRINTF(teamIdError, teamIdError_maxSize,
				"Bad team ID supplied by a Skirmish AI.\n"
				"Is %i, but should be between min %i and max %u.",
				teamId, 0, MAX_SKIRMISH_AIS);
		// log exception to the engine and die
		skirmishAiCallback_Log_exception(teamId, teamIdError, 1, true);
	}
}

static int fillCMap(const std::map<std::string,std::string>* map,
		const char* cMapKeys[], const char* cMapValues[]) {
	std::map<std::string,std::string>::const_iterator it;
	int i;
	for (i=0, it=map->begin(); it != map->end(); ++i, it++) {
		cMapKeys[i] = it->first.c_str();
		cMapValues[i] = it->second.c_str();
	}
	return i;
}

static inline int min(int val1, int val2) {
	return val1 < val2 ? val1 : val2;
}
static inline int max(int val1, int val2) {
	return val1 > val2 ? val1 : val2;
}

static void toFloatArr(const short color[3], float alpha, float arrColor[4]) {
	arrColor[0] = color[0];
	arrColor[1] = color[1];
	arrColor[2] = color[2];
	arrColor[3] = alpha;
}
static void fillVector(std::vector<int>* vector_unitIds, int* unitIds,
		int numUnitIds) {
	for (int i=0; i < numUnitIds; ++i) {
		vector_unitIds->push_back(unitIds[i]);
	}
}

static bool isControlledByLocalPlayer(int teamId) {
	return gu->myTeam == teamId;
}


static const CUnit* getUnit(int unitId) {

	if (unitId < MAX_UNITS) {
		return uh->units[unitId];
	} else {
		return NULL;
	}
}
static bool isAlliedUnit(int teamId, const CUnit* unit) {
	return teamHandler->AlliedTeams(unit->team, teamId);
}

static const UnitDef* getUnitDefById(int teamId, int unitDefId) {
	AIHCGetUnitDefById cmd = {unitDefId, NULL};
	const int ret = team_callback[teamId]->HandleCommand(AIHCGetUnitDefByIdId, &cmd);
	if (ret == 1) {
		return cmd.ret;
	} else {
		return NULL;
	}
}
static const WeaponDef* getWeaponDefById(int teamId, int weaponDefId) {
	AIHCGetWeaponDefById cmd = {weaponDefId, NULL};
	int ret = team_callback[teamId]->HandleCommand(AIHCGetWeaponDefByIdId, &cmd);
	if (ret == 1) {
		return cmd.ret;
	} else {
		return NULL;
	}
}
static const FeatureDef* getFeatureDefById(int teamId, int featureDefId) {
	AIHCGetFeatureDefById cmd = {featureDefId, NULL};
	const int ret = team_callback[teamId]->HandleCommand(AIHCGetFeatureDefByIdId, &cmd);
	if (ret == 1) {
		return cmd.ret;
	} else {
		return NULL;
	}
}

// TODO: FIXME: this function should not be needed anymore, after a clean move from teamId to skirmishAIId
static inline size_t getFirstSkirmishAIIdForTeam(int teamId) {

	const CSkirmishAIHandler::ids_t skirmishAIIds = skirmishAIHandler.GetSkirmishAIsInTeam(teamId);
	assert(skirmishAIIds.size() > 0);
	return skirmishAIIds[0];
}
// TODO: FIXME: this function should not be needed anymore, after a clean move from teamId to skirmishAIId
static inline const SkirmishAIData* getFirstSkirmishAIDataForTeam(int teamId) {
	return skirmishAIHandler.GetSkirmishAI(getFirstSkirmishAIIdForTeam(teamId));
}

static int wrapper_HandleCommand(IAICallback* clb, IAICheats* clbCheat,
		int cmdId, void* cmdData) {

	int ret;

	if (clbCheat != NULL) {
		ret = clbCheat->HandleCommand(cmdId, cmdData);
	} else {
		ret = clb->HandleCommand(cmdId, cmdData);
	}

	return ret;
}
EXPORT(int) skirmishAiCallback_Engine_handleCommand(int teamId, int toId, int commandId,
		int commandTopic, void* commandData) {

	int ret = 0;

	IAICallback* clb    = team_callback[teamId];
	// if this is != NULL, cheating is enabled
	IAICheats* clbCheat = team_cheatCallback[teamId];


	switch (commandTopic) {

		case COMMAND_CHEATS_SET_MY_HANDICAP:
		{
			const SSetMyHandicapCheatCommand* cmd =
					(SSetMyHandicapCheatCommand*) commandData;
			if (clbCheat != NULL) {
				clbCheat->SetMyHandicap(cmd->handicap);
				ret = 0;
			} else {
				ret = -1;
			}
			break;
		}
		case COMMAND_CHEATS_GIVE_ME_RESOURCE:
		{
			const SGiveMeResourceCheatCommand* cmd =
					(SGiveMeResourceCheatCommand*) commandData;
			if (clbCheat != NULL) {
				if (cmd->resourceId == resourceHandler->GetMetalId()) {
					clbCheat->GiveMeMetal(cmd->amount);
					ret = 0;
				} else if (cmd->resourceId == resourceHandler->GetEnergyId()) {
					clbCheat->GiveMeEnergy(cmd->amount);
					ret = 0;
				} else {
					ret = -2;
				}
			} else {
				ret = -1;
			}
			break;
		}
		case COMMAND_CHEATS_GIVE_ME_NEW_UNIT:
		{
			SGiveMeNewUnitCheatCommand* cmd =
					(SGiveMeNewUnitCheatCommand*) commandData;
			if (clbCheat != NULL) {
				cmd->ret_newUnitId = clbCheat->CreateUnit(getUnitDefById(teamId,
						cmd->unitDefId)->name.c_str(), cmd->pos_posF3);
				if (cmd->ret_newUnitId > 0) {
					ret = 0;
				} else {
					ret = -2;
				}
			} else {
				ret = -1;
				cmd->ret_newUnitId = -1;
			}
			break;
		}


		case COMMAND_SEND_START_POS:
		{
			const SSendStartPosCommand* cmd = (SSendStartPosCommand*) commandData;
			AIHCSendStartPos data = {cmd->ready, cmd->pos_posF3};
			wrapper_HandleCommand(clb, clbCheat, AIHCSendStartPosId, &data);
			break;
		}
		case COMMAND_DRAWER_POINT_ADD:
		{
			const SAddPointDrawCommand* cmd = (SAddPointDrawCommand*) commandData;
			AIHCAddMapPoint data = {cmd->pos_posF3, cmd->label};
			wrapper_HandleCommand(clb, clbCheat, AIHCAddMapPointId, &data);
			break;
		}
		case COMMAND_DRAWER_POINT_REMOVE:
		{
			const SRemovePointDrawCommand* cmd =
					(SRemovePointDrawCommand*) commandData;
			AIHCRemoveMapPoint data = {cmd->pos_posF3};
			wrapper_HandleCommand(clb, clbCheat, AIHCRemoveMapPointId, &data);
			break;
		}
		case COMMAND_DRAWER_LINE_ADD:
		{
			const SAddLineDrawCommand* cmd = (SAddLineDrawCommand*) commandData;
			AIHCAddMapLine data = {cmd->posFrom_posF3, cmd->posTo_posF3};
			wrapper_HandleCommand(clb, clbCheat, AIHCAddMapLineId, &data);
			break;
		}


		case COMMAND_SEND_TEXT_MESSAGE:
		{
			const SSendTextMessageCommand* cmd =
					(SSendTextMessageCommand*) commandData;
			clb->SendTextMsg(cmd->text, cmd->zone);
			break;
		}
		case COMMAND_SET_LAST_POS_MESSAGE:
		{
			const SSetLastPosMessageCommand* cmd =
					(SSetLastPosMessageCommand*) commandData;
			clb->SetLastMsgPos(cmd->pos_posF3);
			break;
		}
		case COMMAND_SEND_RESOURCES:
		{
			SSendResourcesCommand* cmd = (SSendResourcesCommand*) commandData;
			if (cmd->resourceId == resourceHandler->GetMetalId()) {
				cmd->ret_isExecuted = clb->SendResources(cmd->amount, 0, cmd->receivingTeamId);
				ret = -2;
			} else if (cmd->resourceId == resourceHandler->GetEnergyId()) {
				cmd->ret_isExecuted = clb->SendResources(0, cmd->amount, cmd->receivingTeamId);
				ret = -3;
			} else {
				cmd->ret_isExecuted = false;
				ret = -4;
			}
			break;
		}

		case COMMAND_SEND_UNITS:
		{
			SSendUnitsCommand* cmd = (SSendUnitsCommand*) commandData;
			std::vector<int> vector_unitIds;
			fillVector(&vector_unitIds, cmd->unitIds, cmd->unitIds_size);
			cmd->ret_sentUnits =
					clb->SendUnits(vector_unitIds, cmd->receivingTeamId);
			break;
		}

		case COMMAND_GROUP_CREATE:
		{
			SCreateGroupCommand* cmd = (SCreateGroupCommand*) commandData;
			cmd->ret_groupId = clb->CreateGroup();
			break;
		}
		case COMMAND_GROUP_ERASE:
		{
			const SEraseGroupCommand* cmd = (SEraseGroupCommand*) commandData;
			clb->EraseGroup(cmd->groupId);
			break;
		}
/*
		case COMMAND_GROUP_ADD_UNIT:
		{
			SAddUnitToGroupCommand* cmd = (SAddUnitToGroupCommand*) commandData;
			cmd->ret_isExecuted =
					clb->AddUnitToGroup(cmd->unitId, cmd->groupId);
			break;
		}
		case COMMAND_GROUP_REMOVE_UNIT:
		{
			SRemoveUnitFromGroupCommand* cmd =
					(SRemoveUnitFromGroupCommand*) commandData;
			cmd->ret_isExecuted = clb->RemoveUnitFromGroup(cmd->unitId);
			break;
		}
*/
		case COMMAND_PATH_INIT:
		{
			SInitPathCommand* cmd = (SInitPathCommand*) commandData;
			cmd->ret_pathId = clb->InitPath(cmd->start_posF3,
					cmd->end_posF3, cmd->pathType);
			break;
		}
		case COMMAND_PATH_GET_APPROXIMATE_LENGTH:
		{
			SGetApproximateLengthPathCommand* cmd =
					(SGetApproximateLengthPathCommand*) commandData;
			cmd->ret_approximatePathLength =
					clb->GetPathLength(cmd->start_posF3, cmd->end_posF3,
							cmd->pathType);
			break;
		}
		case COMMAND_PATH_GET_NEXT_WAYPOINT:
		{
			SGetNextWaypointPathCommand* cmd =
					(SGetNextWaypointPathCommand*) commandData;
			clb->GetNextWaypoint(cmd->pathId).copyInto(cmd->ret_nextWaypoint_posF3_out);
			break;
		}
		case COMMAND_PATH_FREE:
		{
			const SFreePathCommand* cmd = (SFreePathCommand*) commandData;
			clb->FreePath(cmd->pathId);
			break;
		}
		case COMMAND_CALL_LUA_RULES:
		{
			SCallLuaRulesCommand* cmd = (SCallLuaRulesCommand*) commandData;
			cmd->ret_outData = clb->CallLuaRules(cmd->data, cmd->inSize,
					cmd->outSize);
			break;
		}


		case COMMAND_DRAWER_ADD_NOTIFICATION:
		{
			const SAddNotificationDrawerCommand* cmd =
					(SAddNotificationDrawerCommand*) commandData;
			clb->AddNotification(cmd->pos_posF3,
					float3(
						cmd->color_colorS3[0],
						cmd->color_colorS3[1],
						cmd->color_colorS3[2]),
					cmd->alpha);
			break;
		}
		case COMMAND_DRAWER_PATH_START:
		{
			const SStartPathDrawerCommand* cmd =
					(SStartPathDrawerCommand*) commandData;
			float arrColor[4];
			toFloatArr(cmd->color_colorS3, cmd->alpha, arrColor);
			clb->LineDrawerStartPath(cmd->pos_posF3, arrColor);
			break;
		}
		case COMMAND_DRAWER_PATH_FINISH:
		{
			//const SFinishPathDrawerCommand* cmd =
			//		(SFinishPathDrawerCommand*) commandData;
			clb->LineDrawerFinishPath();
			break;
		}
		case COMMAND_DRAWER_PATH_DRAW_LINE:
		{
			const SDrawLinePathDrawerCommand* cmd =
					(SDrawLinePathDrawerCommand*) commandData;
			float arrColor[4];
			toFloatArr(cmd->color_colorS3, cmd->alpha, arrColor);
			clb->LineDrawerDrawLine(cmd->endPos_posF3, arrColor);
			break;
		}
		case COMMAND_DRAWER_PATH_DRAW_LINE_AND_ICON:
		{
			const SDrawLineAndIconPathDrawerCommand* cmd =
					(SDrawLineAndIconPathDrawerCommand*) commandData;
			float arrColor[4];
			toFloatArr(cmd->color_colorS3, cmd->alpha, arrColor);
			clb->LineDrawerDrawLineAndIcon(cmd->cmdId, cmd->endPos_posF3,
					arrColor);
			break;
		}
		case COMMAND_DRAWER_PATH_DRAW_ICON_AT_LAST_POS:
		{
			const SDrawIconAtLastPosPathDrawerCommand* cmd =
					(SDrawIconAtLastPosPathDrawerCommand*) commandData;
			clb->LineDrawerDrawIconAtLastPos(cmd->cmdId);
			break;
		}
		case COMMAND_DRAWER_PATH_BREAK:
		{
			const SBreakPathDrawerCommand* cmd =
					(SBreakPathDrawerCommand*) commandData;
			float arrColor[4];
			toFloatArr(cmd->color_colorS3, cmd->alpha, arrColor);
			clb->LineDrawerBreak(cmd->endPos_posF3, arrColor);
			break;
		}
		case COMMAND_DRAWER_PATH_RESTART:
		{
			const SRestartPathDrawerCommand* cmd =
					(SRestartPathDrawerCommand*) commandData;
			if (cmd->sameColor) {
				clb->LineDrawerRestartSameColor();
			} else {
				clb->LineDrawerRestart();
			}
			break;
		}
		case COMMAND_DRAWER_FIGURE_CREATE_SPLINE:
		{
			SCreateSplineFigureDrawerCommand* cmd =
					(SCreateSplineFigureDrawerCommand*) commandData;
			cmd->ret_newFigureGroupId =
					clb->CreateSplineFigure(cmd->pos1_posF3, cmd->pos2_posF3,
							cmd->pos3_posF3, cmd->pos4_posF3, cmd->width,
							cmd->arrow, cmd->lifeTime, cmd->figureGroupId);
			break;
		}
		case COMMAND_DRAWER_FIGURE_CREATE_LINE:
		{
			SCreateLineFigureDrawerCommand* cmd =
					(SCreateLineFigureDrawerCommand*) commandData;
			cmd->ret_newFigureGroupId = clb->CreateLineFigure(cmd->pos1_posF3,
					cmd->pos2_posF3, cmd->width, cmd->arrow, cmd->lifeTime,
					cmd->figureGroupId);
			break;
		}
		case COMMAND_DRAWER_FIGURE_SET_COLOR:
		{
			const SSetColorFigureDrawerCommand* cmd =
					(SSetColorFigureDrawerCommand*) commandData;
			clb->SetFigureColor(cmd->figureGroupId,
					cmd->color_colorS3[0],
					cmd->color_colorS3[1],
					cmd->color_colorS3[2],
					cmd->alpha);
			break;
		}
		case COMMAND_DRAWER_FIGURE_DELETE:
		{
			const SDeleteFigureDrawerCommand* cmd =
					(SDeleteFigureDrawerCommand*) commandData;
			clb->DeleteFigureGroup(cmd->figureGroupId);
			break;
		}
		case COMMAND_DRAWER_DRAW_UNIT:
		{
			const SDrawUnitDrawerCommand* cmd = (SDrawUnitDrawerCommand*) commandData;
			clb->DrawUnit(getUnitDefById(teamId,
					cmd->toDrawUnitDefId)->name.c_str(), cmd->pos_posF3,
					cmd->rotation, cmd->lifeTime, cmd->teamId, cmd->transparent,
					cmd->drawBorder, cmd->facing);
			break;
		}
		case COMMAND_TRACE_RAY: {
			STraceRayCommand* cCmdData = (STraceRayCommand*) commandData;
			AIHCTraceRay cppCmdData = {
				cCmdData->rayPos_posF3,
				cCmdData->rayDir_posF3,
				cCmdData->rayLen,
				cCmdData->srcUID,
				cCmdData->hitUID,
				cCmdData->flags
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCTraceRayId, &cppCmdData);

			cCmdData->rayLen = cppCmdData.rayLen;
			cCmdData->hitUID = cppCmdData.hitUID;
			break;
		}
		case COMMAND_PAUSE: {
			const SPauseCommand* cmd = (SPauseCommand*) commandData;
			AIHCPause cppCmdData = {
				cmd->enable,
				cmd->reason
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCPauseId, &cppCmdData);
			break;
		}

		default:
		{
			// check if it is a unit command
			Command* c = (Command*) newCommand(commandData, commandTopic);
			if (c != NULL) { // it is a unit command
				const SStopUnitCommand* cmd = (SStopUnitCommand*) commandData;
				if (cmd->unitId >= 0) {
					ret = clb->GiveOrder(cmd->unitId, c);
				} else {
					ret = clb->GiveGroupOrder(cmd->groupId, c);
				}
				delete c;
				c = NULL;
			} else { // it is no known command
				ret = -1;
			}
		}

	}

	return ret;
}


EXPORT(const char*) skirmishAiCallback_Engine_Version_getMajor(int teamId) {
	return aiInterfaceCallback_Engine_Version_getMajor(-1);
}
EXPORT(const char*) skirmishAiCallback_Engine_Version_getMinor(int teamId) {
	return aiInterfaceCallback_Engine_Version_getMinor(-1);
}
EXPORT(const char*) skirmishAiCallback_Engine_Version_getPatchset(int teamId) {
	return aiInterfaceCallback_Engine_Version_getPatchset(-1);
}
EXPORT(const char*) skirmishAiCallback_Engine_Version_getAdditional(int teamId) {
	return aiInterfaceCallback_Engine_Version_getAdditional(-1);
}
EXPORT(const char*) skirmishAiCallback_Engine_Version_getBuildTime(int teamId) {
	return aiInterfaceCallback_Engine_Version_getBuildTime(-1);
}
EXPORT(const char*) skirmishAiCallback_Engine_Version_getNormal(int teamId) {
	return aiInterfaceCallback_Engine_Version_getNormal(-1);
}
EXPORT(const char*) skirmishAiCallback_Engine_Version_getFull(int teamId) {
	return aiInterfaceCallback_Engine_Version_getFull(-1);
}

EXPORT(int) skirmishAiCallback_Teams_getSize(int teamId) {
	return teamHandler->ActiveTeams();
}

EXPORT(int) skirmishAiCallback_SkirmishAIs_getSize(int teamId) {
	return aiInterfaceCallback_SkirmishAIs_getSize(-1);
}
EXPORT(int) skirmishAiCallback_SkirmishAIs_getMax(int teamId) {
	return aiInterfaceCallback_SkirmishAIs_getMax(-1);
}

static inline const CSkirmishAILibraryInfo* getSkirmishAILibraryInfo(int teamId) {

	const CSkirmishAILibraryInfo* info = NULL;

	const SkirmishAIKey* key = skirmishAIHandler.GetLocalSkirmishAILibraryKey(getFirstSkirmishAIIdForTeam(teamId));
	assert(key != NULL);
	const IAILibraryManager* libMan = IAILibraryManager::GetInstance();
	IAILibraryManager::T_skirmishAIInfos infs = libMan->GetSkirmishAIInfos();
	IAILibraryManager::T_skirmishAIInfos::const_iterator inf = infs.find(*key);
	if (inf != infs.end()) {
		info = (const CSkirmishAILibraryInfo*) inf->second;
	}

	return info;
}

EXPORT(int) skirmishAiCallback_SkirmishAI_Info_getSize(int teamId) {

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(teamId);
	return (int)info->size();
}
EXPORT(const char*) skirmishAiCallback_SkirmishAI_Info_getKey(int teamId, int infoIndex) {

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(teamId);
	return info->GetKeyAt(infoIndex).c_str();
}
EXPORT(const char*) skirmishAiCallback_SkirmishAI_Info_getValue(int teamId, int infoIndex) {

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(teamId);
	return info->GetValueAt(infoIndex).c_str();
}
EXPORT(const char*) skirmishAiCallback_SkirmishAI_Info_getDescription(int teamId, int infoIndex) {

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(teamId);
	return info->GetDescriptionAt(infoIndex).c_str();
}
EXPORT(const char*) skirmishAiCallback_SkirmishAI_Info_getValueByKey(int teamId, const char* const key) {

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(teamId);
	return info->GetInfo(key).c_str();
}

static inline bool checkOptionIndex(int optionIndex, const std::vector<std::string>& optionKeys) {
	return ((optionIndex < 0) || ((size_t)optionIndex >= optionKeys.size()));
}
EXPORT(int) skirmishAiCallback_SkirmishAI_OptionValues_getSize(int teamId) {
	return (int)getFirstSkirmishAIDataForTeam(teamId)->options.size();
}
EXPORT(const char*) skirmishAiCallback_SkirmishAI_OptionValues_getKey(int teamId, int optionIndex) {

	const std::vector<std::string>& optionKeys = getFirstSkirmishAIDataForTeam(teamId)->optionKeys;
	if (checkOptionIndex(optionIndex, optionKeys)) {
		return NULL;
	} else {
		const std::string& key = *(optionKeys.begin() + optionIndex);
		return key.c_str();
	}
}
EXPORT(const char*) skirmishAiCallback_SkirmishAI_OptionValues_getValue(int teamId, int optionIndex) {

	const std::vector<std::string>& optionKeys = getFirstSkirmishAIDataForTeam(teamId)->optionKeys;
	if (checkOptionIndex(optionIndex, optionKeys)) {
		return NULL;
	} else {
		const std::map<std::string, std::string>& options = getFirstSkirmishAIDataForTeam(teamId)->options;
		const std::string& key = *(optionKeys.begin() + optionIndex);
		const std::map<std::string, std::string>::const_iterator op = options.find(key);
		if (op == options.end()) {
			return NULL;
		} else {
			return op->second.c_str();
		}
	}
}
EXPORT(const char*) skirmishAiCallback_SkirmishAI_OptionValues_getValueByKey(int teamId, const char* const key) {

	const std::map<std::string, std::string>& options = getFirstSkirmishAIDataForTeam(teamId)->options;
	const std::map<std::string, std::string>::const_iterator op = options.find(key);
	if (op == options.end()) {
		return NULL;
	} else {
		return op->second.c_str();
	}
}


EXPORT(void) skirmishAiCallback_Log_log(int teamId, const char* const msg) {

	checkTeamId(teamId);

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(teamId);
	logOutput.Print("Skirmish AI <%s-%s>: %s", info->GetName().c_str(), info->GetVersion().c_str(), msg);
}
EXPORT(void) skirmishAiCallback_Log_exception(int teamId, const char* const msg, int severety, bool die) {

	checkTeamId(teamId);

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(teamId);
	logOutput.Print("Skirmish AI <%s-%s>: error, severety %i: [%s] %s",
			info->GetName().c_str(), info->GetVersion().c_str(), severety,
			(die ? "AI shutting down" : "AI still running"), msg);
	if (die) {
		const size_t skirmishAIId = getFirstSkirmishAIIdForTeam(teamId);
		skirmishAIHandler.SetLocalSkirmishAIDieing(skirmishAIId, 4 /* = AI crashed */);
	}
}

EXPORT(char) skirmishAiCallback_DataDirs_getPathSeparator(int UNUSED_teamId) {
	return aiInterfaceCallback_DataDirs_getPathSeparator(-1);
}
EXPORT(int) skirmishAiCallback_DataDirs_Roots_getSize(int UNUSED_teamId) {
	return aiInterfaceCallback_DataDirs_Roots_getSize(-1);
}
EXPORT(bool) skirmishAiCallback_DataDirs_Roots_getDir(int UNUSED_teamId, char* path, int path_sizeMax, int dirIndex) {
	return aiInterfaceCallback_DataDirs_Roots_getDir(-1, path, path_sizeMax, dirIndex);
}
EXPORT(bool) skirmishAiCallback_DataDirs_Roots_locatePath(int UNUSED_teamId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir) {
	return aiInterfaceCallback_DataDirs_Roots_locatePath(-1, path, path_sizeMax, relPath, writeable, create, dir);
}
EXPORT(char*) skirmishAiCallback_DataDirs_Roots_allocatePath(int UNUSED_teamId, const char* const relPath, bool writeable, bool create, bool dir) {
	return aiInterfaceCallback_DataDirs_Roots_allocatePath(-1, relPath, writeable, create, dir);
}
EXPORT(const char*) skirmishAiCallback_DataDirs_getConfigDir(int teamId) {

	checkTeamId(teamId);

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(teamId);
	return info->GetDataDir().c_str();
}
EXPORT(bool) skirmishAiCallback_DataDirs_locatePath(int teamId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir, bool common) {

	bool exists = false;

	const char ps = skirmishAiCallback_DataDirs_getPathSeparator(teamId);
	const std::string aiShortName = skirmishAiCallback_SkirmishAI_Info_getValueByKey(teamId, SKIRMISH_AI_PROPERTY_SHORT_NAME);
	std::string aiVersion;
	if (common) {
		aiVersion = SKIRMISH_AIS_VERSION_COMMON;
	} else {
		aiVersion = skirmishAiCallback_SkirmishAI_Info_getValueByKey(teamId, SKIRMISH_AI_PROPERTY_VERSION);
	}
	std::string aiRelPath(SKIRMISH_AI_DATA_DIR);
	aiRelPath += ps + aiShortName + ps + aiVersion + ps + relPath;

	exists = skirmishAiCallback_DataDirs_Roots_locatePath(teamId, path, path_sizeMax, aiRelPath.c_str(), writeable, create, dir);

	return exists;
}
EXPORT(char*) skirmishAiCallback_DataDirs_allocatePath(int teamId, const char* const relPath, bool writeable, bool create, bool dir, bool common) {

	static const unsigned int path_sizeMax = 2048;

	char* path = (char*) calloc(path_sizeMax, sizeof(char*));
	const bool fetchOk = skirmishAiCallback_DataDirs_locatePath(teamId, path, path_sizeMax, relPath, writeable, create, dir, common);

	if (!fetchOk) {
		FREE(path);
	}

	return path;
}
static std::vector<std::string> writeableDataDirs;
EXPORT(const char*) skirmishAiCallback_DataDirs_getWriteableDir(int teamId) {

	checkTeamId(teamId);

	// fill up writeableDataDirs until teamId index is in there
	// if it is not yet
	size_t wdd;
	for (wdd=writeableDataDirs.size(); wdd <= (size_t)teamId; ++wdd) {
		writeableDataDirs.push_back("");
	}
	if (writeableDataDirs[teamId].empty()) {
		static const unsigned int sizeMax = 1024;
		char tmpRes[sizeMax];
		static const char* const rootPath = "";
		const bool exists = skirmishAiCallback_DataDirs_locatePath(teamId,
				tmpRes, sizeMax, rootPath, true, true, true, false);
		writeableDataDirs[teamId] = tmpRes;
		if (!exists) {
			char errorMsg[sizeMax];
			SNPRINTF(errorMsg, sizeMax,
					"Unable to create writable data-dir for interface %i: %s",
					teamId, tmpRes);
			skirmishAiCallback_Log_exception(teamId, errorMsg, 1, true);
			return NULL;
		}
	}

	return writeableDataDirs[teamId].c_str();
}


//##############################################################################
EXPORT(bool) skirmishAiCallback_Cheats_isEnabled(int teamId) {
	team_cheatCallback[teamId] = NULL;
	if (team_cheatingEnabled[teamId]) {
		team_cheatCallback[teamId] =
				team_globalCallback[teamId]->GetCheatInterface();
	}
	return team_cheatCallback[teamId] != NULL;
}

EXPORT(bool) skirmishAiCallback_Cheats_setEnabled(int teamId, bool enabled) {
//	bool isEnabled = _Cheats_isEnabled(teamId);
//
//	if (enabled != isEnabled) {
//		if (enable) {
//			team_cheatCallback[teamId] =
//					team_globalCallback[teamId]->GetCheatInterface();
//		} else {
//			team_cheatCallback[teamId] = NULL;
//		}
//	} else {
//		return true;
//	}
//
//	isEnabled = _Cheats_isEnabled(teamId);
//	return enabled != isEnabled;

	team_cheatingEnabled[teamId] = enabled;
	return enabled == skirmishAiCallback_Cheats_isEnabled(teamId);
}

EXPORT(bool) skirmishAiCallback_Cheats_setEventsEnabled(int teamId, bool enabled) {

	bool success = false;

	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		team_cheatCallback[teamId]->EnableCheatEvents(enabled);
		success = true;
	}

	return success;
}

EXPORT(bool) skirmishAiCallback_Cheats_isOnlyPassive(int teamId) {
	return CAICheats::IsPassive();
}

EXPORT(int) skirmishAiCallback_Game_getAiInterfaceVersion(int teamId) {
	IAICallback* clb = team_callback[teamId];
	return wrapper_HandleCommand(clb, NULL, AIHCQuerySubVersionId, NULL);
}

EXPORT(int) skirmishAiCallback_Game_getCurrentFrame(int teamId) {
	return team_callback[teamId]->GetCurrentFrame();
}

EXPORT(int) skirmishAiCallback_Game_getMyTeam(int teamId) {
	return team_callback[teamId]->GetMyTeam();
}

EXPORT(int) skirmishAiCallback_Game_getMyAllyTeam(int teamId) {
	return team_callback[teamId]->GetMyAllyTeam();
}

EXPORT(int) skirmishAiCallback_Game_getPlayerTeam(int teamId, int player) {
	return team_callback[teamId]->GetPlayerTeam(player);
}

EXPORT(const char*) skirmishAiCallback_Game_getTeamSide(int teamId, int team) {
	return team_callback[teamId]->GetTeamSide(team);
}





EXPORT(int) skirmishAiCallback_WeaponDef_getNumDamageTypes(int teamId) {

	int numDamageTypes;

	const bool fetchOk = team_callback[teamId]->GetValue(AIVAL_NUMDAMAGETYPES, &numDamageTypes);
	if (!fetchOk) {
		numDamageTypes = -1;
	}

	return numDamageTypes;
}

EXPORT(bool) skirmishAiCallback_Game_isExceptionHandlingEnabled(int teamId) {

	bool exceptionHandlingEnabled;

	IAICallback* clb = team_callback[teamId];
	const bool fetchOk = clb->GetValue(AIVAL_EXCEPTION_HANDLING,
			&exceptionHandlingEnabled);
	if (!fetchOk) {
		exceptionHandlingEnabled = false;
	}

	return exceptionHandlingEnabled;
}
EXPORT(bool) skirmishAiCallback_Game_isDebugModeEnabled(int teamId) {

	bool debugModeEnabled;

	IAICallback* clb = team_callback[teamId];
	const bool fetchOk = clb->GetValue(AIVAL_DEBUG_MODE, &debugModeEnabled);
	if (!fetchOk) {
		debugModeEnabled = false;
	}

	return debugModeEnabled;
}
EXPORT(int) skirmishAiCallback_Game_getMode(int teamId) {
	int mode;
	IAICallback* clb = team_callback[teamId];
	const bool fetchOk = clb->GetValue(AIVAL_GAME_MODE, &mode);
	if (!fetchOk) {
		mode = -1;
	}
	return mode;
}
EXPORT(bool) skirmishAiCallback_Game_isPaused(int teamId) {
	bool paused;
	IAICallback* clb = team_callback[teamId];
	const bool fetchOk = clb->GetValue(AIVAL_GAME_PAUSED, &paused);
	if (!fetchOk) {
		paused = false;
	}
	return paused;
}
EXPORT(float) skirmishAiCallback_Game_getSpeedFactor(int teamId) {
	float speedFactor;
	IAICallback* clb = team_callback[teamId];
	const bool fetchOk = clb->GetValue(AIVAL_GAME_SPEED_FACTOR, &speedFactor);
	if (!fetchOk) {
		speedFactor = false;
	}
	return speedFactor;
}

EXPORT(float) skirmishAiCallback_Gui_getViewRange(int teamId) {
	float viewRange;
	IAICallback* clb = team_callback[teamId];
	const bool fetchOk = clb->GetValue(AIVAL_GUI_VIEW_RANGE, &viewRange);
	if (!fetchOk) {
		viewRange = false;
	}
	return viewRange;
}
EXPORT(float) skirmishAiCallback_Gui_getScreenX(int teamId) {
	float screenX;
	IAICallback* clb = team_callback[teamId];
	const bool fetchOk = clb->GetValue(AIVAL_GUI_SCREENX, &screenX);
	if (!fetchOk) {
		screenX = false;
	}
	return screenX;
}
EXPORT(float) skirmishAiCallback_Gui_getScreenY(int teamId) {
	float screenY;
	IAICallback* clb = team_callback[teamId];
	const bool fetchOk = clb->GetValue(AIVAL_GUI_SCREENY, &screenY);
	if (!fetchOk) {
		screenY = false;
	}
	return screenY;
}
EXPORT(void) skirmishAiCallback_Gui_Camera_getDirection(int teamId, float* return_posF3_out) {

	float3 cameraDir;
	IAICallback* clb = team_callback[teamId];
	const bool fetchOk = clb->GetValue(AIVAL_GUI_CAMERA_DIR, &cameraDir);
	if (!fetchOk) {
		cameraDir = float3(1.0f, 0.0f, 0.0f);
	}
	cameraDir.copyInto(return_posF3_out);
}
EXPORT(void) skirmishAiCallback_Gui_Camera_getPosition(int teamId, float* return_posF3_out) {

	float3 cameraPosition;
	IAICallback* clb = team_callback[teamId];
	const bool fetchOk = clb->GetValue(AIVAL_GUI_CAMERA_POS, &cameraPosition);
	if (!fetchOk) {
		cameraPosition = float3(1.0f, 0.0f, 0.0f);
	}
	cameraPosition.copyInto(return_posF3_out);
}

// EXPORT(bool) skirmishAiCallback_File_locateForReading(int teamId, char* fileName, int fileName_sizeMax) {
// 	IAICallback* clb = team_callback[teamId];
// 	return clb->GetValue(AIVAL_LOCATE_FILE_R, fileName, fileName_sizeMax);
// }
// EXPORT(bool) skirmishAiCallback_File_locateForWriting(int teamId, char* fileName, int fileName_sizeMax) {
// 	IAICallback* clb = team_callback[teamId];
// 	return clb->GetValue(AIVAL_LOCATE_FILE_W, fileName, fileName_sizeMax);
// }




//########### BEGINN Mod

EXPORT(const char*) skirmishAiCallback_Mod_getFileName(int teamId) {
	return modInfo.filename.c_str();
}

EXPORT(const char*) skirmishAiCallback_Mod_getHumanName(int teamId) {
	return modInfo.humanName.c_str();
}
EXPORT(const char*) skirmishAiCallback_Mod_getShortName(int teamId) {
	return modInfo.shortName.c_str();
}
EXPORT(const char*) skirmishAiCallback_Mod_getVersion(int teamId) {
	return modInfo.version.c_str();
}
EXPORT(const char*) skirmishAiCallback_Mod_getMutator(int teamId) {
	return modInfo.mutator.c_str();
}
EXPORT(const char*) skirmishAiCallback_Mod_getDescription(int teamId) {
	return modInfo.description.c_str();
}

EXPORT(bool) skirmishAiCallback_Mod_getAllowTeamColors(int teamId) {
	return modInfo.allowTeamColors;
}

EXPORT(bool) skirmishAiCallback_Mod_getConstructionDecay(int teamId) {
	return modInfo.constructionDecay;
}
EXPORT(int) skirmishAiCallback_Mod_getConstructionDecayTime(int teamId) {
	return modInfo.constructionDecayTime;
}
EXPORT(float) skirmishAiCallback_Mod_getConstructionDecaySpeed(int teamId) {
	return modInfo.constructionDecaySpeed;
}

EXPORT(int) skirmishAiCallback_Mod_getMultiReclaim(int teamId) {
	return modInfo.multiReclaim;
}
EXPORT(int) skirmishAiCallback_Mod_getReclaimMethod(int teamId) {
	return modInfo.reclaimMethod;
}
EXPORT(int) skirmishAiCallback_Mod_getReclaimUnitMethod(int teamId) {
	return modInfo.reclaimUnitMethod;
}
EXPORT(float) skirmishAiCallback_Mod_getReclaimUnitEnergyCostFactor(int teamId) {
	return modInfo.reclaimUnitEnergyCostFactor;
}
EXPORT(float) skirmishAiCallback_Mod_getReclaimUnitEfficiency(int teamId) {
	return modInfo.reclaimUnitEfficiency;
}
EXPORT(float) skirmishAiCallback_Mod_getReclaimFeatureEnergyCostFactor(int teamId) {
	return modInfo.reclaimFeatureEnergyCostFactor;
}
EXPORT(bool) skirmishAiCallback_Mod_getReclaimAllowEnemies(int teamId) {
	return modInfo.reclaimAllowEnemies;
}
EXPORT(bool) skirmishAiCallback_Mod_getReclaimAllowAllies(int teamId) {
	return modInfo.reclaimAllowAllies;
}

EXPORT(float) skirmishAiCallback_Mod_getRepairEnergyCostFactor(int teamId) {
	return modInfo.repairEnergyCostFactor;
}

EXPORT(float) skirmishAiCallback_Mod_getResurrectEnergyCostFactor(int teamId) {
	return modInfo.resurrectEnergyCostFactor;
}

EXPORT(float) skirmishAiCallback_Mod_getCaptureEnergyCostFactor(int teamId) {
	return modInfo.captureEnergyCostFactor;
}

EXPORT(int) skirmishAiCallback_Mod_getTransportGround(int teamId) {
	return modInfo.transportGround;
}
EXPORT(int) skirmishAiCallback_Mod_getTransportHover(int teamId) {
	return modInfo.transportHover;
}
EXPORT(int) skirmishAiCallback_Mod_getTransportShip(int teamId) {
	return modInfo.transportShip;
}
EXPORT(int) skirmishAiCallback_Mod_getTransportAir(int teamId) {
	return modInfo.transportAir;
}

EXPORT(int) skirmishAiCallback_Mod_getFireAtKilled(int teamId) {
	return modInfo.fireAtKilled;
}
EXPORT(int) skirmishAiCallback_Mod_getFireAtCrashing(int teamId) {
	return modInfo.fireAtCrashing;
}

EXPORT(int) skirmishAiCallback_Mod_getFlankingBonusModeDefault(int teamId) {
	return modInfo.flankingBonusModeDefault;
}

EXPORT(int) skirmishAiCallback_Mod_getLosMipLevel(int teamId) {
	return modInfo.losMipLevel;
}
EXPORT(int) skirmishAiCallback_Mod_getAirMipLevel(int teamId) {
	return modInfo.airMipLevel;
}
EXPORT(float) skirmishAiCallback_Mod_getLosMul(int teamId) {
	return modInfo.losMul;
}
EXPORT(float) skirmishAiCallback_Mod_getAirLosMul(int teamId) {
	return modInfo.airLosMul;
}
EXPORT(bool) skirmishAiCallback_Mod_getRequireSonarUnderWater(int teamId) {
	return modInfo.requireSonarUnderWater;
}
//########### END Mod



//########### BEGINN Map
EXPORT(bool) skirmishAiCallback_Map_isPosInCamera(int teamId, float* pos_posF3, float radius) {
	return team_callback[teamId]->PosInCamera(pos_posF3, radius);
}
EXPORT(int) skirmishAiCallback_Map_getChecksum(int teamId) {

	unsigned int checksum;

	const bool fetchOk = team_callback[teamId]->GetValue(AIVAL_MAP_CHECKSUM, &checksum);
	if (!fetchOk) {
		checksum = -1;
	}

	return checksum;
}
EXPORT(int) skirmishAiCallback_Map_getWidth(int teamId) {
	return team_callback[teamId]->GetMapWidth();
}

EXPORT(int) skirmishAiCallback_Map_getHeight(int teamId) {
	return team_callback[teamId]->GetMapHeight();
}

EXPORT(int) skirmishAiCallback_Map_getHeightMap(int teamId, float* heights,
		int heights_sizeMax) {

	static const int heights_sizeReal = gs->mapx * gs->mapy;

	int heights_size = heights_sizeReal;

	if (heights != NULL) {
		const float* tmpMap = team_callback[teamId]->GetHeightMap();
		heights_size = min(heights_sizeReal, heights_sizeMax);
		int i;
		for (i=0; i < heights_size; ++i) {
			heights[i] = tmpMap[i];
		}
	}

	return heights_size;
}

EXPORT(int) skirmishAiCallback_Map_getCornersHeightMap(int teamId,
		float* cornerHeights, int cornerHeights_sizeMax) {

	static const int cornerHeights_sizeReal = (gs->mapx + 1) * (gs->mapy + 1);

	int cornerHeights_size = cornerHeights_sizeReal;

	if (cornerHeights != NULL) {
		const float* tmpMap =  team_callback[teamId]->GetCornersHeightMap();
		cornerHeights_size = min(cornerHeights_sizeReal, cornerHeights_sizeMax);
		int i;
		for (i=0; i < cornerHeights_size; ++i) {
			cornerHeights[i] = tmpMap[i];
		}
	}

	return cornerHeights_size;
}

EXPORT(float) skirmishAiCallback_Map_getMinHeight(int teamId) {
	return team_callback[teamId]->GetMinHeight();
}

EXPORT(float) skirmishAiCallback_Map_getMaxHeight(int teamId) {
	return team_callback[teamId]->GetMaxHeight();
}

EXPORT(int) skirmishAiCallback_Map_getSlopeMap(int teamId,
		float* slopes, int slopes_sizeMax) {

	static const int slopes_sizeReal = gs->hmapx * gs->hmapy;

	int slopes_size = slopes_sizeReal;

	if (slopes != NULL) {
		const float* tmpMap =  team_callback[teamId]->GetSlopeMap();
		slopes_size = min(slopes_sizeReal, slopes_sizeMax);
		int i;
		for (i=0; i < slopes_size; ++i) {
			slopes[i] = tmpMap[i];
		}
	}

	return slopes_size;
}

EXPORT(int) skirmishAiCallback_Map_getLosMap(int teamId,
		int* losValues, int losValues_sizeMax) {

	static const int losValues_sizeReal = loshandler->losSizeX * loshandler->losSizeY;

	int losValues_size = losValues_sizeReal;

	if (losValues != NULL) {
		const unsigned short* tmpMap =  team_callback[teamId]->GetLosMap();
		losValues_size = min(losValues_sizeReal, losValues_sizeMax);
		int i;
		for (i=0; i < losValues_size; ++i) {
			losValues[i] = tmpMap[i];
		}
	}

	return losValues_size;
}

EXPORT(int) skirmishAiCallback_Map_getRadarMap(int teamId,
		int* radarValues, int radarValues_sizeMax) {

	static const int radarValues_sizeReal = radarhandler->xsize * radarhandler->zsize;

	int radarValues_size = radarValues_sizeReal;

	if (radarValues != NULL) {
		const unsigned short* tmpMap =  team_callback[teamId]->GetRadarMap();
		radarValues_size = min(radarValues_sizeReal, radarValues_sizeMax);
		int i;
		for (i=0; i < radarValues_size; ++i) {
			radarValues[i] = tmpMap[i];
		}
	}

	return radarValues_size;
}

EXPORT(int) skirmishAiCallback_Map_getJammerMap(int teamId,
		int* jammerValues, int jammerValues_sizeMax) {

	// Yes, it is correct, jammer-map has the same size as the radar map
	static const int jammerValues_sizeReal = radarhandler->xsize * radarhandler->zsize;

	int jammerValues_size = jammerValues_sizeReal;

	if (jammerValues != NULL) {
		const unsigned short* tmpMap =  team_callback[teamId]->GetJammerMap();
		jammerValues_size = min(jammerValues_sizeReal, jammerValues_sizeMax);
		int i;
		for (i=0; i < jammerValues_size; ++i) {
			jammerValues[i] = tmpMap[i];
		}
	}

	return jammerValues_size;
}

EXPORT(int) skirmishAiCallback_Map_getResourceMapRaw(
		int teamId, int resourceId, short* resources, int resources_sizeMax) {

	int resources_sizeReal = 0;
	if (resourceId == resourceHandler->GetMetalId()) {
		resources_sizeReal = readmap->metalMap->GetSizeX() * readmap->metalMap->GetSizeZ();
	}

	int resources_size = resources_sizeReal;

	if ((resources != NULL) && (resources_sizeReal > 0)) {
		resources_size = min(resources_sizeReal, resources_sizeMax);

		const unsigned char* tmpMap;
		if (resourceId == resourceHandler->GetMetalId()) {
			tmpMap = team_callback[teamId]->GetMetalMap();
		} else {
			tmpMap = NULL;
			resources_size = 0;
		}

		int i;
		for (i=0; i < resources_size; ++i) {
			resources[i] = tmpMap[i];
		}
	}

	return resources_size;
}
static inline const CResourceMapAnalyzer* getResourceMapAnalyzer(int resourceId) {
	return resourceHandler->GetResourceMapAnalyzer(resourceId);
}
EXPORT(int) skirmishAiCallback_Map_getResourceMapSpotsPositions(
		int teamId, int resourceId, float* spots, int spots_sizeMax) {

	const std::vector<float3>& intSpots = getResourceMapAnalyzer(resourceId)->GetSpots();
	const size_t spots_sizeReal = intSpots.size() * 3;

	size_t spots_size = spots_sizeReal;

	if (spots != NULL) {
		spots_size = min(spots_sizeReal, max(0, spots_sizeMax));

		std::vector<float3>::const_iterator s;
		size_t si = 0;
		for (s = intSpots.begin(); s != intSpots.end() && si < spots_size; ++s) {
			spots[si++] = s->x;
			spots[si++] = s->y;
			spots[si++] = s->z;
		}
	}

	return static_cast<int>(spots_size);
}
EXPORT(float) skirmishAiCallback_Map_getResourceMapSpotsAverageIncome(
		int teamId, int resourceId) {
	return getResourceMapAnalyzer(resourceId)->GetAverageIncome();
}
EXPORT(void) skirmishAiCallback_Map_getResourceMapSpotsNearest(
		int teamId, int resourceId, float* pos_posF3, float* return_posF3_out) {
	getResourceMapAnalyzer(resourceId)->GetNearestSpot(pos_posF3, teamId).copyInto(return_posF3_out);
}

EXPORT(const char*) skirmishAiCallback_Map_getName(int teamId) {
	return team_callback[teamId]->GetMapName();
}

EXPORT(float) skirmishAiCallback_Map_getElevationAt(int teamId, float x, float z) {
	return team_callback[teamId]->GetElevation(x, z);
}



EXPORT(float) skirmishAiCallback_Map_getMaxResource(
		int teamId, int resourceId) {

	if (resourceId == resourceHandler->GetMetalId()) {
		return team_callback[teamId]->GetMaxMetal();
	} else {
		return NULL;
	}
}
EXPORT(float) skirmishAiCallback_Map_getExtractorRadius(int teamId,
		int resourceId) {

	const IAICallback* clb = team_callback[teamId];
	if (resourceId == resourceHandler->GetMetalId()) {
		return clb->GetExtractorRadius();
	} else {
		return NULL;
	}
}

EXPORT(float) skirmishAiCallback_Map_getMinWind(int teamId) {
	return team_callback[teamId]->GetMinWind();
}
EXPORT(float) skirmishAiCallback_Map_getMaxWind(int teamId) {
	return team_callback[teamId]->GetMaxWind();
}
EXPORT(float) skirmishAiCallback_Map_getCurWind(int teamId) {
	return team_callback[teamId]->GetCurWind();
}

EXPORT(float) skirmishAiCallback_Map_getTidalStrength(int teamId) {
	return team_callback[teamId]->GetTidalStrength();
}
EXPORT(float) skirmishAiCallback_Map_getGravity(int teamId) {
	return team_callback[teamId]->GetGravity();
}



EXPORT(bool) skirmishAiCallback_Map_isPossibleToBuildAt(int teamId, int unitDefId,
		float* pos_posF3, int facing) {

	const UnitDef* unitDef = getUnitDefById(teamId, unitDefId);
	return team_callback[teamId]->CanBuildAt(unitDef, pos_posF3, facing);
}

EXPORT(void) skirmishAiCallback_Map_findClosestBuildSite(int teamId, int unitDefId,
		float* pos_posF3, float searchRadius, int minDist, int facing, float* return_posF3_out) {

			const UnitDef* unitDef = getUnitDefById(teamId, unitDefId);
	team_callback[teamId]->ClosestBuildSite(unitDef, pos_posF3, searchRadius, minDist, facing)
			.copyInto(return_posF3_out);
}

EXPORT(int) skirmishAiCallback_Map_getPoints(int teamId, bool includeAllies) {
	return team_callback[teamId]->GetMapPoints(tmpPointMarkerArr[teamId],
			TMP_ARR_SIZE, includeAllies);
}
EXPORT(void) skirmishAiCallback_Map_Point_getPosition(int teamId, int pointId, float* return_posF3_out) {
	tmpPointMarkerArr[teamId][pointId].pos.copyInto(return_posF3_out);
}
EXPORT(void) skirmishAiCallback_Map_Point_getColor(int teamId, int pointId, short* return_colorS3_out) {

	const unsigned char* color = tmpPointMarkerArr[teamId][pointId].color;
	return_colorS3_out[0] = color[0];
	return_colorS3_out[1] = color[1];
	return_colorS3_out[2] = color[2];
}
EXPORT(const char*) skirmishAiCallback_Map_Point_getLabel(int teamId, int pointId) {
	return tmpPointMarkerArr[teamId][pointId].label;
}

EXPORT(int) skirmishAiCallback_Map_getLines(int teamId, bool includeAllies) {
	return team_callback[teamId]->GetMapLines(tmpLineMarkerArr[teamId],
			TMP_ARR_SIZE, includeAllies);
}
EXPORT(void) skirmishAiCallback_Map_Line_getFirstPosition(int teamId, int lineId, float* return_posF3_out) {
	tmpLineMarkerArr[teamId][lineId].pos.copyInto(return_posF3_out);
}
EXPORT(void) skirmishAiCallback_Map_Line_getSecondPosition(int teamId, int lineId, float* return_posF3_out) {
	tmpLineMarkerArr[teamId][lineId].pos2.copyInto(return_posF3_out);
}
EXPORT(void) skirmishAiCallback_Map_Line_getColor(int teamId, int lineId, short* return_colorS3_out) {

	const unsigned char* color = tmpLineMarkerArr[teamId][lineId].color;
	return_colorS3_out[0] = color[0];
	return_colorS3_out[1] = color[1];
	return_colorS3_out[2] = color[2];
}
EXPORT(void) skirmishAiCallback_Map_getStartPos(int teamId, float* return_posF3_out) {
	team_callback[teamId]->GetStartPos()->copyInto(return_posF3_out);
}
EXPORT(void) skirmishAiCallback_Map_getMousePos(int teamId, float* return_posF3_out) {
	team_callback[teamId]->GetMousePos().copyInto(return_posF3_out);
}
//########### END Map


// DEPRECATED
/*
EXPORT(bool) skirmishAiCallback_getProperty(int teamId, int id, int property, void* dst) {
//	return team_callback[teamId]->GetProperty(id, property, dst);
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetProperty(id, property, dst);
	} else {
		return team_callback[teamId]->GetProperty(id, property, dst);
	}
}

EXPORT(bool) skirmishAiCallback_getValue(int teamId, int id, void* dst) {
//	return team_callback[teamId]->GetValue(id, dst);
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetValue(id, dst);
	} else {
		return team_callback[teamId]->GetValue(id, dst);
	}
}
*/

EXPORT(int) skirmishAiCallback_File_getSize(int teamId, const char* fileName) {
	return team_callback[teamId]->GetFileSize(fileName);
}

EXPORT(bool) skirmishAiCallback_File_getContent(int teamId, const char* fileName, void* buffer, int bufferLen) {
	return team_callback[teamId]->ReadFile(fileName, buffer, bufferLen);
}





// BEGINN OBJECT Resource
EXPORT(int) skirmishAiCallback_getResources(int teamId) {
	return resourceHandler->GetNumResources();
}
EXPORT(int) skirmishAiCallback_getResourceByName(int teamId,
		const char* resourceName) {
	return resourceHandler->GetResourceId(resourceName);
}
EXPORT(const char*) skirmishAiCallback_Resource_getName(int teamId, int resourceId) {
	return resourceHandler->GetResource(resourceId)->name.c_str();
}
EXPORT(float) skirmishAiCallback_Resource_getOptimum(int teamId, int resourceId) {
	return resourceHandler->GetResource(resourceId)->optimum;
}
EXPORT(float) skirmishAiCallback_Economy_getCurrent(int teamId,
		int resourceId) {

	IAICallback* clb = team_callback[teamId];
	if (resourceId == resourceHandler->GetMetalId()) {
		return clb->GetMetal();
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return clb->GetEnergy();
	} else {
		return -1.0f;
	}
}

EXPORT(float) skirmishAiCallback_Economy_getIncome(int teamId,
		int resourceId) {

	IAICallback* clb = team_callback[teamId];
	if (resourceId == resourceHandler->GetMetalId()) {
		return clb->GetMetalIncome();
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return clb->GetEnergyIncome();
	} else {
		return -1.0f;
	}
}

EXPORT(float) skirmishAiCallback_Economy_getUsage(int teamId,
		int resourceId) {

	IAICallback* clb = team_callback[teamId];
	if (resourceId == resourceHandler->GetMetalId()) {
		return clb->GetMetalUsage();
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return clb->GetEnergyUsage();
	} else {
		return -1.0f;
	}
}

EXPORT(float) skirmishAiCallback_Economy_getStorage(int teamId,
		int resourceId) {

	IAICallback* clb = team_callback[teamId];
	if (resourceId == resourceHandler->GetMetalId()) {
		return clb->GetMetalStorage();
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return clb->GetEnergyStorage();
	} else {
		return -1.0f;
	}
}

EXPORT(const char*) skirmishAiCallback_Game_getSetupScript(int teamId) {
	const char* setupScript;
	IAICallback* clb = team_callback[teamId];
	const bool fetchOk = clb->GetValue(AIVAL_SCRIPT, &setupScript);
	if (!fetchOk) {
		return NULL;
	}
	return setupScript;
}







//########### BEGINN UnitDef
EXPORT(int) skirmishAiCallback_getUnitDefs(int teamId, int* unitDefIds,
		int unitDefIds_sizeMax) {

	const int unitDefIds_sizeReal = team_callback[teamId]->GetNumUnitDefs();

	int unitDefIds_size = unitDefIds_sizeReal;

	if (unitDefIds != NULL) {
		const UnitDef** defList = (const UnitDef**) new UnitDef*[unitDefIds_sizeReal];
		team_callback[teamId]->GetUnitDefList(defList);
		unitDefIds_size = min(unitDefIds_sizeReal, unitDefIds_sizeMax);
		int ud;
		for (ud = 0; ud < unitDefIds_size; ++ud) {
			// AI's should double-check for this
			unitDefIds[ud] = (defList[ud] != NULL)? defList[ud]->id: -1;
		}
		delete [] defList;
		defList = NULL;
	}

	return unitDefIds_size;
}
EXPORT(int) skirmishAiCallback_getUnitDefByName(int teamId,
		const char* unitName) {

	int unitDefId = -1;

	const UnitDef* ud = team_callback[teamId]->GetUnitDef(unitName);
	if (ud != NULL) {
		unitDefId = ud->id;
	}

	return unitDefId;
}

EXPORT(float) skirmishAiCallback_UnitDef_getHeight(int teamId, int unitDefId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetUnitDefHeight(unitDefId);
}
EXPORT(float) skirmishAiCallback_UnitDef_getRadius(int teamId, int unitDefId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetUnitDefRadius(unitDefId);
}

EXPORT(bool) skirmishAiCallback_UnitDef_isValid(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->valid;
}
EXPORT(const char*) skirmishAiCallback_UnitDef_getName(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->name.c_str();
}
EXPORT(const char*) skirmishAiCallback_UnitDef_getHumanName(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->humanName.c_str();
}
EXPORT(const char*) skirmishAiCallback_UnitDef_getFileName(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->filename.c_str();
}
//EXPORT(int) skirmishAiCallback_UnitDef_getId(int teamId, int unitDefId) {
//	return getUnitDefById(teamId, unitDefId)->id;
//}
EXPORT(int) skirmishAiCallback_UnitDef_getAiHint(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->aihint;
}
EXPORT(int) skirmishAiCallback_UnitDef_getCobId(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->cobID;
}
EXPORT(int) skirmishAiCallback_UnitDef_getTechLevel(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->techLevel;
}
EXPORT(const char*) skirmishAiCallback_UnitDef_getGaia(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->gaia.c_str();
}
EXPORT(float) skirmishAiCallback_UnitDef_getUpkeep(int teamId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->metalUpkeep;
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return ud->energyUpkeep;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_UnitDef_getResourceMake(int teamId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->metalMake;
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return ud->energyMake;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_UnitDef_getMakesResource(int teamId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->makesMetal;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_UnitDef_getCost(int teamId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->metalCost;
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return ud->energyCost;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_UnitDef_getExtractsResource(
		int teamId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->extractsMetal;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_UnitDef_getResourceExtractorRange(
		int teamId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->extractRange;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_UnitDef_getWindResourceGenerator(
		int teamId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == resourceHandler->GetEnergyId()) {
		return ud->windGenerator;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_UnitDef_getTidalResourceGenerator(
		int teamId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == resourceHandler->GetEnergyId()) {
		return ud->tidalGenerator;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_UnitDef_getStorage(int teamId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->metalStorage;
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return ud->energyStorage;
	} else {
		return 0.0f;
	}
}
EXPORT(bool) skirmishAiCallback_UnitDef_isSquareResourceExtractor(
		int teamId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->extractSquare;
	} else {
		return false;
	}
}
EXPORT(float) skirmishAiCallback_UnitDef_getBuildTime(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->buildTime;
}
EXPORT(float) skirmishAiCallback_UnitDef_getAutoHeal(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->autoHeal;
}
EXPORT(float) skirmishAiCallback_UnitDef_getIdleAutoHeal(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->idleAutoHeal;
}
EXPORT(int) skirmishAiCallback_UnitDef_getIdleTime(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->idleTime;
}
EXPORT(float) skirmishAiCallback_UnitDef_getPower(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->power;
}
EXPORT(float) skirmishAiCallback_UnitDef_getHealth(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->health;
}
EXPORT(int) skirmishAiCallback_UnitDef_getCategory(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->category;
}
EXPORT(float) skirmishAiCallback_UnitDef_getSpeed(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->speed;
}
EXPORT(float) skirmishAiCallback_UnitDef_getTurnRate(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->turnRate;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isTurnInPlace(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->turnInPlace;
}
EXPORT(float) skirmishAiCallback_UnitDef_getTurnInPlaceDistance(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->turnInPlaceDistance;
}
EXPORT(float) skirmishAiCallback_UnitDef_getTurnInPlaceSpeedLimit(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->turnInPlaceSpeedLimit;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isUpright(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->upright;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isCollide(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->collide;
}
EXPORT(float) skirmishAiCallback_UnitDef_getControlRadius(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->controlRadius;
}
EXPORT(float) skirmishAiCallback_UnitDef_getLosRadius(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->losRadius;
}
EXPORT(float) skirmishAiCallback_UnitDef_getAirLosRadius(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->airLosRadius;
}
EXPORT(float) skirmishAiCallback_UnitDef_getLosHeight(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->losHeight;
}
EXPORT(int) skirmishAiCallback_UnitDef_getRadarRadius(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->radarRadius;
}
EXPORT(int) skirmishAiCallback_UnitDef_getSonarRadius(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->sonarRadius;
}
EXPORT(int) skirmishAiCallback_UnitDef_getJammerRadius(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->jammerRadius;
}
EXPORT(int) skirmishAiCallback_UnitDef_getSonarJamRadius(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->sonarJamRadius;
}
EXPORT(int) skirmishAiCallback_UnitDef_getSeismicRadius(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->seismicRadius;
}
EXPORT(float) skirmishAiCallback_UnitDef_getSeismicSignature(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->seismicSignature;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isStealth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->stealth;}
EXPORT(bool) skirmishAiCallback_UnitDef_isSonarStealth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->sonarStealth;}
EXPORT(bool) skirmishAiCallback_UnitDef_isBuildRange3D(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildRange3D;}
EXPORT(float) skirmishAiCallback_UnitDef_getBuildDistance(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildDistance;}
EXPORT(float) skirmishAiCallback_UnitDef_getBuildSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildSpeed;}
EXPORT(float) skirmishAiCallback_UnitDef_getReclaimSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->reclaimSpeed;}
EXPORT(float) skirmishAiCallback_UnitDef_getRepairSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->repairSpeed;}
EXPORT(float) skirmishAiCallback_UnitDef_getMaxRepairSpeed(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->maxRepairSpeed;
}
EXPORT(float) skirmishAiCallback_UnitDef_getResurrectSpeed(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->resurrectSpeed;
}
EXPORT(float) skirmishAiCallback_UnitDef_getCaptureSpeed(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->captureSpeed;
}
EXPORT(float) skirmishAiCallback_UnitDef_getTerraformSpeed(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->terraformSpeed;
}
EXPORT(float) skirmishAiCallback_UnitDef_getMass(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->mass;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isPushResistant(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->pushResistant;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isStrafeToAttack(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->strafeToAttack;
}
EXPORT(float) skirmishAiCallback_UnitDef_getMinCollisionSpeed(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->minCollisionSpeed;
}
EXPORT(float) skirmishAiCallback_UnitDef_getSlideTolerance(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->slideTolerance;
}
EXPORT(float) skirmishAiCallback_UnitDef_getMaxSlope(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxSlope;}
EXPORT(float) skirmishAiCallback_UnitDef_getMaxHeightDif(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxHeightDif;}
EXPORT(float) skirmishAiCallback_UnitDef_getMinWaterDepth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->minWaterDepth;}
EXPORT(float) skirmishAiCallback_UnitDef_getWaterline(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->waterline;}
EXPORT(float) skirmishAiCallback_UnitDef_getMaxWaterDepth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxWaterDepth;}
EXPORT(float) skirmishAiCallback_UnitDef_getArmoredMultiple(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->armoredMultiple;
}
EXPORT(int) skirmishAiCallback_UnitDef_getArmorType(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->armorType;}
EXPORT(int) skirmishAiCallback_UnitDef_FlankingBonus_getMode(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->flankingBonusMode;
}
EXPORT(void) skirmishAiCallback_UnitDef_FlankingBonus_getDir(int teamId, int unitDefId, float* return_posF3_out) {
	getUnitDefById(teamId, unitDefId)->flankingBonusDir.copyInto(return_posF3_out);
}
EXPORT(float) skirmishAiCallback_UnitDef_FlankingBonus_getMax(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->flankingBonusMax;
}
EXPORT(float) skirmishAiCallback_UnitDef_FlankingBonus_getMin(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->flankingBonusMin;
}
EXPORT(float) skirmishAiCallback_UnitDef_FlankingBonus_getMobilityAdd(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->flankingBonusMobilityAdd;
}
EXPORT(const char*) skirmishAiCallback_UnitDef_CollisionVolume_getType(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->collisionVolumeTypeStr.c_str();
}
EXPORT(void) skirmishAiCallback_UnitDef_CollisionVolume_getScales(int teamId, int unitDefId, float* return_posF3_out) {
	getUnitDefById(teamId, unitDefId)->collisionVolumeScales.copyInto(return_posF3_out);
}
EXPORT(void) skirmishAiCallback_UnitDef_CollisionVolume_getOffsets(int teamId, int unitDefId, float* return_posF3_out) {
	getUnitDefById(teamId, unitDefId)->collisionVolumeOffsets.copyInto(return_posF3_out);
}
EXPORT(int) skirmishAiCallback_UnitDef_CollisionVolume_getTest(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->collisionVolumeTest;
}
EXPORT(float) skirmishAiCallback_UnitDef_getMaxWeaponRange(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->maxWeaponRange;
}
EXPORT(const char*) skirmishAiCallback_UnitDef_getType(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->type.c_str();
}
EXPORT(const char*) skirmishAiCallback_UnitDef_getTooltip(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->tooltip.c_str();
}
EXPORT(const char*) skirmishAiCallback_UnitDef_getWreckName(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->wreckName.c_str();
}
EXPORT(const char*) skirmishAiCallback_UnitDef_getDeathExplosion(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->deathExplosion.c_str();
}
EXPORT(const char*) skirmishAiCallback_UnitDef_getSelfDExplosion(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->selfDExplosion.c_str();
}
EXPORT(const char*) skirmishAiCallback_UnitDef_getTedClassString(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->TEDClassString.c_str();
}
EXPORT(const char*) skirmishAiCallback_UnitDef_getCategoryString(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->categoryString.c_str();
}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToSelfD(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canSelfD;}
EXPORT(int) skirmishAiCallback_UnitDef_getSelfDCountdown(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->selfDCountdown;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToSubmerge(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canSubmerge;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToFly(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canfly;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToMove(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canmove;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToHover(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canhover;}
EXPORT(bool) skirmishAiCallback_UnitDef_isFloater(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->floater;}
EXPORT(bool) skirmishAiCallback_UnitDef_isBuilder(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->builder;}
EXPORT(bool) skirmishAiCallback_UnitDef_isActivateWhenBuilt(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->activateWhenBuilt;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isOnOffable(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->onoffable;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isFullHealthFactory(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->fullHealthFactory;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isFactoryHeadingTakeoff(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->factoryHeadingTakeoff;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isReclaimable(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->reclaimable;}
EXPORT(bool) skirmishAiCallback_UnitDef_isCapturable(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->capturable;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToRestore(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canRestore;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToRepair(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canRepair;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToSelfRepair(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canSelfRepair;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToReclaim(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canReclaim;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToAttack(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canAttack;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToPatrol(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canPatrol;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToFight(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canFight;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToGuard(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canGuard;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToAssist(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canAssist;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAssistable(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canBeAssisted;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToRepeat(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canRepeat;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToFireControl(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->canFireControl;
}
EXPORT(int) skirmishAiCallback_UnitDef_getFireState(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->fireState;}
EXPORT(int) skirmishAiCallback_UnitDef_getMoveState(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->moveState;}
EXPORT(float) skirmishAiCallback_UnitDef_getWingDrag(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->wingDrag;}
EXPORT(float) skirmishAiCallback_UnitDef_getWingAngle(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->wingAngle;}
EXPORT(float) skirmishAiCallback_UnitDef_getDrag(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->drag;}
EXPORT(float) skirmishAiCallback_UnitDef_getFrontToSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->frontToSpeed;}
EXPORT(float) skirmishAiCallback_UnitDef_getSpeedToFront(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->speedToFront;}
EXPORT(float) skirmishAiCallback_UnitDef_getMyGravity(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->myGravity;}
EXPORT(float) skirmishAiCallback_UnitDef_getMaxBank(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxBank;}
EXPORT(float) skirmishAiCallback_UnitDef_getMaxPitch(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxPitch;}
EXPORT(float) skirmishAiCallback_UnitDef_getTurnRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->turnRadius;}
EXPORT(float) skirmishAiCallback_UnitDef_getWantedHeight(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->wantedHeight;}
EXPORT(float) skirmishAiCallback_UnitDef_getVerticalSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->verticalSpeed;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToCrash(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canCrash;}
EXPORT(bool) skirmishAiCallback_UnitDef_isHoverAttack(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->hoverAttack;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAirStrafe(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->airStrafe;}
EXPORT(float) skirmishAiCallback_UnitDef_getDlHoverFactor(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->dlHoverFactor;}
EXPORT(float) skirmishAiCallback_UnitDef_getMaxAcceleration(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxAcc;}
EXPORT(float) skirmishAiCallback_UnitDef_getMaxDeceleration(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxDec;}
EXPORT(float) skirmishAiCallback_UnitDef_getMaxAileron(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxAileron;}
EXPORT(float) skirmishAiCallback_UnitDef_getMaxElevator(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxElevator;}
EXPORT(float) skirmishAiCallback_UnitDef_getMaxRudder(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxRudder;}
EXPORT(int) skirmishAiCallback_UnitDef_getYardMap(int teamId, int unitDefId, int facing, short* yardMap, int yardMap_sizeMax) {

	if (facing < 0 || facing >=4) {
		return 0;
	}

	const UnitDef* unitDef = getUnitDefById(teamId, unitDefId);
	const int yardMap_sizeReal = unitDef->xsize * unitDef->zsize;

	int yardMap_size = yardMap_sizeReal;

	if (yardMap != NULL) {
		yardMap_size = min(yardMap_sizeReal, yardMap_sizeMax);
		const unsigned char* const ym = unitDef->yardmaps[facing];
		int i;
		for (i = 0; i < yardMap_size; ++i) {
			yardMap[i] = (short) ym[i];
		}
	}

	return yardMap_size;
}
EXPORT(int) skirmishAiCallback_UnitDef_getXSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->xsize;}
EXPORT(int) skirmishAiCallback_UnitDef_getZSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->zsize;}
EXPORT(int) skirmishAiCallback_UnitDef_getBuildAngle(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildangle;}
EXPORT(float) skirmishAiCallback_UnitDef_getLoadingRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->loadingRadius;}
EXPORT(float) skirmishAiCallback_UnitDef_getUnloadSpread(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->unloadSpread;}
EXPORT(int) skirmishAiCallback_UnitDef_getTransportCapacity(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->transportCapacity;
}
EXPORT(int) skirmishAiCallback_UnitDef_getTransportSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->transportSize;}
EXPORT(int) skirmishAiCallback_UnitDef_getMinTransportSize(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->minTransportSize;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isAirBase(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->isAirBase;}
EXPORT(float) skirmishAiCallback_UnitDef_getTransportMass(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->transportMass;}
EXPORT(float) skirmishAiCallback_UnitDef_getMinTransportMass(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->minTransportMass;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isHoldSteady(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->holdSteady;}
EXPORT(bool) skirmishAiCallback_UnitDef_isReleaseHeld(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->releaseHeld;}
EXPORT(bool) skirmishAiCallback_UnitDef_isNotTransportable(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->cantBeTransported;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isTransportByEnemy(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->transportByEnemy;
}
EXPORT(int) skirmishAiCallback_UnitDef_getTransportUnloadMethod(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->transportUnloadMethod;
}
EXPORT(float) skirmishAiCallback_UnitDef_getFallSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->fallSpeed;}
EXPORT(float) skirmishAiCallback_UnitDef_getUnitFallSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->unitFallSpeed;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToCloak(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canCloak;}
EXPORT(bool) skirmishAiCallback_UnitDef_isStartCloaked(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->startCloaked;}
EXPORT(float) skirmishAiCallback_UnitDef_getCloakCost(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->cloakCost;}
EXPORT(float) skirmishAiCallback_UnitDef_getCloakCostMoving(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->cloakCostMoving;
}
EXPORT(float) skirmishAiCallback_UnitDef_getDecloakDistance(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->decloakDistance;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isDecloakSpherical(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->decloakSpherical;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isDecloakOnFire(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->decloakOnFire;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToKamikaze(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canKamikaze;}
EXPORT(float) skirmishAiCallback_UnitDef_getKamikazeDist(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->kamikazeDist;}
EXPORT(bool) skirmishAiCallback_UnitDef_isTargetingFacility(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->targfac;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToDGun(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canDGun;}
EXPORT(bool) skirmishAiCallback_UnitDef_isNeedGeo(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->needGeo;}
EXPORT(bool) skirmishAiCallback_UnitDef_isFeature(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->isFeature;}
EXPORT(bool) skirmishAiCallback_UnitDef_isHideDamage(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->hideDamage;}
EXPORT(bool) skirmishAiCallback_UnitDef_isCommander(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->isCommander;}
EXPORT(bool) skirmishAiCallback_UnitDef_isShowPlayerName(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->showPlayerName;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToResurrect(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canResurrect;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToCapture(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canCapture;}
EXPORT(int) skirmishAiCallback_UnitDef_getHighTrajectoryType(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->highTrajectoryType;
}
EXPORT(int) skirmishAiCallback_UnitDef_getNoChaseCategory(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->noChaseCategory;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isLeaveTracks(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->leaveTracks;}
EXPORT(float) skirmishAiCallback_UnitDef_getTrackWidth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->trackWidth;}
EXPORT(float) skirmishAiCallback_UnitDef_getTrackOffset(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->trackOffset;}
EXPORT(float) skirmishAiCallback_UnitDef_getTrackStrength(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->trackStrength;}
EXPORT(float) skirmishAiCallback_UnitDef_getTrackStretch(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->trackStretch;}
EXPORT(int) skirmishAiCallback_UnitDef_getTrackType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->trackType;}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToDropFlare(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canDropFlare;}
EXPORT(float) skirmishAiCallback_UnitDef_getFlareReloadTime(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->flareReloadTime;
}
EXPORT(float) skirmishAiCallback_UnitDef_getFlareEfficiency(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->flareEfficiency;
}
EXPORT(float) skirmishAiCallback_UnitDef_getFlareDelay(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareDelay;}
EXPORT(void) skirmishAiCallback_UnitDef_getFlareDropVector(int teamId, int unitDefId, float* return_posF3_out) {
	getUnitDefById(teamId, unitDefId)->flareDropVector.copyInto(return_posF3_out);
}
EXPORT(int) skirmishAiCallback_UnitDef_getFlareTime(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareTime;}
EXPORT(int) skirmishAiCallback_UnitDef_getFlareSalvoSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareSalvoSize;}
EXPORT(int) skirmishAiCallback_UnitDef_getFlareSalvoDelay(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->flareSalvoDelay;
}
//EXPORT(bool) skirmishAiCallback_UnitDef_isSmoothAnim(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->smoothAnim;}
EXPORT(bool) skirmishAiCallback_UnitDef_isResourceMaker(int teamId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->isMetalMaker;
	} else {
		return false;
	}
}
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToLoopbackAttack(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->canLoopbackAttack;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isLevelGround(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->levelGround;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isUseBuildingGroundDecal(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->useBuildingGroundDecal;
}
EXPORT(int) skirmishAiCallback_UnitDef_getBuildingDecalType(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->buildingDecalType;
}
EXPORT(int) skirmishAiCallback_UnitDef_getBuildingDecalSizeX(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->buildingDecalSizeX;
}
EXPORT(int) skirmishAiCallback_UnitDef_getBuildingDecalSizeY(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->buildingDecalSizeY;
}
EXPORT(float) skirmishAiCallback_UnitDef_getBuildingDecalDecaySpeed(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->buildingDecalDecaySpeed;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isFirePlatform(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->isFirePlatform;
}
EXPORT(float) skirmishAiCallback_UnitDef_getMaxFuel(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->maxFuel;
}
EXPORT(float) skirmishAiCallback_UnitDef_getRefuelTime(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->refuelTime;
}
EXPORT(float) skirmishAiCallback_UnitDef_getMinAirBasePower(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->minAirBasePower;
}
EXPORT(int) skirmishAiCallback_UnitDef_getMaxThisUnit(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->maxThisUnit;
}
EXPORT(int) skirmishAiCallback_UnitDef_getDecoyDef(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->decoyDef->id;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isDontLand(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->DontLand();
}
EXPORT(int) skirmishAiCallback_UnitDef_getShieldDef(int teamId, int unitDefId) {

	const WeaponDef* wd = getUnitDefById(teamId, unitDefId)->shieldWeaponDef;
	if (wd == NULL) {
		return -1;
	} else {
		return wd->id;
	}
}
EXPORT(int) skirmishAiCallback_UnitDef_getStockpileDef(int teamId,
		int unitDefId) {

	const WeaponDef* wd = getUnitDefById(teamId, unitDefId)->stockpileWeaponDef;
	if (wd == NULL) {
		return -1;
	} else {
		return wd->id;
	}
}
EXPORT(int) skirmishAiCallback_UnitDef_getBuildOptions(int teamId,
		int unitDefId, int* unitDefIds, int unitDefIds_sizeMax) {

	const std::map<int,std::string>& bo = getUnitDefById(teamId, unitDefId)->buildOptions;
	const size_t unitDefIds_sizeReal = bo.size();

	size_t unitDefIds_size = unitDefIds_sizeReal;

	if (unitDefIds != NULL) {
		unitDefIds_size = min(unitDefIds_sizeReal, unitDefIds_sizeMax);
		std::map<int,std::string>::const_iterator bb;
		size_t b;
		for (b=0, bb = bo.begin(); bb != bo.end() && b < unitDefIds_size; ++b, ++bb) {
			unitDefIds[b] = skirmishAiCallback_getUnitDefByName(teamId, bb->second.c_str());
		}
	}

	return unitDefIds_size;
}
EXPORT(int) skirmishAiCallback_UnitDef_getCustomParams(int teamId, int unitDefId,
		const char** keys, const char** values) {

	const std::map<std::string,std::string>& ps = getUnitDefById(teamId, unitDefId)->customParams;
	const size_t params_sizeReal = ps.size();

	if ((keys != NULL) && (values != NULL)) {
		fillCMap(&ps, keys, values);
	}

	return params_sizeReal;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isMoveDataAvailable(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata != NULL; }
EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getMaxAcceleration(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->maxAcceleration; }
EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getMaxBreaking(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->maxBreaking; }
EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getMaxSpeed(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->maxSpeed; }
EXPORT(short) skirmishAiCallback_UnitDef_MoveData_getMaxTurnRate(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->maxTurnRate; }

EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getSize(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->size; }
EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getDepth(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->depth; }
EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getMaxSlope(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->maxSlope; }
EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getSlopeMod(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->slopeMod; }
EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getDepthMod(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->depthMod; }
EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getPathType(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->pathType; }
EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getCrushStrength(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->crushStrength; }

EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getMoveType(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->moveType; }
EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getMoveFamily(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->moveFamily; }
EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getTerrainClass(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->terrainClass; }

EXPORT(bool) skirmishAiCallback_UnitDef_MoveData_getFollowGround(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->followGround; }
EXPORT(bool) skirmishAiCallback_UnitDef_MoveData_isSubMarine(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->subMarine; }
EXPORT(const char*) skirmishAiCallback_UnitDef_MoveData_getName(int teamId, int unitDefId) { return getUnitDefById(teamId, unitDefId)->movedata->name.c_str(); }



EXPORT(int) skirmishAiCallback_UnitDef_getWeaponMounts(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->weapons.size();}
EXPORT(const char*) skirmishAiCallback_UnitDef_WeaponMount_getName(int teamId, int unitDefId, int weaponMountId) {
	return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).name.c_str();
}
EXPORT(int) skirmishAiCallback_UnitDef_WeaponMount_getWeaponDef(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).def->id;}
EXPORT(int) skirmishAiCallback_UnitDef_WeaponMount_getSlavedTo(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).slavedTo;}
EXPORT(void) skirmishAiCallback_UnitDef_WeaponMount_getMainDir(int teamId, int unitDefId, int weaponMountId, float* return_posF3_out) {
	getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).mainDir.copyInto(return_posF3_out);
}
EXPORT(float) skirmishAiCallback_UnitDef_WeaponMount_getMaxAngleDif(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).maxAngleDif;}
EXPORT(float) skirmishAiCallback_UnitDef_WeaponMount_getFuelUsage(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).fuelUsage;}
EXPORT(int) skirmishAiCallback_UnitDef_WeaponMount_getBadTargetCategory(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).badTargetCat;}
EXPORT(int) skirmishAiCallback_UnitDef_WeaponMount_getOnlyTargetCategory(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).onlyTargetCat;}
//########### END UnitDef





//########### BEGINN Unit
EXPORT(int) skirmishAiCallback_Unit_getLimit(int teamId) {
	int unitLimit;
	IAICallback* clb = team_callback[teamId];
	const bool fetchOk = clb->GetValue(AIVAL_UNIT_LIMIT, &unitLimit);
	if (!fetchOk) {
		unitLimit = -1;
	}
	return unitLimit;
}
EXPORT(int) skirmishAiCallback_Unit_getDef(int teamId, int unitId) {
	const UnitDef* unitDef;
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		unitDef = team_cheatCallback[teamId]->GetUnitDef(unitId);
	} else {
		unitDef = team_callback[teamId]->GetUnitDef(unitId);
	}
//	IAICallback* clb = team_callback[teamId];
//	const UnitDef* unitDef = clb->GetUnitDef(unitId);
	if (unitDef != NULL) {
		return unitDef->id;
	} else {
		return -1;
	}
}


EXPORT(int) skirmishAiCallback_Unit_getModParams(int teamId, int unitId) {

	const CUnit* unit = getUnit(unitId);
	if (unit && /*(skirmishAiCallback_Cheats_isEnabled(teamId) || */isAlliedUnit(teamId, unit)/*)*/) {
		return unit->modParams.size();
	} else {
		return 0;
	}
}
EXPORT(const char*) skirmishAiCallback_Unit_ModParam_getName(int teamId, int unitId,
		int modParamId)
{
	const CUnit* unit = getUnit(unitId);
	if (unit && /*(skirmishAiCallback_Cheats_isEnabled(teamId) || */isAlliedUnit(teamId, unit)/*)*/
			&& modParamId >= 0
			&& (unsigned int)modParamId < unit->modParams.size()) {

		std::map<std::string, int>::const_iterator mi, mb, me;
		mb = unit->modParamsMap.begin();
		me = unit->modParamsMap.end();
		for (mi = mb; mi != me; ++mi) {
			if (mi->second == modParamId) {
				return mi->first.c_str();
			}
		}
	}

	// not all mod parameter have a name
	return "";
}
EXPORT(float) skirmishAiCallback_Unit_ModParam_getValue(int teamId, int unitId,
		int modParamId)
{
	const CUnit* unit = getUnit(unitId);
	if (unit && /*(skirmishAiCallback_Cheats_isEnabled(teamId) || */isAlliedUnit(teamId, unit)/*)*/
			&& modParamId >= 0
			&& (unsigned int)modParamId < unit->modParams.size()) {
		return unit->modParams[modParamId];
	} else {
		return 0.0f;
	}
}

EXPORT(int) skirmishAiCallback_Unit_getTeam(int teamId, int unitId) {
//	return team_callback[teamId]->GetUnitTeam(unitId);
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitTeam(unitId);
	} else {
		return team_callback[teamId]->GetUnitTeam(unitId);
	}
}
EXPORT(int) skirmishAiCallback_Unit_getAllyTeam(int teamId, int unitId) {
//	return team_callback[teamId]->GetUnitAllyTeam(unitId);
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitAllyTeam(unitId);
	} else {
		return team_callback[teamId]->GetUnitAllyTeam(unitId);
	}
}
EXPORT(int) skirmishAiCallback_Unit_getLineage(int teamId, int unitId) {

	const CUnit* unit = getUnit(unitId);
	if (unit == NULL) {
		return -1;
	}
	return unit->lineage;
}
EXPORT(int) skirmishAiCallback_Unit_getAiHint(int teamId, int unitId) {
	return team_callback[teamId]->GetUnitAiHint(unitId);
}

EXPORT(int) skirmishAiCallback_Unit_getSupportedCommands(int teamId, int unitId) {
	return team_callback[teamId]->GetUnitCommands(unitId)->size();
}
EXPORT(int) skirmishAiCallback_Unit_SupportedCommand_getId(int teamId, int unitId, int commandId) {
	return team_callback[teamId]->GetUnitCommands(unitId)->at(commandId).id;
}
EXPORT(const char*) skirmishAiCallback_Unit_SupportedCommand_getName(int teamId, int unitId, int commandId) {
	return team_callback[teamId]->GetUnitCommands(unitId)->at(commandId).name.c_str();
}
EXPORT(const char*) skirmishAiCallback_Unit_SupportedCommand_getToolTip(int teamId, int unitId, int commandId) {
	return team_callback[teamId]->GetUnitCommands(unitId)->at(commandId).tooltip.c_str();
}
EXPORT(bool) skirmishAiCallback_Unit_SupportedCommand_isShowUnique(int teamId, int unitId, int commandId) {
	return team_callback[teamId]->GetUnitCommands(unitId)->at(commandId).showUnique;
}
EXPORT(bool) skirmishAiCallback_Unit_SupportedCommand_isDisabled(int teamId, int unitId, int commandId) {
	return team_callback[teamId]->GetUnitCommands(unitId)->at(commandId).disabled;
}
EXPORT(int) skirmishAiCallback_Unit_SupportedCommand_getParams(int teamId,
		int unitId, int commandId, const char** params, int params_sizeMax) {

	const std::vector<std::string> ps = team_callback[teamId]->GetUnitCommands(unitId)->at(commandId).params;
	const size_t params_sizeReal = ps.size();

	size_t params_size = params_sizeReal;

	if (params != NULL) {
		params_size = min(params_sizeReal, params_sizeMax);
		int p;
		for (p=0; p < params_size; p++) {
			params[p] = ps.at(p).c_str();
		}
	}

	return params_size;
}

EXPORT(int) skirmishAiCallback_Unit_getStockpile(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId];
	int stockpile;
	const bool fetchOk = clb->GetProperty(unitId, AIVAL_STOCKPILED, &stockpile);
	if (!fetchOk) {
		stockpile = -1;
	}
	return stockpile;
}
EXPORT(int) skirmishAiCallback_Unit_getStockpileQueued(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId];
	int stockpileQueue;
	const bool fetchOk = clb->GetProperty(unitId, AIVAL_STOCKPILE_QUED, &stockpileQueue);
	if (!fetchOk) {
		stockpileQueue = -1;
	}
	return stockpileQueue;
}
EXPORT(float) skirmishAiCallback_Unit_getCurrentFuel(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId];
	float currentFuel;
	const bool fetchOk = clb->GetProperty(unitId, AIVAL_CURRENT_FUEL, &currentFuel);
	if (!fetchOk) {
		currentFuel = -1.0f;
	}
	return currentFuel;
}
EXPORT(float) skirmishAiCallback_Unit_getMaxSpeed(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId];
	float maxSpeed;
	const bool fetchOk = clb->GetProperty(unitId, AIVAL_UNIT_MAXSPEED, &maxSpeed);
	if (!fetchOk) {
		maxSpeed = -1.0f;
	}
	return maxSpeed;
}

EXPORT(float) skirmishAiCallback_Unit_getMaxRange(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetUnitMaxRange(unitId);
}

EXPORT(float) skirmishAiCallback_Unit_getMaxHealth(int teamId, int unitId) {
//	return team_callback[teamId]->GetUnitMaxHealth(unitId);
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitMaxHealth(unitId);
	} else {
		return team_callback[teamId]->GetUnitMaxHealth(unitId);
	}
}


/**
 * Returns a units command queue.
 * The return value may be <code>NULL</code> in some cases,
 * eg. when cheats are disabled and we try to fetch from an enemy unit.
 * For internal use only.
 */
static inline const CCommandQueue* _intern_Unit_getCurrentCommandQueue(int teamId, int unitId) {
	const CCommandQueue* q = NULL;

	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		q = team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId);
	} else {
		q = team_callback[teamId]->GetCurrentUnitCommands(unitId);
	}

	return q;
}
/**
 * Checks if a given commandId is valid for a commandQueue.
 * For internal use only.
 */
#define CHECK_COMMAND_ID(commandQueue, commandId) \
		(commandQueue != NULL && \
			commandId >= 0 && \
			static_cast<unsigned int>(commandId) < commandQueue->size())

EXPORT(int) skirmishAiCallback_Unit_getCurrentCommands(int teamId, int unitId) {
	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(teamId, unitId);
	return (q? q->size(): 0);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getType(int teamId, int unitId) {
	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(teamId, unitId);
	return (q? q->GetType(): -1);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getId(int teamId, int unitId, int commandId) {

	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(teamId, unitId);
	return (CHECK_COMMAND_ID(q, commandId) ? q->at(commandId).id : 0);
}

EXPORT(short) skirmishAiCallback_Unit_CurrentCommand_getOptions(int teamId, int unitId, int commandId) {

	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(teamId, unitId);
	return (CHECK_COMMAND_ID(q, commandId) ? q->at(commandId).options : 0);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getTag(int teamId, int unitId, int commandId) {

	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(teamId, unitId);
	return (CHECK_COMMAND_ID(q, commandId) ? q->at(commandId).tag : 0);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getTimeOut(int teamId, int unitId, int commandId) {

	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(teamId, unitId);
	return (CHECK_COMMAND_ID(q, commandId) ? q->at(commandId).timeOut : 0);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getParams(int teamId,
		int unitId, int commandId, float* params, int params_sizeMax) {

	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(teamId, unitId);

	if (!CHECK_COMMAND_ID(q, commandId)) {
		return -1;
	}

	const std::vector<float>& ps = q->at(commandId).params;
	const size_t params_sizeReal = ps.size();

	size_t params_size = params_sizeReal;

	if (params != NULL) {
		params_size = min(params_sizeReal, params_sizeMax);

		for (size_t p=0; p < params_size; p++) {
			params[p] = ps.at(p);
		}
	}

	return params_size;
}
#undef CHECK_COMMAND_ID



EXPORT(float) skirmishAiCallback_Unit_getExperience(int teamId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitExperience(unitId);
	} else {
		return team_callback[teamId]->GetUnitExperience(unitId);
	}
}
EXPORT(int) skirmishAiCallback_Unit_getGroup(int teamId, int unitId) {
	return team_callback[teamId]->GetUnitGroup(unitId);
}

EXPORT(float) skirmishAiCallback_Unit_getHealth(int teamId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitHealth(unitId);
	} else {
		return team_callback[teamId]->GetUnitHealth(unitId);
	}
}
EXPORT(float) skirmishAiCallback_Unit_getSpeed(int teamId, int unitId) {
	return team_callback[teamId]->GetUnitSpeed(unitId);
}
EXPORT(float) skirmishAiCallback_Unit_getPower(int teamId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitPower(unitId);
	} else {
		return team_callback[teamId]->GetUnitPower(unitId);
	}
}
EXPORT(void) skirmishAiCallback_Unit_getPos(int teamId, int unitId, float* return_posF3_out) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		team_cheatCallback[teamId]->GetUnitPos(unitId).copyInto(return_posF3_out);
	} else {
		team_callback[teamId]->GetUnitPos(unitId).copyInto(return_posF3_out);
	}
}
//EXPORT(int) skirmishAiCallback_Unit_0MULTI1SIZE0ResourceInfo(int teamId, int unitId) {
//	return skirmishAiCallback_0MULTI1SIZE0Resource(teamId);
//}
EXPORT(float) skirmishAiCallback_Unit_getResourceUse(int teamId,
		int unitId, int resourceId) {

	int res = -1.0F;
	UnitResourceInfo resourceInfo;

	bool fetchOk;
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		fetchOk = team_cheatCallback[teamId]->GetUnitResourceInfo(unitId,
				&resourceInfo);
	} else {
		fetchOk = team_callback[teamId]->GetUnitResourceInfo(unitId,
				&resourceInfo);
	}
	if (fetchOk) {
		if (resourceId == resourceHandler->GetMetalId()) {
			res = resourceInfo.metalUse;
		} else if (resourceId == resourceHandler->GetEnergyId()) {
			res = resourceInfo.energyUse;
		}
	}

	return res;
}
EXPORT(float) skirmishAiCallback_Unit_getResourceMake(int teamId,
		int unitId, int resourceId) {

	int res = -1.0F;
	UnitResourceInfo resourceInfo;

	bool fetchOk;
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		fetchOk = team_cheatCallback[teamId]->GetUnitResourceInfo(unitId,
				&resourceInfo);
	} else {
		fetchOk = team_callback[teamId]->GetUnitResourceInfo(unitId,
				&resourceInfo);
	}
	if (fetchOk) {
		if (resourceId == resourceHandler->GetMetalId()) {
			res = resourceInfo.metalMake;
		} else if (resourceId == resourceHandler->GetEnergyId()) {
			res = resourceInfo.energyMake;
		}
	}

	return res;
}
EXPORT(bool) skirmishAiCallback_Unit_isActivated(int teamId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->IsUnitActivated(unitId);
	} else {
		return team_callback[teamId]->IsUnitActivated(unitId);
	}
}
EXPORT(bool) skirmishAiCallback_Unit_isBeingBuilt(int teamId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->UnitBeingBuilt(unitId);
	} else {
		return team_callback[teamId]->UnitBeingBuilt(unitId);
	}
}
EXPORT(bool) skirmishAiCallback_Unit_isCloaked(int teamId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->IsUnitCloaked(unitId);
	} else {
		return team_callback[teamId]->IsUnitCloaked(unitId);
	}
}
EXPORT(bool) skirmishAiCallback_Unit_isParalyzed(int teamId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->IsUnitParalyzed(unitId);
	} else {
		return team_callback[teamId]->IsUnitParalyzed(unitId);
	}
}
EXPORT(bool) skirmishAiCallback_Unit_isNeutral(int teamId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->IsUnitNeutral(unitId);
	} else {
		return team_callback[teamId]->IsUnitNeutral(unitId);
	}
}
EXPORT(int) skirmishAiCallback_Unit_getBuildingFacing(int teamId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetBuildingFacing(unitId);
	} else {
		return team_callback[teamId]->GetBuildingFacing(unitId);
	}
}
EXPORT(int) skirmishAiCallback_Unit_getLastUserOrderFrame(int teamId, int unitId) {

	if (!isControlledByLocalPlayer(teamId)) return -1;

	return uh->units[unitId]->commandAI->lastUserCommand;
}
//########### END Unit

EXPORT(int) skirmishAiCallback_getEnemyUnits(int teamId, int* unitIds, int unitIds_sizeMax) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetEnemyUnits(unitIds, unitIds_sizeMax);
	} else {
		return team_callback[teamId]->GetEnemyUnits(unitIds, unitIds_sizeMax);
	}
}

EXPORT(int) skirmishAiCallback_getEnemyUnitsIn(int teamId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetEnemyUnits(unitIds, pos_posF3, radius, unitIds_sizeMax);
	} else {
		return team_callback[teamId]->GetEnemyUnits(unitIds, pos_posF3, radius, unitIds_sizeMax);
	}
}

EXPORT(int) skirmishAiCallback_getEnemyUnitsInRadarAndLos(int teamId, int* unitIds, int unitIds_sizeMax) {
	return team_callback[teamId]->GetEnemyUnitsInRadarAndLos(unitIds, unitIds_sizeMax);
}

EXPORT(int) skirmishAiCallback_getFriendlyUnits(int teamId, int* unitIds, int unitIds_sizeMax) {
	return team_callback[teamId]->GetFriendlyUnits(unitIds);
}

EXPORT(int) skirmishAiCallback_getFriendlyUnitsIn(int teamId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax) {
	return team_callback[teamId]->GetFriendlyUnits(unitIds, pos_posF3, radius, unitIds_sizeMax);
}

EXPORT(int) skirmishAiCallback_getNeutralUnits(int teamId, int* unitIds, int unitIds_sizeMax) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetNeutralUnits(unitIds, unitIds_sizeMax);
	} else {
		return team_callback[teamId]->GetNeutralUnits(unitIds, unitIds_sizeMax);
	}
}

EXPORT(int) skirmishAiCallback_getNeutralUnitsIn(int teamId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetNeutralUnits(unitIds, pos_posF3, radius, unitIds_sizeMax);
	} else {
		return team_callback[teamId]->GetNeutralUnits(unitIds, pos_posF3, radius, unitIds_sizeMax);
	}
}

EXPORT(int) skirmishAiCallback_getSelectedUnits(int teamId, int* unitIds, int unitIds_sizeMax) {
	return team_callback[teamId]->GetSelectedUnits(unitIds, unitIds_sizeMax);
}

EXPORT(int) skirmishAiCallback_getTeamUnits(int teamId, int* unitIds, int unitIds_sizeMax) {

	int a = 0;

	for (std::list<CUnit*>::iterator ui = uh->activeUnits.begin();
			ui != uh->activeUnits.end(); ++ui) {
		CUnit* u = *ui;

		if (u->team == teamId) {
			if (a < unitIds_sizeMax) {
				if (unitIds != NULL) {
					unitIds[a] = u->id;
				}
				a++;
			} else {
				break;
			}
		}
	}

	return a;
}

//########### BEGINN FeatureDef
EXPORT(int) skirmishAiCallback_getFeatureDefs(int teamId, int* featureDefIds, int featureDefIds_sizeMax) {

	const std::map<std::string, const FeatureDef*>& fds
			= featureHandler->GetFeatureDefs();
	const int featureDefIds_sizeReal = fds.size();

	int featureDefIds_size = featureDefIds_sizeReal;

	if (featureDefIds != NULL) {
		featureDefIds_size = min(featureDefIds_sizeReal, featureDefIds_sizeMax);
		int f;
		std::map<std::string, const FeatureDef*>::const_iterator fdi;
		for (f=0, fdi=fds.begin(); f < featureDefIds_size; ++f, ++fdi) {
			featureDefIds[f] = fdi->second->id;
		}
	}

	return featureDefIds_size;
}

EXPORT(const char*) skirmishAiCallback_FeatureDef_getName(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->myName.c_str();}
EXPORT(const char*) skirmishAiCallback_FeatureDef_getDescription(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->description.c_str();}
EXPORT(const char*) skirmishAiCallback_FeatureDef_getFileName(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->filename.c_str();}
//EXPORT(int) skirmishAiCallback_FeatureDef_getId(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->id;}
EXPORT(float) skirmishAiCallback_FeatureDef_getContainedResource(int teamId, int featureDefId, int resourceId) {

	const FeatureDef* fd = getFeatureDefById(teamId, featureDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return fd->metal;
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return fd->energy;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_FeatureDef_getMaxHealth(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->maxHealth;}
EXPORT(float) skirmishAiCallback_FeatureDef_getReclaimTime(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->reclaimTime;}
EXPORT(float) skirmishAiCallback_FeatureDef_getMass(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->mass;}
EXPORT(const char*) skirmishAiCallback_FeatureDef_CollisionVolume_getType(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->collisionVolumeTypeStr.c_str();}
EXPORT(void) skirmishAiCallback_FeatureDef_CollisionVolume_getScales(int teamId, int featureDefId, float* return_posF3_out) {
	getFeatureDefById(teamId, featureDefId)->collisionVolumeScales.copyInto(return_posF3_out);
}
EXPORT(void) skirmishAiCallback_FeatureDef_CollisionVolume_getOffsets(int teamId, int featureDefId, float* return_posF3_out) {
	getFeatureDefById(teamId, featureDefId)->collisionVolumeOffsets.copyInto(return_posF3_out);
}
EXPORT(int) skirmishAiCallback_FeatureDef_CollisionVolume_getTest(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->collisionVolumeTest;}
EXPORT(bool) skirmishAiCallback_FeatureDef_isUpright(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->upright;}
EXPORT(int) skirmishAiCallback_FeatureDef_getDrawType(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->drawType;}
EXPORT(const char*) skirmishAiCallback_FeatureDef_getModelName(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->modelname.c_str();}
EXPORT(int) skirmishAiCallback_FeatureDef_getResurrectable(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->resurrectable;}
EXPORT(int) skirmishAiCallback_FeatureDef_getSmokeTime(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->smokeTime;}
EXPORT(bool) skirmishAiCallback_FeatureDef_isDestructable(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->destructable;}
EXPORT(bool) skirmishAiCallback_FeatureDef_isReclaimable(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->reclaimable;}
EXPORT(bool) skirmishAiCallback_FeatureDef_isBlocking(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->blocking;}
EXPORT(bool) skirmishAiCallback_FeatureDef_isBurnable(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->burnable;}
EXPORT(bool) skirmishAiCallback_FeatureDef_isFloating(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->floating;}
EXPORT(bool) skirmishAiCallback_FeatureDef_isNoSelect(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->noSelect;}
EXPORT(bool) skirmishAiCallback_FeatureDef_isGeoThermal(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->geoThermal;}
EXPORT(const char*) skirmishAiCallback_FeatureDef_getDeathFeature(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->deathFeature.c_str();}
EXPORT(int) skirmishAiCallback_FeatureDef_getXSize(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->xsize;}
EXPORT(int) skirmishAiCallback_FeatureDef_getZSize(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->zsize;}
EXPORT(int) skirmishAiCallback_FeatureDef_getCustomParams(int teamId, int featureDefId,
		const char** keys, const char** values) {

	const std::map<std::string,std::string>& ps = getFeatureDefById(teamId, featureDefId)->customParams;
	const size_t params_sizeReal = ps.size();

	if ((keys != NULL) && (values != NULL)) {
		fillCMap(&ps, keys, values);
	}

	return params_sizeReal;
}
//########### END FeatureDef


EXPORT(int) skirmishAiCallback_getFeatures(int teamId, int* featureIds, int featureIds_sizeMax) {

	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		// cheating
		const CFeatureSet& fset = featureHandler->GetActiveFeatures();
		const int featureIds_sizeReal = fset.size();

		int featureIds_size = featureIds_sizeReal;
		
		if (featureIds != NULL) {
			featureIds_size = min(featureIds_sizeReal, featureIds_sizeMax);
			CFeatureSet::const_iterator it;
			size_t f = 0;
			for (it = fset.begin(); it != fset.end() && f < featureIds_size; ++it) {
				CFeature* feature = *it;
				assert(feature);
				featureIds[f++] = feature->id;
			}
		}

		return featureIds_size;
	} else {
		// non cheating
		int featureIds_size = -1;

		if (featureIds == NULL) {
			// only return size
			featureIds_size = team_callback[teamId]->GetFeatures(tmpIntArr[teamId], TMP_ARR_SIZE);
		} else {
			// return size and values
			featureIds_size = team_callback[teamId]->GetFeatures(featureIds, featureIds_sizeMax);
		}

		return featureIds_size;
	}
}

EXPORT(int) skirmishAiCallback_getFeaturesIn(int teamId, float* pos_posF3, float radius, int* featureIds, int featureIds_sizeMax) {

	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		// cheating
		const std::vector<CFeature*>& fset = qf->GetFeaturesExact(pos_posF3, radius);
		const int featureIds_sizeReal = fset.size();

		int featureIds_size = featureIds_sizeReal;
		
		if (featureIds != NULL) {
			featureIds_size = min(featureIds_sizeReal, featureIds_sizeMax);
			std::vector<CFeature*>::const_iterator it;
			size_t f = 0;
			for (it = fset.begin(); it != fset.end() && f < featureIds_size; ++it) {
				CFeature* feature = *it;
				assert(feature);
				featureIds[f++] = feature->id;
			}
		}

		return featureIds_size;
	} else {
		// non cheating
		int featureIds_size = -1;

		if (featureIds == NULL) {
			// only return size
			featureIds_size = team_callback[teamId]->GetFeatures(tmpIntArr[teamId], TMP_ARR_SIZE, pos_posF3, radius);
		} else {
			// return size and values
			featureIds_size = team_callback[teamId]->GetFeatures(featureIds, featureIds_sizeMax, pos_posF3, radius);
		}

		return featureIds_size;
	}
}

EXPORT(int) skirmishAiCallback_Feature_getDef(int teamId, int featureId) {

	const FeatureDef* def = team_callback[teamId]->GetFeatureDef(featureId);
	if (def == NULL) {
		 return -1;
	} else {
		return def->id;
	}
}

EXPORT(float) skirmishAiCallback_Feature_getHealth(int teamId, int featureId) {
	return team_callback[teamId]->GetFeatureHealth(featureId);
}

EXPORT(float) skirmishAiCallback_Feature_getReclaimLeft(int teamId, int featureId) {
	return team_callback[teamId]->GetFeatureReclaimLeft(featureId);
}

EXPORT(void) skirmishAiCallback_Feature_getPosition(int teamId, int featureId, float* return_posF3_out) {
	team_callback[teamId]->GetFeaturePos(featureId).copyInto(return_posF3_out);
}


//########### BEGINN WeaponDef
EXPORT(int) skirmishAiCallback_getWeaponDefs(int teamId) {
	return weaponDefHandler->numWeaponDefs;
}
EXPORT(int) skirmishAiCallback_getWeaponDefByName(int teamId, const char* weaponDefName) {

	int weaponDefId = -1;

	const WeaponDef* wd = team_callback[teamId]->GetWeapon(weaponDefName);
	if (wd != NULL) {
		weaponDefId = wd->id;
	}

	return weaponDefId;
}
//EXPORT(int) skirmishAiCallback_WeaponDef_getId(int teamId, int weaponDefId) {
//	return getWeaponDefById(teamId, weaponDefId)->id;
//}
EXPORT(const char*) skirmishAiCallback_WeaponDef_getName(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->name.c_str();
}
EXPORT(const char*) skirmishAiCallback_WeaponDef_getType(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->type.c_str();
}
EXPORT(const char*) skirmishAiCallback_WeaponDef_getDescription(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->description.c_str();
}
EXPORT(const char*) skirmishAiCallback_WeaponDef_getFileName(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->filename.c_str();
}
EXPORT(const char*) skirmishAiCallback_WeaponDef_getCegTag(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->cegTag.c_str();
}
EXPORT(float) skirmishAiCallback_WeaponDef_getRange(int teamId, int weaponDefId) {
return getWeaponDefById(teamId, weaponDefId)->range;
}
EXPORT(float) skirmishAiCallback_WeaponDef_getHeightMod(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->heightmod;
}
EXPORT(float) skirmishAiCallback_WeaponDef_getAccuracy(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->accuracy;
}
EXPORT(float) skirmishAiCallback_WeaponDef_getSprayAngle(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->sprayAngle;
}
EXPORT(float) skirmishAiCallback_WeaponDef_getMovingAccuracy(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->movingAccuracy;
}
EXPORT(float) skirmishAiCallback_WeaponDef_getTargetMoveError(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->targetMoveError;
}
EXPORT(float) skirmishAiCallback_WeaponDef_getLeadLimit(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->leadLimit;
}
EXPORT(float) skirmishAiCallback_WeaponDef_getLeadBonus(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->leadBonus;
}
EXPORT(float) skirmishAiCallback_WeaponDef_getPredictBoost(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->predictBoost;
}
EXPORT(int) skirmishAiCallback_WeaponDef_Damage_getParalyzeDamageTime(int teamId, int weaponDefId) {
	DamageArray da = getWeaponDefById(teamId, weaponDefId)->damages;
	return da.paralyzeDamageTime;
}
EXPORT(float) skirmishAiCallback_WeaponDef_Damage_getImpulseFactor(int teamId, int weaponDefId) {
	DamageArray da = getWeaponDefById(teamId, weaponDefId)->damages;
	return da.impulseFactor;
}
EXPORT(float) skirmishAiCallback_WeaponDef_Damage_getImpulseBoost(int teamId, int weaponDefId) {
	DamageArray da = getWeaponDefById(teamId, weaponDefId)->damages;
	return da.impulseBoost;
}
EXPORT(float) skirmishAiCallback_WeaponDef_Damage_getCraterMult(int teamId, int weaponDefId) {
	DamageArray da = getWeaponDefById(teamId, weaponDefId)->damages;
	return da.craterMult;
}
EXPORT(float) skirmishAiCallback_WeaponDef_Damage_getCraterBoost(int teamId, int weaponDefId) {
	DamageArray da = getWeaponDefById(teamId, weaponDefId)->damages;
	return da.craterBoost;
}
EXPORT(int) skirmishAiCallback_WeaponDef_Damage_getTypes(int teamId, int weaponDefId, float* types, int types_sizeMax) {

	const WeaponDef* weaponDef = getWeaponDefById(teamId, weaponDefId);
	const size_t types_sizeReal = weaponDef->damages.GetNumTypes();

	size_t types_size = types_sizeReal;

	if (types != NULL) {
		types_size = min(types_sizeReal, types_sizeMax);
	
		for (size_t i=0; i < types_size; ++i) {
			types[i] = weaponDef->damages[i];
		}
	}

	return types_size;
}
EXPORT(float) skirmishAiCallback_WeaponDef_getAreaOfEffect(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->areaOfEffect;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isNoSelfDamage(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->noSelfDamage;}
EXPORT(float) skirmishAiCallback_WeaponDef_getFireStarter(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->fireStarter;}
EXPORT(float) skirmishAiCallback_WeaponDef_getEdgeEffectiveness(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->edgeEffectiveness;}
EXPORT(float) skirmishAiCallback_WeaponDef_getSize(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->size;}
EXPORT(float) skirmishAiCallback_WeaponDef_getSizeGrowth(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->sizeGrowth;}
EXPORT(float) skirmishAiCallback_WeaponDef_getCollisionSize(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->collisionSize;}
EXPORT(int) skirmishAiCallback_WeaponDef_getSalvoSize(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->salvosize;}
EXPORT(float) skirmishAiCallback_WeaponDef_getSalvoDelay(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->salvodelay;}
EXPORT(float) skirmishAiCallback_WeaponDef_getReload(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->reload;}
EXPORT(float) skirmishAiCallback_WeaponDef_getBeamTime(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->beamtime;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isBeamBurst(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->beamburst;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isWaterBounce(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->waterBounce;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isGroundBounce(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->groundBounce;}
EXPORT(float) skirmishAiCallback_WeaponDef_getBounceRebound(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->bounceRebound;}
EXPORT(float) skirmishAiCallback_WeaponDef_getBounceSlip(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->bounceSlip;}
EXPORT(int) skirmishAiCallback_WeaponDef_getNumBounce(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->numBounce;}
EXPORT(float) skirmishAiCallback_WeaponDef_getMaxAngle(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->maxAngle;}
EXPORT(float) skirmishAiCallback_WeaponDef_getRestTime(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->restTime;}
EXPORT(float) skirmishAiCallback_WeaponDef_getUpTime(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->uptime;}
EXPORT(int) skirmishAiCallback_WeaponDef_getFlightTime(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->flighttime;}
EXPORT(float) skirmishAiCallback_WeaponDef_getCost(int teamId, int weaponDefId, int resourceId) {

	const WeaponDef* wd = getWeaponDefById(teamId, weaponDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return wd->metalcost;
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return wd->energycost;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_WeaponDef_getSupplyCost(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->supplycost;}
EXPORT(int) skirmishAiCallback_WeaponDef_getProjectilesPerShot(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->projectilespershot;}
//EXPORT(int) skirmishAiCallback_WeaponDef_getTdfId(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->tdfId;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isTurret(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->turret;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isOnlyForward(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->onlyForward;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isFixedLauncher(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->fixedLauncher;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isWaterWeapon(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->waterweapon;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isFireSubmersed(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->fireSubmersed;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isSubMissile(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->submissile;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isTracks(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->tracks;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isDropped(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dropped;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isParalyzer(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->paralyzer;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isImpactOnly(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->impactOnly;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isNoAutoTarget(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->noAutoTarget;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isManualFire(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->manualfire;}
EXPORT(int) skirmishAiCallback_WeaponDef_getInterceptor(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->interceptor;}
EXPORT(int) skirmishAiCallback_WeaponDef_getTargetable(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->targetable;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isStockpileable(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->stockpile;}
EXPORT(float) skirmishAiCallback_WeaponDef_getCoverageRange(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->coverageRange;}
EXPORT(float) skirmishAiCallback_WeaponDef_getStockpileTime(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->stockpileTime;}
EXPORT(float) skirmishAiCallback_WeaponDef_getIntensity(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->intensity;}
EXPORT(float) skirmishAiCallback_WeaponDef_getThickness(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->thickness;}
EXPORT(float) skirmishAiCallback_WeaponDef_getLaserFlareSize(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->laserflaresize;}
EXPORT(float) skirmishAiCallback_WeaponDef_getCoreThickness(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->corethickness;}
EXPORT(float) skirmishAiCallback_WeaponDef_getDuration(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->duration;}
EXPORT(int) skirmishAiCallback_WeaponDef_getLodDistance(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->lodDistance;}
EXPORT(float) skirmishAiCallback_WeaponDef_getFalloffRate(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->falloffRate;}
EXPORT(int) skirmishAiCallback_WeaponDef_getGraphicsType(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->graphicsType;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isSoundTrigger(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->soundTrigger;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isSelfExplode(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->selfExplode;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isGravityAffected(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->gravityAffected;}
EXPORT(int) skirmishAiCallback_WeaponDef_getHighTrajectory(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->highTrajectory;}
EXPORT(float) skirmishAiCallback_WeaponDef_getMyGravity(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->myGravity;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isNoExplode(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->noExplode;}
EXPORT(float) skirmishAiCallback_WeaponDef_getStartVelocity(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->startvelocity;}
EXPORT(float) skirmishAiCallback_WeaponDef_getWeaponAcceleration(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->weaponacceleration;}
EXPORT(float) skirmishAiCallback_WeaponDef_getTurnRate(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->turnrate;}
EXPORT(float) skirmishAiCallback_WeaponDef_getMaxVelocity(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->maxvelocity;}
EXPORT(float) skirmishAiCallback_WeaponDef_getProjectileSpeed(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->projectilespeed;}
EXPORT(float) skirmishAiCallback_WeaponDef_getExplosionSpeed(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->explosionSpeed;}
EXPORT(int) skirmishAiCallback_WeaponDef_getOnlyTargetCategory(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->onlyTargetCategory;}
EXPORT(float) skirmishAiCallback_WeaponDef_getWobble(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->wobble;}
EXPORT(float) skirmishAiCallback_WeaponDef_getDance(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dance;}
EXPORT(float) skirmishAiCallback_WeaponDef_getTrajectoryHeight(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->trajectoryHeight;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isLargeBeamLaser(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->largeBeamLaser;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isShield(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->isShield;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isShieldRepulser(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldRepulser;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isSmartShield(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->smartShield;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isExteriorShield(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->exteriorShield;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isVisibleShield(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->visibleShield;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isVisibleShieldRepulse(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->visibleShieldRepulse;}
EXPORT(int) skirmishAiCallback_WeaponDef_getVisibleShieldHitFrames(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->visibleShieldHitFrames;}
EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getResourceUse(int teamId, int weaponDefId, int resourceId) {

	const WeaponDef* wd = getWeaponDefById(teamId, weaponDefId);
	if (resourceId == resourceHandler->GetEnergyId()) {
		return wd->shieldEnergyUse;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getRadius(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldRadius;}
EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getForce(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldForce;}
EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getMaxSpeed(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldMaxSpeed;}
EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getPower(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldPower;}
EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getPowerRegen(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldPowerRegen;}
EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getPowerRegenResource(int teamId, int weaponDefId, int resourceId) {

	const WeaponDef* wd = getWeaponDefById(teamId, weaponDefId);
	if (resourceId == resourceHandler->GetEnergyId()) {
		return wd->shieldPowerRegenEnergy;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getStartingPower(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldStartingPower;}
EXPORT(int) skirmishAiCallback_WeaponDef_Shield_getRechargeDelay(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldRechargeDelay;}
EXPORT(void) skirmishAiCallback_WeaponDef_Shield_getGoodColor(int teamId, int weaponDefId, short* return_colorS3_out) {

	const float3& color = getWeaponDefById(teamId, weaponDefId)->shieldGoodColor;
	return_colorS3_out[0] = color.x;
	return_colorS3_out[1] = color.y;
	return_colorS3_out[2] = color.z;
}
EXPORT(void) skirmishAiCallback_WeaponDef_Shield_getBadColor(int teamId, int weaponDefId, short* return_colorS3_out) {

	const float3& color = getWeaponDefById(teamId, weaponDefId)->shieldBadColor;
	return_colorS3_out[0] = color.x;
	return_colorS3_out[1] = color.y;
	return_colorS3_out[2] = color.z;
}
EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getAlpha(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldAlpha;}
EXPORT(int) skirmishAiCallback_WeaponDef_Shield_getInterceptType(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldInterceptType;}
EXPORT(int) skirmishAiCallback_WeaponDef_getInterceptedByShieldType(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->interceptedByShieldType;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isAvoidFriendly(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->avoidFriendly;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isAvoidFeature(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->avoidFeature;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isAvoidNeutral(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->avoidNeutral;}
EXPORT(float) skirmishAiCallback_WeaponDef_getTargetBorder(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->targetBorder;}
EXPORT(float) skirmishAiCallback_WeaponDef_getCylinderTargetting(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->cylinderTargetting;}
EXPORT(float) skirmishAiCallback_WeaponDef_getMinIntensity(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->minIntensity;}
EXPORT(float) skirmishAiCallback_WeaponDef_getHeightBoostFactor(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->heightBoostFactor;}
EXPORT(float) skirmishAiCallback_WeaponDef_getProximityPriority(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->proximityPriority;}
EXPORT(int) skirmishAiCallback_WeaponDef_getCollisionFlags(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->collisionFlags;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isSweepFire(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->sweepFire;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isAbleToAttackGround(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->canAttackGround;}
EXPORT(float) skirmishAiCallback_WeaponDef_getCameraShake(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->cameraShake;}
EXPORT(float) skirmishAiCallback_WeaponDef_getDynDamageExp(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dynDamageExp;}
EXPORT(float) skirmishAiCallback_WeaponDef_getDynDamageMin(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dynDamageMin;}
EXPORT(float) skirmishAiCallback_WeaponDef_getDynDamageRange(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dynDamageRange;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isDynDamageInverted(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dynDamageInverted;}
EXPORT(int) skirmishAiCallback_WeaponDef_getCustomParams(int teamId, int weaponDefId,
		const char** keys, const char** values) {

	const std::map<std::string,std::string>& ps = getWeaponDefById(teamId, weaponDefId)->customParams;
	const size_t params_sizeReal = ps.size();

	if ((keys != NULL) && (values != NULL)) {
		fillCMap(&ps, keys, values);
	}

	return params_sizeReal;
}
//########### END WeaponDef


EXPORT(int) skirmishAiCallback_getGroups(int teamId, int* groupIds, int groupIds_sizeMax) {

	const std::vector<CGroup*>& gs = grouphandlers[teamId]->groups;
	const int groupIds_sizeReal = gs.size();

	int groupIds_size = groupIds_sizeReal;

	if (groupIds != NULL) {
		groupIds_size = min(groupIds_sizeReal, groupIds_sizeMax);
		int g;
		for (g=0; g < groupIds_size; ++g) {
			groupIds[g] = gs[g]->id;
		}
	}

	return groupIds_size;
}

EXPORT(int) skirmishAiCallback_Group_getSupportedCommands(int teamId, int groupId) {
	return team_callback[teamId]->GetGroupCommands(groupId)->size();
}
EXPORT(int) skirmishAiCallback_Group_SupportedCommand_getId(int teamId, int groupId, int commandId) {
	return team_callback[teamId]->GetGroupCommands(groupId)->at(commandId).id;
}
EXPORT(const char*) skirmishAiCallback_Group_SupportedCommand_getName(int teamId, int groupId, int commandId) {
	return team_callback[teamId]->GetGroupCommands(groupId)->at(commandId).name.c_str();
}
EXPORT(const char*) skirmishAiCallback_Group_SupportedCommand_getToolTip(int teamId, int groupId, int commandId) {
	return team_callback[teamId]->GetGroupCommands(groupId)->at(commandId).tooltip.c_str();
}
EXPORT(bool) skirmishAiCallback_Group_SupportedCommand_isShowUnique(int teamId, int groupId, int commandId) {
	return team_callback[teamId]->GetGroupCommands(groupId)->at(commandId).showUnique;
}
EXPORT(bool) skirmishAiCallback_Group_SupportedCommand_isDisabled(int teamId, int groupId, int commandId) {
	return team_callback[teamId]->GetGroupCommands(groupId)->at(commandId).disabled;
}
EXPORT(int) skirmishAiCallback_Group_SupportedCommand_getParams(int teamId,
		int groupId, int commandId, const char** params, int params_sizeMax) {

	const std::vector<std::string> ps = team_callback[teamId]->GetGroupCommands(groupId)->at(commandId).params;
	const size_t params_sizeReal = ps.size();

	size_t params_size = params_sizeReal;

	if (params != NULL) {
		params_size = min(params_sizeReal, params_sizeMax);
	
		for (size_t p=0; p < params_size; ++p) {
			params[p] = ps.at(p).c_str();
		}
	}

	return params_size;
}

EXPORT(int) skirmishAiCallback_Group_OrderPreview_getId(int teamId, int groupId) {

	if (!isControlledByLocalPlayer(teamId)) return -1;

	//TODO: need to add support for new gui
	Command tmpCmd = guihandler->GetOrderPreview();
	return tmpCmd.id;
}
EXPORT(short) skirmishAiCallback_Group_OrderPreview_getOptions(int teamId, int groupId) {

	if (!isControlledByLocalPlayer(teamId)) return '\0';

	//TODO: need to add support for new gui
	Command tmpCmd = guihandler->GetOrderPreview();
	return tmpCmd.options;
}
EXPORT(int) skirmishAiCallback_Group_OrderPreview_getTag(int teamId, int groupId) {

	if (!isControlledByLocalPlayer(teamId)) return 0;

	//TODO: need to add support for new gui
	Command tmpCmd = guihandler->GetOrderPreview();
	return tmpCmd.tag;
}
EXPORT(int) skirmishAiCallback_Group_OrderPreview_getTimeOut(int teamId, int groupId) {

	if (!isControlledByLocalPlayer(teamId)) return -1;

	//TODO: need to add support for new gui
	Command tmpCmd = guihandler->GetOrderPreview();
	return tmpCmd.timeOut;
}
EXPORT(int) skirmishAiCallback_Group_OrderPreview_getParams(int teamId,
		int groupId, float* params, int params_sizeMax) {

	if (!isControlledByLocalPlayer(teamId)) { return 0; }

	const std::vector<float>& ps = guihandler->GetOrderPreview().params;
	const size_t params_sizeReal = ps.size();

	size_t params_size = params_sizeReal;

	if (params != NULL) {
		// TODO: need to add support for new gui
		params_size = min(params_sizeReal, params_sizeMax);
		int p;
		for (p = 0; p < params_size; p++) {
			params[p] = ps[p];
		}
	}

	return params_size;
}

EXPORT(bool) skirmishAiCallback_Group_isSelected(int teamId, int groupId) {

	if (!isControlledByLocalPlayer(teamId)) return false;

	return selectedUnits.selectedGroup == groupId;
}
//##############################################################################




static void skirmishAiCallback_init(SSkirmishAICallback* callback) {
	//! register function pointers to the accessors
	callback->Engine_handleCommand = &skirmishAiCallback_Engine_handleCommand;
	callback->Engine_Version_getMajor = &skirmishAiCallback_Engine_Version_getMajor;
	callback->Engine_Version_getMinor = &skirmishAiCallback_Engine_Version_getMinor;
	callback->Engine_Version_getPatchset = &skirmishAiCallback_Engine_Version_getPatchset;
	callback->Engine_Version_getAdditional = &skirmishAiCallback_Engine_Version_getAdditional;
	callback->Engine_Version_getBuildTime = &skirmishAiCallback_Engine_Version_getBuildTime;
	callback->Engine_Version_getNormal = &skirmishAiCallback_Engine_Version_getNormal;
	callback->Engine_Version_getFull = &skirmishAiCallback_Engine_Version_getFull;
	callback->Teams_getSize = &skirmishAiCallback_Teams_getSize;
	callback->SkirmishAIs_getSize = &skirmishAiCallback_SkirmishAIs_getSize;
	callback->SkirmishAIs_getMax = &skirmishAiCallback_SkirmishAIs_getMax;
	callback->SkirmishAI_Info_getSize = &skirmishAiCallback_SkirmishAI_Info_getSize;
	callback->SkirmishAI_Info_getKey = &skirmishAiCallback_SkirmishAI_Info_getKey;
	callback->SkirmishAI_Info_getValue = &skirmishAiCallback_SkirmishAI_Info_getValue;
	callback->SkirmishAI_Info_getDescription = &skirmishAiCallback_SkirmishAI_Info_getDescription;
	callback->SkirmishAI_Info_getValueByKey = &skirmishAiCallback_SkirmishAI_Info_getValueByKey;
	callback->SkirmishAI_OptionValues_getSize = &skirmishAiCallback_SkirmishAI_OptionValues_getSize;
	callback->SkirmishAI_OptionValues_getKey = &skirmishAiCallback_SkirmishAI_OptionValues_getKey;
	callback->SkirmishAI_OptionValues_getValue = &skirmishAiCallback_SkirmishAI_OptionValues_getValue;
	callback->SkirmishAI_OptionValues_getValueByKey = &skirmishAiCallback_SkirmishAI_OptionValues_getValueByKey;
	callback->Log_log = &skirmishAiCallback_Log_log;
	callback->Log_exception = &skirmishAiCallback_Log_exception;
	callback->DataDirs_getPathSeparator = &skirmishAiCallback_DataDirs_getPathSeparator;
	callback->DataDirs_getConfigDir = &skirmishAiCallback_DataDirs_getConfigDir;
	callback->DataDirs_getWriteableDir = &skirmishAiCallback_DataDirs_getWriteableDir;
	callback->DataDirs_locatePath = &skirmishAiCallback_DataDirs_locatePath;
	callback->DataDirs_allocatePath = &skirmishAiCallback_DataDirs_allocatePath;
	callback->DataDirs_Roots_getSize = &skirmishAiCallback_DataDirs_Roots_getSize;
	callback->DataDirs_Roots_getDir = &skirmishAiCallback_DataDirs_Roots_getDir;
	callback->DataDirs_Roots_locatePath = &skirmishAiCallback_DataDirs_Roots_locatePath;
	callback->DataDirs_Roots_allocatePath = &skirmishAiCallback_DataDirs_Roots_allocatePath;
	callback->Game_getCurrentFrame = &skirmishAiCallback_Game_getCurrentFrame;
	callback->Game_getAiInterfaceVersion = &skirmishAiCallback_Game_getAiInterfaceVersion;
	callback->Game_getMyTeam = &skirmishAiCallback_Game_getMyTeam;
	callback->Game_getMyAllyTeam = &skirmishAiCallback_Game_getMyAllyTeam;
	callback->Game_getPlayerTeam = &skirmishAiCallback_Game_getPlayerTeam;
	callback->Game_getTeamSide = &skirmishAiCallback_Game_getTeamSide;
	callback->Game_isExceptionHandlingEnabled = &skirmishAiCallback_Game_isExceptionHandlingEnabled;
	callback->Game_isDebugModeEnabled = &skirmishAiCallback_Game_isDebugModeEnabled;
	callback->Game_getMode = &skirmishAiCallback_Game_getMode;
	callback->Game_isPaused = &skirmishAiCallback_Game_isPaused;
	callback->Game_getSpeedFactor = &skirmishAiCallback_Game_getSpeedFactor;
	callback->Game_getSetupScript = &skirmishAiCallback_Game_getSetupScript;
	callback->Gui_getViewRange = &skirmishAiCallback_Gui_getViewRange;
	callback->Gui_getScreenX = &skirmishAiCallback_Gui_getScreenX;
	callback->Gui_getScreenY = &skirmishAiCallback_Gui_getScreenY;
	callback->Gui_Camera_getDirection = &skirmishAiCallback_Gui_Camera_getDirection;
	callback->Gui_Camera_getPosition = &skirmishAiCallback_Gui_Camera_getPosition;
	callback->Cheats_isEnabled = &skirmishAiCallback_Cheats_isEnabled;
	callback->Cheats_setEnabled = &skirmishAiCallback_Cheats_setEnabled;
	callback->Cheats_setEventsEnabled = &skirmishAiCallback_Cheats_setEventsEnabled;
	callback->Cheats_isOnlyPassive = &skirmishAiCallback_Cheats_isOnlyPassive;
	callback->getResources = &skirmishAiCallback_getResources;
	callback->getResourceByName = &skirmishAiCallback_getResourceByName;
	callback->Resource_getName = &skirmishAiCallback_Resource_getName;
	callback->Resource_getOptimum = &skirmishAiCallback_Resource_getOptimum;
	callback->Economy_getCurrent = &skirmishAiCallback_Economy_getCurrent;
	callback->Economy_getIncome = &skirmishAiCallback_Economy_getIncome;
	callback->Economy_getUsage = &skirmishAiCallback_Economy_getUsage;
	callback->Economy_getStorage = &skirmishAiCallback_Economy_getStorage;
	callback->File_getSize = &skirmishAiCallback_File_getSize;
	callback->File_getContent = &skirmishAiCallback_File_getContent;
	callback->getUnitDefs = &skirmishAiCallback_getUnitDefs;
	callback->getUnitDefByName = &skirmishAiCallback_getUnitDefByName;
	callback->UnitDef_getHeight = &skirmishAiCallback_UnitDef_getHeight;
	callback->UnitDef_getRadius = &skirmishAiCallback_UnitDef_getRadius;
	callback->UnitDef_isValid = &skirmishAiCallback_UnitDef_isValid;
	callback->UnitDef_getName = &skirmishAiCallback_UnitDef_getName;
	callback->UnitDef_getHumanName = &skirmishAiCallback_UnitDef_getHumanName;
	callback->UnitDef_getFileName = &skirmishAiCallback_UnitDef_getFileName;
	callback->UnitDef_getAiHint = &skirmishAiCallback_UnitDef_getAiHint;
	callback->UnitDef_getCobId = &skirmishAiCallback_UnitDef_getCobId;
	callback->UnitDef_getTechLevel = &skirmishAiCallback_UnitDef_getTechLevel;
	callback->UnitDef_getGaia = &skirmishAiCallback_UnitDef_getGaia;
	callback->UnitDef_getUpkeep = &skirmishAiCallback_UnitDef_getUpkeep;
	callback->UnitDef_getResourceMake = &skirmishAiCallback_UnitDef_getResourceMake;
	callback->UnitDef_getMakesResource = &skirmishAiCallback_UnitDef_getMakesResource;
	callback->UnitDef_getCost = &skirmishAiCallback_UnitDef_getCost;
	callback->UnitDef_getExtractsResource = &skirmishAiCallback_UnitDef_getExtractsResource;
	callback->UnitDef_getResourceExtractorRange = &skirmishAiCallback_UnitDef_getResourceExtractorRange;
	callback->UnitDef_getWindResourceGenerator = &skirmishAiCallback_UnitDef_getWindResourceGenerator;
	callback->UnitDef_getTidalResourceGenerator = &skirmishAiCallback_UnitDef_getTidalResourceGenerator;
	callback->UnitDef_getStorage = &skirmishAiCallback_UnitDef_getStorage;
	callback->UnitDef_isSquareResourceExtractor = &skirmishAiCallback_UnitDef_isSquareResourceExtractor;
	callback->UnitDef_isResourceMaker = &skirmishAiCallback_UnitDef_isResourceMaker;
	callback->UnitDef_getBuildTime = &skirmishAiCallback_UnitDef_getBuildTime;
	callback->UnitDef_getAutoHeal = &skirmishAiCallback_UnitDef_getAutoHeal;
	callback->UnitDef_getIdleAutoHeal = &skirmishAiCallback_UnitDef_getIdleAutoHeal;
	callback->UnitDef_getIdleTime = &skirmishAiCallback_UnitDef_getIdleTime;
	callback->UnitDef_getPower = &skirmishAiCallback_UnitDef_getPower;
	callback->UnitDef_getHealth = &skirmishAiCallback_UnitDef_getHealth;
	callback->UnitDef_getCategory = &skirmishAiCallback_UnitDef_getCategory;
	callback->UnitDef_getSpeed = &skirmishAiCallback_UnitDef_getSpeed;
	callback->UnitDef_getTurnRate = &skirmishAiCallback_UnitDef_getTurnRate;
	callback->UnitDef_isTurnInPlace = &skirmishAiCallback_UnitDef_isTurnInPlace;
	callback->UnitDef_getTurnInPlaceDistance = &skirmishAiCallback_UnitDef_getTurnInPlaceDistance;
	callback->UnitDef_getTurnInPlaceSpeedLimit = &skirmishAiCallback_UnitDef_getTurnInPlaceSpeedLimit;
	callback->UnitDef_isUpright = &skirmishAiCallback_UnitDef_isUpright;
	callback->UnitDef_isCollide = &skirmishAiCallback_UnitDef_isCollide;
	callback->UnitDef_getControlRadius = &skirmishAiCallback_UnitDef_getControlRadius;
	callback->UnitDef_getLosRadius = &skirmishAiCallback_UnitDef_getLosRadius;
	callback->UnitDef_getAirLosRadius = &skirmishAiCallback_UnitDef_getAirLosRadius;
	callback->UnitDef_getLosHeight = &skirmishAiCallback_UnitDef_getLosHeight;
	callback->UnitDef_getRadarRadius = &skirmishAiCallback_UnitDef_getRadarRadius;
	callback->UnitDef_getSonarRadius = &skirmishAiCallback_UnitDef_getSonarRadius;
	callback->UnitDef_getJammerRadius = &skirmishAiCallback_UnitDef_getJammerRadius;
	callback->UnitDef_getSonarJamRadius = &skirmishAiCallback_UnitDef_getSonarJamRadius;
	callback->UnitDef_getSeismicRadius = &skirmishAiCallback_UnitDef_getSeismicRadius;
	callback->UnitDef_getSeismicSignature = &skirmishAiCallback_UnitDef_getSeismicSignature;
	callback->UnitDef_isStealth = &skirmishAiCallback_UnitDef_isStealth;
	callback->UnitDef_isSonarStealth = &skirmishAiCallback_UnitDef_isSonarStealth;
	callback->UnitDef_isBuildRange3D = &skirmishAiCallback_UnitDef_isBuildRange3D;
	callback->UnitDef_getBuildDistance = &skirmishAiCallback_UnitDef_getBuildDistance;
	callback->UnitDef_getBuildSpeed = &skirmishAiCallback_UnitDef_getBuildSpeed;
	callback->UnitDef_getReclaimSpeed = &skirmishAiCallback_UnitDef_getReclaimSpeed;
	callback->UnitDef_getRepairSpeed = &skirmishAiCallback_UnitDef_getRepairSpeed;
	callback->UnitDef_getMaxRepairSpeed = &skirmishAiCallback_UnitDef_getMaxRepairSpeed;
	callback->UnitDef_getResurrectSpeed = &skirmishAiCallback_UnitDef_getResurrectSpeed;
	callback->UnitDef_getCaptureSpeed = &skirmishAiCallback_UnitDef_getCaptureSpeed;
	callback->UnitDef_getTerraformSpeed = &skirmishAiCallback_UnitDef_getTerraformSpeed;
	callback->UnitDef_getMass = &skirmishAiCallback_UnitDef_getMass;
	callback->UnitDef_isPushResistant = &skirmishAiCallback_UnitDef_isPushResistant;
	callback->UnitDef_isStrafeToAttack = &skirmishAiCallback_UnitDef_isStrafeToAttack;
	callback->UnitDef_getMinCollisionSpeed = &skirmishAiCallback_UnitDef_getMinCollisionSpeed;
	callback->UnitDef_getSlideTolerance = &skirmishAiCallback_UnitDef_getSlideTolerance;
	callback->UnitDef_getMaxSlope = &skirmishAiCallback_UnitDef_getMaxSlope;
	callback->UnitDef_getMaxHeightDif = &skirmishAiCallback_UnitDef_getMaxHeightDif;
	callback->UnitDef_getMinWaterDepth = &skirmishAiCallback_UnitDef_getMinWaterDepth;
	callback->UnitDef_getWaterline = &skirmishAiCallback_UnitDef_getWaterline;
	callback->UnitDef_getMaxWaterDepth = &skirmishAiCallback_UnitDef_getMaxWaterDepth;
	callback->UnitDef_getArmoredMultiple = &skirmishAiCallback_UnitDef_getArmoredMultiple;
	callback->UnitDef_getArmorType = &skirmishAiCallback_UnitDef_getArmorType;
	callback->UnitDef_FlankingBonus_getMode = &skirmishAiCallback_UnitDef_FlankingBonus_getMode;
	callback->UnitDef_FlankingBonus_getDir = &skirmishAiCallback_UnitDef_FlankingBonus_getDir;
	callback->UnitDef_FlankingBonus_getMax = &skirmishAiCallback_UnitDef_FlankingBonus_getMax;
	callback->UnitDef_FlankingBonus_getMin = &skirmishAiCallback_UnitDef_FlankingBonus_getMin;
	callback->UnitDef_FlankingBonus_getMobilityAdd = &skirmishAiCallback_UnitDef_FlankingBonus_getMobilityAdd;
	callback->UnitDef_CollisionVolume_getType = &skirmishAiCallback_UnitDef_CollisionVolume_getType;
	callback->UnitDef_CollisionVolume_getScales = &skirmishAiCallback_UnitDef_CollisionVolume_getScales;
	callback->UnitDef_CollisionVolume_getOffsets = &skirmishAiCallback_UnitDef_CollisionVolume_getOffsets;
	callback->UnitDef_CollisionVolume_getTest = &skirmishAiCallback_UnitDef_CollisionVolume_getTest;
	callback->UnitDef_getMaxWeaponRange = &skirmishAiCallback_UnitDef_getMaxWeaponRange;
	callback->UnitDef_getType = &skirmishAiCallback_UnitDef_getType;
	callback->UnitDef_getTooltip = &skirmishAiCallback_UnitDef_getTooltip;
	callback->UnitDef_getWreckName = &skirmishAiCallback_UnitDef_getWreckName;
	callback->UnitDef_getDeathExplosion = &skirmishAiCallback_UnitDef_getDeathExplosion;
	callback->UnitDef_getSelfDExplosion = &skirmishAiCallback_UnitDef_getSelfDExplosion;
	callback->UnitDef_getTedClassString = &skirmishAiCallback_UnitDef_getTedClassString;
	callback->UnitDef_getCategoryString = &skirmishAiCallback_UnitDef_getCategoryString;
	callback->UnitDef_isAbleToSelfD = &skirmishAiCallback_UnitDef_isAbleToSelfD;
	callback->UnitDef_getSelfDCountdown = &skirmishAiCallback_UnitDef_getSelfDCountdown;
	callback->UnitDef_isAbleToSubmerge = &skirmishAiCallback_UnitDef_isAbleToSubmerge;
	callback->UnitDef_isAbleToFly = &skirmishAiCallback_UnitDef_isAbleToFly;
	callback->UnitDef_isAbleToMove = &skirmishAiCallback_UnitDef_isAbleToMove;
	callback->UnitDef_isAbleToHover = &skirmishAiCallback_UnitDef_isAbleToHover;
	callback->UnitDef_isFloater = &skirmishAiCallback_UnitDef_isFloater;
	callback->UnitDef_isBuilder = &skirmishAiCallback_UnitDef_isBuilder;
	callback->UnitDef_isActivateWhenBuilt = &skirmishAiCallback_UnitDef_isActivateWhenBuilt;
	callback->UnitDef_isOnOffable = &skirmishAiCallback_UnitDef_isOnOffable;
	callback->UnitDef_isFullHealthFactory = &skirmishAiCallback_UnitDef_isFullHealthFactory;
	callback->UnitDef_isFactoryHeadingTakeoff = &skirmishAiCallback_UnitDef_isFactoryHeadingTakeoff;
	callback->UnitDef_isReclaimable = &skirmishAiCallback_UnitDef_isReclaimable;
	callback->UnitDef_isCapturable = &skirmishAiCallback_UnitDef_isCapturable;
	callback->UnitDef_isAbleToRestore = &skirmishAiCallback_UnitDef_isAbleToRestore;
	callback->UnitDef_isAbleToRepair = &skirmishAiCallback_UnitDef_isAbleToRepair;
	callback->UnitDef_isAbleToSelfRepair = &skirmishAiCallback_UnitDef_isAbleToSelfRepair;
	callback->UnitDef_isAbleToReclaim = &skirmishAiCallback_UnitDef_isAbleToReclaim;
	callback->UnitDef_isAbleToAttack = &skirmishAiCallback_UnitDef_isAbleToAttack;
	callback->UnitDef_isAbleToPatrol = &skirmishAiCallback_UnitDef_isAbleToPatrol;
	callback->UnitDef_isAbleToFight = &skirmishAiCallback_UnitDef_isAbleToFight;
	callback->UnitDef_isAbleToGuard = &skirmishAiCallback_UnitDef_isAbleToGuard;
	callback->UnitDef_isAbleToAssist = &skirmishAiCallback_UnitDef_isAbleToAssist;
	callback->UnitDef_isAssistable = &skirmishAiCallback_UnitDef_isAssistable;
	callback->UnitDef_isAbleToRepeat = &skirmishAiCallback_UnitDef_isAbleToRepeat;
	callback->UnitDef_isAbleToFireControl = &skirmishAiCallback_UnitDef_isAbleToFireControl;
	callback->UnitDef_getFireState = &skirmishAiCallback_UnitDef_getFireState;
	callback->UnitDef_getMoveState = &skirmishAiCallback_UnitDef_getMoveState;
	callback->UnitDef_getWingDrag = &skirmishAiCallback_UnitDef_getWingDrag;
	callback->UnitDef_getWingAngle = &skirmishAiCallback_UnitDef_getWingAngle;
	callback->UnitDef_getDrag = &skirmishAiCallback_UnitDef_getDrag;
	callback->UnitDef_getFrontToSpeed = &skirmishAiCallback_UnitDef_getFrontToSpeed;
	callback->UnitDef_getSpeedToFront = &skirmishAiCallback_UnitDef_getSpeedToFront;
	callback->UnitDef_getMyGravity = &skirmishAiCallback_UnitDef_getMyGravity;
	callback->UnitDef_getMaxBank = &skirmishAiCallback_UnitDef_getMaxBank;
	callback->UnitDef_getMaxPitch = &skirmishAiCallback_UnitDef_getMaxPitch;
	callback->UnitDef_getTurnRadius = &skirmishAiCallback_UnitDef_getTurnRadius;
	callback->UnitDef_getWantedHeight = &skirmishAiCallback_UnitDef_getWantedHeight;
	callback->UnitDef_getVerticalSpeed = &skirmishAiCallback_UnitDef_getVerticalSpeed;
	callback->UnitDef_isAbleToCrash = &skirmishAiCallback_UnitDef_isAbleToCrash;
	callback->UnitDef_isHoverAttack = &skirmishAiCallback_UnitDef_isHoverAttack;
	callback->UnitDef_isAirStrafe = &skirmishAiCallback_UnitDef_isAirStrafe;
	callback->UnitDef_getDlHoverFactor = &skirmishAiCallback_UnitDef_getDlHoverFactor;
	callback->UnitDef_getMaxAcceleration = &skirmishAiCallback_UnitDef_getMaxAcceleration;
	callback->UnitDef_getMaxDeceleration = &skirmishAiCallback_UnitDef_getMaxDeceleration;
	callback->UnitDef_getMaxAileron = &skirmishAiCallback_UnitDef_getMaxAileron;
	callback->UnitDef_getMaxElevator = &skirmishAiCallback_UnitDef_getMaxElevator;
	callback->UnitDef_getMaxRudder = &skirmishAiCallback_UnitDef_getMaxRudder;
	callback->UnitDef_getYardMap = &skirmishAiCallback_UnitDef_getYardMap;
	callback->UnitDef_getXSize = &skirmishAiCallback_UnitDef_getXSize;
	callback->UnitDef_getZSize = &skirmishAiCallback_UnitDef_getZSize;
	callback->UnitDef_getBuildAngle = &skirmishAiCallback_UnitDef_getBuildAngle;
	callback->UnitDef_getLoadingRadius = &skirmishAiCallback_UnitDef_getLoadingRadius;
	callback->UnitDef_getUnloadSpread = &skirmishAiCallback_UnitDef_getUnloadSpread;
	callback->UnitDef_getTransportCapacity = &skirmishAiCallback_UnitDef_getTransportCapacity;
	callback->UnitDef_getTransportSize = &skirmishAiCallback_UnitDef_getTransportSize;
	callback->UnitDef_getMinTransportSize = &skirmishAiCallback_UnitDef_getMinTransportSize;
	callback->UnitDef_isAirBase = &skirmishAiCallback_UnitDef_isAirBase;
	callback->UnitDef_isFirePlatform = &skirmishAiCallback_UnitDef_isFirePlatform;
	callback->UnitDef_getTransportMass = &skirmishAiCallback_UnitDef_getTransportMass;
	callback->UnitDef_getMinTransportMass = &skirmishAiCallback_UnitDef_getMinTransportMass;
	callback->UnitDef_isHoldSteady = &skirmishAiCallback_UnitDef_isHoldSteady;
	callback->UnitDef_isReleaseHeld = &skirmishAiCallback_UnitDef_isReleaseHeld;
	callback->UnitDef_isNotTransportable = &skirmishAiCallback_UnitDef_isNotTransportable;
	callback->UnitDef_isTransportByEnemy = &skirmishAiCallback_UnitDef_isTransportByEnemy;
	callback->UnitDef_getTransportUnloadMethod = &skirmishAiCallback_UnitDef_getTransportUnloadMethod;
	callback->UnitDef_getFallSpeed = &skirmishAiCallback_UnitDef_getFallSpeed;
	callback->UnitDef_getUnitFallSpeed = &skirmishAiCallback_UnitDef_getUnitFallSpeed;
	callback->UnitDef_isAbleToCloak = &skirmishAiCallback_UnitDef_isAbleToCloak;
	callback->UnitDef_isStartCloaked = &skirmishAiCallback_UnitDef_isStartCloaked;
	callback->UnitDef_getCloakCost = &skirmishAiCallback_UnitDef_getCloakCost;
	callback->UnitDef_getCloakCostMoving = &skirmishAiCallback_UnitDef_getCloakCostMoving;
	callback->UnitDef_getDecloakDistance = &skirmishAiCallback_UnitDef_getDecloakDistance;
	callback->UnitDef_isDecloakSpherical = &skirmishAiCallback_UnitDef_isDecloakSpherical;
	callback->UnitDef_isDecloakOnFire = &skirmishAiCallback_UnitDef_isDecloakOnFire;
	callback->UnitDef_isAbleToKamikaze = &skirmishAiCallback_UnitDef_isAbleToKamikaze;
	callback->UnitDef_getKamikazeDist = &skirmishAiCallback_UnitDef_getKamikazeDist;
	callback->UnitDef_isTargetingFacility = &skirmishAiCallback_UnitDef_isTargetingFacility;
	callback->UnitDef_isAbleToDGun = &skirmishAiCallback_UnitDef_isAbleToDGun;
	callback->UnitDef_isNeedGeo = &skirmishAiCallback_UnitDef_isNeedGeo;
	callback->UnitDef_isFeature = &skirmishAiCallback_UnitDef_isFeature;
	callback->UnitDef_isHideDamage = &skirmishAiCallback_UnitDef_isHideDamage;
	callback->UnitDef_isCommander = &skirmishAiCallback_UnitDef_isCommander;
	callback->UnitDef_isShowPlayerName = &skirmishAiCallback_UnitDef_isShowPlayerName;
	callback->UnitDef_isAbleToResurrect = &skirmishAiCallback_UnitDef_isAbleToResurrect;
	callback->UnitDef_isAbleToCapture = &skirmishAiCallback_UnitDef_isAbleToCapture;
	callback->UnitDef_getHighTrajectoryType = &skirmishAiCallback_UnitDef_getHighTrajectoryType;
	callback->UnitDef_getNoChaseCategory = &skirmishAiCallback_UnitDef_getNoChaseCategory;
	callback->UnitDef_isLeaveTracks = &skirmishAiCallback_UnitDef_isLeaveTracks;
	callback->UnitDef_getTrackWidth = &skirmishAiCallback_UnitDef_getTrackWidth;
	callback->UnitDef_getTrackOffset = &skirmishAiCallback_UnitDef_getTrackOffset;
	callback->UnitDef_getTrackStrength = &skirmishAiCallback_UnitDef_getTrackStrength;
	callback->UnitDef_getTrackStretch = &skirmishAiCallback_UnitDef_getTrackStretch;
	callback->UnitDef_getTrackType = &skirmishAiCallback_UnitDef_getTrackType;
	callback->UnitDef_isAbleToDropFlare = &skirmishAiCallback_UnitDef_isAbleToDropFlare;
	callback->UnitDef_getFlareReloadTime = &skirmishAiCallback_UnitDef_getFlareReloadTime;
	callback->UnitDef_getFlareEfficiency = &skirmishAiCallback_UnitDef_getFlareEfficiency;
	callback->UnitDef_getFlareDelay = &skirmishAiCallback_UnitDef_getFlareDelay;
	callback->UnitDef_getFlareDropVector = &skirmishAiCallback_UnitDef_getFlareDropVector;
	callback->UnitDef_getFlareTime = &skirmishAiCallback_UnitDef_getFlareTime;
	callback->UnitDef_getFlareSalvoSize = &skirmishAiCallback_UnitDef_getFlareSalvoSize;
	callback->UnitDef_getFlareSalvoDelay = &skirmishAiCallback_UnitDef_getFlareSalvoDelay;
	callback->UnitDef_isAbleToLoopbackAttack = &skirmishAiCallback_UnitDef_isAbleToLoopbackAttack;
	callback->UnitDef_isLevelGround = &skirmishAiCallback_UnitDef_isLevelGround;
	callback->UnitDef_isUseBuildingGroundDecal = &skirmishAiCallback_UnitDef_isUseBuildingGroundDecal;
	callback->UnitDef_getBuildingDecalType = &skirmishAiCallback_UnitDef_getBuildingDecalType;
	callback->UnitDef_getBuildingDecalSizeX = &skirmishAiCallback_UnitDef_getBuildingDecalSizeX;
	callback->UnitDef_getBuildingDecalSizeY = &skirmishAiCallback_UnitDef_getBuildingDecalSizeY;
	callback->UnitDef_getBuildingDecalDecaySpeed = &skirmishAiCallback_UnitDef_getBuildingDecalDecaySpeed;
	callback->UnitDef_getMaxFuel = &skirmishAiCallback_UnitDef_getMaxFuel;
	callback->UnitDef_getRefuelTime = &skirmishAiCallback_UnitDef_getRefuelTime;
	callback->UnitDef_getMinAirBasePower = &skirmishAiCallback_UnitDef_getMinAirBasePower;
	callback->UnitDef_getMaxThisUnit = &skirmishAiCallback_UnitDef_getMaxThisUnit;
	callback->UnitDef_getDecoyDef = &skirmishAiCallback_UnitDef_getDecoyDef;
	callback->UnitDef_isDontLand = &skirmishAiCallback_UnitDef_isDontLand;
	callback->UnitDef_getShieldDef = &skirmishAiCallback_UnitDef_getShieldDef;
	callback->UnitDef_getStockpileDef = &skirmishAiCallback_UnitDef_getStockpileDef;
	callback->UnitDef_getBuildOptions = &skirmishAiCallback_UnitDef_getBuildOptions;
	callback->UnitDef_getCustomParams = &skirmishAiCallback_UnitDef_getCustomParams;
	callback->UnitDef_isMoveDataAvailable = &skirmishAiCallback_UnitDef_isMoveDataAvailable;
	callback->UnitDef_MoveData_getMaxAcceleration = &skirmishAiCallback_UnitDef_MoveData_getMaxAcceleration;
	callback->UnitDef_MoveData_getMaxBreaking = &skirmishAiCallback_UnitDef_MoveData_getMaxBreaking;
	callback->UnitDef_MoveData_getMaxSpeed = &skirmishAiCallback_UnitDef_MoveData_getMaxSpeed;
	callback->UnitDef_MoveData_getMaxTurnRate = &skirmishAiCallback_UnitDef_MoveData_getMaxTurnRate;
	callback->UnitDef_MoveData_getSize = &skirmishAiCallback_UnitDef_MoveData_getSize;
	callback->UnitDef_MoveData_getDepth = &skirmishAiCallback_UnitDef_MoveData_getDepth;
	callback->UnitDef_MoveData_getMaxSlope = &skirmishAiCallback_UnitDef_MoveData_getMaxSlope;
	callback->UnitDef_MoveData_getSlopeMod = &skirmishAiCallback_UnitDef_MoveData_getSlopeMod;
	callback->UnitDef_MoveData_getDepthMod = &skirmishAiCallback_UnitDef_MoveData_getDepthMod;
	callback->UnitDef_MoveData_getPathType = &skirmishAiCallback_UnitDef_MoveData_getPathType;
	callback->UnitDef_MoveData_getCrushStrength = &skirmishAiCallback_UnitDef_MoveData_getCrushStrength;
	callback->UnitDef_MoveData_getMoveType = &skirmishAiCallback_UnitDef_MoveData_getMoveType;
	callback->UnitDef_MoveData_getMoveFamily = &skirmishAiCallback_UnitDef_MoveData_getMoveFamily;
	callback->UnitDef_MoveData_getTerrainClass = &skirmishAiCallback_UnitDef_MoveData_getTerrainClass;
	callback->UnitDef_MoveData_getFollowGround = &skirmishAiCallback_UnitDef_MoveData_getFollowGround;
	callback->UnitDef_MoveData_isSubMarine = &skirmishAiCallback_UnitDef_MoveData_isSubMarine;
	callback->UnitDef_MoveData_getName = &skirmishAiCallback_UnitDef_MoveData_getName;
	callback->UnitDef_getWeaponMounts = &skirmishAiCallback_UnitDef_getWeaponMounts;
	callback->UnitDef_WeaponMount_getName = &skirmishAiCallback_UnitDef_WeaponMount_getName;
	callback->UnitDef_WeaponMount_getWeaponDef = &skirmishAiCallback_UnitDef_WeaponMount_getWeaponDef;
	callback->UnitDef_WeaponMount_getSlavedTo = &skirmishAiCallback_UnitDef_WeaponMount_getSlavedTo;
	callback->UnitDef_WeaponMount_getMainDir = &skirmishAiCallback_UnitDef_WeaponMount_getMainDir;
	callback->UnitDef_WeaponMount_getMaxAngleDif = &skirmishAiCallback_UnitDef_WeaponMount_getMaxAngleDif;
	callback->UnitDef_WeaponMount_getFuelUsage = &skirmishAiCallback_UnitDef_WeaponMount_getFuelUsage;
	callback->UnitDef_WeaponMount_getBadTargetCategory = &skirmishAiCallback_UnitDef_WeaponMount_getBadTargetCategory;
	callback->UnitDef_WeaponMount_getOnlyTargetCategory = &skirmishAiCallback_UnitDef_WeaponMount_getOnlyTargetCategory;
	callback->Unit_getLimit = &skirmishAiCallback_Unit_getLimit;
	callback->getEnemyUnits = &skirmishAiCallback_getEnemyUnits;
	callback->getEnemyUnitsIn = &skirmishAiCallback_getEnemyUnitsIn;
	callback->getEnemyUnitsInRadarAndLos = &skirmishAiCallback_getEnemyUnitsInRadarAndLos;
	callback->getFriendlyUnits = &skirmishAiCallback_getFriendlyUnits;
	callback->getFriendlyUnitsIn = &skirmishAiCallback_getFriendlyUnitsIn;
	callback->getNeutralUnits = &skirmishAiCallback_getNeutralUnits;
	callback->getNeutralUnitsIn = &skirmishAiCallback_getNeutralUnitsIn;
	callback->getTeamUnits = &skirmishAiCallback_getTeamUnits;
	callback->getSelectedUnits = &skirmishAiCallback_getSelectedUnits;
	callback->Unit_getDef = &skirmishAiCallback_Unit_getDef;
	callback->Unit_getModParams = &skirmishAiCallback_Unit_getModParams;
	callback->Unit_ModParam_getName = &skirmishAiCallback_Unit_ModParam_getName;
	callback->Unit_ModParam_getValue = &skirmishAiCallback_Unit_ModParam_getValue;
	callback->Unit_getTeam = &skirmishAiCallback_Unit_getTeam;
	callback->Unit_getAllyTeam = &skirmishAiCallback_Unit_getAllyTeam;
	callback->Unit_getLineage = &skirmishAiCallback_Unit_getLineage;
	callback->Unit_getAiHint = &skirmishAiCallback_Unit_getAiHint;
	callback->Unit_getStockpile = &skirmishAiCallback_Unit_getStockpile;
	callback->Unit_getStockpileQueued = &skirmishAiCallback_Unit_getStockpileQueued;
	callback->Unit_getCurrentFuel = &skirmishAiCallback_Unit_getCurrentFuel;
	callback->Unit_getMaxSpeed = &skirmishAiCallback_Unit_getMaxSpeed;
	callback->Unit_getMaxRange = &skirmishAiCallback_Unit_getMaxRange;
	callback->Unit_getMaxHealth = &skirmishAiCallback_Unit_getMaxHealth;
	callback->Unit_getExperience = &skirmishAiCallback_Unit_getExperience;
	callback->Unit_getGroup = &skirmishAiCallback_Unit_getGroup;
	callback->Unit_getCurrentCommands = &skirmishAiCallback_Unit_getCurrentCommands;
	callback->Unit_CurrentCommand_getType = &skirmishAiCallback_Unit_CurrentCommand_getType;
	callback->Unit_CurrentCommand_getId = &skirmishAiCallback_Unit_CurrentCommand_getId;
	callback->Unit_CurrentCommand_getOptions = &skirmishAiCallback_Unit_CurrentCommand_getOptions;
	callback->Unit_CurrentCommand_getTag = &skirmishAiCallback_Unit_CurrentCommand_getTag;
	callback->Unit_CurrentCommand_getTimeOut = &skirmishAiCallback_Unit_CurrentCommand_getTimeOut;
	callback->Unit_CurrentCommand_getParams = &skirmishAiCallback_Unit_CurrentCommand_getParams;
	callback->Unit_getSupportedCommands = &skirmishAiCallback_Unit_getSupportedCommands;
	callback->Unit_SupportedCommand_getId = &skirmishAiCallback_Unit_SupportedCommand_getId;
	callback->Unit_SupportedCommand_getName = &skirmishAiCallback_Unit_SupportedCommand_getName;
	callback->Unit_SupportedCommand_getToolTip = &skirmishAiCallback_Unit_SupportedCommand_getToolTip;
	callback->Unit_SupportedCommand_isShowUnique = &skirmishAiCallback_Unit_SupportedCommand_isShowUnique;
	callback->Unit_SupportedCommand_isDisabled = &skirmishAiCallback_Unit_SupportedCommand_isDisabled;
	callback->Unit_SupportedCommand_getParams = &skirmishAiCallback_Unit_SupportedCommand_getParams;
	callback->Unit_getHealth = &skirmishAiCallback_Unit_getHealth;
	callback->Unit_getSpeed = &skirmishAiCallback_Unit_getSpeed;
	callback->Unit_getPower = &skirmishAiCallback_Unit_getPower;
	callback->Unit_getResourceUse = &skirmishAiCallback_Unit_getResourceUse;
	callback->Unit_getResourceMake = &skirmishAiCallback_Unit_getResourceMake;
	callback->Unit_getPos = &skirmishAiCallback_Unit_getPos;
	callback->Unit_isActivated = &skirmishAiCallback_Unit_isActivated;
	callback->Unit_isBeingBuilt = &skirmishAiCallback_Unit_isBeingBuilt;
	callback->Unit_isCloaked = &skirmishAiCallback_Unit_isCloaked;
	callback->Unit_isParalyzed = &skirmishAiCallback_Unit_isParalyzed;
	callback->Unit_isNeutral = &skirmishAiCallback_Unit_isNeutral;
	callback->Unit_getBuildingFacing = &skirmishAiCallback_Unit_getBuildingFacing;
	callback->Unit_getLastUserOrderFrame = &skirmishAiCallback_Unit_getLastUserOrderFrame;
	callback->getGroups = &skirmishAiCallback_getGroups;
	callback->Group_getSupportedCommands = &skirmishAiCallback_Group_getSupportedCommands;
	callback->Group_SupportedCommand_getId = &skirmishAiCallback_Group_SupportedCommand_getId;
	callback->Group_SupportedCommand_getName = &skirmishAiCallback_Group_SupportedCommand_getName;
	callback->Group_SupportedCommand_getToolTip = &skirmishAiCallback_Group_SupportedCommand_getToolTip;
	callback->Group_SupportedCommand_isShowUnique = &skirmishAiCallback_Group_SupportedCommand_isShowUnique;
	callback->Group_SupportedCommand_isDisabled = &skirmishAiCallback_Group_SupportedCommand_isDisabled;
	callback->Group_SupportedCommand_getParams = &skirmishAiCallback_Group_SupportedCommand_getParams;
	callback->Group_OrderPreview_getId = &skirmishAiCallback_Group_OrderPreview_getId;
	callback->Group_OrderPreview_getOptions = &skirmishAiCallback_Group_OrderPreview_getOptions;
	callback->Group_OrderPreview_getTag = &skirmishAiCallback_Group_OrderPreview_getTag;
	callback->Group_OrderPreview_getTimeOut = &skirmishAiCallback_Group_OrderPreview_getTimeOut;
	callback->Group_OrderPreview_getParams = &skirmishAiCallback_Group_OrderPreview_getParams;
	callback->Group_isSelected = &skirmishAiCallback_Group_isSelected;
	callback->Mod_getFileName = &skirmishAiCallback_Mod_getFileName;
	callback->Mod_getHumanName = &skirmishAiCallback_Mod_getHumanName;
	callback->Mod_getShortName = &skirmishAiCallback_Mod_getShortName;
	callback->Mod_getVersion = &skirmishAiCallback_Mod_getVersion;
	callback->Mod_getMutator = &skirmishAiCallback_Mod_getMutator;
	callback->Mod_getDescription = &skirmishAiCallback_Mod_getDescription;
	callback->Mod_getAllowTeamColors = &skirmishAiCallback_Mod_getAllowTeamColors;
	callback->Mod_getConstructionDecay = &skirmishAiCallback_Mod_getConstructionDecay;
	callback->Mod_getConstructionDecayTime = &skirmishAiCallback_Mod_getConstructionDecayTime;
	callback->Mod_getConstructionDecaySpeed = &skirmishAiCallback_Mod_getConstructionDecaySpeed;
	callback->Mod_getMultiReclaim = &skirmishAiCallback_Mod_getMultiReclaim;
	callback->Mod_getReclaimMethod = &skirmishAiCallback_Mod_getReclaimMethod;
	callback->Mod_getReclaimUnitMethod = &skirmishAiCallback_Mod_getReclaimUnitMethod;
	callback->Mod_getReclaimUnitEnergyCostFactor = &skirmishAiCallback_Mod_getReclaimUnitEnergyCostFactor;
	callback->Mod_getReclaimUnitEfficiency = &skirmishAiCallback_Mod_getReclaimUnitEfficiency;
	callback->Mod_getReclaimFeatureEnergyCostFactor = &skirmishAiCallback_Mod_getReclaimFeatureEnergyCostFactor;
	callback->Mod_getReclaimAllowEnemies = &skirmishAiCallback_Mod_getReclaimAllowEnemies;
	callback->Mod_getReclaimAllowAllies = &skirmishAiCallback_Mod_getReclaimAllowAllies;
	callback->Mod_getRepairEnergyCostFactor = &skirmishAiCallback_Mod_getRepairEnergyCostFactor;
	callback->Mod_getResurrectEnergyCostFactor = &skirmishAiCallback_Mod_getResurrectEnergyCostFactor;
	callback->Mod_getCaptureEnergyCostFactor = &skirmishAiCallback_Mod_getCaptureEnergyCostFactor;
	callback->Mod_getTransportGround = &skirmishAiCallback_Mod_getTransportGround;
	callback->Mod_getTransportHover = &skirmishAiCallback_Mod_getTransportHover;
	callback->Mod_getTransportShip = &skirmishAiCallback_Mod_getTransportShip;
	callback->Mod_getTransportAir = &skirmishAiCallback_Mod_getTransportAir;
	callback->Mod_getFireAtKilled = &skirmishAiCallback_Mod_getFireAtKilled;
	callback->Mod_getFireAtCrashing = &skirmishAiCallback_Mod_getFireAtCrashing;
	callback->Mod_getFlankingBonusModeDefault = &skirmishAiCallback_Mod_getFlankingBonusModeDefault;
	callback->Mod_getLosMipLevel = &skirmishAiCallback_Mod_getLosMipLevel;
	callback->Mod_getAirMipLevel = &skirmishAiCallback_Mod_getAirMipLevel;
	callback->Mod_getLosMul = &skirmishAiCallback_Mod_getLosMul;
	callback->Mod_getAirLosMul = &skirmishAiCallback_Mod_getAirLosMul;
	callback->Mod_getRequireSonarUnderWater = &skirmishAiCallback_Mod_getRequireSonarUnderWater;
	callback->Map_getChecksum = &skirmishAiCallback_Map_getChecksum;
	callback->Map_getStartPos = &skirmishAiCallback_Map_getStartPos;
	callback->Map_getMousePos = &skirmishAiCallback_Map_getMousePos;
	callback->Map_isPosInCamera = &skirmishAiCallback_Map_isPosInCamera;
	callback->Map_getWidth = &skirmishAiCallback_Map_getWidth;
	callback->Map_getHeight = &skirmishAiCallback_Map_getHeight;
	callback->Map_getHeightMap = &skirmishAiCallback_Map_getHeightMap;
	callback->Map_getCornersHeightMap = &skirmishAiCallback_Map_getCornersHeightMap;
	callback->Map_getMinHeight = &skirmishAiCallback_Map_getMinHeight;
	callback->Map_getMaxHeight = &skirmishAiCallback_Map_getMaxHeight;
	callback->Map_getSlopeMap = &skirmishAiCallback_Map_getSlopeMap;
	callback->Map_getLosMap = &skirmishAiCallback_Map_getLosMap;
	callback->Map_getRadarMap = &skirmishAiCallback_Map_getRadarMap;
	callback->Map_getJammerMap = &skirmishAiCallback_Map_getJammerMap;
	callback->Map_getResourceMapRaw = &skirmishAiCallback_Map_getResourceMapRaw;
	callback->Map_getResourceMapSpotsPositions = &skirmishAiCallback_Map_getResourceMapSpotsPositions;
	callback->Map_getResourceMapSpotsAverageIncome = &skirmishAiCallback_Map_getResourceMapSpotsAverageIncome;
	callback->Map_getResourceMapSpotsNearest = &skirmishAiCallback_Map_getResourceMapSpotsNearest;
	callback->Map_getName = &skirmishAiCallback_Map_getName;
	callback->Map_getElevationAt = &skirmishAiCallback_Map_getElevationAt;
	callback->Map_getMaxResource = &skirmishAiCallback_Map_getMaxResource;
	callback->Map_getExtractorRadius = &skirmishAiCallback_Map_getExtractorRadius;
	callback->Map_getMinWind = &skirmishAiCallback_Map_getMinWind;
	callback->Map_getMaxWind = &skirmishAiCallback_Map_getMaxWind;
	callback->Map_getCurWind = &skirmishAiCallback_Map_getCurWind;
	callback->Map_getTidalStrength = &skirmishAiCallback_Map_getTidalStrength;
	callback->Map_getGravity = &skirmishAiCallback_Map_getGravity;
	callback->Map_getPoints = &skirmishAiCallback_Map_getPoints;
	callback->Map_Point_getPosition = &skirmishAiCallback_Map_Point_getPosition;
	callback->Map_Point_getColor = &skirmishAiCallback_Map_Point_getColor;
	callback->Map_Point_getLabel = &skirmishAiCallback_Map_Point_getLabel;
	callback->Map_getLines = &skirmishAiCallback_Map_getLines;
	callback->Map_Line_getFirstPosition = &skirmishAiCallback_Map_Line_getFirstPosition;
	callback->Map_Line_getSecondPosition = &skirmishAiCallback_Map_Line_getSecondPosition;
	callback->Map_Line_getColor = &skirmishAiCallback_Map_Line_getColor;
	callback->Map_isPossibleToBuildAt = &skirmishAiCallback_Map_isPossibleToBuildAt;
	callback->Map_findClosestBuildSite = &skirmishAiCallback_Map_findClosestBuildSite;
	callback->getFeatureDefs = &skirmishAiCallback_getFeatureDefs;
	callback->FeatureDef_getName = &skirmishAiCallback_FeatureDef_getName;
	callback->FeatureDef_getDescription = &skirmishAiCallback_FeatureDef_getDescription;
	callback->FeatureDef_getFileName = &skirmishAiCallback_FeatureDef_getFileName;
	callback->FeatureDef_getContainedResource = &skirmishAiCallback_FeatureDef_getContainedResource;
	callback->FeatureDef_getMaxHealth = &skirmishAiCallback_FeatureDef_getMaxHealth;
	callback->FeatureDef_getReclaimTime = &skirmishAiCallback_FeatureDef_getReclaimTime;
	callback->FeatureDef_getMass = &skirmishAiCallback_FeatureDef_getMass;
	callback->FeatureDef_CollisionVolume_getType = &skirmishAiCallback_FeatureDef_CollisionVolume_getType;
	callback->FeatureDef_CollisionVolume_getScales = &skirmishAiCallback_FeatureDef_CollisionVolume_getScales;
	callback->FeatureDef_CollisionVolume_getOffsets = &skirmishAiCallback_FeatureDef_CollisionVolume_getOffsets;
	callback->FeatureDef_CollisionVolume_getTest = &skirmishAiCallback_FeatureDef_CollisionVolume_getTest;
	callback->FeatureDef_isUpright = &skirmishAiCallback_FeatureDef_isUpright;
	callback->FeatureDef_getDrawType = &skirmishAiCallback_FeatureDef_getDrawType;
	callback->FeatureDef_getModelName = &skirmishAiCallback_FeatureDef_getModelName;
	callback->FeatureDef_getResurrectable = &skirmishAiCallback_FeatureDef_getResurrectable;
	callback->FeatureDef_getSmokeTime = &skirmishAiCallback_FeatureDef_getSmokeTime;
	callback->FeatureDef_isDestructable = &skirmishAiCallback_FeatureDef_isDestructable;
	callback->FeatureDef_isReclaimable = &skirmishAiCallback_FeatureDef_isReclaimable;
	callback->FeatureDef_isBlocking = &skirmishAiCallback_FeatureDef_isBlocking;
	callback->FeatureDef_isBurnable = &skirmishAiCallback_FeatureDef_isBurnable;
	callback->FeatureDef_isFloating = &skirmishAiCallback_FeatureDef_isFloating;
	callback->FeatureDef_isNoSelect = &skirmishAiCallback_FeatureDef_isNoSelect;
	callback->FeatureDef_isGeoThermal = &skirmishAiCallback_FeatureDef_isGeoThermal;
	callback->FeatureDef_getDeathFeature = &skirmishAiCallback_FeatureDef_getDeathFeature;
	callback->FeatureDef_getXSize = &skirmishAiCallback_FeatureDef_getXSize;
	callback->FeatureDef_getZSize = &skirmishAiCallback_FeatureDef_getZSize;
	callback->FeatureDef_getCustomParams = &skirmishAiCallback_FeatureDef_getCustomParams;
	callback->getFeatures = &skirmishAiCallback_getFeatures;
	callback->getFeaturesIn = &skirmishAiCallback_getFeaturesIn;
	callback->Feature_getDef = &skirmishAiCallback_Feature_getDef;
	callback->Feature_getHealth = &skirmishAiCallback_Feature_getHealth;
	callback->Feature_getReclaimLeft = &skirmishAiCallback_Feature_getReclaimLeft;
	callback->Feature_getPosition = &skirmishAiCallback_Feature_getPosition;
	callback->getWeaponDefs = &skirmishAiCallback_getWeaponDefs;
	callback->getWeaponDefByName = &skirmishAiCallback_getWeaponDefByName;
	callback->WeaponDef_getName = &skirmishAiCallback_WeaponDef_getName;
	callback->WeaponDef_getType = &skirmishAiCallback_WeaponDef_getType;
	callback->WeaponDef_getDescription = &skirmishAiCallback_WeaponDef_getDescription;
	callback->WeaponDef_getFileName = &skirmishAiCallback_WeaponDef_getFileName;
	callback->WeaponDef_getCegTag = &skirmishAiCallback_WeaponDef_getCegTag;
	callback->WeaponDef_getRange = &skirmishAiCallback_WeaponDef_getRange;
	callback->WeaponDef_getHeightMod = &skirmishAiCallback_WeaponDef_getHeightMod;
	callback->WeaponDef_getAccuracy = &skirmishAiCallback_WeaponDef_getAccuracy;
	callback->WeaponDef_getSprayAngle = &skirmishAiCallback_WeaponDef_getSprayAngle;
	callback->WeaponDef_getMovingAccuracy = &skirmishAiCallback_WeaponDef_getMovingAccuracy;
	callback->WeaponDef_getTargetMoveError = &skirmishAiCallback_WeaponDef_getTargetMoveError;
	callback->WeaponDef_getLeadLimit = &skirmishAiCallback_WeaponDef_getLeadLimit;
	callback->WeaponDef_getLeadBonus = &skirmishAiCallback_WeaponDef_getLeadBonus;
	callback->WeaponDef_getPredictBoost = &skirmishAiCallback_WeaponDef_getPredictBoost;
	callback->WeaponDef_getNumDamageTypes = &skirmishAiCallback_WeaponDef_getNumDamageTypes;
	callback->WeaponDef_Damage_getParalyzeDamageTime = &skirmishAiCallback_WeaponDef_Damage_getParalyzeDamageTime;
	callback->WeaponDef_Damage_getImpulseFactor = &skirmishAiCallback_WeaponDef_Damage_getImpulseFactor;
	callback->WeaponDef_Damage_getImpulseBoost = &skirmishAiCallback_WeaponDef_Damage_getImpulseBoost;
	callback->WeaponDef_Damage_getCraterMult = &skirmishAiCallback_WeaponDef_Damage_getCraterMult;
	callback->WeaponDef_Damage_getCraterBoost = &skirmishAiCallback_WeaponDef_Damage_getCraterBoost;
	callback->WeaponDef_Damage_getTypes = &skirmishAiCallback_WeaponDef_Damage_getTypes;
	callback->WeaponDef_getAreaOfEffect = &skirmishAiCallback_WeaponDef_getAreaOfEffect;
	callback->WeaponDef_isNoSelfDamage = &skirmishAiCallback_WeaponDef_isNoSelfDamage;
	callback->WeaponDef_getFireStarter = &skirmishAiCallback_WeaponDef_getFireStarter;
	callback->WeaponDef_getEdgeEffectiveness = &skirmishAiCallback_WeaponDef_getEdgeEffectiveness;
	callback->WeaponDef_getSize = &skirmishAiCallback_WeaponDef_getSize;
	callback->WeaponDef_getSizeGrowth = &skirmishAiCallback_WeaponDef_getSizeGrowth;
	callback->WeaponDef_getCollisionSize = &skirmishAiCallback_WeaponDef_getCollisionSize;
	callback->WeaponDef_getSalvoSize = &skirmishAiCallback_WeaponDef_getSalvoSize;
	callback->WeaponDef_getSalvoDelay = &skirmishAiCallback_WeaponDef_getSalvoDelay;
	callback->WeaponDef_getReload = &skirmishAiCallback_WeaponDef_getReload;
	callback->WeaponDef_getBeamTime = &skirmishAiCallback_WeaponDef_getBeamTime;
	callback->WeaponDef_isBeamBurst = &skirmishAiCallback_WeaponDef_isBeamBurst;
	callback->WeaponDef_isWaterBounce = &skirmishAiCallback_WeaponDef_isWaterBounce;
	callback->WeaponDef_isGroundBounce = &skirmishAiCallback_WeaponDef_isGroundBounce;
	callback->WeaponDef_getBounceRebound = &skirmishAiCallback_WeaponDef_getBounceRebound;
	callback->WeaponDef_getBounceSlip = &skirmishAiCallback_WeaponDef_getBounceSlip;
	callback->WeaponDef_getNumBounce = &skirmishAiCallback_WeaponDef_getNumBounce;
	callback->WeaponDef_getMaxAngle = &skirmishAiCallback_WeaponDef_getMaxAngle;
	callback->WeaponDef_getRestTime = &skirmishAiCallback_WeaponDef_getRestTime;
	callback->WeaponDef_getUpTime = &skirmishAiCallback_WeaponDef_getUpTime;
	callback->WeaponDef_getFlightTime = &skirmishAiCallback_WeaponDef_getFlightTime;
	callback->WeaponDef_getCost = &skirmishAiCallback_WeaponDef_getCost;
	callback->WeaponDef_getSupplyCost = &skirmishAiCallback_WeaponDef_getSupplyCost;
	callback->WeaponDef_getProjectilesPerShot = &skirmishAiCallback_WeaponDef_getProjectilesPerShot;
	callback->WeaponDef_isTurret = &skirmishAiCallback_WeaponDef_isTurret;
	callback->WeaponDef_isOnlyForward = &skirmishAiCallback_WeaponDef_isOnlyForward;
	callback->WeaponDef_isFixedLauncher = &skirmishAiCallback_WeaponDef_isFixedLauncher;
	callback->WeaponDef_isWaterWeapon = &skirmishAiCallback_WeaponDef_isWaterWeapon;
	callback->WeaponDef_isFireSubmersed = &skirmishAiCallback_WeaponDef_isFireSubmersed;
	callback->WeaponDef_isSubMissile = &skirmishAiCallback_WeaponDef_isSubMissile;
	callback->WeaponDef_isTracks = &skirmishAiCallback_WeaponDef_isTracks;
	callback->WeaponDef_isDropped = &skirmishAiCallback_WeaponDef_isDropped;
	callback->WeaponDef_isParalyzer = &skirmishAiCallback_WeaponDef_isParalyzer;
	callback->WeaponDef_isImpactOnly = &skirmishAiCallback_WeaponDef_isImpactOnly;
	callback->WeaponDef_isNoAutoTarget = &skirmishAiCallback_WeaponDef_isNoAutoTarget;
	callback->WeaponDef_isManualFire = &skirmishAiCallback_WeaponDef_isManualFire;
	callback->WeaponDef_getInterceptor = &skirmishAiCallback_WeaponDef_getInterceptor;
	callback->WeaponDef_getTargetable = &skirmishAiCallback_WeaponDef_getTargetable;
	callback->WeaponDef_isStockpileable = &skirmishAiCallback_WeaponDef_isStockpileable;
	callback->WeaponDef_getCoverageRange = &skirmishAiCallback_WeaponDef_getCoverageRange;
	callback->WeaponDef_getStockpileTime = &skirmishAiCallback_WeaponDef_getStockpileTime;
	callback->WeaponDef_getIntensity = &skirmishAiCallback_WeaponDef_getIntensity;
	callback->WeaponDef_getThickness = &skirmishAiCallback_WeaponDef_getThickness;
	callback->WeaponDef_getLaserFlareSize = &skirmishAiCallback_WeaponDef_getLaserFlareSize;
	callback->WeaponDef_getCoreThickness = &skirmishAiCallback_WeaponDef_getCoreThickness;
	callback->WeaponDef_getDuration = &skirmishAiCallback_WeaponDef_getDuration;
	callback->WeaponDef_getLodDistance = &skirmishAiCallback_WeaponDef_getLodDistance;
	callback->WeaponDef_getFalloffRate = &skirmishAiCallback_WeaponDef_getFalloffRate;
	callback->WeaponDef_getGraphicsType = &skirmishAiCallback_WeaponDef_getGraphicsType;
	callback->WeaponDef_isSoundTrigger = &skirmishAiCallback_WeaponDef_isSoundTrigger;
	callback->WeaponDef_isSelfExplode = &skirmishAiCallback_WeaponDef_isSelfExplode;
	callback->WeaponDef_isGravityAffected = &skirmishAiCallback_WeaponDef_isGravityAffected;
	callback->WeaponDef_getHighTrajectory = &skirmishAiCallback_WeaponDef_getHighTrajectory;
	callback->WeaponDef_getMyGravity = &skirmishAiCallback_WeaponDef_getMyGravity;
	callback->WeaponDef_isNoExplode = &skirmishAiCallback_WeaponDef_isNoExplode;
	callback->WeaponDef_getStartVelocity = &skirmishAiCallback_WeaponDef_getStartVelocity;
	callback->WeaponDef_getWeaponAcceleration = &skirmishAiCallback_WeaponDef_getWeaponAcceleration;
	callback->WeaponDef_getTurnRate = &skirmishAiCallback_WeaponDef_getTurnRate;
	callback->WeaponDef_getMaxVelocity = &skirmishAiCallback_WeaponDef_getMaxVelocity;
	callback->WeaponDef_getProjectileSpeed = &skirmishAiCallback_WeaponDef_getProjectileSpeed;
	callback->WeaponDef_getExplosionSpeed = &skirmishAiCallback_WeaponDef_getExplosionSpeed;
	callback->WeaponDef_getOnlyTargetCategory = &skirmishAiCallback_WeaponDef_getOnlyTargetCategory;
	callback->WeaponDef_getWobble = &skirmishAiCallback_WeaponDef_getWobble;
	callback->WeaponDef_getDance = &skirmishAiCallback_WeaponDef_getDance;
	callback->WeaponDef_getTrajectoryHeight = &skirmishAiCallback_WeaponDef_getTrajectoryHeight;
	callback->WeaponDef_isLargeBeamLaser = &skirmishAiCallback_WeaponDef_isLargeBeamLaser;
	callback->WeaponDef_isShield = &skirmishAiCallback_WeaponDef_isShield;
	callback->WeaponDef_isShieldRepulser = &skirmishAiCallback_WeaponDef_isShieldRepulser;
	callback->WeaponDef_isSmartShield = &skirmishAiCallback_WeaponDef_isSmartShield;
	callback->WeaponDef_isExteriorShield = &skirmishAiCallback_WeaponDef_isExteriorShield;
	callback->WeaponDef_isVisibleShield = &skirmishAiCallback_WeaponDef_isVisibleShield;
	callback->WeaponDef_isVisibleShieldRepulse = &skirmishAiCallback_WeaponDef_isVisibleShieldRepulse;
	callback->WeaponDef_getVisibleShieldHitFrames = &skirmishAiCallback_WeaponDef_getVisibleShieldHitFrames;
	callback->WeaponDef_Shield_getResourceUse = &skirmishAiCallback_WeaponDef_Shield_getResourceUse;
	callback->WeaponDef_Shield_getRadius = &skirmishAiCallback_WeaponDef_Shield_getRadius;
	callback->WeaponDef_Shield_getForce = &skirmishAiCallback_WeaponDef_Shield_getForce;
	callback->WeaponDef_Shield_getMaxSpeed = &skirmishAiCallback_WeaponDef_Shield_getMaxSpeed;
	callback->WeaponDef_Shield_getPower = &skirmishAiCallback_WeaponDef_Shield_getPower;
	callback->WeaponDef_Shield_getPowerRegen = &skirmishAiCallback_WeaponDef_Shield_getPowerRegen;
	callback->WeaponDef_Shield_getPowerRegenResource = &skirmishAiCallback_WeaponDef_Shield_getPowerRegenResource;
	callback->WeaponDef_Shield_getStartingPower = &skirmishAiCallback_WeaponDef_Shield_getStartingPower;
	callback->WeaponDef_Shield_getRechargeDelay = &skirmishAiCallback_WeaponDef_Shield_getRechargeDelay;
	callback->WeaponDef_Shield_getGoodColor = &skirmishAiCallback_WeaponDef_Shield_getGoodColor;
	callback->WeaponDef_Shield_getBadColor = &skirmishAiCallback_WeaponDef_Shield_getBadColor;
	callback->WeaponDef_Shield_getAlpha = &skirmishAiCallback_WeaponDef_Shield_getAlpha;
	callback->WeaponDef_Shield_getInterceptType = &skirmishAiCallback_WeaponDef_Shield_getInterceptType;
	callback->WeaponDef_getInterceptedByShieldType = &skirmishAiCallback_WeaponDef_getInterceptedByShieldType;
	callback->WeaponDef_isAvoidFriendly = &skirmishAiCallback_WeaponDef_isAvoidFriendly;
	callback->WeaponDef_isAvoidFeature = &skirmishAiCallback_WeaponDef_isAvoidFeature;
	callback->WeaponDef_isAvoidNeutral = &skirmishAiCallback_WeaponDef_isAvoidNeutral;
	callback->WeaponDef_getTargetBorder = &skirmishAiCallback_WeaponDef_getTargetBorder;
	callback->WeaponDef_getCylinderTargetting = &skirmishAiCallback_WeaponDef_getCylinderTargetting;
	callback->WeaponDef_getMinIntensity = &skirmishAiCallback_WeaponDef_getMinIntensity;
	callback->WeaponDef_getHeightBoostFactor = &skirmishAiCallback_WeaponDef_getHeightBoostFactor;
	callback->WeaponDef_getProximityPriority = &skirmishAiCallback_WeaponDef_getProximityPriority;
	callback->WeaponDef_getCollisionFlags = &skirmishAiCallback_WeaponDef_getCollisionFlags;
	callback->WeaponDef_isSweepFire = &skirmishAiCallback_WeaponDef_isSweepFire;
	callback->WeaponDef_isAbleToAttackGround = &skirmishAiCallback_WeaponDef_isAbleToAttackGround;
	callback->WeaponDef_getCameraShake = &skirmishAiCallback_WeaponDef_getCameraShake;
	callback->WeaponDef_getDynDamageExp = &skirmishAiCallback_WeaponDef_getDynDamageExp;
	callback->WeaponDef_getDynDamageMin = &skirmishAiCallback_WeaponDef_getDynDamageMin;
	callback->WeaponDef_getDynDamageRange = &skirmishAiCallback_WeaponDef_getDynDamageRange;
	callback->WeaponDef_isDynDamageInverted = &skirmishAiCallback_WeaponDef_isDynDamageInverted;
	callback->WeaponDef_getCustomParams = &skirmishAiCallback_WeaponDef_getCustomParams;
}

SSkirmishAICallback* skirmishAiCallback_getInstanceFor(int teamId, IGlobalAICallback* globalAICallback) {

	SSkirmishAICallback* callback = new SSkirmishAICallback();
	skirmishAiCallback_init(callback);

	team_globalCallback[teamId] = globalAICallback;
	team_callback[teamId]       = globalAICallback->GetAICallback();
	team_cCallback[teamId]      = callback;

	return callback;
}

void skirmishAiCallback_release(int teamId) {

	team_globalCallback.erase(teamId);
	team_callback.erase(teamId);

	SSkirmishAICallback* callback = team_cCallback[teamId];
	team_cCallback.erase(teamId);
	delete callback;
}
