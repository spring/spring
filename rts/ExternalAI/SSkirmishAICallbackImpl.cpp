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

#include "IGlobalAICallback.h"
#include "IAICallback.h"
#include "IAICheats.h"
#include "AICheats.h" // only for: CAICheats::IsPassive()
#include "IAILibraryManager.h"
#include "SSkirmishAICallbackImpl.h"
#include "SkirmishAILibraryInfo.h"
#include "SAIInterfaceCallbackImpl.h"
#include "EngineOutHandler.h"
#include "Interface/AISCommands.h"
#include "Interface/SSkirmishAILibrary.h"
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
static int tmpSize[MAX_SKIRMISH_AIS];
// FIXME: the following lines are relatively memory intensive (~1MB per line)
// this memory is only freed at exit of the application
// There is quite some CPU and Memory performance waste present
// in them and their use.
static int tmpIntArr[MAX_SKIRMISH_AIS][TMP_ARR_SIZE];
static PointMarker tmpPointMarkerArr[MAX_SKIRMISH_AIS][TMP_ARR_SIZE];
static LineMarker tmpLineMarkerArr[MAX_SKIRMISH_AIS][TMP_ARR_SIZE];
static const char* tmpKeysArr[MAX_SKIRMISH_AIS][TMP_ARR_SIZE];
static const char* tmpValuesArr[MAX_SKIRMISH_AIS][TMP_ARR_SIZE];

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
/*
static int fillCMapKeys(const std::map<std::string,std::string>* map,
		const char* cMapKeys[]) {
	std::map<std::string,std::string>::const_iterator it;
	int i;
	for (i=0, it=map->begin(); it != map->end(); ++i, it++) {
		cMapKeys[i] = it->first.c_str();
	}
	return i;
}
static int fillCMapValues(const std::map<std::string,std::string>* map,
		const char* cMapValues[]) {
	std::map<std::string,std::string>::const_iterator it;
	int i;
	for (i=0, it=map->begin(); it != map->end(); ++i, it++) {
		cMapValues[i] = it->second.c_str();
	}
	return i;
}
*/
static inline int min(int val1, int val2) {
	return val1 < val2 ? val1 : val2;
}
static inline int max(int val1, int val2) {
	return val1 > val2 ? val1 : val2;
}
static int copyIntArr(int* dest, int* src, int size) {

	for (int i=0; i < size; ++i) {
		dest[i] = src[i];
	}
	return size;
}
/*
static int copyFloatArr(float* dest, float* src, int size) {

	for (int i=0; i < size; ++i) {
		dest[i] = src[i];
	}
	return size;
}
*/
static int copyStringArr(const char** dest, const char** src, int size) {

	for (int i=0; i < size; ++i) {
		dest[i] = src[i];
	}
	return size;
}
static void toFloatArr(const SAIFloat3* color, float alpha, float arrColor[4]) {
	arrColor[0] = color->x;
	arrColor[1] = color->y;
	arrColor[2] = color->z;
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

	if ((unsigned int)unitId < (unsigned int)MAX_UNITS) {
		return uh->units[unitId];
	} else {
		return NULL;
	}
}
static bool isAlliedUnit(int teamId, const CUnit* unit) {
	return teamHandler->AlliedTeams(unit->team, teamId);
}
//static bool isAlliedUnitId(int teamId, int unitId) {
//	return isAlliedUnit(teamId, getUnit(unitId));
//}

static const UnitDef* getUnitDefById(int teamId, int unitDefId) {
	AIHCGetUnitDefById cmd = {unitDefId, NULL};
	int ret = team_callback[teamId]->HandleCommand(AIHCGetUnitDefByIdId, &cmd);
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
	int ret = team_callback[teamId]->HandleCommand(AIHCGetFeatureDefByIdId, &cmd);
	if (ret == 1) {
		return cmd.ret;
	} else {
		return NULL;
	}
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

	IAICallback* clb = team_callback[teamId];
	// if this is != NULL, cheating is enabled
	IAICheats* clbCheat = team_cheatCallback[teamId];


	switch (commandTopic) {

		case COMMAND_CHEATS_SET_MY_HANDICAP:
		{
			SSetMyHandicapCheatCommand* cmd =
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
			SGiveMeResourceCheatCommand* cmd =
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
						cmd->unitDefId)->name.c_str(), float3(cmd->pos));
				if (cmd->ret_newUnitId > 0) {
					ret = 0;
				} else {
					ret = -2;
				}
			} else {
				ret = -1;
			}
			break;
		}


		case COMMAND_SEND_START_POS:
		{
			SSendStartPosCommand* cmd = (SSendStartPosCommand*) commandData;
			AIHCSendStartPos data = {cmd->ready, float3(cmd->pos)};
			wrapper_HandleCommand(clb, clbCheat, AIHCSendStartPosId, &data);
			break;
		}
		case COMMAND_DRAWER_POINT_ADD:
		{
			SAddPointDrawCommand* cmd = (SAddPointDrawCommand*) commandData;
			AIHCAddMapPoint data = {float3(cmd->pos), cmd->label};
			wrapper_HandleCommand(clb, clbCheat, AIHCAddMapPointId, &data);
			break;
		}
		case COMMAND_DRAWER_POINT_REMOVE:
		{
			SRemovePointDrawCommand* cmd =
					(SRemovePointDrawCommand*) commandData;
			AIHCRemoveMapPoint data = {float3(cmd->pos)};
			wrapper_HandleCommand(clb, clbCheat, AIHCRemoveMapPointId, &data);
			break;
		}
		case COMMAND_DRAWER_LINE_ADD:
		{
			SAddLineDrawCommand* cmd = (SAddLineDrawCommand*) commandData;
			AIHCAddMapLine data = {float3(cmd->posFrom), float3(cmd->posTo)};
			wrapper_HandleCommand(clb, clbCheat, AIHCAddMapLineId, &data);
			break;
		}


		case COMMAND_SEND_TEXT_MESSAGE:
		{
			SSendTextMessageCommand* cmd =
					(SSendTextMessageCommand*) commandData;
			clb->SendTextMsg(cmd->text, cmd->zone);
			break;
		}
		case COMMAND_SET_LAST_POS_MESSAGE:
		{
			SSetLastPosMessageCommand* cmd =
					(SSetLastPosMessageCommand*) commandData;
			clb->SetLastMsgPos(cmd->pos);
			break;
		}
		case COMMAND_SEND_RESOURCES:
		{
			SSendResourcesCommand* cmd = (SSendResourcesCommand*) commandData;
			if (cmd->resourceId == resourceHandler->GetMetalId()) {
				cmd->ret_isExecuted = clb->SendResources(cmd->amount, 0, cmd->receivingTeam);
				ret = -2;
			} else if (cmd->resourceId == resourceHandler->GetEnergyId()) {
				cmd->ret_isExecuted = clb->SendResources(0, cmd->amount, cmd->receivingTeam);
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
			fillVector(&vector_unitIds, cmd->unitIds, cmd->numUnitIds);
			cmd->ret_sentUnits =
					clb->SendUnits(vector_unitIds, cmd->receivingTeam);
			break;
		}

		case COMMAND_GROUP_CREATE:
		{
			SCreateGroupCommand* cmd = (SCreateGroupCommand*) commandData;
			cmd->ret_groupId =
					clb->CreateGroup();
			break;
		}
		case COMMAND_GROUP_ERASE:
		{
			SEraseGroupCommand* cmd = (SEraseGroupCommand*) commandData;
			clb->EraseGroup(cmd->groupId);
			break;
		}
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
		case COMMAND_PATH_INIT:
		{
			SInitPathCommand* cmd = (SInitPathCommand*) commandData;
			cmd->ret_pathId = clb->InitPath(float3(cmd->start),
					float3(cmd->end), cmd->pathType);
			break;
		}
		case COMMAND_PATH_GET_APPROXIMATE_LENGTH:
		{
			SGetApproximateLengthPathCommand* cmd =
					(SGetApproximateLengthPathCommand*) commandData;
			cmd->ret_approximatePathLength =
					clb->GetPathLength(float3(cmd->start), float3(cmd->end),
							cmd->pathType);
			break;
		}
		case COMMAND_PATH_GET_NEXT_WAYPOINT:
		{
			SGetNextWaypointPathCommand* cmd =
					(SGetNextWaypointPathCommand*) commandData;
			cmd->ret_nextWaypoint =
					clb->GetNextWaypoint(cmd->pathId).toSAIFloat3();
			break;
		}
		case COMMAND_PATH_FREE:
		{
			SFreePathCommand* cmd = (SFreePathCommand*) commandData;
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
			SAddNotificationDrawerCommand* cmd =
					(SAddNotificationDrawerCommand*) commandData;
			clb->AddNotification(float3(cmd->pos), float3(cmd->color),
					cmd->alpha);
			break;
		}
		case COMMAND_DRAWER_PATH_START:
		{
			SStartPathDrawerCommand* cmd =
					(SStartPathDrawerCommand*) commandData;
			float arrColor[4];
			toFloatArr(&cmd->color, cmd->alpha, arrColor);
			clb->LineDrawerStartPath(float3(cmd->pos), arrColor);
			break;
		}
		case COMMAND_DRAWER_PATH_FINISH:
		{
			//SFinishPathDrawerCommand* cmd =
			//		(SFinishPathDrawerCommand*) commandData;
			clb->LineDrawerFinishPath();
			break;
		}
		case COMMAND_DRAWER_PATH_DRAW_LINE:
		{
			SDrawLinePathDrawerCommand* cmd =
					(SDrawLinePathDrawerCommand*) commandData;
			float arrColor[4];
			toFloatArr(&cmd->color, cmd->alpha, arrColor);
			clb->LineDrawerDrawLine(float3(cmd->endPos), arrColor);
			break;
		}
		case COMMAND_DRAWER_PATH_DRAW_LINE_AND_ICON:
		{
			SDrawLineAndIconPathDrawerCommand* cmd =
					(SDrawLineAndIconPathDrawerCommand*) commandData;
			float arrColor[4];
			toFloatArr(&cmd->color, cmd->alpha, arrColor);
			clb->LineDrawerDrawLineAndIcon(cmd->cmdId, float3(cmd->endPos),
					arrColor);
			break;
		}
		case COMMAND_DRAWER_PATH_DRAW_ICON_AT_LAST_POS:
		{
			SDrawIconAtLastPosPathDrawerCommand* cmd =
					(SDrawIconAtLastPosPathDrawerCommand*) commandData;
			clb->LineDrawerDrawIconAtLastPos(cmd->cmdId);
			break;
		}
		case COMMAND_DRAWER_PATH_BREAK:
		{
			SBreakPathDrawerCommand* cmd =
					(SBreakPathDrawerCommand*) commandData;
			float arrColor[4];
			toFloatArr(&cmd->color, cmd->alpha, arrColor);
			clb->LineDrawerBreak(float3(cmd->endPos), arrColor);
			break;
		}
		case COMMAND_DRAWER_PATH_RESTART:
		{
			SRestartPathDrawerCommand* cmd =
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
					clb->CreateSplineFigure(float3(cmd->pos1),
							float3(cmd->pos2), float3(cmd->pos3),
							float3(cmd->pos4), cmd->width, cmd->arrow,
							cmd->lifeTime, cmd->figureGroupId);
			break;
		}
		case COMMAND_DRAWER_FIGURE_CREATE_LINE:
		{
			SCreateLineFigureDrawerCommand* cmd =
					(SCreateLineFigureDrawerCommand*) commandData;
			cmd->ret_newFigureGroupId = clb->CreateLineFigure(float3(cmd->pos1),
					float3(cmd->pos2), cmd->width, cmd->arrow, cmd->lifeTime,
					cmd->figureGroupId);
			break;
		}
		case COMMAND_DRAWER_FIGURE_SET_COLOR:
		{
			SSetColorFigureDrawerCommand* cmd =
					(SSetColorFigureDrawerCommand*) commandData;
			clb->SetFigureColor(cmd->figureGroupId, cmd->color.x, cmd->color.y,
					cmd->color.z, cmd->alpha);
			break;
		}
		case COMMAND_DRAWER_FIGURE_DELETE:
		{
			SDeleteFigureDrawerCommand* cmd =
					(SDeleteFigureDrawerCommand*) commandData;
			clb->DeleteFigureGroup(cmd->figureGroupId);
			break;
		}
		case COMMAND_DRAWER_DRAW_UNIT:
		{
			SDrawUnitDrawerCommand* cmd = (SDrawUnitDrawerCommand*) commandData;
			clb->DrawUnit(getUnitDefById(teamId,
					cmd->toDrawUnitDefId)->name.c_str(), float3(cmd->pos),
					cmd->rotation, cmd->lifeTime, cmd->teamId, cmd->transparent,
					cmd->drawBorder, cmd->facing);
			break;
		}
		case COMMAND_TRACE_RAY: {
			STraceRayCommand* cCmdData = (STraceRayCommand*) commandData;
			AIHCTraceRay cppCmdData = {
				cCmdData->rayPos,
				cCmdData->rayDir,
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
			SPauseCommand* cmd = (SPauseCommand*) commandData;
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
				SStopUnitCommand* cmd = (SStopUnitCommand*) commandData;
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
	return SpringVersion::Major;
}
EXPORT(const char*) skirmishAiCallback_Engine_Version_getMinor(int teamId) {
	return SpringVersion::Minor;
}
EXPORT(const char*) skirmishAiCallback_Engine_Version_getPatchset(int teamId) {
	return SpringVersion::Patchset;
}
EXPORT(const char*) skirmishAiCallback_Engine_Version_getAdditional(int teamId) {
	return SpringVersion::Additional;
}
EXPORT(const char*) skirmishAiCallback_Engine_Version_getBuildTime(int teamId) {
	return SpringVersion::BuildTime;
}
EXPORT(const char*) skirmishAiCallback_Engine_Version_getNormal(int teamId) {
	return SpringVersion::Get().c_str();
}
EXPORT(const char*) skirmishAiCallback_Engine_Version_getFull(int teamId) {
	return SpringVersion::GetFull().c_str();
}

EXPORT(int) skirmishAiCallback_Teams_getSize(int teamId) {
	return teamHandler->ActiveTeams();
}

EXPORT(int) skirmishAiCallback_SkirmishAIs_getSize(int teamId) {
	return gameSetup->GetSkirmishAIs();
}
EXPORT(int) skirmishAiCallback_SkirmishAIs_getMax(int teamId) {
	return MAX_TEAMS;
}

static inline const CSkirmishAILibraryInfo* getSkirmishAILibraryInfo(int teamId) {

	const SkirmishAIKey* key = eoh->GetSkirmishAIKey(teamId);
	const IAILibraryManager* libMan = IAILibraryManager::GetInstance();
	IAILibraryManager::T_skirmishAIInfos infs = libMan->GetSkirmishAIInfos();
	IAILibraryManager::T_skirmishAIInfos::const_iterator inf = infs.find(*key);
	if (key != NULL && inf != infs.end()) {
		return inf->second;
	} else {
		return NULL;
	}
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

	const IAILibraryManager* libMan = IAILibraryManager::GetInstance();
	const std::map<std::string, std::string>& options = libMan->GetSkirmishAIOptionValues(teamId);
	return (int)options.size();
}
EXPORT(const char*) skirmishAiCallback_SkirmishAI_OptionValues_getKey(int teamId, int optionIndex) {

	const IAILibraryManager* libMan = IAILibraryManager::GetInstance();
	const std::vector<std::string>& optionKeys = libMan->GetSkirmishAIOptionValueKeys(teamId);
	if (checkOptionIndex(optionIndex, optionKeys)) {
		return NULL;
	} else {
		const std::string& key = *(optionKeys.begin() + optionIndex);
		return key.c_str();
	}
}
EXPORT(const char*) skirmishAiCallback_SkirmishAI_OptionValues_getValue(int teamId, int optionIndex) {

	const IAILibraryManager* libMan = IAILibraryManager::GetInstance();
	const std::vector<std::string>& optionKeys = libMan->GetSkirmishAIOptionValueKeys(teamId);
	if (checkOptionIndex(optionIndex, optionKeys)) {
		return NULL;
	} else {
		const std::map<std::string, std::string>& options = libMan->GetSkirmishAIOptionValues(teamId);
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

	const IAILibraryManager* libMan = IAILibraryManager::GetInstance();
	const std::map<std::string, std::string>& options = libMan->GetSkirmishAIOptionValues(teamId);
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
		eoh->DestroySkirmishAI(teamId);
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

	char ps = skirmishAiCallback_DataDirs_getPathSeparator(teamId);
	std::string aiShortName = skirmishAiCallback_SkirmishAI_Info_getValueByKey(teamId, SKIRMISH_AI_PROPERTY_SHORT_NAME);
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
	bool fetchOk = skirmishAiCallback_DataDirs_locatePath(teamId, path, path_sizeMax, relPath, writeable, create, dir, common);

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
		bool exists = skirmishAiCallback_DataDirs_locatePath(teamId,
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
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		team_cheatCallback[teamId]->EnableCheatEvents(enabled);
		return true;
	} else {
		return false;
	}
}

EXPORT(bool) skirmishAiCallback_Cheats_isOnlyPassive(int teamId) {
	return CAICheats::IsPassive();
}

EXPORT(int) skirmishAiCallback_Game_getAiInterfaceVersion(int teamId) {
	IAICallback* clb = team_callback[teamId];
	return wrapper_HandleCommand(clb, NULL, AIHCQuerySubVersionId, NULL);
}

EXPORT(int) skirmishAiCallback_Game_getCurrentFrame(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetCurrentFrame();
}

EXPORT(int) skirmishAiCallback_Game_getMyTeam(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMyTeam();
}

EXPORT(int) skirmishAiCallback_Game_getMyAllyTeam(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMyAllyTeam();
}

EXPORT(int) skirmishAiCallback_Game_getPlayerTeam(int teamId, int player) {
	IAICallback* clb = team_callback[teamId]; return clb->GetPlayerTeam(player);
}

EXPORT(const char*) skirmishAiCallback_Game_getTeamSide(int teamId, int team) {
	IAICallback* clb = team_callback[teamId]; return clb->GetTeamSide(team);
}





EXPORT(int) skirmishAiCallback_WeaponDef_0STATIC0getNumDamageTypes(int teamId) {
	int numDamageTypes;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_NUMDAMAGETYPES, &numDamageTypes);
	if (!fetchOk) {
		numDamageTypes = -1;
	}
	return numDamageTypes;
}

EXPORT(bool) skirmishAiCallback_Game_isExceptionHandlingEnabled(int teamId) {
	bool exceptionHandlingEnabled;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_EXCEPTION_HANDLING,
			&exceptionHandlingEnabled);
	if (!fetchOk) {
		exceptionHandlingEnabled = false;
	}
	return exceptionHandlingEnabled;
}
EXPORT(bool) skirmishAiCallback_Game_isDebugModeEnabled(int teamId) {
	bool debugModeEnabled;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_DEBUG_MODE, &debugModeEnabled);
	if (!fetchOk) {
		debugModeEnabled = false;
	}
	return debugModeEnabled;
}
EXPORT(int) skirmishAiCallback_Game_getMode(int teamId) {
	int mode;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_GAME_MODE, &mode);
	if (!fetchOk) {
		mode = -1;
	}
	return mode;
}
EXPORT(bool) skirmishAiCallback_Game_isPaused(int teamId) {
	bool paused;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_GAME_PAUSED, &paused);
	if (!fetchOk) {
		paused = false;
	}
	return paused;
}
EXPORT(float) skirmishAiCallback_Game_getSpeedFactor(int teamId) {
	float speedFactor;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_GAME_SPEED_FACTOR, &speedFactor);
	if (!fetchOk) {
		speedFactor = false;
	}
	return speedFactor;
}

EXPORT(float) skirmishAiCallback_Gui_getViewRange(int teamId) {
	float viewRange;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_GUI_VIEW_RANGE, &viewRange);
	if (!fetchOk) {
		viewRange = false;
	}
	return viewRange;
}
EXPORT(float) skirmishAiCallback_Gui_getScreenX(int teamId) {
	float screenX;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_GUI_SCREENX, &screenX);
	if (!fetchOk) {
		screenX = false;
	}
	return screenX;
}
EXPORT(float) skirmishAiCallback_Gui_getScreenY(int teamId) {
	float screenY;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_GUI_SCREENY, &screenY);
	if (!fetchOk) {
		screenY = false;
	}
	return screenY;
}
EXPORT(SAIFloat3) skirmishAiCallback_Gui_Camera_getDirection(int teamId) {
	float3 cameraDir;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_GUI_CAMERA_DIR, &cameraDir);
	if (!fetchOk) {
		cameraDir = float3(1.0f, 0.0f, 0.0f);
	}
	return cameraDir.toSAIFloat3();
}
EXPORT(SAIFloat3) skirmishAiCallback_Gui_Camera_getPosition(int teamId) {
	float3 cameraPosition;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_GUI_CAMERA_POS, &cameraPosition);
	if (!fetchOk) {
		cameraPosition = float3(1.0f, 0.0f, 0.0f);
	}
	return cameraPosition.toSAIFloat3();
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
EXPORT(bool) skirmishAiCallback_Map_isPosInCamera(int teamId, SAIFloat3 pos, float radius) {
	IAICallback* clb = team_callback[teamId];
	return clb->PosInCamera(SAIFloat3(pos), radius);
}
EXPORT(unsigned int) skirmishAiCallback_Map_getChecksum(int teamId) {
	unsigned int checksum;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_MAP_CHECKSUM, &checksum);
	if (!fetchOk) {
		checksum = -1;
	}
	return checksum;
}
EXPORT(int) skirmishAiCallback_Map_getWidth(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMapWidth();
}

EXPORT(int) skirmishAiCallback_Map_getHeight(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMapHeight();
}

EXPORT(int) skirmishAiCallback_Map_0ARRAY1SIZE0getHeightMap(int teamId) {
	return gs->mapx * gs->mapy;
}
EXPORT(int) skirmishAiCallback_Map_0ARRAY1VALS0getHeightMap(int teamId, float heights[],
		int heights_max) {

	IAICallback* clb = team_callback[teamId];
	const float* tmpMap = clb->GetHeightMap();
	int size = skirmishAiCallback_Map_0ARRAY1SIZE0getHeightMap(teamId);
	size = min(size, heights_max);
	int i;
	for (i=0; i < size; ++i) {
		heights[i] = tmpMap[i];
	}
	return size;
}

EXPORT(float) skirmishAiCallback_Map_getMinHeight(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMinHeight();
}

EXPORT(float) skirmishAiCallback_Map_getMaxHeight(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMaxHeight();
}

EXPORT(int) skirmishAiCallback_Map_0ARRAY1SIZE0getSlopeMap(int teamId) {
	return gs->hmapx * gs->hmapy;
}
EXPORT(int) skirmishAiCallback_Map_0ARRAY1VALS0getSlopeMap(int teamId, float slopes[],
		int slopes_max) {

	IAICallback* clb = team_callback[teamId];
	const float* tmpMap = clb->GetSlopeMap();
	int size = skirmishAiCallback_Map_0ARRAY1SIZE0getSlopeMap(teamId);
	size = min(size, slopes_max);
	int i;
	for (i=0; i < size; ++i) {
		slopes[i] = tmpMap[i];
	}
	return size;
}

EXPORT(int) skirmishAiCallback_Map_0ARRAY1SIZE0getLosMap(int teamId) {
	return loshandler->losSizeX * loshandler->losSizeY;
}
EXPORT(int) skirmishAiCallback_Map_0ARRAY1VALS0getLosMap(int teamId,
		unsigned short losValues[], int losValues_max) {

	IAICallback* clb = team_callback[teamId];
	const unsigned short* tmpMap = clb->GetLosMap();
	int size = skirmishAiCallback_Map_0ARRAY1SIZE0getLosMap(teamId);
	size = min(size, losValues_max);
	int i;
	for (i=0; i < size; ++i) {
		losValues[i] = tmpMap[i];
	}
	return size;
}

EXPORT(int) skirmishAiCallback_Map_0ARRAY1SIZE0getRadarMap(int teamId) {
	return radarhandler->xsize * radarhandler->zsize;
}
EXPORT(int) skirmishAiCallback_Map_0ARRAY1VALS0getRadarMap(int teamId,
		unsigned short radarValues[], int radarValues_max) {

	IAICallback* clb = team_callback[teamId];
	const unsigned short* tmpMap = clb->GetRadarMap();
	int size = skirmishAiCallback_Map_0ARRAY1SIZE0getRadarMap(teamId);
	size = min(size, radarValues_max);
	int i;
	for (i=0; i < size; ++i) {
		radarValues[i] = tmpMap[i];
	}
	return size;
}

EXPORT(int) skirmishAiCallback_Map_0ARRAY1SIZE0getJammerMap(int teamId) {
	// Yes, it is correct, jammer-map has the same size as the radar map
	return radarhandler->xsize * radarhandler->zsize;
}
EXPORT(int) skirmishAiCallback_Map_0ARRAY1VALS0getJammerMap(int teamId,
		unsigned short jammerValues[], int jammerValues_max) {

	IAICallback* clb = team_callback[teamId];
	const unsigned short* tmpMap = clb->GetJammerMap();
	int size = skirmishAiCallback_Map_0ARRAY1SIZE0getJammerMap(teamId);
	size = min(size, jammerValues_max);
	int i;
	for (i=0; i < size; ++i) {
		jammerValues[i] = tmpMap[i];
	}
	return size;
}

EXPORT(int) skirmishAiCallback_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMapRaw(
		int teamId, int resourceId) {

	if (resourceId == resourceHandler->GetMetalId()) {
		return readmap->metalMap->GetSizeX() * readmap->metalMap->GetSizeZ();
	} else {
		return 0;
	}
}
EXPORT(int) skirmishAiCallback_Map_0ARRAY1VALS0REF1Resource2resourceId0getResourceMapRaw(
		int teamId, int resourceId, unsigned char resources[], int resources_max) {

	IAICallback* clb = team_callback[teamId];
	int size = skirmishAiCallback_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMapRaw(
			teamId, resourceId);
	size = min(size, resources_max);

	const unsigned char* tmpMap;
	if (resourceId == resourceHandler->GetMetalId()) {
		tmpMap = clb->GetMetalMap();
	} else {
		tmpMap = NULL;
		size = 0;
	}

	int i;
	for (i=0; i < size; ++i) {
		resources[i] = tmpMap[i];
	}

	return size;
}
static inline const CResourceMapAnalyzer* getResourceMapAnalyzer(int resourceId) {
	return resourceHandler->GetResourceMapAnalyzer(resourceId);
}
EXPORT(int) skirmishAiCallback_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMapSpotsPositions(
		int teamId, int resourceId) {

	const std::vector<float3>& intSpots = getResourceMapAnalyzer(resourceId)->GetSpots();
	const size_t intSpots_size = intSpots.size();
	return static_cast<int>(intSpots_size);
}
EXPORT(int) skirmishAiCallback_Map_0ARRAY1VALS0REF1Resource2resourceId0getResourceMapSpotsPositions(
		int teamId, int resourceId, SAIFloat3 spots[], int spots_max) {

	const std::vector<float3>& intSpots = getResourceMapAnalyzer(resourceId)->GetSpots();
	const size_t spots_size = min(intSpots.size(), max(0, spots_max));

	std::vector<float3>::const_iterator s;
	size_t si = 0;
	for (s = intSpots.begin(); s != intSpots.end() && si < spots_size; ++s) {
		spots[si++] = s->toSAIFloat3();
	}

	return static_cast<int>(spots_size);
}
EXPORT(float) skirmishAiCallback_Map_0ARRAY1VALS0REF1Resource2resourceId0initResourceMapSpotsAverageIncome(
		int teamId, int resourceId) {
	return getResourceMapAnalyzer(resourceId)->GetAverageIncome();
}
EXPORT(struct SAIFloat3) skirmishAiCallback_Map_0ARRAY1VALS0REF1Resource2resourceId0initResourceMapSpotsNearest(
		int teamId, int resourceId, struct SAIFloat3 pos) {
	return getResourceMapAnalyzer(resourceId)->GetNearestSpot(pos, teamId).toSAIFloat3();
}

EXPORT(const char*) skirmishAiCallback_Map_getName(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMapName();
}

EXPORT(float) skirmishAiCallback_Map_getElevationAt(int teamId, float x, float z) {
	IAICallback* clb = team_callback[teamId]; return clb->GetElevation(x, z);
}

EXPORT(float) skirmishAiCallback_Map_0REF1Resource2resourceId0getMaxResource(int teamId,
		int resourceId) {

	IAICallback* clb = team_callback[teamId];
	if (resourceId == resourceHandler->GetMetalId()) {
		return clb->GetMaxMetal();
	} else {
		return NULL;
	}
}

EXPORT(float) skirmishAiCallback_Map_0REF1Resource2resourceId0getExtractorRadius(int teamId,
		int resourceId) {

	IAICallback* clb = team_callback[teamId];
	if (resourceId == resourceHandler->GetMetalId()) {
		return clb->GetExtractorRadius();
	} else {
		return NULL;
	}
}

EXPORT(float) skirmishAiCallback_Map_getMinWind(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMinWind();
}

EXPORT(float) skirmishAiCallback_Map_getMaxWind(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMaxWind();
}

EXPORT(float) skirmishAiCallback_Map_getTidalStrength(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetTidalStrength();
}

EXPORT(float) skirmishAiCallback_Map_getGravity(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetGravity();
}

EXPORT(bool) skirmishAiCallback_Map_0REF1UnitDef2unitDefId0isPossibleToBuildAt(int teamId, int unitDefId,
		SAIFloat3 pos, int facing) {
	IAICallback* clb = team_callback[teamId];
	const UnitDef* unitDef = getUnitDefById(teamId, unitDefId);
	return clb->CanBuildAt(unitDef, pos, facing);
}

EXPORT(SAIFloat3) skirmishAiCallback_Map_0REF1UnitDef2unitDefId0findClosestBuildSite(int teamId, int unitDefId,
		SAIFloat3 pos, float searchRadius, int minDist, int facing) {
	IAICallback* clb = team_callback[teamId];
	const UnitDef* unitDef = getUnitDefById(teamId, unitDefId);
	return clb->ClosestBuildSite(unitDef, pos, searchRadius, minDist, facing)
			.toSAIFloat3();
}

EXPORT(int) skirmishAiCallback_Map_0MULTI1SIZE0Point(int teamId, bool includeAllies) {
	return team_callback[teamId]->GetMapPoints(tmpPointMarkerArr[teamId],
			TMP_ARR_SIZE, includeAllies);
}
EXPORT(struct SAIFloat3) skirmishAiCallback_Map_Point_getPosition(int teamId, int pointId) {
	return tmpPointMarkerArr[teamId][pointId].pos.toSAIFloat3();
}
EXPORT(struct SAIFloat3) skirmishAiCallback_Map_Point_getColor(int teamId, int pointId) {

	unsigned char* color = tmpPointMarkerArr[teamId][pointId].color;
	SAIFloat3 f3color = {color[0], color[1], color[2]};
	return f3color;
}
EXPORT(const char*) skirmishAiCallback_Map_Point_getLabel(int teamId, int pointId) {
	return tmpPointMarkerArr[teamId][pointId].label;
}

EXPORT(int) skirmishAiCallback_Map_0MULTI1SIZE0Line(int teamId, bool includeAllies) {
	return team_callback[teamId]->GetMapLines(tmpLineMarkerArr[teamId],
			TMP_ARR_SIZE, includeAllies);
}
EXPORT(struct SAIFloat3) skirmishAiCallback_Map_Line_getFirstPosition(int teamId, int lineId) {
	return tmpLineMarkerArr[teamId][lineId].pos.toSAIFloat3();
}
EXPORT(struct SAIFloat3) skirmishAiCallback_Map_Line_getSecondPosition(int teamId, int lineId) {
	return tmpLineMarkerArr[teamId][lineId].pos2.toSAIFloat3();
}
EXPORT(struct SAIFloat3) skirmishAiCallback_Map_Line_getColor(int teamId, int lineId) {

	unsigned char* color = tmpLineMarkerArr[teamId][lineId].color;
	SAIFloat3 f3color = {color[0], color[1], color[2]};
	return f3color;
}
EXPORT(SAIFloat3) skirmishAiCallback_Map_getStartPos(int teamId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetStartPos()->toSAIFloat3();
}
EXPORT(SAIFloat3) skirmishAiCallback_Map_getMousePos(int teamId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetMousePos().toSAIFloat3();
}
//########### END Map


// DEPRECATED
/*
EXPORT(bool) skirmishAiCallback_getProperty(int teamId, int id, int property, void* dst) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetProperty(id, property, dst);
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetProperty(id, property, dst);
	} else {
		return team_callback[teamId]->GetProperty(id, property, dst);
	}
}

EXPORT(bool) skirmishAiCallback_getValue(int teamId, int id, void* dst) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetValue(id, dst);
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetValue(id, dst);
	} else {
		return team_callback[teamId]->GetValue(id, dst);
	}
}
*/

EXPORT(int) skirmishAiCallback_File_getSize(int teamId, const char* fileName) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFileSize(fileName);
}

EXPORT(bool) skirmishAiCallback_File_getContent(int teamId, const char* fileName, void* buffer,
		int bufferLen) {
	IAICallback* clb = team_callback[teamId]; return clb->ReadFile(fileName,
			buffer, bufferLen);
}





// BEGINN OBJECT Resource
EXPORT(int) skirmishAiCallback_0MULTI1SIZE0Resource(int teamId) {
	return resourceHandler->GetNumResources();
}
EXPORT(int) skirmishAiCallback_0MULTI1FETCH3ResourceByName0Resource(int teamId,
		const char* resourceName) {
	return resourceHandler->GetResourceId(resourceName);
}
EXPORT(const char*) skirmishAiCallback_Resource_getName(int teamId, int resourceId) {
	return resourceHandler->GetResource(resourceId)->name.c_str();
}
EXPORT(float) skirmishAiCallback_Resource_getOptimum(int teamId, int resourceId) {
	return resourceHandler->GetResource(resourceId)->optimum;
}
EXPORT(float) skirmishAiCallback_Economy_0REF1Resource2resourceId0getCurrent(int teamId,
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

EXPORT(float) skirmishAiCallback_Economy_0REF1Resource2resourceId0getIncome(int teamId,
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

EXPORT(float) skirmishAiCallback_Economy_0REF1Resource2resourceId0getUsage(int teamId,
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

EXPORT(float) skirmishAiCallback_Economy_0REF1Resource2resourceId0getStorage(int teamId,
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
	bool fetchOk = clb->GetValue(AIVAL_SCRIPT, &setupScript);
	if (!fetchOk) {
		return NULL;
	}
	return setupScript;
}







//########### BEGINN UnitDef
EXPORT(int) skirmishAiCallback_0MULTI1SIZE0UnitDef(int teamId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetNumUnitDefs();
}
EXPORT(int) skirmishAiCallback_0MULTI1VALS0UnitDef(int teamId, int unitDefIds[],
		int unitDefIds_max) {

	IAICallback* clb = team_callback[teamId];
	int size = clb->GetNumUnitDefs();
	const UnitDef** defList = (const UnitDef**) new UnitDef*[size];
	clb->GetUnitDefList(defList);

	size = min(size, unitDefIds_max);
	int i;
	for (i=0; i < size; ++i) {
		unitDefIds[i] = defList[i]->id;
	}
	delete [] defList;
	defList = NULL;

	return size;
}
EXPORT(int) skirmishAiCallback_0MULTI1FETCH3UnitDefByName0UnitDef(int teamId,
		const char* unitName) {

	int unitDefId = -1;

	IAICallback* clb = team_callback[teamId];
	const UnitDef* ud = clb->GetUnitDef(unitName);
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
EXPORT(float) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getUpkeep(int teamId,
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
EXPORT(float) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getResourceMake(int teamId,
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
EXPORT(float) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getMakesResource(int teamId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->makesMetal;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getCost(int teamId,
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
EXPORT(float) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getExtractsResource(
		int teamId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->extractsMetal;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getResourceExtractorRange(
		int teamId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->extractRange;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getWindResourceGenerator(
		int teamId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == resourceHandler->GetEnergyId()) {
		return ud->windGenerator;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getTidalResourceGenerator(
		int teamId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == resourceHandler->GetEnergyId()) {
		return ud->tidalGenerator;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getStorage(int teamId,
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
EXPORT(bool) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0isSquareResourceExtractor(
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
EXPORT(unsigned int) skirmishAiCallback_UnitDef_getCategory(int teamId, int unitDefId) {
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
EXPORT(SAIFloat3) skirmishAiCallback_UnitDef_FlankingBonus_getDir(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->flankingBonusDir.toSAIFloat3();
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
EXPORT(SAIFloat3) skirmishAiCallback_UnitDef_CollisionVolume_getScales(int teamId,
		int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->collisionVolumeScales
			.toSAIFloat3();
}
EXPORT(SAIFloat3) skirmishAiCallback_UnitDef_CollisionVolume_getOffsets(int teamId,
		int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->collisionVolumeOffsets
			.toSAIFloat3();
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
EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToBuild(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canBuild;}
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
//const unsigned char** _UnitDef_getYardMaps(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->yardmaps;}
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
EXPORT(unsigned int) skirmishAiCallback_UnitDef_getNoChaseCategory(int teamId, int unitDefId) {
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
EXPORT(SAIFloat3) skirmishAiCallback_UnitDef_getFlareDropVector(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->flareDropVector.toSAIFloat3();
}
EXPORT(int) skirmishAiCallback_UnitDef_getFlareTime(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareTime;}
EXPORT(int) skirmishAiCallback_UnitDef_getFlareSalvoSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareSalvoSize;}
EXPORT(int) skirmishAiCallback_UnitDef_getFlareSalvoDelay(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->flareSalvoDelay;
}
//EXPORT(bool) skirmishAiCallback_UnitDef_isSmoothAnim(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->smoothAnim;}
EXPORT(bool) skirmishAiCallback_UnitDef_0REF1Resource2resourceId0isResourceMaker(int teamId,
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
EXPORT(bool) skirmishAiCallback_UnitDef_isLevelGround(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->levelGround;}
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
EXPORT(bool) skirmishAiCallback_UnitDef_isFirePlatform(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->isFirePlatform;}
EXPORT(float) skirmishAiCallback_UnitDef_getMaxFuel(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxFuel;}
EXPORT(float) skirmishAiCallback_UnitDef_getRefuelTime(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->refuelTime;}
EXPORT(float) skirmishAiCallback_UnitDef_getMinAirBasePower(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->minAirBasePower;
}
EXPORT(int) skirmishAiCallback_UnitDef_getMaxThisUnit(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxThisUnit;}
EXPORT(int) skirmishAiCallback_UnitDef_0SINGLE1FETCH2UnitDef0getDecoyDef(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->decoyDef->id;
}
EXPORT(bool) skirmishAiCallback_UnitDef_isDontLand(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->DontLand();}
EXPORT(int) skirmishAiCallback_UnitDef_0SINGLE1FETCH2WeaponDef0getShieldDef(int teamId,
		int unitDefId) {
	const WeaponDef* wd = getUnitDefById(teamId, unitDefId)->shieldWeaponDef;
	if (wd == NULL)
		return -1;
	else
		return wd->id;
}
EXPORT(int) skirmishAiCallback_UnitDef_0SINGLE1FETCH2WeaponDef0getStockpileDef(int teamId,
		int unitDefId) {
	const WeaponDef* wd = getUnitDefById(teamId, unitDefId)->stockpileWeaponDef;
	if (wd == NULL)
		return -1;
	else
		return wd->id;
}
EXPORT(int) skirmishAiCallback_UnitDef_0ARRAY1SIZE1UnitDef0getBuildOptions(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->buildOptions.size();
}
EXPORT(int) skirmishAiCallback_UnitDef_0ARRAY1VALS1UnitDef0getBuildOptions(int teamId,
		int unitDefId, int* unitDefIds, int unitDefIds_max) {
	const std::map<int,std::string>& bo = getUnitDefById(teamId, unitDefId)->buildOptions;
	std::map<int,std::string>::const_iterator bb;
	int b;
	for (b=0, bb = bo.begin(); bb != bo.end() && b < unitDefIds_max; ++b, ++bb) {
		unitDefIds[b] = skirmishAiCallback_0MULTI1FETCH3UnitDefByName0UnitDef(teamId, bb->second.c_str());
	}
	return b;
}
EXPORT(int) skirmishAiCallback_UnitDef_0MAP1SIZE0getCustomParams(int teamId, int unitDefId) {

	tmpSize[teamId] =
			fillCMap(&(getUnitDefById(teamId, unitDefId)->customParams),
					tmpKeysArr[teamId], tmpValuesArr[teamId]);
	return tmpSize[teamId];
}
EXPORT(void) skirmishAiCallback_UnitDef_0MAP1KEYS0getCustomParams(int teamId, int unitDefId,
		const char* keys[]) {
	copyStringArr(keys, tmpKeysArr[teamId], tmpSize[teamId]);
}
EXPORT(void) skirmishAiCallback_UnitDef_0MAP1VALS0getCustomParams(int teamId, int unitDefId,
		const char* values[]) {
	copyStringArr(values, tmpValuesArr[teamId], tmpSize[teamId]);
}
EXPORT(bool) skirmishAiCallback_UnitDef_0AVAILABLE0MoveData(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata != NULL;}
EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getMoveType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->moveType;}
EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getMoveFamily(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->moveFamily;}
EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->size;}
EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getDepth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->depth;}
EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getMaxSlope(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->maxSlope;}
EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getSlopeMod(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->slopeMod;}
EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getDepthMod(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->depthMod;}
EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getPathType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->pathType;}
EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getCrushStrength(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->crushStrength;}
EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getMaxSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->maxSpeed;}
EXPORT(short) skirmishAiCallback_UnitDef_MoveData_getMaxTurnRate(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->maxTurnRate;}
EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getMaxAcceleration(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->maxAcceleration;}
EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getMaxBreaking(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->maxBreaking;}
EXPORT(bool) skirmishAiCallback_UnitDef_MoveData_isSubMarine(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->subMarine;}
EXPORT(int) skirmishAiCallback_UnitDef_0MULTI1SIZE0WeaponMount(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->weapons.size();}
EXPORT(const char*) skirmishAiCallback_UnitDef_WeaponMount_getName(int teamId, int unitDefId, int weaponMountId) {
	return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).name.c_str();
}
EXPORT(int) skirmishAiCallback_UnitDef_WeaponMount_0SINGLE1FETCH2WeaponDef0getWeaponDef(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).def->id;}
EXPORT(int) skirmishAiCallback_UnitDef_WeaponMount_getSlavedTo(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).slavedTo;}
EXPORT(SAIFloat3) skirmishAiCallback_UnitDef_WeaponMount_getMainDir(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).mainDir.toSAIFloat3();}
EXPORT(float) skirmishAiCallback_UnitDef_WeaponMount_getMaxAngleDif(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).maxAngleDif;}
EXPORT(float) skirmishAiCallback_UnitDef_WeaponMount_getFuelUsage(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).fuelUsage;}
EXPORT(unsigned int) skirmishAiCallback_UnitDef_WeaponMount_getBadTargetCategory(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).badTargetCat;}
EXPORT(unsigned int) skirmishAiCallback_UnitDef_WeaponMount_getOnlyTargetCategory(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).onlyTargetCat;}
//########### END UnitDef





//########### BEGINN Unit
EXPORT(int) skirmishAiCallback_Unit_0STATIC0getLimit(int teamId) {
	int unitLimit;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_UNIT_LIMIT, &unitLimit);
	if (!fetchOk) {
		unitLimit = -1;
	}
	return unitLimit;
}
EXPORT(int) skirmishAiCallback_Unit_0SINGLE1FETCH2UnitDef0getDef(int teamId, int unitId) {
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


EXPORT(int) skirmishAiCallback_Unit_0MULTI1SIZE0ModParam(int teamId, int unitId) {

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
//	IAICallback* clb = team_callback[teamId]; return clb->GetUnitTeam(unitId);
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitTeam(unitId);
	} else {
		return team_callback[teamId]->GetUnitTeam(unitId);
	}
}
EXPORT(int) skirmishAiCallback_Unit_getAllyTeam(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetUnitAllyTeam(unitId);
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
	IAICallback* clb = team_callback[teamId]; return clb->GetUnitAiHint(unitId);
}

EXPORT(int) skirmishAiCallback_Unit_0MULTI1SIZE0SupportedCommand(int teamId, int unitId) {
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
EXPORT(int) skirmishAiCallback_Unit_SupportedCommand_0ARRAY1SIZE0getParams(int teamId, int unitId, int commandId) {
	return team_callback[teamId]->GetUnitCommands(unitId)->at(commandId).params.size();
}
EXPORT(int) skirmishAiCallback_Unit_SupportedCommand_0ARRAY1VALS0getParams(int teamId,
		int unitId, int commandId, const char** params, int params_max) {

	const std::vector<std::string> ps
			= team_callback[teamId]->GetUnitCommands(unitId)->at(commandId).params;
	int size = ps.size();
	if (params_max < size) {
		size = params_max;
	}

	int p;
	for (p=0; p < size; p++) {
		params[p] = ps.at(p).c_str();
	}

	return size;
}

EXPORT(int) skirmishAiCallback_Unit_getStockpile(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId];
	int stockpile;
	bool fetchOk = clb->GetProperty(unitId, AIVAL_STOCKPILED, &stockpile);
	if (!fetchOk) {
		stockpile = -1;
	}
	return stockpile;
}
EXPORT(int) skirmishAiCallback_Unit_getStockpileQueued(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId];
	int stockpileQueue;
	bool fetchOk = clb->GetProperty(unitId, AIVAL_STOCKPILE_QUED, &stockpileQueue);
	if (!fetchOk) {
		stockpileQueue = -1;
	}
	return stockpileQueue;
}
EXPORT(float) skirmishAiCallback_Unit_getCurrentFuel(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId];
	float currentFuel;
	bool fetchOk = clb->GetProperty(unitId, AIVAL_CURRENT_FUEL, &currentFuel);
	if (!fetchOk) {
		currentFuel = -1.0f;
	}
	return currentFuel;
}
EXPORT(float) skirmishAiCallback_Unit_getMaxSpeed(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId];
	float maxSpeed;
	bool fetchOk = clb->GetProperty(unitId, AIVAL_UNIT_MAXSPEED, &maxSpeed);
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
//	IAICallback* clb = team_callback[teamId]; return clb->GetUnitMaxHealth(unitId);
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitMaxHealth(unitId);
	} else {
		return team_callback[teamId]->GetUnitMaxHealth(unitId);
	}
}


/**
 * Returns a units command queue.
 * The return value may be <code>NULL</code> in some cases,
 * eg when cheats are disabled and we try to fetch from an enemy unit.
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
 * Returns a untis command at a specific position in its command queue.
 * The return value may be <code>NULL</code> in some cases,
 * eg when cheats are disabled and we try to fetch from an enemy unit.
 * For internal use only.
 */
static inline const Command* _intern_Unit_getCurrentCommandAt(int teamId, int unitId, int commandId) {
	const Command* c = NULL;

	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(teamId, unitId);
	if (q != NULL && commandId > 0 && static_cast<unsigned int>(commandId) < q->size()) {
		c = &(q->at(commandId));
	}

	return c;
}

EXPORT(int) skirmishAiCallback_Unit_0MULTI1SIZE1Command0CurrentCommand(int teamId, int unitId) {
	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(teamId, unitId);
	return (q? q->size(): 0);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_0STATIC0getType(int teamId, int unitId) {
	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(teamId, unitId);
	return (q? q->GetType(): -1);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getId(int teamId, int unitId, int commandId) {
	const Command* c = _intern_Unit_getCurrentCommandAt(teamId, unitId, commandId);
	return (c? c->id: 0);
}

EXPORT(unsigned char) skirmishAiCallback_Unit_CurrentCommand_getOptions(int teamId, int unitId, int commandId) {
	const Command* c = _intern_Unit_getCurrentCommandAt(teamId, unitId, commandId);
	return (c? c->options: 0);
}

EXPORT(unsigned int) skirmishAiCallback_Unit_CurrentCommand_getTag(int teamId, int unitId, int commandId) {
	const Command* c = _intern_Unit_getCurrentCommandAt(teamId, unitId, commandId);
	return (c? c->tag: 0);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getTimeOut(int teamId, int unitId, int commandId) {
	const Command* c = _intern_Unit_getCurrentCommandAt(teamId, unitId, commandId);
	return (c? c->timeOut: 0);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_0ARRAY1SIZE0getParams(int teamId, int unitId, int commandId) {
	const Command* c = _intern_Unit_getCurrentCommandAt(teamId, unitId, commandId);
	return (c? c->params.size(): 0);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_0ARRAY1VALS0getParams(int teamId,
		int unitId, int commandId, float* params, int params_sizeMax) {

	const Command* c = _intern_Unit_getCurrentCommandAt(teamId, unitId, commandId);

	if (c == NULL) {
		return -1;
	}

	const std::vector<float>& ps = c->params;
	int params_size = ps.size();
	if (params_sizeMax < params_size ) {
		params_size = params_sizeMax;
	}

	int p;
	for (p=0; p < params_size; p++) {
		params[p] = ps.at(p);
	}

	return params_size;
}




EXPORT(float) skirmishAiCallback_Unit_getExperience(int teamId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitExperience(unitId);
	} else {
		return team_callback[teamId]->GetUnitExperience(unitId);
	}
}
EXPORT(int) skirmishAiCallback_Unit_getGroup(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetUnitGroup(unitId);
}

EXPORT(float) skirmishAiCallback_Unit_getHealth(int teamId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitHealth(unitId);
	} else {
		return team_callback[teamId]->GetUnitHealth(unitId);
	}
}
EXPORT(float) skirmishAiCallback_Unit_getSpeed(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetUnitSpeed(unitId);
}
EXPORT(float) skirmishAiCallback_Unit_getPower(int teamId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitPower(unitId);
	} else {
		return team_callback[teamId]->GetUnitPower(unitId);
	}
}
EXPORT(SAIFloat3) skirmishAiCallback_Unit_getPos(int teamId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitPos(unitId).toSAIFloat3();
	} else {
		return team_callback[teamId]->GetUnitPos(unitId).toSAIFloat3();
	}
}
EXPORT(int) skirmishAiCallback_Unit_0MULTI1SIZE0ResourceInfo(int teamId, int unitId) {
	return skirmishAiCallback_0MULTI1SIZE0Resource(teamId);
}
EXPORT(float) skirmishAiCallback_Unit_0REF1Resource2resourceId0getResourceUse(int teamId,
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
EXPORT(float) skirmishAiCallback_Unit_0REF1Resource2resourceId0getResourceMake(int teamId,
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

EXPORT(int) skirmishAiCallback_0MULTI1SIZE3EnemyUnits0Unit(int teamId) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetEnemyUnits(tmpIntArr[teamId]);
	} else {
		return team_callback[teamId]->GetEnemyUnits(tmpIntArr[teamId]);
	}
}
EXPORT(int) skirmishAiCallback_0MULTI1VALS3EnemyUnits0Unit(int teamId, int* unitIds, int unitIds_max) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetEnemyUnits(unitIds, unitIds_max);
	} else {
		return team_callback[teamId]->GetEnemyUnits(unitIds, unitIds_max);
	}
}

EXPORT(int) skirmishAiCallback_0MULTI1SIZE3EnemyUnitsIn0Unit(int teamId, SAIFloat3 pos, float radius) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetEnemyUnits(tmpIntArr[teamId], float3(pos), radius);
	} else {
		return team_callback[teamId]->GetEnemyUnits(tmpIntArr[teamId], float3(pos), radius);
	}
}
EXPORT(int) skirmishAiCallback_0MULTI1VALS3EnemyUnitsIn0Unit(int teamId, SAIFloat3 pos, float radius, int* unitIds, int unitIds_max) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetEnemyUnits(unitIds, float3(pos), radius, unitIds_max);
	} else {
		return team_callback[teamId]->GetEnemyUnits(unitIds, float3(pos), radius, unitIds_max);
	}
}

EXPORT(int) skirmishAiCallback_0MULTI1SIZE3EnemyUnitsInRadarAndLos0Unit(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetEnemyUnitsInRadarAndLos(tmpIntArr[teamId]);
}
EXPORT(int) skirmishAiCallback_0MULTI1VALS3EnemyUnitsInRadarAndLos0Unit(int teamId, int* unitIds, int unitIds_max) {
	IAICallback* clb = team_callback[teamId]; return clb->GetEnemyUnitsInRadarAndLos(unitIds, unitIds_max);
}

EXPORT(int) skirmishAiCallback_0MULTI1SIZE3FriendlyUnits0Unit(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFriendlyUnits(tmpIntArr[teamId]);
}
EXPORT(int) skirmishAiCallback_0MULTI1VALS3FriendlyUnits0Unit(int teamId, int* unitIds, int unitIds_max) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFriendlyUnits(unitIds);
}

EXPORT(int) skirmishAiCallback_0MULTI1SIZE3FriendlyUnitsIn0Unit(int teamId, SAIFloat3 pos, float radius) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFriendlyUnits(tmpIntArr[teamId], float3(pos), radius);
}
EXPORT(int) skirmishAiCallback_0MULTI1VALS3FriendlyUnitsIn0Unit(int teamId, SAIFloat3 pos, float radius, int* unitIds, int unitIds_max) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFriendlyUnits(unitIds, float3(pos), radius, unitIds_max);
}

EXPORT(int) skirmishAiCallback_0MULTI1SIZE3NeutralUnits0Unit(int teamId) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetNeutralUnits(tmpIntArr[teamId]);
	} else {
		return team_callback[teamId]->GetNeutralUnits(tmpIntArr[teamId]);
	}
}
EXPORT(int) skirmishAiCallback_0MULTI1VALS3NeutralUnits0Unit(int teamId, int* unitIds, int unitIds_max) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetNeutralUnits(unitIds, unitIds_max);
	} else {
		return team_callback[teamId]->GetNeutralUnits(unitIds, unitIds_max);
	}
}

EXPORT(int) skirmishAiCallback_0MULTI1SIZE3NeutralUnitsIn0Unit(int teamId, SAIFloat3 pos, float radius) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetNeutralUnits(tmpIntArr[teamId], float3(pos), radius);
	} else {
		return team_callback[teamId]->GetNeutralUnits(tmpIntArr[teamId], float3(pos), radius);
	}
}
EXPORT(int) skirmishAiCallback_0MULTI1VALS3NeutralUnitsIn0Unit(int teamId, SAIFloat3 pos, float radius, int* unitIds, int unitIds_max) {
	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetNeutralUnits(unitIds, float3(pos), radius, unitIds_max);
	} else {
		return team_callback[teamId]->GetNeutralUnits(unitIds, float3(pos), radius, unitIds_max);
	}
}

EXPORT(int) skirmishAiCallback_0MULTI1SIZE3SelectedUnits0Unit(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetSelectedUnits(tmpIntArr[teamId]);
}
EXPORT(int) skirmishAiCallback_0MULTI1VALS3SelectedUnits0Unit(int teamId, int* unitIds, int unitIds_max) {
	IAICallback* clb = team_callback[teamId]; return clb->GetSelectedUnits(unitIds, unitIds_max);
}

EXPORT(int) skirmishAiCallback_0MULTI1SIZE3TeamUnits0Unit(int teamId) {

	int a = 0;

	for (std::list<CUnit*>::iterator ui = uh->activeUnits.begin();
			ui != uh->activeUnits.end(); ++ui) {
		CUnit* u = *ui;

		if (u->team == teamId) {
			a++;
		}
	}

	return a;
}
EXPORT(int) skirmishAiCallback_0MULTI1VALS3TeamUnits0Unit(int teamId, int* unitIds, int unitIds_max) {

	int a = 0;

	for (std::list<CUnit*>::iterator ui = uh->activeUnits.begin();
			ui != uh->activeUnits.end(); ++ui) {
		CUnit* u = *ui;

		if (u->team == teamId) {
			if (a < unitIds_max) {
				unitIds[a++] = u->id;
			} else {
				break;
			}
		}
	}

	return a;
}

//########### BEGINN FeatureDef
EXPORT(int) skirmishAiCallback_0MULTI1SIZE0FeatureDef(int teamId) {
	return featureHandler->GetFeatureDefs().size();
}
EXPORT(int) skirmishAiCallback_0MULTI1VALS0FeatureDef(int teamId, int featureDefIds[], int featureDefIds_max) {

	const std::map<std::string, const FeatureDef*>& fds
			= featureHandler->GetFeatureDefs();
	int size = fds.size();

	size = min(size, featureDefIds_max);
	int i;
	std::map<std::string, const FeatureDef*>::const_iterator fdi;
	for (i=0, fdi=fds.begin(); i < size; ++i, ++fdi) {
		featureDefIds[i] = fdi->second->id;
	}

	return size;
}

EXPORT(const char*) skirmishAiCallback_FeatureDef_getName(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->myName.c_str();}
EXPORT(const char*) skirmishAiCallback_FeatureDef_getDescription(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->description.c_str();}
EXPORT(const char*) skirmishAiCallback_FeatureDef_getFileName(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->filename.c_str();}
//EXPORT(int) skirmishAiCallback_FeatureDef_getId(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->id;}
EXPORT(float) skirmishAiCallback_FeatureDef_0REF1Resource2resourceId0getContainedResource(int teamId, int featureDefId, int resourceId) {

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
EXPORT(SAIFloat3) skirmishAiCallback_FeatureDef_CollisionVolume_getScales(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->collisionVolumeScales.toSAIFloat3();}
EXPORT(SAIFloat3) skirmishAiCallback_FeatureDef_CollisionVolume_getOffsets(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->collisionVolumeOffsets.toSAIFloat3();}
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
EXPORT(int) skirmishAiCallback_FeatureDef_0MAP1SIZE0getCustomParams(int teamId, int featureDefId) {

	tmpSize[teamId] = fillCMap(&(getFeatureDefById(teamId, featureDefId)->customParams), tmpKeysArr[teamId], tmpValuesArr[teamId]);
	return tmpSize[teamId];
}
EXPORT(void) skirmishAiCallback_FeatureDef_0MAP1KEYS0getCustomParams(int teamId, int featureDefId, const char* keys[]) {
	copyStringArr(keys, tmpKeysArr[teamId], tmpSize[teamId]);
}
EXPORT(void) skirmishAiCallback_FeatureDef_0MAP1VALS0getCustomParams(int teamId, int featureDefId, const char* values[]) {
	copyStringArr(values, tmpValuesArr[teamId], tmpSize[teamId]);
}
//########### END FeatureDef


EXPORT(int) skirmishAiCallback_0MULTI1SIZE0Feature(int teamId) {

	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return featureHandler->GetActiveFeatures().size();
	} else {
		tmpSize[teamId] = team_callback[teamId]->GetFeatures(tmpIntArr[teamId], TMP_ARR_SIZE);
		return tmpSize[teamId];
	}
}
EXPORT(int) skirmishAiCallback_0MULTI1VALS0Feature(int teamId, int* featureIds, int _featureIds_max) {

	if (_featureIds_max <= 0) {
		return 0;
	}
	const size_t featureIds_max = static_cast<size_t>(_featureIds_max);

	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		const CFeatureSet& fset = featureHandler->GetActiveFeatures();
		CFeatureSet::const_iterator it;
		size_t i=0;
		for (it = fset.begin(); it != fset.end() && i < featureIds_max; ++it) {
			CFeature *f = *it;
			assert(f);

			assert(i < featureIds_max);
			featureIds[i++] = f->id;
		}
		return i;
	} else {
		return copyIntArr(featureIds, tmpIntArr[teamId], min(tmpSize[teamId], featureIds_max));
	}
}

EXPORT(int) skirmishAiCallback_0MULTI1SIZE3FeaturesIn0Feature(int teamId, SAIFloat3 pos, float radius) {

	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		return qf->GetFeaturesExact(pos, radius).size();
	} else {
		tmpSize[teamId] = team_callback[teamId]->GetFeatures(tmpIntArr[teamId], TMP_ARR_SIZE, pos, radius);
		return tmpSize[teamId];
	}
}
EXPORT(int) skirmishAiCallback_0MULTI1VALS3FeaturesIn0Feature(int teamId, SAIFloat3 pos, float radius, int* featureIds, int _featureIds_max) {

	if (_featureIds_max <= 0) {
		return 0;
	}
	const size_t featureIds_max = static_cast<size_t>(_featureIds_max);

	if (skirmishAiCallback_Cheats_isEnabled(teamId)) {
		const vector<CFeature*>& fset = qf->GetFeaturesExact(pos, radius);
		vector<CFeature*>::const_iterator it;
		size_t i=0;
		for (it = fset.begin(); it != fset.end() && i < featureIds_max; ++it) {
			CFeature *f = *it;
			assert(f);

			assert(i < featureIds_max);
			featureIds[i++] = f->id;
		}
		return i;
	} else {
		return copyIntArr(featureIds, tmpIntArr[teamId], min(tmpSize[teamId], featureIds_max));
	}
}

EXPORT(int) skirmishAiCallback_Feature_0SINGLE1FETCH2FeatureDef0getDef(int teamId, int featureId) {
	IAICallback* clb = team_callback[teamId];
	const FeatureDef* def = clb->GetFeatureDef(featureId);
	if (def == NULL) {
		 return -1;
	} else {
		return def->id;
	}
}

EXPORT(float) skirmishAiCallback_Feature_getHealth(int teamId, int featureId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFeatureHealth(featureId);
}

EXPORT(float) skirmishAiCallback_Feature_getReclaimLeft(int teamId, int featureId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFeatureReclaimLeft(featureId);
}

EXPORT(SAIFloat3) skirmishAiCallback_Feature_getPosition(int teamId, int featureId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFeaturePos(featureId).toSAIFloat3();
}


//########### BEGINN WeaponDef
EXPORT(int) skirmishAiCallback_0MULTI1SIZE0WeaponDef(int teamId) {
	return weaponDefHandler->numWeaponDefs;
}
EXPORT(int) skirmishAiCallback_0MULTI1FETCH3WeaponDefByName0WeaponDef(int teamId, const char* weaponDefName) {

	int weaponDefId = -1;

	IAICallback* clb = team_callback[teamId];
	const WeaponDef* wd = clb->GetWeapon(weaponDefName);
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
EXPORT(const char*) skirmishAiCallback_WeaponDef_getDescription(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->description.c_str();}
EXPORT(const char*) skirmishAiCallback_WeaponDef_getFileName(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->filename.c_str();}
EXPORT(const char*) skirmishAiCallback_WeaponDef_getCegTag(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->cegTag.c_str();}
EXPORT(float) skirmishAiCallback_WeaponDef_getRange(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->range;}
EXPORT(float) skirmishAiCallback_WeaponDef_getHeightMod(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->heightmod;}
EXPORT(float) skirmishAiCallback_WeaponDef_getAccuracy(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->accuracy;}
EXPORT(float) skirmishAiCallback_WeaponDef_getSprayAngle(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->sprayAngle;}
EXPORT(float) skirmishAiCallback_WeaponDef_getMovingAccuracy(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->movingAccuracy;}
EXPORT(float) skirmishAiCallback_WeaponDef_getTargetMoveError(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->targetMoveError;}
EXPORT(float) skirmishAiCallback_WeaponDef_getLeadLimit(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->leadLimit;}
EXPORT(float) skirmishAiCallback_WeaponDef_getLeadBonus(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->leadBonus;}
EXPORT(float) skirmishAiCallback_WeaponDef_getPredictBoost(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->predictBoost;}
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
EXPORT(int) skirmishAiCallback_WeaponDef_Damage_0ARRAY1SIZE0getTypes(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->damages.GetNumTypes();
}
EXPORT(int) skirmishAiCallback_WeaponDef_Damage_0ARRAY1VALS0getTypes(int teamId, int weaponDefId, float* types, int types_max) {

	const WeaponDef* weaponDef = getWeaponDefById(teamId, weaponDefId);
	int types_size = min(weaponDef->damages.GetNumTypes(), types_max);

	for (int i=0; i < types_size; ++i) {
		types[i] = weaponDef->damages[i];
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
EXPORT(float) skirmishAiCallback_WeaponDef_0REF1Resource2resourceId0getCost(int teamId, int weaponDefId, int resourceId) {

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
EXPORT(unsigned int) skirmishAiCallback_WeaponDef_getOnlyTargetCategory(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->onlyTargetCategory;}
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
EXPORT(float) skirmishAiCallback_WeaponDef_Shield_0REF1Resource2resourceId0getResourceUse(int teamId, int weaponDefId, int resourceId) {

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
EXPORT(float) skirmishAiCallback_WeaponDef_Shield_0REF1Resource2resourceId0getPowerRegenResource(int teamId, int weaponDefId, int resourceId) {

	const WeaponDef* wd = getWeaponDefById(teamId, weaponDefId);
	if (resourceId == resourceHandler->GetEnergyId()) {
		return wd->shieldPowerRegenEnergy;
	} else {
		return 0.0f;
	}
}
EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getStartingPower(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldStartingPower;}
EXPORT(int) skirmishAiCallback_WeaponDef_Shield_getRechargeDelay(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldRechargeDelay;}
EXPORT(struct SAIFloat3) skirmishAiCallback_WeaponDef_Shield_getGoodColor(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldGoodColor.toSAIFloat3();}
EXPORT(struct SAIFloat3) skirmishAiCallback_WeaponDef_Shield_getBadColor(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldBadColor.toSAIFloat3();}
EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getAlpha(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldAlpha;}
EXPORT(unsigned int) skirmishAiCallback_WeaponDef_Shield_getInterceptType(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldInterceptType;}
EXPORT(unsigned int) skirmishAiCallback_WeaponDef_getInterceptedByShieldType(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->interceptedByShieldType;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isAvoidFriendly(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->avoidFriendly;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isAvoidFeature(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->avoidFeature;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isAvoidNeutral(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->avoidNeutral;}
EXPORT(float) skirmishAiCallback_WeaponDef_getTargetBorder(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->targetBorder;}
EXPORT(float) skirmishAiCallback_WeaponDef_getCylinderTargetting(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->cylinderTargetting;}
EXPORT(float) skirmishAiCallback_WeaponDef_getMinIntensity(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->minIntensity;}
EXPORT(float) skirmishAiCallback_WeaponDef_getHeightBoostFactor(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->heightBoostFactor;}
EXPORT(float) skirmishAiCallback_WeaponDef_getProximityPriority(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->proximityPriority;}
EXPORT(unsigned int) skirmishAiCallback_WeaponDef_getCollisionFlags(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->collisionFlags;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isSweepFire(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->sweepFire;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isAbleToAttackGround(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->canAttackGround;}
EXPORT(float) skirmishAiCallback_WeaponDef_getCameraShake(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->cameraShake;}
EXPORT(float) skirmishAiCallback_WeaponDef_getDynDamageExp(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dynDamageExp;}
EXPORT(float) skirmishAiCallback_WeaponDef_getDynDamageMin(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dynDamageMin;}
EXPORT(float) skirmishAiCallback_WeaponDef_getDynDamageRange(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dynDamageRange;}
EXPORT(bool) skirmishAiCallback_WeaponDef_isDynDamageInverted(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dynDamageInverted;}
EXPORT(int) skirmishAiCallback_WeaponDef_0MAP1SIZE0getCustomParams(int teamId, int weaponDefId) {

	tmpSize[teamId] = fillCMap(&(getWeaponDefById(teamId, weaponDefId)->customParams), tmpKeysArr[teamId], tmpValuesArr[teamId]);
	return tmpSize[teamId];
}
EXPORT(void) skirmishAiCallback_WeaponDef_0MAP1KEYS0getCustomParams(int teamId, int weaponDefId, const char* keys[]) {
	copyStringArr(keys, tmpKeysArr[teamId], tmpSize[teamId]);
}
EXPORT(void) skirmishAiCallback_WeaponDef_0MAP1VALS0getCustomParams(int teamId, int weaponDefId, const char* values[]) {
	copyStringArr(values, tmpValuesArr[teamId], tmpSize[teamId]);
}
//########### END WeaponDef


EXPORT(int) skirmishAiCallback_0MULTI1SIZE0Group(int teamId) {
	return grouphandlers[teamId]->groups.size();
}
EXPORT(int) skirmishAiCallback_0MULTI1VALS0Group(int teamId, int groupIds[], int groupIds_max) {

	int size = grouphandlers[teamId]->groups.size();

	size = min(size, groupIds_max);
	int i;
	for (i=0; i < size; ++i) {
		groupIds[i] = grouphandlers[teamId]->groups[i]->id;
	}

	return size;
}

EXPORT(int) skirmishAiCallback_Group_0MULTI1SIZE0SupportedCommand(int teamId, int groupId) {
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
EXPORT(int) skirmishAiCallback_Group_SupportedCommand_0ARRAY1SIZE0getParams(int teamId, int groupId, int commandId) {
	return team_callback[teamId]->GetGroupCommands(groupId)->at(commandId).params.size();
}
EXPORT(int) skirmishAiCallback_Group_SupportedCommand_0ARRAY1VALS0getParams(int teamId,
		int groupId, int commandId, const char** params, int params_max) {

	if (params_max < 0) {
		params_max = 0;
	}

	const std::vector<std::string> ps
			= team_callback[teamId]->GetGroupCommands(groupId)->at(commandId).params;
	size_t size = ps.size();
	if ((size_t)params_max < size) {
		size = params_max;
	}

	size_t p;
	for (p=0; p < size; p++) {
		params[p] = ps.at(p).c_str();
	}

	return size;
}

EXPORT(int) skirmishAiCallback_Group_OrderPreview_getId(int teamId, int groupId) {

	if (!isControlledByLocalPlayer(teamId)) return -1;

	//TODO: need to add support for new gui
	Command tmpCmd = guihandler->GetOrderPreview();
	return tmpCmd.id;
}
EXPORT(unsigned char) skirmishAiCallback_Group_OrderPreview_getOptions(int teamId, int groupId) {

	if (!isControlledByLocalPlayer(teamId)) return '\0';

	//TODO: need to add support for new gui
	Command tmpCmd = guihandler->GetOrderPreview();
	return tmpCmd.options;
}
EXPORT(unsigned int) skirmishAiCallback_Group_OrderPreview_getTag(int teamId, int groupId) {

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
EXPORT(int) skirmishAiCallback_Group_OrderPreview_0ARRAY1SIZE0getParams(int teamId,
		int groupId) {

	if (!isControlledByLocalPlayer(teamId)) return 0;

	//TODO: need to add support for new gui
	return guihandler->GetOrderPreview().params.size();
}
EXPORT(int) skirmishAiCallback_Group_OrderPreview_0ARRAY1VALS0getParams(int teamId,
		int groupId, float params[], int params_max) {

	if (!isControlledByLocalPlayer(teamId)) return 0;

	//TODO: need to add support for new gui
	Command tmpCmd = guihandler->GetOrderPreview();
	int numParams = params_max < (int)(tmpCmd.params.size()) ? params_max
			: tmpCmd.params.size();

	int i;
	for (i = 0; i < numParams; i++) {
		params[i] = tmpCmd.params[i];
	}

	return numParams;
}

EXPORT(bool) skirmishAiCallback_Group_isSelected(int teamId, int groupId) {

	if (!isControlledByLocalPlayer(teamId)) return false;

	return selectedUnits.selectedGroup == groupId;
}
//##############################################################################




static void skirmishAiCallback_init(SSkirmishAICallback* callback) {

	callback->Clb_Engine_handleCommand = &skirmishAiCallback_Engine_handleCommand;
	callback->Clb_Engine_Version_getMajor = &skirmishAiCallback_Engine_Version_getMajor;
	callback->Clb_Engine_Version_getMinor = &skirmishAiCallback_Engine_Version_getMinor;
	callback->Clb_Engine_Version_getPatchset = &skirmishAiCallback_Engine_Version_getPatchset;
	callback->Clb_Engine_Version_getAdditional = &skirmishAiCallback_Engine_Version_getAdditional;
	callback->Clb_Engine_Version_getBuildTime = &skirmishAiCallback_Engine_Version_getBuildTime;
	callback->Clb_Engine_Version_getNormal = &skirmishAiCallback_Engine_Version_getNormal;
	callback->Clb_Engine_Version_getFull = &skirmishAiCallback_Engine_Version_getFull;
	callback->Clb_Teams_getSize = &skirmishAiCallback_Teams_getSize;
	callback->Clb_SkirmishAIs_getSize = &skirmishAiCallback_SkirmishAIs_getSize;
	callback->Clb_SkirmishAIs_getMax = &skirmishAiCallback_SkirmishAIs_getMax;
	callback->Clb_SkirmishAI_Info_getSize = &skirmishAiCallback_SkirmishAI_Info_getSize;
	callback->Clb_SkirmishAI_Info_getKey = &skirmishAiCallback_SkirmishAI_Info_getKey;
	callback->Clb_SkirmishAI_Info_getValue = &skirmishAiCallback_SkirmishAI_Info_getValue;
	callback->Clb_SkirmishAI_Info_getDescription = &skirmishAiCallback_SkirmishAI_Info_getDescription;
	callback->Clb_SkirmishAI_Info_getValueByKey = &skirmishAiCallback_SkirmishAI_Info_getValueByKey;
	callback->Clb_SkirmishAI_OptionValues_getSize = &skirmishAiCallback_SkirmishAI_OptionValues_getSize;
	callback->Clb_SkirmishAI_OptionValues_getKey = &skirmishAiCallback_SkirmishAI_OptionValues_getKey;
	callback->Clb_SkirmishAI_OptionValues_getValue = &skirmishAiCallback_SkirmishAI_OptionValues_getValue;
	callback->Clb_SkirmishAI_OptionValues_getValueByKey = &skirmishAiCallback_SkirmishAI_OptionValues_getValueByKey;
	callback->Clb_Log_log = &skirmishAiCallback_Log_log;
	callback->Clb_Log_exception = &skirmishAiCallback_Log_exception;
	callback->Clb_DataDirs_getPathSeparator = &skirmishAiCallback_DataDirs_getPathSeparator;
	callback->Clb_DataDirs_getConfigDir = &skirmishAiCallback_DataDirs_getConfigDir;
	callback->Clb_DataDirs_getWriteableDir = &skirmishAiCallback_DataDirs_getWriteableDir;
	callback->Clb_DataDirs_locatePath = &skirmishAiCallback_DataDirs_locatePath;
	callback->Clb_DataDirs_allocatePath = &skirmishAiCallback_DataDirs_allocatePath;
	callback->Clb_DataDirs_Roots_getSize = &skirmishAiCallback_DataDirs_Roots_getSize;
	callback->Clb_DataDirs_Roots_getDir = &skirmishAiCallback_DataDirs_Roots_getDir;
	callback->Clb_DataDirs_Roots_locatePath = &skirmishAiCallback_DataDirs_Roots_locatePath;
	callback->Clb_DataDirs_Roots_allocatePath = &skirmishAiCallback_DataDirs_Roots_allocatePath;
	callback->Clb_Game_getCurrentFrame = &skirmishAiCallback_Game_getCurrentFrame;
	callback->Clb_Game_getAiInterfaceVersion = &skirmishAiCallback_Game_getAiInterfaceVersion;
	callback->Clb_Game_getMyTeam = &skirmishAiCallback_Game_getMyTeam;
	callback->Clb_Game_getMyAllyTeam = &skirmishAiCallback_Game_getMyAllyTeam;
	callback->Clb_Game_getPlayerTeam = &skirmishAiCallback_Game_getPlayerTeam;
	callback->Clb_Game_getTeamSide = &skirmishAiCallback_Game_getTeamSide;
	callback->Clb_Game_isExceptionHandlingEnabled = &skirmishAiCallback_Game_isExceptionHandlingEnabled;
	callback->Clb_Game_isDebugModeEnabled = &skirmishAiCallback_Game_isDebugModeEnabled;
	callback->Clb_Game_getMode = &skirmishAiCallback_Game_getMode;
	callback->Clb_Game_isPaused = &skirmishAiCallback_Game_isPaused;
	callback->Clb_Game_getSpeedFactor = &skirmishAiCallback_Game_getSpeedFactor;
	callback->Clb_Game_getSetupScript = &skirmishAiCallback_Game_getSetupScript;
	callback->Clb_Gui_getViewRange = &skirmishAiCallback_Gui_getViewRange;
	callback->Clb_Gui_getScreenX = &skirmishAiCallback_Gui_getScreenX;
	callback->Clb_Gui_getScreenY = &skirmishAiCallback_Gui_getScreenY;
	callback->Clb_Gui_Camera_getDirection = &skirmishAiCallback_Gui_Camera_getDirection;
	callback->Clb_Gui_Camera_getPosition = &skirmishAiCallback_Gui_Camera_getPosition;
	callback->Clb_Cheats_isEnabled = &skirmishAiCallback_Cheats_isEnabled;
	callback->Clb_Cheats_setEnabled = &skirmishAiCallback_Cheats_setEnabled;
	callback->Clb_Cheats_setEventsEnabled = &skirmishAiCallback_Cheats_setEventsEnabled;
	callback->Clb_Cheats_isOnlyPassive = &skirmishAiCallback_Cheats_isOnlyPassive;
	callback->Clb_0MULTI1SIZE0Resource = &skirmishAiCallback_0MULTI1SIZE0Resource;
	callback->Clb_0MULTI1FETCH3ResourceByName0Resource = &skirmishAiCallback_0MULTI1FETCH3ResourceByName0Resource;
	callback->Clb_Resource_getName = &skirmishAiCallback_Resource_getName;
	callback->Clb_Resource_getOptimum = &skirmishAiCallback_Resource_getOptimum;
	callback->Clb_Economy_0REF1Resource2resourceId0getCurrent = &skirmishAiCallback_Economy_0REF1Resource2resourceId0getCurrent;
	callback->Clb_Economy_0REF1Resource2resourceId0getIncome = &skirmishAiCallback_Economy_0REF1Resource2resourceId0getIncome;
	callback->Clb_Economy_0REF1Resource2resourceId0getUsage = &skirmishAiCallback_Economy_0REF1Resource2resourceId0getUsage;
	callback->Clb_Economy_0REF1Resource2resourceId0getStorage = &skirmishAiCallback_Economy_0REF1Resource2resourceId0getStorage;
	callback->Clb_File_getSize = &skirmishAiCallback_File_getSize;
	callback->Clb_File_getContent = &skirmishAiCallback_File_getContent;
	callback->Clb_0MULTI1SIZE0UnitDef = &skirmishAiCallback_0MULTI1SIZE0UnitDef;
	callback->Clb_0MULTI1VALS0UnitDef = &skirmishAiCallback_0MULTI1VALS0UnitDef;
	callback->Clb_0MULTI1FETCH3UnitDefByName0UnitDef = &skirmishAiCallback_0MULTI1FETCH3UnitDefByName0UnitDef;
	callback->Clb_UnitDef_getHeight = &skirmishAiCallback_UnitDef_getHeight;
	callback->Clb_UnitDef_getRadius = &skirmishAiCallback_UnitDef_getRadius;
	callback->Clb_UnitDef_isValid = &skirmishAiCallback_UnitDef_isValid;
	callback->Clb_UnitDef_getName = &skirmishAiCallback_UnitDef_getName;
	callback->Clb_UnitDef_getHumanName = &skirmishAiCallback_UnitDef_getHumanName;
	callback->Clb_UnitDef_getFileName = &skirmishAiCallback_UnitDef_getFileName;
	callback->Clb_UnitDef_getAiHint = &skirmishAiCallback_UnitDef_getAiHint;
	callback->Clb_UnitDef_getCobId = &skirmishAiCallback_UnitDef_getCobId;
	callback->Clb_UnitDef_getTechLevel = &skirmishAiCallback_UnitDef_getTechLevel;
	callback->Clb_UnitDef_getGaia = &skirmishAiCallback_UnitDef_getGaia;
	callback->Clb_UnitDef_0REF1Resource2resourceId0getUpkeep = &skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getUpkeep;
	callback->Clb_UnitDef_0REF1Resource2resourceId0getResourceMake = &skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getResourceMake;
	callback->Clb_UnitDef_0REF1Resource2resourceId0getMakesResource = &skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getMakesResource;
	callback->Clb_UnitDef_0REF1Resource2resourceId0getCost = &skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getCost;
	callback->Clb_UnitDef_0REF1Resource2resourceId0getExtractsResource = &skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getExtractsResource;
	callback->Clb_UnitDef_0REF1Resource2resourceId0getResourceExtractorRange = &skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getResourceExtractorRange;
	callback->Clb_UnitDef_0REF1Resource2resourceId0getWindResourceGenerator = &skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getWindResourceGenerator;
	callback->Clb_UnitDef_0REF1Resource2resourceId0getTidalResourceGenerator = &skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getTidalResourceGenerator;
	callback->Clb_UnitDef_0REF1Resource2resourceId0getStorage = &skirmishAiCallback_UnitDef_0REF1Resource2resourceId0getStorage;
	callback->Clb_UnitDef_0REF1Resource2resourceId0isSquareResourceExtractor = &skirmishAiCallback_UnitDef_0REF1Resource2resourceId0isSquareResourceExtractor;
	callback->Clb_UnitDef_0REF1Resource2resourceId0isResourceMaker = &skirmishAiCallback_UnitDef_0REF1Resource2resourceId0isResourceMaker;
	callback->Clb_UnitDef_getBuildTime = &skirmishAiCallback_UnitDef_getBuildTime;
	callback->Clb_UnitDef_getAutoHeal = &skirmishAiCallback_UnitDef_getAutoHeal;
	callback->Clb_UnitDef_getIdleAutoHeal = &skirmishAiCallback_UnitDef_getIdleAutoHeal;
	callback->Clb_UnitDef_getIdleTime = &skirmishAiCallback_UnitDef_getIdleTime;
	callback->Clb_UnitDef_getPower = &skirmishAiCallback_UnitDef_getPower;
	callback->Clb_UnitDef_getHealth = &skirmishAiCallback_UnitDef_getHealth;
	callback->Clb_UnitDef_getCategory = &skirmishAiCallback_UnitDef_getCategory;
	callback->Clb_UnitDef_getSpeed = &skirmishAiCallback_UnitDef_getSpeed;
	callback->Clb_UnitDef_getTurnRate = &skirmishAiCallback_UnitDef_getTurnRate;
	callback->Clb_UnitDef_isTurnInPlace = &skirmishAiCallback_UnitDef_isTurnInPlace;
	callback->Clb_UnitDef_getTurnInPlaceDistance = &skirmishAiCallback_UnitDef_getTurnInPlaceDistance;
	callback->Clb_UnitDef_getTurnInPlaceSpeedLimit = &skirmishAiCallback_UnitDef_getTurnInPlaceSpeedLimit;
	callback->Clb_UnitDef_isUpright = &skirmishAiCallback_UnitDef_isUpright;
	callback->Clb_UnitDef_isCollide = &skirmishAiCallback_UnitDef_isCollide;
	callback->Clb_UnitDef_getControlRadius = &skirmishAiCallback_UnitDef_getControlRadius;
	callback->Clb_UnitDef_getLosRadius = &skirmishAiCallback_UnitDef_getLosRadius;
	callback->Clb_UnitDef_getAirLosRadius = &skirmishAiCallback_UnitDef_getAirLosRadius;
	callback->Clb_UnitDef_getLosHeight = &skirmishAiCallback_UnitDef_getLosHeight;
	callback->Clb_UnitDef_getRadarRadius = &skirmishAiCallback_UnitDef_getRadarRadius;
	callback->Clb_UnitDef_getSonarRadius = &skirmishAiCallback_UnitDef_getSonarRadius;
	callback->Clb_UnitDef_getJammerRadius = &skirmishAiCallback_UnitDef_getJammerRadius;
	callback->Clb_UnitDef_getSonarJamRadius = &skirmishAiCallback_UnitDef_getSonarJamRadius;
	callback->Clb_UnitDef_getSeismicRadius = &skirmishAiCallback_UnitDef_getSeismicRadius;
	callback->Clb_UnitDef_getSeismicSignature = &skirmishAiCallback_UnitDef_getSeismicSignature;
	callback->Clb_UnitDef_isStealth = &skirmishAiCallback_UnitDef_isStealth;
	callback->Clb_UnitDef_isSonarStealth = &skirmishAiCallback_UnitDef_isSonarStealth;
	callback->Clb_UnitDef_isBuildRange3D = &skirmishAiCallback_UnitDef_isBuildRange3D;
	callback->Clb_UnitDef_getBuildDistance = &skirmishAiCallback_UnitDef_getBuildDistance;
	callback->Clb_UnitDef_getBuildSpeed = &skirmishAiCallback_UnitDef_getBuildSpeed;
	callback->Clb_UnitDef_getReclaimSpeed = &skirmishAiCallback_UnitDef_getReclaimSpeed;
	callback->Clb_UnitDef_getRepairSpeed = &skirmishAiCallback_UnitDef_getRepairSpeed;
	callback->Clb_UnitDef_getMaxRepairSpeed = &skirmishAiCallback_UnitDef_getMaxRepairSpeed;
	callback->Clb_UnitDef_getResurrectSpeed = &skirmishAiCallback_UnitDef_getResurrectSpeed;
	callback->Clb_UnitDef_getCaptureSpeed = &skirmishAiCallback_UnitDef_getCaptureSpeed;
	callback->Clb_UnitDef_getTerraformSpeed = &skirmishAiCallback_UnitDef_getTerraformSpeed;
	callback->Clb_UnitDef_getMass = &skirmishAiCallback_UnitDef_getMass;
	callback->Clb_UnitDef_isPushResistant = &skirmishAiCallback_UnitDef_isPushResistant;
	callback->Clb_UnitDef_isStrafeToAttack = &skirmishAiCallback_UnitDef_isStrafeToAttack;
	callback->Clb_UnitDef_getMinCollisionSpeed = &skirmishAiCallback_UnitDef_getMinCollisionSpeed;
	callback->Clb_UnitDef_getSlideTolerance = &skirmishAiCallback_UnitDef_getSlideTolerance;
	callback->Clb_UnitDef_getMaxSlope = &skirmishAiCallback_UnitDef_getMaxSlope;
	callback->Clb_UnitDef_getMaxHeightDif = &skirmishAiCallback_UnitDef_getMaxHeightDif;
	callback->Clb_UnitDef_getMinWaterDepth = &skirmishAiCallback_UnitDef_getMinWaterDepth;
	callback->Clb_UnitDef_getWaterline = &skirmishAiCallback_UnitDef_getWaterline;
	callback->Clb_UnitDef_getMaxWaterDepth = &skirmishAiCallback_UnitDef_getMaxWaterDepth;
	callback->Clb_UnitDef_getArmoredMultiple = &skirmishAiCallback_UnitDef_getArmoredMultiple;
	callback->Clb_UnitDef_getArmorType = &skirmishAiCallback_UnitDef_getArmorType;
	callback->Clb_UnitDef_FlankingBonus_getMode = &skirmishAiCallback_UnitDef_FlankingBonus_getMode;
	callback->Clb_UnitDef_FlankingBonus_getDir = &skirmishAiCallback_UnitDef_FlankingBonus_getDir;
	callback->Clb_UnitDef_FlankingBonus_getMax = &skirmishAiCallback_UnitDef_FlankingBonus_getMax;
	callback->Clb_UnitDef_FlankingBonus_getMin = &skirmishAiCallback_UnitDef_FlankingBonus_getMin;
	callback->Clb_UnitDef_FlankingBonus_getMobilityAdd = &skirmishAiCallback_UnitDef_FlankingBonus_getMobilityAdd;
	callback->Clb_UnitDef_CollisionVolume_getType = &skirmishAiCallback_UnitDef_CollisionVolume_getType;
	callback->Clb_UnitDef_CollisionVolume_getScales = &skirmishAiCallback_UnitDef_CollisionVolume_getScales;
	callback->Clb_UnitDef_CollisionVolume_getOffsets = &skirmishAiCallback_UnitDef_CollisionVolume_getOffsets;
	callback->Clb_UnitDef_CollisionVolume_getTest = &skirmishAiCallback_UnitDef_CollisionVolume_getTest;
	callback->Clb_UnitDef_getMaxWeaponRange = &skirmishAiCallback_UnitDef_getMaxWeaponRange;
	callback->Clb_UnitDef_getType = &skirmishAiCallback_UnitDef_getType;
	callback->Clb_UnitDef_getTooltip = &skirmishAiCallback_UnitDef_getTooltip;
	callback->Clb_UnitDef_getWreckName = &skirmishAiCallback_UnitDef_getWreckName;
	callback->Clb_UnitDef_getDeathExplosion = &skirmishAiCallback_UnitDef_getDeathExplosion;
	callback->Clb_UnitDef_getSelfDExplosion = &skirmishAiCallback_UnitDef_getSelfDExplosion;
	callback->Clb_UnitDef_getTedClassString = &skirmishAiCallback_UnitDef_getTedClassString;
	callback->Clb_UnitDef_getCategoryString = &skirmishAiCallback_UnitDef_getCategoryString;
	callback->Clb_UnitDef_isAbleToSelfD = &skirmishAiCallback_UnitDef_isAbleToSelfD;
	callback->Clb_UnitDef_getSelfDCountdown = &skirmishAiCallback_UnitDef_getSelfDCountdown;
	callback->Clb_UnitDef_isAbleToSubmerge = &skirmishAiCallback_UnitDef_isAbleToSubmerge;
	callback->Clb_UnitDef_isAbleToFly = &skirmishAiCallback_UnitDef_isAbleToFly;
	callback->Clb_UnitDef_isAbleToMove = &skirmishAiCallback_UnitDef_isAbleToMove;
	callback->Clb_UnitDef_isAbleToHover = &skirmishAiCallback_UnitDef_isAbleToHover;
	callback->Clb_UnitDef_isFloater = &skirmishAiCallback_UnitDef_isFloater;
	callback->Clb_UnitDef_isBuilder = &skirmishAiCallback_UnitDef_isBuilder;
	callback->Clb_UnitDef_isActivateWhenBuilt = &skirmishAiCallback_UnitDef_isActivateWhenBuilt;
	callback->Clb_UnitDef_isOnOffable = &skirmishAiCallback_UnitDef_isOnOffable;
	callback->Clb_UnitDef_isFullHealthFactory = &skirmishAiCallback_UnitDef_isFullHealthFactory;
	callback->Clb_UnitDef_isFactoryHeadingTakeoff = &skirmishAiCallback_UnitDef_isFactoryHeadingTakeoff;
	callback->Clb_UnitDef_isReclaimable = &skirmishAiCallback_UnitDef_isReclaimable;
	callback->Clb_UnitDef_isCapturable = &skirmishAiCallback_UnitDef_isCapturable;
	callback->Clb_UnitDef_isAbleToRestore = &skirmishAiCallback_UnitDef_isAbleToRestore;
	callback->Clb_UnitDef_isAbleToRepair = &skirmishAiCallback_UnitDef_isAbleToRepair;
	callback->Clb_UnitDef_isAbleToSelfRepair = &skirmishAiCallback_UnitDef_isAbleToSelfRepair;
	callback->Clb_UnitDef_isAbleToReclaim = &skirmishAiCallback_UnitDef_isAbleToReclaim;
	callback->Clb_UnitDef_isAbleToAttack = &skirmishAiCallback_UnitDef_isAbleToAttack;
	callback->Clb_UnitDef_isAbleToPatrol = &skirmishAiCallback_UnitDef_isAbleToPatrol;
	callback->Clb_UnitDef_isAbleToFight = &skirmishAiCallback_UnitDef_isAbleToFight;
	callback->Clb_UnitDef_isAbleToGuard = &skirmishAiCallback_UnitDef_isAbleToGuard;
	callback->Clb_UnitDef_isAbleToBuild = &skirmishAiCallback_UnitDef_isAbleToBuild;
	callback->Clb_UnitDef_isAbleToAssist = &skirmishAiCallback_UnitDef_isAbleToAssist;
	callback->Clb_UnitDef_isAssistable = &skirmishAiCallback_UnitDef_isAssistable;
	callback->Clb_UnitDef_isAbleToRepeat = &skirmishAiCallback_UnitDef_isAbleToRepeat;
	callback->Clb_UnitDef_isAbleToFireControl = &skirmishAiCallback_UnitDef_isAbleToFireControl;
	callback->Clb_UnitDef_getFireState = &skirmishAiCallback_UnitDef_getFireState;
	callback->Clb_UnitDef_getMoveState = &skirmishAiCallback_UnitDef_getMoveState;
	callback->Clb_UnitDef_getWingDrag = &skirmishAiCallback_UnitDef_getWingDrag;
	callback->Clb_UnitDef_getWingAngle = &skirmishAiCallback_UnitDef_getWingAngle;
	callback->Clb_UnitDef_getDrag = &skirmishAiCallback_UnitDef_getDrag;
	callback->Clb_UnitDef_getFrontToSpeed = &skirmishAiCallback_UnitDef_getFrontToSpeed;
	callback->Clb_UnitDef_getSpeedToFront = &skirmishAiCallback_UnitDef_getSpeedToFront;
	callback->Clb_UnitDef_getMyGravity = &skirmishAiCallback_UnitDef_getMyGravity;
	callback->Clb_UnitDef_getMaxBank = &skirmishAiCallback_UnitDef_getMaxBank;
	callback->Clb_UnitDef_getMaxPitch = &skirmishAiCallback_UnitDef_getMaxPitch;
	callback->Clb_UnitDef_getTurnRadius = &skirmishAiCallback_UnitDef_getTurnRadius;
	callback->Clb_UnitDef_getWantedHeight = &skirmishAiCallback_UnitDef_getWantedHeight;
	callback->Clb_UnitDef_getVerticalSpeed = &skirmishAiCallback_UnitDef_getVerticalSpeed;
	callback->Clb_UnitDef_isAbleToCrash = &skirmishAiCallback_UnitDef_isAbleToCrash;
	callback->Clb_UnitDef_isHoverAttack = &skirmishAiCallback_UnitDef_isHoverAttack;
	callback->Clb_UnitDef_isAirStrafe = &skirmishAiCallback_UnitDef_isAirStrafe;
	callback->Clb_UnitDef_getDlHoverFactor = &skirmishAiCallback_UnitDef_getDlHoverFactor;
	callback->Clb_UnitDef_getMaxAcceleration = &skirmishAiCallback_UnitDef_getMaxAcceleration;
	callback->Clb_UnitDef_getMaxDeceleration = &skirmishAiCallback_UnitDef_getMaxDeceleration;
	callback->Clb_UnitDef_getMaxAileron = &skirmishAiCallback_UnitDef_getMaxAileron;
	callback->Clb_UnitDef_getMaxElevator = &skirmishAiCallback_UnitDef_getMaxElevator;
	callback->Clb_UnitDef_getMaxRudder = &skirmishAiCallback_UnitDef_getMaxRudder;
	callback->Clb_UnitDef_getXSize = &skirmishAiCallback_UnitDef_getXSize;
	callback->Clb_UnitDef_getZSize = &skirmishAiCallback_UnitDef_getZSize;
	callback->Clb_UnitDef_getBuildAngle = &skirmishAiCallback_UnitDef_getBuildAngle;
	callback->Clb_UnitDef_getLoadingRadius = &skirmishAiCallback_UnitDef_getLoadingRadius;
	callback->Clb_UnitDef_getUnloadSpread = &skirmishAiCallback_UnitDef_getUnloadSpread;
	callback->Clb_UnitDef_getTransportCapacity = &skirmishAiCallback_UnitDef_getTransportCapacity;
	callback->Clb_UnitDef_getTransportSize = &skirmishAiCallback_UnitDef_getTransportSize;
	callback->Clb_UnitDef_getMinTransportSize = &skirmishAiCallback_UnitDef_getMinTransportSize;
	callback->Clb_UnitDef_isAirBase = &skirmishAiCallback_UnitDef_isAirBase;
	callback->Clb_UnitDef_isFirePlatform = &skirmishAiCallback_UnitDef_isFirePlatform;
	callback->Clb_UnitDef_getTransportMass = &skirmishAiCallback_UnitDef_getTransportMass;
	callback->Clb_UnitDef_getMinTransportMass = &skirmishAiCallback_UnitDef_getMinTransportMass;
	callback->Clb_UnitDef_isHoldSteady = &skirmishAiCallback_UnitDef_isHoldSteady;
	callback->Clb_UnitDef_isReleaseHeld = &skirmishAiCallback_UnitDef_isReleaseHeld;
	callback->Clb_UnitDef_isNotTransportable = &skirmishAiCallback_UnitDef_isNotTransportable;
	callback->Clb_UnitDef_isTransportByEnemy = &skirmishAiCallback_UnitDef_isTransportByEnemy;
	callback->Clb_UnitDef_getTransportUnloadMethod = &skirmishAiCallback_UnitDef_getTransportUnloadMethod;
	callback->Clb_UnitDef_getFallSpeed = &skirmishAiCallback_UnitDef_getFallSpeed;
	callback->Clb_UnitDef_getUnitFallSpeed = &skirmishAiCallback_UnitDef_getUnitFallSpeed;
	callback->Clb_UnitDef_isAbleToCloak = &skirmishAiCallback_UnitDef_isAbleToCloak;
	callback->Clb_UnitDef_isStartCloaked = &skirmishAiCallback_UnitDef_isStartCloaked;
	callback->Clb_UnitDef_getCloakCost = &skirmishAiCallback_UnitDef_getCloakCost;
	callback->Clb_UnitDef_getCloakCostMoving = &skirmishAiCallback_UnitDef_getCloakCostMoving;
	callback->Clb_UnitDef_getDecloakDistance = &skirmishAiCallback_UnitDef_getDecloakDistance;
	callback->Clb_UnitDef_isDecloakSpherical = &skirmishAiCallback_UnitDef_isDecloakSpherical;
	callback->Clb_UnitDef_isDecloakOnFire = &skirmishAiCallback_UnitDef_isDecloakOnFire;
	callback->Clb_UnitDef_isAbleToKamikaze = &skirmishAiCallback_UnitDef_isAbleToKamikaze;
	callback->Clb_UnitDef_getKamikazeDist = &skirmishAiCallback_UnitDef_getKamikazeDist;
	callback->Clb_UnitDef_isTargetingFacility = &skirmishAiCallback_UnitDef_isTargetingFacility;
	callback->Clb_UnitDef_isAbleToDGun = &skirmishAiCallback_UnitDef_isAbleToDGun;
	callback->Clb_UnitDef_isNeedGeo = &skirmishAiCallback_UnitDef_isNeedGeo;
	callback->Clb_UnitDef_isFeature = &skirmishAiCallback_UnitDef_isFeature;
	callback->Clb_UnitDef_isHideDamage = &skirmishAiCallback_UnitDef_isHideDamage;
	callback->Clb_UnitDef_isCommander = &skirmishAiCallback_UnitDef_isCommander;
	callback->Clb_UnitDef_isShowPlayerName = &skirmishAiCallback_UnitDef_isShowPlayerName;
	callback->Clb_UnitDef_isAbleToResurrect = &skirmishAiCallback_UnitDef_isAbleToResurrect;
	callback->Clb_UnitDef_isAbleToCapture = &skirmishAiCallback_UnitDef_isAbleToCapture;
	callback->Clb_UnitDef_getHighTrajectoryType = &skirmishAiCallback_UnitDef_getHighTrajectoryType;
	callback->Clb_UnitDef_getNoChaseCategory = &skirmishAiCallback_UnitDef_getNoChaseCategory;
	callback->Clb_UnitDef_isLeaveTracks = &skirmishAiCallback_UnitDef_isLeaveTracks;
	callback->Clb_UnitDef_getTrackWidth = &skirmishAiCallback_UnitDef_getTrackWidth;
	callback->Clb_UnitDef_getTrackOffset = &skirmishAiCallback_UnitDef_getTrackOffset;
	callback->Clb_UnitDef_getTrackStrength = &skirmishAiCallback_UnitDef_getTrackStrength;
	callback->Clb_UnitDef_getTrackStretch = &skirmishAiCallback_UnitDef_getTrackStretch;
	callback->Clb_UnitDef_getTrackType = &skirmishAiCallback_UnitDef_getTrackType;
	callback->Clb_UnitDef_isAbleToDropFlare = &skirmishAiCallback_UnitDef_isAbleToDropFlare;
	callback->Clb_UnitDef_getFlareReloadTime = &skirmishAiCallback_UnitDef_getFlareReloadTime;
	callback->Clb_UnitDef_getFlareEfficiency = &skirmishAiCallback_UnitDef_getFlareEfficiency;
	callback->Clb_UnitDef_getFlareDelay = &skirmishAiCallback_UnitDef_getFlareDelay;
	callback->Clb_UnitDef_getFlareDropVector = &skirmishAiCallback_UnitDef_getFlareDropVector;
	callback->Clb_UnitDef_getFlareTime = &skirmishAiCallback_UnitDef_getFlareTime;
	callback->Clb_UnitDef_getFlareSalvoSize = &skirmishAiCallback_UnitDef_getFlareSalvoSize;
	callback->Clb_UnitDef_getFlareSalvoDelay = &skirmishAiCallback_UnitDef_getFlareSalvoDelay;
	callback->Clb_UnitDef_isAbleToLoopbackAttack = &skirmishAiCallback_UnitDef_isAbleToLoopbackAttack;
	callback->Clb_UnitDef_isLevelGround = &skirmishAiCallback_UnitDef_isLevelGround;
	callback->Clb_UnitDef_isUseBuildingGroundDecal = &skirmishAiCallback_UnitDef_isUseBuildingGroundDecal;
	callback->Clb_UnitDef_getBuildingDecalType = &skirmishAiCallback_UnitDef_getBuildingDecalType;
	callback->Clb_UnitDef_getBuildingDecalSizeX = &skirmishAiCallback_UnitDef_getBuildingDecalSizeX;
	callback->Clb_UnitDef_getBuildingDecalSizeY = &skirmishAiCallback_UnitDef_getBuildingDecalSizeY;
	callback->Clb_UnitDef_getBuildingDecalDecaySpeed = &skirmishAiCallback_UnitDef_getBuildingDecalDecaySpeed;
	callback->Clb_UnitDef_getMaxFuel = &skirmishAiCallback_UnitDef_getMaxFuel;
	callback->Clb_UnitDef_getRefuelTime = &skirmishAiCallback_UnitDef_getRefuelTime;
	callback->Clb_UnitDef_getMinAirBasePower = &skirmishAiCallback_UnitDef_getMinAirBasePower;
	callback->Clb_UnitDef_getMaxThisUnit = &skirmishAiCallback_UnitDef_getMaxThisUnit;
	callback->Clb_UnitDef_0SINGLE1FETCH2UnitDef0getDecoyDef = &skirmishAiCallback_UnitDef_0SINGLE1FETCH2UnitDef0getDecoyDef;
	callback->Clb_UnitDef_isDontLand = &skirmishAiCallback_UnitDef_isDontLand;
	callback->Clb_UnitDef_0SINGLE1FETCH2WeaponDef0getShieldDef = &skirmishAiCallback_UnitDef_0SINGLE1FETCH2WeaponDef0getShieldDef;
	callback->Clb_UnitDef_0SINGLE1FETCH2WeaponDef0getStockpileDef = &skirmishAiCallback_UnitDef_0SINGLE1FETCH2WeaponDef0getStockpileDef;
	callback->Clb_UnitDef_0ARRAY1SIZE1UnitDef0getBuildOptions = &skirmishAiCallback_UnitDef_0ARRAY1SIZE1UnitDef0getBuildOptions;
	callback->Clb_UnitDef_0ARRAY1VALS1UnitDef0getBuildOptions = &skirmishAiCallback_UnitDef_0ARRAY1VALS1UnitDef0getBuildOptions;
	callback->Clb_UnitDef_0MAP1SIZE0getCustomParams = &skirmishAiCallback_UnitDef_0MAP1SIZE0getCustomParams;
	callback->Clb_UnitDef_0MAP1KEYS0getCustomParams = &skirmishAiCallback_UnitDef_0MAP1KEYS0getCustomParams;
	callback->Clb_UnitDef_0MAP1VALS0getCustomParams = &skirmishAiCallback_UnitDef_0MAP1VALS0getCustomParams;
	callback->Clb_UnitDef_0AVAILABLE0MoveData = &skirmishAiCallback_UnitDef_0AVAILABLE0MoveData;
	callback->Clb_UnitDef_MoveData_getMoveType = &skirmishAiCallback_UnitDef_MoveData_getMoveType;
	callback->Clb_UnitDef_MoveData_getMoveFamily = &skirmishAiCallback_UnitDef_MoveData_getMoveFamily;
	callback->Clb_UnitDef_MoveData_getSize = &skirmishAiCallback_UnitDef_MoveData_getSize;
	callback->Clb_UnitDef_MoveData_getDepth = &skirmishAiCallback_UnitDef_MoveData_getDepth;
	callback->Clb_UnitDef_MoveData_getMaxSlope = &skirmishAiCallback_UnitDef_MoveData_getMaxSlope;
	callback->Clb_UnitDef_MoveData_getSlopeMod = &skirmishAiCallback_UnitDef_MoveData_getSlopeMod;
	callback->Clb_UnitDef_MoveData_getDepthMod = &skirmishAiCallback_UnitDef_MoveData_getDepthMod;
	callback->Clb_UnitDef_MoveData_getPathType = &skirmishAiCallback_UnitDef_MoveData_getPathType;
	callback->Clb_UnitDef_MoveData_getCrushStrength = &skirmishAiCallback_UnitDef_MoveData_getCrushStrength;
	callback->Clb_UnitDef_MoveData_getMaxSpeed = &skirmishAiCallback_UnitDef_MoveData_getMaxSpeed;
	callback->Clb_UnitDef_MoveData_getMaxTurnRate = &skirmishAiCallback_UnitDef_MoveData_getMaxTurnRate;
	callback->Clb_UnitDef_MoveData_getMaxAcceleration = &skirmishAiCallback_UnitDef_MoveData_getMaxAcceleration;
	callback->Clb_UnitDef_MoveData_getMaxBreaking = &skirmishAiCallback_UnitDef_MoveData_getMaxBreaking;
	callback->Clb_UnitDef_MoveData_isSubMarine = &skirmishAiCallback_UnitDef_MoveData_isSubMarine;
	callback->Clb_UnitDef_0MULTI1SIZE0WeaponMount = &skirmishAiCallback_UnitDef_0MULTI1SIZE0WeaponMount;
	callback->Clb_UnitDef_WeaponMount_getName = &skirmishAiCallback_UnitDef_WeaponMount_getName;
	callback->Clb_UnitDef_WeaponMount_0SINGLE1FETCH2WeaponDef0getWeaponDef = &skirmishAiCallback_UnitDef_WeaponMount_0SINGLE1FETCH2WeaponDef0getWeaponDef;
	callback->Clb_UnitDef_WeaponMount_getSlavedTo = &skirmishAiCallback_UnitDef_WeaponMount_getSlavedTo;
	callback->Clb_UnitDef_WeaponMount_getMainDir = &skirmishAiCallback_UnitDef_WeaponMount_getMainDir;
	callback->Clb_UnitDef_WeaponMount_getMaxAngleDif = &skirmishAiCallback_UnitDef_WeaponMount_getMaxAngleDif;
	callback->Clb_UnitDef_WeaponMount_getFuelUsage = &skirmishAiCallback_UnitDef_WeaponMount_getFuelUsage;
	callback->Clb_UnitDef_WeaponMount_getBadTargetCategory = &skirmishAiCallback_UnitDef_WeaponMount_getBadTargetCategory;
	callback->Clb_UnitDef_WeaponMount_getOnlyTargetCategory = &skirmishAiCallback_UnitDef_WeaponMount_getOnlyTargetCategory;
	callback->Clb_Unit_0STATIC0getLimit = &skirmishAiCallback_Unit_0STATIC0getLimit;
	callback->Clb_0MULTI1SIZE3EnemyUnits0Unit = &skirmishAiCallback_0MULTI1SIZE3EnemyUnits0Unit;
	callback->Clb_0MULTI1VALS3EnemyUnits0Unit = &skirmishAiCallback_0MULTI1VALS3EnemyUnits0Unit;
	callback->Clb_0MULTI1SIZE3EnemyUnitsIn0Unit = &skirmishAiCallback_0MULTI1SIZE3EnemyUnitsIn0Unit;
	callback->Clb_0MULTI1VALS3EnemyUnitsIn0Unit = &skirmishAiCallback_0MULTI1VALS3EnemyUnitsIn0Unit;
	callback->Clb_0MULTI1SIZE3EnemyUnitsInRadarAndLos0Unit = &skirmishAiCallback_0MULTI1SIZE3EnemyUnitsInRadarAndLos0Unit;
	callback->Clb_0MULTI1VALS3EnemyUnitsInRadarAndLos0Unit = &skirmishAiCallback_0MULTI1VALS3EnemyUnitsInRadarAndLos0Unit;
	callback->Clb_0MULTI1SIZE3FriendlyUnits0Unit = &skirmishAiCallback_0MULTI1SIZE3FriendlyUnits0Unit;
	callback->Clb_0MULTI1VALS3FriendlyUnits0Unit = &skirmishAiCallback_0MULTI1VALS3FriendlyUnits0Unit;
	callback->Clb_0MULTI1SIZE3FriendlyUnitsIn0Unit = &skirmishAiCallback_0MULTI1SIZE3FriendlyUnitsIn0Unit;
	callback->Clb_0MULTI1VALS3FriendlyUnitsIn0Unit = &skirmishAiCallback_0MULTI1VALS3FriendlyUnitsIn0Unit;
	callback->Clb_0MULTI1SIZE3NeutralUnits0Unit = &skirmishAiCallback_0MULTI1SIZE3NeutralUnits0Unit;
	callback->Clb_0MULTI1VALS3NeutralUnits0Unit = &skirmishAiCallback_0MULTI1VALS3NeutralUnits0Unit;
	callback->Clb_0MULTI1SIZE3NeutralUnitsIn0Unit = &skirmishAiCallback_0MULTI1SIZE3NeutralUnitsIn0Unit;
	callback->Clb_0MULTI1VALS3NeutralUnitsIn0Unit = &skirmishAiCallback_0MULTI1VALS3NeutralUnitsIn0Unit;
	callback->Clb_0MULTI1SIZE3TeamUnits0Unit = &skirmishAiCallback_0MULTI1SIZE3TeamUnits0Unit;
	callback->Clb_0MULTI1VALS3TeamUnits0Unit = &skirmishAiCallback_0MULTI1VALS3TeamUnits0Unit;
	callback->Clb_0MULTI1SIZE3SelectedUnits0Unit = &skirmishAiCallback_0MULTI1SIZE3SelectedUnits0Unit;
	callback->Clb_0MULTI1VALS3SelectedUnits0Unit = &skirmishAiCallback_0MULTI1VALS3SelectedUnits0Unit;
	callback->Clb_Unit_0SINGLE1FETCH2UnitDef0getDef = &skirmishAiCallback_Unit_0SINGLE1FETCH2UnitDef0getDef;
	callback->Clb_Unit_0MULTI1SIZE0ModParam = &skirmishAiCallback_Unit_0MULTI1SIZE0ModParam;
	callback->Clb_Unit_ModParam_getName = &skirmishAiCallback_Unit_ModParam_getName;
	callback->Clb_Unit_ModParam_getValue = &skirmishAiCallback_Unit_ModParam_getValue;
	callback->Clb_Unit_getTeam = &skirmishAiCallback_Unit_getTeam;
	callback->Clb_Unit_getAllyTeam = &skirmishAiCallback_Unit_getAllyTeam;
	callback->Clb_Unit_getLineage = &skirmishAiCallback_Unit_getLineage;
	callback->Clb_Unit_getAiHint = &skirmishAiCallback_Unit_getAiHint;
	callback->Clb_Unit_getStockpile = &skirmishAiCallback_Unit_getStockpile;
	callback->Clb_Unit_getStockpileQueued = &skirmishAiCallback_Unit_getStockpileQueued;
	callback->Clb_Unit_getCurrentFuel = &skirmishAiCallback_Unit_getCurrentFuel;
	callback->Clb_Unit_getMaxSpeed = &skirmishAiCallback_Unit_getMaxSpeed;
	callback->Clb_Unit_getMaxRange = &skirmishAiCallback_Unit_getMaxRange;
	callback->Clb_Unit_getMaxHealth = &skirmishAiCallback_Unit_getMaxHealth;
	callback->Clb_Unit_getExperience = &skirmishAiCallback_Unit_getExperience;
	callback->Clb_Unit_getGroup = &skirmishAiCallback_Unit_getGroup;
	callback->Clb_Unit_0MULTI1SIZE1Command0CurrentCommand = &skirmishAiCallback_Unit_0MULTI1SIZE1Command0CurrentCommand;
	callback->Clb_Unit_CurrentCommand_0STATIC0getType = &skirmishAiCallback_Unit_CurrentCommand_0STATIC0getType;
	callback->Clb_Unit_CurrentCommand_getId = &skirmishAiCallback_Unit_CurrentCommand_getId;
	callback->Clb_Unit_CurrentCommand_getOptions = &skirmishAiCallback_Unit_CurrentCommand_getOptions;
	callback->Clb_Unit_CurrentCommand_getTag = &skirmishAiCallback_Unit_CurrentCommand_getTag;
	callback->Clb_Unit_CurrentCommand_getTimeOut = &skirmishAiCallback_Unit_CurrentCommand_getTimeOut;
	callback->Clb_Unit_CurrentCommand_0ARRAY1SIZE0getParams = &skirmishAiCallback_Unit_CurrentCommand_0ARRAY1SIZE0getParams;
	callback->Clb_Unit_CurrentCommand_0ARRAY1VALS0getParams = &skirmishAiCallback_Unit_CurrentCommand_0ARRAY1VALS0getParams;
	callback->Clb_Unit_0MULTI1SIZE0SupportedCommand = &skirmishAiCallback_Unit_0MULTI1SIZE0SupportedCommand;
	callback->Clb_Unit_SupportedCommand_getId = &skirmishAiCallback_Unit_SupportedCommand_getId;
	callback->Clb_Unit_SupportedCommand_getName = &skirmishAiCallback_Unit_SupportedCommand_getName;
	callback->Clb_Unit_SupportedCommand_getToolTip = &skirmishAiCallback_Unit_SupportedCommand_getToolTip;
	callback->Clb_Unit_SupportedCommand_isShowUnique = &skirmishAiCallback_Unit_SupportedCommand_isShowUnique;
	callback->Clb_Unit_SupportedCommand_isDisabled = &skirmishAiCallback_Unit_SupportedCommand_isDisabled;
	callback->Clb_Unit_SupportedCommand_0ARRAY1SIZE0getParams = &skirmishAiCallback_Unit_SupportedCommand_0ARRAY1SIZE0getParams;
	callback->Clb_Unit_SupportedCommand_0ARRAY1VALS0getParams = &skirmishAiCallback_Unit_SupportedCommand_0ARRAY1VALS0getParams;
	callback->Clb_Unit_getHealth = &skirmishAiCallback_Unit_getHealth;
	callback->Clb_Unit_getSpeed = &skirmishAiCallback_Unit_getSpeed;
	callback->Clb_Unit_getPower = &skirmishAiCallback_Unit_getPower;
	callback->Clb_Unit_0MULTI1SIZE0ResourceInfo = &skirmishAiCallback_Unit_0MULTI1SIZE0ResourceInfo;
	callback->Clb_Unit_0REF1Resource2resourceId0getResourceUse = &skirmishAiCallback_Unit_0REF1Resource2resourceId0getResourceUse;
	callback->Clb_Unit_0REF1Resource2resourceId0getResourceMake = &skirmishAiCallback_Unit_0REF1Resource2resourceId0getResourceMake;
	callback->Clb_Unit_getPos = &skirmishAiCallback_Unit_getPos;
	callback->Clb_Unit_isActivated = &skirmishAiCallback_Unit_isActivated;
	callback->Clb_Unit_isBeingBuilt = &skirmishAiCallback_Unit_isBeingBuilt;
	callback->Clb_Unit_isCloaked = &skirmishAiCallback_Unit_isCloaked;
	callback->Clb_Unit_isParalyzed = &skirmishAiCallback_Unit_isParalyzed;
	callback->Clb_Unit_isNeutral = &skirmishAiCallback_Unit_isNeutral;
	callback->Clb_Unit_getBuildingFacing = &skirmishAiCallback_Unit_getBuildingFacing;
	callback->Clb_Unit_getLastUserOrderFrame = &skirmishAiCallback_Unit_getLastUserOrderFrame;
	callback->Clb_0MULTI1SIZE0Group = &skirmishAiCallback_0MULTI1SIZE0Group;
	callback->Clb_0MULTI1VALS0Group = &skirmishAiCallback_0MULTI1VALS0Group;
	callback->Clb_Group_0MULTI1SIZE0SupportedCommand = &skirmishAiCallback_Group_0MULTI1SIZE0SupportedCommand;
	callback->Clb_Group_SupportedCommand_getId = &skirmishAiCallback_Group_SupportedCommand_getId;
	callback->Clb_Group_SupportedCommand_getName = &skirmishAiCallback_Group_SupportedCommand_getName;
	callback->Clb_Group_SupportedCommand_getToolTip = &skirmishAiCallback_Group_SupportedCommand_getToolTip;
	callback->Clb_Group_SupportedCommand_isShowUnique = &skirmishAiCallback_Group_SupportedCommand_isShowUnique;
	callback->Clb_Group_SupportedCommand_isDisabled = &skirmishAiCallback_Group_SupportedCommand_isDisabled;
	callback->Clb_Group_SupportedCommand_0ARRAY1SIZE0getParams = &skirmishAiCallback_Group_SupportedCommand_0ARRAY1SIZE0getParams;
	callback->Clb_Group_SupportedCommand_0ARRAY1VALS0getParams = &skirmishAiCallback_Group_SupportedCommand_0ARRAY1VALS0getParams;
	callback->Clb_Group_OrderPreview_getId = &skirmishAiCallback_Group_OrderPreview_getId;
	callback->Clb_Group_OrderPreview_getOptions = &skirmishAiCallback_Group_OrderPreview_getOptions;
	callback->Clb_Group_OrderPreview_getTag = &skirmishAiCallback_Group_OrderPreview_getTag;
	callback->Clb_Group_OrderPreview_getTimeOut = &skirmishAiCallback_Group_OrderPreview_getTimeOut;
	callback->Clb_Group_OrderPreview_0ARRAY1SIZE0getParams = &skirmishAiCallback_Group_OrderPreview_0ARRAY1SIZE0getParams;
	callback->Clb_Group_OrderPreview_0ARRAY1VALS0getParams = &skirmishAiCallback_Group_OrderPreview_0ARRAY1VALS0getParams;
	callback->Clb_Group_isSelected = &skirmishAiCallback_Group_isSelected;
	callback->Clb_Mod_getFileName = &skirmishAiCallback_Mod_getFileName;
	callback->Clb_Mod_getHumanName = &skirmishAiCallback_Mod_getHumanName;
	callback->Clb_Mod_getShortName = &skirmishAiCallback_Mod_getShortName;
	callback->Clb_Mod_getVersion = &skirmishAiCallback_Mod_getVersion;
	callback->Clb_Mod_getMutator = &skirmishAiCallback_Mod_getMutator;
	callback->Clb_Mod_getDescription = &skirmishAiCallback_Mod_getDescription;
	callback->Clb_Mod_getAllowTeamColors = &skirmishAiCallback_Mod_getAllowTeamColors;
	callback->Clb_Mod_getConstructionDecay = &skirmishAiCallback_Mod_getConstructionDecay;
	callback->Clb_Mod_getConstructionDecayTime = &skirmishAiCallback_Mod_getConstructionDecayTime;
	callback->Clb_Mod_getConstructionDecaySpeed = &skirmishAiCallback_Mod_getConstructionDecaySpeed;
	callback->Clb_Mod_getMultiReclaim = &skirmishAiCallback_Mod_getMultiReclaim;
	callback->Clb_Mod_getReclaimMethod = &skirmishAiCallback_Mod_getReclaimMethod;
	callback->Clb_Mod_getReclaimUnitMethod = &skirmishAiCallback_Mod_getReclaimUnitMethod;
	callback->Clb_Mod_getReclaimUnitEnergyCostFactor = &skirmishAiCallback_Mod_getReclaimUnitEnergyCostFactor;
	callback->Clb_Mod_getReclaimUnitEfficiency = &skirmishAiCallback_Mod_getReclaimUnitEfficiency;
	callback->Clb_Mod_getReclaimFeatureEnergyCostFactor = &skirmishAiCallback_Mod_getReclaimFeatureEnergyCostFactor;
	callback->Clb_Mod_getReclaimAllowEnemies = &skirmishAiCallback_Mod_getReclaimAllowEnemies;
	callback->Clb_Mod_getReclaimAllowAllies = &skirmishAiCallback_Mod_getReclaimAllowAllies;
	callback->Clb_Mod_getRepairEnergyCostFactor = &skirmishAiCallback_Mod_getRepairEnergyCostFactor;
	callback->Clb_Mod_getResurrectEnergyCostFactor = &skirmishAiCallback_Mod_getResurrectEnergyCostFactor;
	callback->Clb_Mod_getCaptureEnergyCostFactor = &skirmishAiCallback_Mod_getCaptureEnergyCostFactor;
	callback->Clb_Mod_getTransportGround = &skirmishAiCallback_Mod_getTransportGround;
	callback->Clb_Mod_getTransportHover = &skirmishAiCallback_Mod_getTransportHover;
	callback->Clb_Mod_getTransportShip = &skirmishAiCallback_Mod_getTransportShip;
	callback->Clb_Mod_getTransportAir = &skirmishAiCallback_Mod_getTransportAir;
	callback->Clb_Mod_getFireAtKilled = &skirmishAiCallback_Mod_getFireAtKilled;
	callback->Clb_Mod_getFireAtCrashing = &skirmishAiCallback_Mod_getFireAtCrashing;
	callback->Clb_Mod_getFlankingBonusModeDefault = &skirmishAiCallback_Mod_getFlankingBonusModeDefault;
	callback->Clb_Mod_getLosMipLevel = &skirmishAiCallback_Mod_getLosMipLevel;
	callback->Clb_Mod_getAirMipLevel = &skirmishAiCallback_Mod_getAirMipLevel;
	callback->Clb_Mod_getLosMul = &skirmishAiCallback_Mod_getLosMul;
	callback->Clb_Mod_getAirLosMul = &skirmishAiCallback_Mod_getAirLosMul;
	callback->Clb_Mod_getRequireSonarUnderWater = &skirmishAiCallback_Mod_getRequireSonarUnderWater;
	callback->Clb_Map_getChecksum = &skirmishAiCallback_Map_getChecksum;
	callback->Clb_Map_getStartPos = &skirmishAiCallback_Map_getStartPos;
	callback->Clb_Map_getMousePos = &skirmishAiCallback_Map_getMousePos;
	callback->Clb_Map_isPosInCamera = &skirmishAiCallback_Map_isPosInCamera;
	callback->Clb_Map_getWidth = &skirmishAiCallback_Map_getWidth;
	callback->Clb_Map_getHeight = &skirmishAiCallback_Map_getHeight;
	callback->Clb_Map_0ARRAY1SIZE0getHeightMap = &skirmishAiCallback_Map_0ARRAY1SIZE0getHeightMap;
	callback->Clb_Map_0ARRAY1VALS0getHeightMap = &skirmishAiCallback_Map_0ARRAY1VALS0getHeightMap;
	callback->Clb_Map_getMinHeight = &skirmishAiCallback_Map_getMinHeight;
	callback->Clb_Map_getMaxHeight = &skirmishAiCallback_Map_getMaxHeight;
	callback->Clb_Map_0ARRAY1SIZE0getSlopeMap = &skirmishAiCallback_Map_0ARRAY1SIZE0getSlopeMap;
	callback->Clb_Map_0ARRAY1VALS0getSlopeMap = &skirmishAiCallback_Map_0ARRAY1VALS0getSlopeMap;
	callback->Clb_Map_0ARRAY1SIZE0getLosMap = &skirmishAiCallback_Map_0ARRAY1SIZE0getLosMap;
	callback->Clb_Map_0ARRAY1VALS0getLosMap = &skirmishAiCallback_Map_0ARRAY1VALS0getLosMap;
	callback->Clb_Map_0ARRAY1SIZE0getRadarMap = &skirmishAiCallback_Map_0ARRAY1SIZE0getRadarMap;
	callback->Clb_Map_0ARRAY1VALS0getRadarMap = &skirmishAiCallback_Map_0ARRAY1VALS0getRadarMap;
	callback->Clb_Map_0ARRAY1SIZE0getJammerMap = &skirmishAiCallback_Map_0ARRAY1SIZE0getJammerMap;
	callback->Clb_Map_0ARRAY1VALS0getJammerMap = &skirmishAiCallback_Map_0ARRAY1VALS0getJammerMap;
	callback->Clb_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMapRaw = &skirmishAiCallback_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMapRaw;
	callback->Clb_Map_0ARRAY1VALS0REF1Resource2resourceId0getResourceMapRaw = &skirmishAiCallback_Map_0ARRAY1VALS0REF1Resource2resourceId0getResourceMapRaw;
	callback->Clb_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMapSpotsPositions = &skirmishAiCallback_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMapSpotsPositions;
	callback->Clb_Map_0ARRAY1VALS0REF1Resource2resourceId0getResourceMapSpotsPositions = &skirmishAiCallback_Map_0ARRAY1VALS0REF1Resource2resourceId0getResourceMapSpotsPositions;
	callback->Clb_Map_0ARRAY1VALS0REF1Resource2resourceId0initResourceMapSpotsAverageIncome = &skirmishAiCallback_Map_0ARRAY1VALS0REF1Resource2resourceId0initResourceMapSpotsAverageIncome;
	callback->Clb_Map_0ARRAY1VALS0REF1Resource2resourceId0initResourceMapSpotsNearest = &skirmishAiCallback_Map_0ARRAY1VALS0REF1Resource2resourceId0initResourceMapSpotsNearest;
	callback->Clb_Map_getName = &skirmishAiCallback_Map_getName;
	callback->Clb_Map_getElevationAt = &skirmishAiCallback_Map_getElevationAt;
	callback->Clb_Map_0REF1Resource2resourceId0getMaxResource = &skirmishAiCallback_Map_0REF1Resource2resourceId0getMaxResource;
	callback->Clb_Map_0REF1Resource2resourceId0getExtractorRadius = &skirmishAiCallback_Map_0REF1Resource2resourceId0getExtractorRadius;
	callback->Clb_Map_getMinWind = &skirmishAiCallback_Map_getMinWind;
	callback->Clb_Map_getMaxWind = &skirmishAiCallback_Map_getMaxWind;
	callback->Clb_Map_getTidalStrength = &skirmishAiCallback_Map_getTidalStrength;
	callback->Clb_Map_getGravity = &skirmishAiCallback_Map_getGravity;
	callback->Clb_Map_0MULTI1SIZE0Point = &skirmishAiCallback_Map_0MULTI1SIZE0Point;
	callback->Clb_Map_Point_getPosition = &skirmishAiCallback_Map_Point_getPosition;
	callback->Clb_Map_Point_getColor = &skirmishAiCallback_Map_Point_getColor;
	callback->Clb_Map_Point_getLabel = &skirmishAiCallback_Map_Point_getLabel;
	callback->Clb_Map_0MULTI1SIZE0Line = &skirmishAiCallback_Map_0MULTI1SIZE0Line;
	callback->Clb_Map_Line_getFirstPosition = &skirmishAiCallback_Map_Line_getFirstPosition;
	callback->Clb_Map_Line_getSecondPosition = &skirmishAiCallback_Map_Line_getSecondPosition;
	callback->Clb_Map_Line_getColor = &skirmishAiCallback_Map_Line_getColor;
	callback->Clb_Map_0REF1UnitDef2unitDefId0isPossibleToBuildAt = &skirmishAiCallback_Map_0REF1UnitDef2unitDefId0isPossibleToBuildAt;
	callback->Clb_Map_0REF1UnitDef2unitDefId0findClosestBuildSite = &skirmishAiCallback_Map_0REF1UnitDef2unitDefId0findClosestBuildSite;
	callback->Clb_0MULTI1SIZE0FeatureDef = &skirmishAiCallback_0MULTI1SIZE0FeatureDef;
	callback->Clb_0MULTI1VALS0FeatureDef = &skirmishAiCallback_0MULTI1VALS0FeatureDef;
	callback->Clb_FeatureDef_getName = &skirmishAiCallback_FeatureDef_getName;
	callback->Clb_FeatureDef_getDescription = &skirmishAiCallback_FeatureDef_getDescription;
	callback->Clb_FeatureDef_getFileName = &skirmishAiCallback_FeatureDef_getFileName;
	callback->Clb_FeatureDef_0REF1Resource2resourceId0getContainedResource = &skirmishAiCallback_FeatureDef_0REF1Resource2resourceId0getContainedResource;
	callback->Clb_FeatureDef_getMaxHealth = &skirmishAiCallback_FeatureDef_getMaxHealth;
	callback->Clb_FeatureDef_getReclaimTime = &skirmishAiCallback_FeatureDef_getReclaimTime;
	callback->Clb_FeatureDef_getMass = &skirmishAiCallback_FeatureDef_getMass;
	callback->Clb_FeatureDef_CollisionVolume_getType = &skirmishAiCallback_FeatureDef_CollisionVolume_getType;
	callback->Clb_FeatureDef_CollisionVolume_getScales = &skirmishAiCallback_FeatureDef_CollisionVolume_getScales;
	callback->Clb_FeatureDef_CollisionVolume_getOffsets = &skirmishAiCallback_FeatureDef_CollisionVolume_getOffsets;
	callback->Clb_FeatureDef_CollisionVolume_getTest = &skirmishAiCallback_FeatureDef_CollisionVolume_getTest;
	callback->Clb_FeatureDef_isUpright = &skirmishAiCallback_FeatureDef_isUpright;
	callback->Clb_FeatureDef_getDrawType = &skirmishAiCallback_FeatureDef_getDrawType;
	callback->Clb_FeatureDef_getModelName = &skirmishAiCallback_FeatureDef_getModelName;
	callback->Clb_FeatureDef_getResurrectable = &skirmishAiCallback_FeatureDef_getResurrectable;
	callback->Clb_FeatureDef_getSmokeTime = &skirmishAiCallback_FeatureDef_getSmokeTime;
	callback->Clb_FeatureDef_isDestructable = &skirmishAiCallback_FeatureDef_isDestructable;
	callback->Clb_FeatureDef_isReclaimable = &skirmishAiCallback_FeatureDef_isReclaimable;
	callback->Clb_FeatureDef_isBlocking = &skirmishAiCallback_FeatureDef_isBlocking;
	callback->Clb_FeatureDef_isBurnable = &skirmishAiCallback_FeatureDef_isBurnable;
	callback->Clb_FeatureDef_isFloating = &skirmishAiCallback_FeatureDef_isFloating;
	callback->Clb_FeatureDef_isNoSelect = &skirmishAiCallback_FeatureDef_isNoSelect;
	callback->Clb_FeatureDef_isGeoThermal = &skirmishAiCallback_FeatureDef_isGeoThermal;
	callback->Clb_FeatureDef_getDeathFeature = &skirmishAiCallback_FeatureDef_getDeathFeature;
	callback->Clb_FeatureDef_getXSize = &skirmishAiCallback_FeatureDef_getXSize;
	callback->Clb_FeatureDef_getZSize = &skirmishAiCallback_FeatureDef_getZSize;
	callback->Clb_FeatureDef_0MAP1SIZE0getCustomParams = &skirmishAiCallback_FeatureDef_0MAP1SIZE0getCustomParams;
	callback->Clb_FeatureDef_0MAP1KEYS0getCustomParams = &skirmishAiCallback_FeatureDef_0MAP1KEYS0getCustomParams;
	callback->Clb_FeatureDef_0MAP1VALS0getCustomParams = &skirmishAiCallback_FeatureDef_0MAP1VALS0getCustomParams;
	callback->Clb_0MULTI1SIZE0Feature = &skirmishAiCallback_0MULTI1SIZE0Feature;
	callback->Clb_0MULTI1VALS0Feature = &skirmishAiCallback_0MULTI1VALS0Feature;
	callback->Clb_0MULTI1SIZE3FeaturesIn0Feature = &skirmishAiCallback_0MULTI1SIZE3FeaturesIn0Feature;
	callback->Clb_0MULTI1VALS3FeaturesIn0Feature = &skirmishAiCallback_0MULTI1VALS3FeaturesIn0Feature;
	callback->Clb_Feature_0SINGLE1FETCH2FeatureDef0getDef = &skirmishAiCallback_Feature_0SINGLE1FETCH2FeatureDef0getDef;
	callback->Clb_Feature_getHealth = &skirmishAiCallback_Feature_getHealth;
	callback->Clb_Feature_getReclaimLeft = &skirmishAiCallback_Feature_getReclaimLeft;
	callback->Clb_Feature_getPosition = &skirmishAiCallback_Feature_getPosition;
	callback->Clb_0MULTI1SIZE0WeaponDef = &skirmishAiCallback_0MULTI1SIZE0WeaponDef;
	callback->Clb_0MULTI1FETCH3WeaponDefByName0WeaponDef = &skirmishAiCallback_0MULTI1FETCH3WeaponDefByName0WeaponDef;
	callback->Clb_WeaponDef_getName = &skirmishAiCallback_WeaponDef_getName;
	callback->Clb_WeaponDef_getType = &skirmishAiCallback_WeaponDef_getType;
	callback->Clb_WeaponDef_getDescription = &skirmishAiCallback_WeaponDef_getDescription;
	callback->Clb_WeaponDef_getFileName = &skirmishAiCallback_WeaponDef_getFileName;
	callback->Clb_WeaponDef_getCegTag = &skirmishAiCallback_WeaponDef_getCegTag;
	callback->Clb_WeaponDef_getRange = &skirmishAiCallback_WeaponDef_getRange;
	callback->Clb_WeaponDef_getHeightMod = &skirmishAiCallback_WeaponDef_getHeightMod;
	callback->Clb_WeaponDef_getAccuracy = &skirmishAiCallback_WeaponDef_getAccuracy;
	callback->Clb_WeaponDef_getSprayAngle = &skirmishAiCallback_WeaponDef_getSprayAngle;
	callback->Clb_WeaponDef_getMovingAccuracy = &skirmishAiCallback_WeaponDef_getMovingAccuracy;
	callback->Clb_WeaponDef_getTargetMoveError = &skirmishAiCallback_WeaponDef_getTargetMoveError;
	callback->Clb_WeaponDef_getLeadLimit = &skirmishAiCallback_WeaponDef_getLeadLimit;
	callback->Clb_WeaponDef_getLeadBonus = &skirmishAiCallback_WeaponDef_getLeadBonus;
	callback->Clb_WeaponDef_getPredictBoost = &skirmishAiCallback_WeaponDef_getPredictBoost;
	callback->Clb_WeaponDef_0STATIC0getNumDamageTypes = &skirmishAiCallback_WeaponDef_0STATIC0getNumDamageTypes;
	callback->Clb_WeaponDef_Damage_getParalyzeDamageTime = &skirmishAiCallback_WeaponDef_Damage_getParalyzeDamageTime;
	callback->Clb_WeaponDef_Damage_getImpulseFactor = &skirmishAiCallback_WeaponDef_Damage_getImpulseFactor;
	callback->Clb_WeaponDef_Damage_getImpulseBoost = &skirmishAiCallback_WeaponDef_Damage_getImpulseBoost;
	callback->Clb_WeaponDef_Damage_getCraterMult = &skirmishAiCallback_WeaponDef_Damage_getCraterMult;
	callback->Clb_WeaponDef_Damage_getCraterBoost = &skirmishAiCallback_WeaponDef_Damage_getCraterBoost;
	callback->Clb_WeaponDef_Damage_0ARRAY1SIZE0getTypes = &skirmishAiCallback_WeaponDef_Damage_0ARRAY1SIZE0getTypes;
	callback->Clb_WeaponDef_Damage_0ARRAY1VALS0getTypes = &skirmishAiCallback_WeaponDef_Damage_0ARRAY1VALS0getTypes;
	callback->Clb_WeaponDef_getAreaOfEffect = &skirmishAiCallback_WeaponDef_getAreaOfEffect;
	callback->Clb_WeaponDef_isNoSelfDamage = &skirmishAiCallback_WeaponDef_isNoSelfDamage;
	callback->Clb_WeaponDef_getFireStarter = &skirmishAiCallback_WeaponDef_getFireStarter;
	callback->Clb_WeaponDef_getEdgeEffectiveness = &skirmishAiCallback_WeaponDef_getEdgeEffectiveness;
	callback->Clb_WeaponDef_getSize = &skirmishAiCallback_WeaponDef_getSize;
	callback->Clb_WeaponDef_getSizeGrowth = &skirmishAiCallback_WeaponDef_getSizeGrowth;
	callback->Clb_WeaponDef_getCollisionSize = &skirmishAiCallback_WeaponDef_getCollisionSize;
	callback->Clb_WeaponDef_getSalvoSize = &skirmishAiCallback_WeaponDef_getSalvoSize;
	callback->Clb_WeaponDef_getSalvoDelay = &skirmishAiCallback_WeaponDef_getSalvoDelay;
	callback->Clb_WeaponDef_getReload = &skirmishAiCallback_WeaponDef_getReload;
	callback->Clb_WeaponDef_getBeamTime = &skirmishAiCallback_WeaponDef_getBeamTime;
	callback->Clb_WeaponDef_isBeamBurst = &skirmishAiCallback_WeaponDef_isBeamBurst;
	callback->Clb_WeaponDef_isWaterBounce = &skirmishAiCallback_WeaponDef_isWaterBounce;
	callback->Clb_WeaponDef_isGroundBounce = &skirmishAiCallback_WeaponDef_isGroundBounce;
	callback->Clb_WeaponDef_getBounceRebound = &skirmishAiCallback_WeaponDef_getBounceRebound;
	callback->Clb_WeaponDef_getBounceSlip = &skirmishAiCallback_WeaponDef_getBounceSlip;
	callback->Clb_WeaponDef_getNumBounce = &skirmishAiCallback_WeaponDef_getNumBounce;
	callback->Clb_WeaponDef_getMaxAngle = &skirmishAiCallback_WeaponDef_getMaxAngle;
	callback->Clb_WeaponDef_getRestTime = &skirmishAiCallback_WeaponDef_getRestTime;
	callback->Clb_WeaponDef_getUpTime = &skirmishAiCallback_WeaponDef_getUpTime;
	callback->Clb_WeaponDef_getFlightTime = &skirmishAiCallback_WeaponDef_getFlightTime;
	callback->Clb_WeaponDef_0REF1Resource2resourceId0getCost = &skirmishAiCallback_WeaponDef_0REF1Resource2resourceId0getCost;
	callback->Clb_WeaponDef_getSupplyCost = &skirmishAiCallback_WeaponDef_getSupplyCost;
	callback->Clb_WeaponDef_getProjectilesPerShot = &skirmishAiCallback_WeaponDef_getProjectilesPerShot;
	callback->Clb_WeaponDef_isTurret = &skirmishAiCallback_WeaponDef_isTurret;
	callback->Clb_WeaponDef_isOnlyForward = &skirmishAiCallback_WeaponDef_isOnlyForward;
	callback->Clb_WeaponDef_isFixedLauncher = &skirmishAiCallback_WeaponDef_isFixedLauncher;
	callback->Clb_WeaponDef_isWaterWeapon = &skirmishAiCallback_WeaponDef_isWaterWeapon;
	callback->Clb_WeaponDef_isFireSubmersed = &skirmishAiCallback_WeaponDef_isFireSubmersed;
	callback->Clb_WeaponDef_isSubMissile = &skirmishAiCallback_WeaponDef_isSubMissile;
	callback->Clb_WeaponDef_isTracks = &skirmishAiCallback_WeaponDef_isTracks;
	callback->Clb_WeaponDef_isDropped = &skirmishAiCallback_WeaponDef_isDropped;
	callback->Clb_WeaponDef_isParalyzer = &skirmishAiCallback_WeaponDef_isParalyzer;
	callback->Clb_WeaponDef_isImpactOnly = &skirmishAiCallback_WeaponDef_isImpactOnly;
	callback->Clb_WeaponDef_isNoAutoTarget = &skirmishAiCallback_WeaponDef_isNoAutoTarget;
	callback->Clb_WeaponDef_isManualFire = &skirmishAiCallback_WeaponDef_isManualFire;
	callback->Clb_WeaponDef_getInterceptor = &skirmishAiCallback_WeaponDef_getInterceptor;
	callback->Clb_WeaponDef_getTargetable = &skirmishAiCallback_WeaponDef_getTargetable;
	callback->Clb_WeaponDef_isStockpileable = &skirmishAiCallback_WeaponDef_isStockpileable;
	callback->Clb_WeaponDef_getCoverageRange = &skirmishAiCallback_WeaponDef_getCoverageRange;
	callback->Clb_WeaponDef_getStockpileTime = &skirmishAiCallback_WeaponDef_getStockpileTime;
	callback->Clb_WeaponDef_getIntensity = &skirmishAiCallback_WeaponDef_getIntensity;
	callback->Clb_WeaponDef_getThickness = &skirmishAiCallback_WeaponDef_getThickness;
	callback->Clb_WeaponDef_getLaserFlareSize = &skirmishAiCallback_WeaponDef_getLaserFlareSize;
	callback->Clb_WeaponDef_getCoreThickness = &skirmishAiCallback_WeaponDef_getCoreThickness;
	callback->Clb_WeaponDef_getDuration = &skirmishAiCallback_WeaponDef_getDuration;
	callback->Clb_WeaponDef_getLodDistance = &skirmishAiCallback_WeaponDef_getLodDistance;
	callback->Clb_WeaponDef_getFalloffRate = &skirmishAiCallback_WeaponDef_getFalloffRate;
	callback->Clb_WeaponDef_getGraphicsType = &skirmishAiCallback_WeaponDef_getGraphicsType;
	callback->Clb_WeaponDef_isSoundTrigger = &skirmishAiCallback_WeaponDef_isSoundTrigger;
	callback->Clb_WeaponDef_isSelfExplode = &skirmishAiCallback_WeaponDef_isSelfExplode;
	callback->Clb_WeaponDef_isGravityAffected = &skirmishAiCallback_WeaponDef_isGravityAffected;
	callback->Clb_WeaponDef_getHighTrajectory = &skirmishAiCallback_WeaponDef_getHighTrajectory;
	callback->Clb_WeaponDef_getMyGravity = &skirmishAiCallback_WeaponDef_getMyGravity;
	callback->Clb_WeaponDef_isNoExplode = &skirmishAiCallback_WeaponDef_isNoExplode;
	callback->Clb_WeaponDef_getStartVelocity = &skirmishAiCallback_WeaponDef_getStartVelocity;
	callback->Clb_WeaponDef_getWeaponAcceleration = &skirmishAiCallback_WeaponDef_getWeaponAcceleration;
	callback->Clb_WeaponDef_getTurnRate = &skirmishAiCallback_WeaponDef_getTurnRate;
	callback->Clb_WeaponDef_getMaxVelocity = &skirmishAiCallback_WeaponDef_getMaxVelocity;
	callback->Clb_WeaponDef_getProjectileSpeed = &skirmishAiCallback_WeaponDef_getProjectileSpeed;
	callback->Clb_WeaponDef_getExplosionSpeed = &skirmishAiCallback_WeaponDef_getExplosionSpeed;
	callback->Clb_WeaponDef_getOnlyTargetCategory = &skirmishAiCallback_WeaponDef_getOnlyTargetCategory;
	callback->Clb_WeaponDef_getWobble = &skirmishAiCallback_WeaponDef_getWobble;
	callback->Clb_WeaponDef_getDance = &skirmishAiCallback_WeaponDef_getDance;
	callback->Clb_WeaponDef_getTrajectoryHeight = &skirmishAiCallback_WeaponDef_getTrajectoryHeight;
	callback->Clb_WeaponDef_isLargeBeamLaser = &skirmishAiCallback_WeaponDef_isLargeBeamLaser;
	callback->Clb_WeaponDef_isShield = &skirmishAiCallback_WeaponDef_isShield;
	callback->Clb_WeaponDef_isShieldRepulser = &skirmishAiCallback_WeaponDef_isShieldRepulser;
	callback->Clb_WeaponDef_isSmartShield = &skirmishAiCallback_WeaponDef_isSmartShield;
	callback->Clb_WeaponDef_isExteriorShield = &skirmishAiCallback_WeaponDef_isExteriorShield;
	callback->Clb_WeaponDef_isVisibleShield = &skirmishAiCallback_WeaponDef_isVisibleShield;
	callback->Clb_WeaponDef_isVisibleShieldRepulse = &skirmishAiCallback_WeaponDef_isVisibleShieldRepulse;
	callback->Clb_WeaponDef_getVisibleShieldHitFrames = &skirmishAiCallback_WeaponDef_getVisibleShieldHitFrames;
	callback->Clb_WeaponDef_Shield_0REF1Resource2resourceId0getResourceUse = &skirmishAiCallback_WeaponDef_Shield_0REF1Resource2resourceId0getResourceUse;
	callback->Clb_WeaponDef_Shield_getRadius = &skirmishAiCallback_WeaponDef_Shield_getRadius;
	callback->Clb_WeaponDef_Shield_getForce = &skirmishAiCallback_WeaponDef_Shield_getForce;
	callback->Clb_WeaponDef_Shield_getMaxSpeed = &skirmishAiCallback_WeaponDef_Shield_getMaxSpeed;
	callback->Clb_WeaponDef_Shield_getPower = &skirmishAiCallback_WeaponDef_Shield_getPower;
	callback->Clb_WeaponDef_Shield_getPowerRegen = &skirmishAiCallback_WeaponDef_Shield_getPowerRegen;
	callback->Clb_WeaponDef_Shield_0REF1Resource2resourceId0getPowerRegenResource = &skirmishAiCallback_WeaponDef_Shield_0REF1Resource2resourceId0getPowerRegenResource;
	callback->Clb_WeaponDef_Shield_getStartingPower = &skirmishAiCallback_WeaponDef_Shield_getStartingPower;
	callback->Clb_WeaponDef_Shield_getRechargeDelay = &skirmishAiCallback_WeaponDef_Shield_getRechargeDelay;
	callback->Clb_WeaponDef_Shield_getGoodColor = &skirmishAiCallback_WeaponDef_Shield_getGoodColor;
	callback->Clb_WeaponDef_Shield_getBadColor = &skirmishAiCallback_WeaponDef_Shield_getBadColor;
	callback->Clb_WeaponDef_Shield_getAlpha = &skirmishAiCallback_WeaponDef_Shield_getAlpha;
	callback->Clb_WeaponDef_Shield_getInterceptType = &skirmishAiCallback_WeaponDef_Shield_getInterceptType;
	callback->Clb_WeaponDef_getInterceptedByShieldType = &skirmishAiCallback_WeaponDef_getInterceptedByShieldType;
	callback->Clb_WeaponDef_isAvoidFriendly = &skirmishAiCallback_WeaponDef_isAvoidFriendly;
	callback->Clb_WeaponDef_isAvoidFeature = &skirmishAiCallback_WeaponDef_isAvoidFeature;
	callback->Clb_WeaponDef_isAvoidNeutral = &skirmishAiCallback_WeaponDef_isAvoidNeutral;
	callback->Clb_WeaponDef_getTargetBorder = &skirmishAiCallback_WeaponDef_getTargetBorder;
	callback->Clb_WeaponDef_getCylinderTargetting = &skirmishAiCallback_WeaponDef_getCylinderTargetting;
	callback->Clb_WeaponDef_getMinIntensity = &skirmishAiCallback_WeaponDef_getMinIntensity;
	callback->Clb_WeaponDef_getHeightBoostFactor = &skirmishAiCallback_WeaponDef_getHeightBoostFactor;
	callback->Clb_WeaponDef_getProximityPriority = &skirmishAiCallback_WeaponDef_getProximityPriority;
	callback->Clb_WeaponDef_getCollisionFlags = &skirmishAiCallback_WeaponDef_getCollisionFlags;
	callback->Clb_WeaponDef_isSweepFire = &skirmishAiCallback_WeaponDef_isSweepFire;
	callback->Clb_WeaponDef_isAbleToAttackGround = &skirmishAiCallback_WeaponDef_isAbleToAttackGround;
	callback->Clb_WeaponDef_getCameraShake = &skirmishAiCallback_WeaponDef_getCameraShake;
	callback->Clb_WeaponDef_getDynDamageExp = &skirmishAiCallback_WeaponDef_getDynDamageExp;
	callback->Clb_WeaponDef_getDynDamageMin = &skirmishAiCallback_WeaponDef_getDynDamageMin;
	callback->Clb_WeaponDef_getDynDamageRange = &skirmishAiCallback_WeaponDef_getDynDamageRange;
	callback->Clb_WeaponDef_isDynDamageInverted = &skirmishAiCallback_WeaponDef_isDynDamageInverted;
	callback->Clb_WeaponDef_0MAP1SIZE0getCustomParams = &skirmishAiCallback_WeaponDef_0MAP1SIZE0getCustomParams;
	callback->Clb_WeaponDef_0MAP1KEYS0getCustomParams = &skirmishAiCallback_WeaponDef_0MAP1KEYS0getCustomParams;
	callback->Clb_WeaponDef_0MAP1VALS0getCustomParams = &skirmishAiCallback_WeaponDef_0MAP1VALS0getCustomParams;
}

SSkirmishAICallback* skirmishAiCallback_getInstanceFor(int teamId, IGlobalAICallback* globalAICallback) {

	SSkirmishAICallback* callback = new SSkirmishAICallback();
	skirmishAiCallback_init(callback);

	team_globalCallback[teamId] = globalAICallback;
	team_callback[teamId] = globalAICallback->GetAICallback();
	team_cCallback[teamId] = callback;

	return callback;
}
void skirmishAiCallback_release(int teamId) {

	team_globalCallback.erase(teamId);
	team_callback.erase(teamId);

	SSkirmishAICallback* callback = team_cCallback[teamId];
	team_cCallback.erase(teamId);
	delete callback;
}
