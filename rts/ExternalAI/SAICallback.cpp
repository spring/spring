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

#include "Interface/SAICallback.h"

#include "IGlobalAICallback.h"
#include "IAICallback.h"
#include "IAICheats.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Misc/Resource.h"
#include "Sim/Misc/ResourceHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Map/ReadMap.h"
#include "Map/MetalMap.h"
#include "Game/SelectedUnits.h"
#include "Game/UI/GuiHandler.h"		//TODO: fix some switch for new gui
#include "ExternalAI/Group.h"
#include "ExternalAI/GroupHandler.h"
#include "Interface/AISCommands.h"

static IGlobalAICallback* team_globalCallback[MAX_SKIRMISH_AIS];
static IAICallback* team_callback[MAX_SKIRMISH_AIS];
static bool team_cheatingEnabled[MAX_SKIRMISH_AIS];
static IAICheats* team_cheatCallback[MAX_SKIRMISH_AIS];

static const unsigned int TMP_ARR_SIZE = 16384;
static int tmpSize[MAX_SKIRMISH_AIS];
// the following lines are relatively memory intensive (~1MB per line)
// this memory is only freed at exit of the application
static int tmpIntArr[MAX_SKIRMISH_AIS][TMP_ARR_SIZE];
static PointMarker tmpPointMarkerArr[MAX_SKIRMISH_AIS][TMP_ARR_SIZE];
static LineMarker tmpLineMarkerArr[MAX_SKIRMISH_AIS][TMP_ARR_SIZE];
static const char* tmpKeysArr[MAX_SKIRMISH_AIS][TMP_ARR_SIZE];
static const char* tmpValuesArr[MAX_SKIRMISH_AIS][TMP_ARR_SIZE];

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
	//TODO
	//???
	//bool gs->players[i]->CanControlTeam(int teamID);
	//bool gs->Team(teamId)->gaia;
	//bool gs->Team(teamId)->isAI;
	//bool gs->players[0]->;
	return true;
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
EXPORT(int) _Clb_handleCommand(int teamId, int toId, int commandId,
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
				if (cmd->resourceId == rh->GetMetalId()) {
					clbCheat->GiveMeMetal(cmd->amount);
					ret = 0;
				} else if (cmd->resourceId == rh->GetEnergyId()) {
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
			cmd->ret_isExecuted = clb->SendResources(cmd->mAmount, cmd->eAmount,
					cmd->receivingTeam);
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

		case COMMAND_SHARED_MEM_AREA_CREATE:
		{
			SCreateSharedMemAreaCommand* cmd =
					(SCreateSharedMemAreaCommand*) commandData;
			cmd->ret_sharedMemArea =
					clb->CreateSharedMemArea(cmd->name, cmd->size);
			break;
		}
		case COMMAND_SHARED_MEM_AREA_RELEASE:
		{
			SReleaseSharedMemAreaCommand* cmd =
					(SReleaseSharedMemAreaCommand*) commandData;
			clb->ReleasedSharedMemArea(cmd->name);
			break;
		}

		case COMMAND_GROUP_CREATE:
		{
			SCreateGroupCommand* cmd = (SCreateGroupCommand*) commandData;
			cmd->ret_groupId =
					clb->CreateGroup(cmd->libraryName, cmd->aiNumber);
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
			} else { // it is no known command
				ret = -1;
			}
		}

	}

	return ret;
}




//##############################################################################
EXPORT(bool) _Clb_Cheats_isEnabled(int teamId) {
	team_cheatCallback[teamId] = NULL;
	if (team_cheatingEnabled[teamId]) {
		team_cheatCallback[teamId] =
				team_globalCallback[teamId]->GetCheatInterface();
	}
	return team_cheatCallback[teamId] != NULL;
}

EXPORT(bool) _Clb_Cheats_setEnabled(int teamId, bool enabled) {
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
	return enabled == _Clb_Cheats_isEnabled(teamId);
}

EXPORT(bool) _Clb_Cheats_setEventsEnabled(int teamId, bool enabled) {
	if (_Clb_Cheats_isEnabled(teamId)) {
		team_cheatCallback[teamId]->EnableCheatEvents(enabled);
		return true;
	} else {
		return false;
	}
}

EXPORT(bool) _Clb_Cheats_isOnlyPassive(int teamId) {
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->OnlyPassiveCheats();
	} else {
		return true; //TODO: is this correct?
	}
}

EXPORT(int) _Clb_Game_getAiInterfaceVersion(int teamId) {
	IAICallback* clb = team_callback[teamId];
	return wrapper_HandleCommand(clb, NULL, AIHCQuerySubVersionId, NULL);
}

EXPORT(int) _Clb_Game_getCurrentFrame(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetCurrentFrame();
}

EXPORT(int) _Clb_Game_getMyTeam(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMyTeam();
}

EXPORT(int) _Clb_Game_getMyAllyTeam(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMyAllyTeam();
}

EXPORT(int) _Clb_Game_getPlayerTeam(int teamId, int player) {
	IAICallback* clb = team_callback[teamId]; return clb->GetPlayerTeam(player);
}

EXPORT(const char*) _Clb_Game_getTeamSide(int teamId, int team) {
	IAICallback* clb = team_callback[teamId]; return clb->GetTeamSide(team);
}





EXPORT(int) _Clb_WeaponDef_0STATIC0getNumDamageTypes(int teamId) {
	int numDamageTypes;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_NUMDAMAGETYPES, &numDamageTypes);
	if (!fetchOk) {
		numDamageTypes = -1;
	}
	return numDamageTypes;
}

EXPORT(bool) _Clb_Game_isExceptionHandlingEnabled(int teamId) {
	bool exceptionHandlingEnabled;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_EXCEPTION_HANDLING,
			&exceptionHandlingEnabled);
	if (!fetchOk) {
		exceptionHandlingEnabled = false;
	}
	return exceptionHandlingEnabled;
}
EXPORT(bool) _Clb_Game_isDebugModeEnabled(int teamId) {
	bool debugModeEnabled;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_DEBUG_MODE, &debugModeEnabled);
	if (!fetchOk) {
		debugModeEnabled = false;
	}
	return debugModeEnabled;
}
EXPORT(int) _Clb_Game_getMode(int teamId) {
	int mode;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_GAME_MODE, &mode);
	if (!fetchOk) {
		mode = -1;
	}
	return mode;
}
EXPORT(bool) _Clb_Game_isPaused(int teamId) {
	bool paused;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_GAME_PAUSED, &paused);
	if (!fetchOk) {
		paused = false;
	}
	return paused;
}
EXPORT(float) _Clb_Game_getSpeedFactor(int teamId) {
	float speedFactor;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_GAME_SPEED_FACTOR, &speedFactor);
	if (!fetchOk) {
		speedFactor = false;
	}
	return speedFactor;
}

EXPORT(float) _Clb_Gui_getViewRange(int teamId) {
	float viewRange;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_GUI_VIEW_RANGE, &viewRange);
	if (!fetchOk) {
		viewRange = false;
	}
	return viewRange;
}
EXPORT(float) _Clb_Gui_getScreenX(int teamId) {
	float screenX;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_GUI_SCREENX, &screenX);
	if (!fetchOk) {
		screenX = false;
	}
	return screenX;
}
EXPORT(float) _Clb_Gui_getScreenY(int teamId) {
	float screenY;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_GUI_SCREENY, &screenY);
	if (!fetchOk) {
		screenY = false;
	}
	return screenY;
}
EXPORT(SAIFloat3) _Clb_Gui_Camera_getDirection(int teamId) {
	float3 cameraDir;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_GUI_CAMERA_DIR, &cameraDir);
	if (!fetchOk) {
		cameraDir = float3(1.0f, 0.0f, 0.0f);
	}
	return cameraDir.toSAIFloat3();
}
EXPORT(SAIFloat3) _Clb_Gui_Camera_getPosition(int teamId) {
	float3 cameraPosition;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_GUI_CAMERA_POS, &cameraPosition);
	if (!fetchOk) {
		cameraPosition = float3(1.0f, 0.0f, 0.0f);
	}
	return cameraPosition.toSAIFloat3();
}

EXPORT(bool) _Clb_File_locateForReading(int teamId, char* fileName) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetValue(AIVAL_LOCATE_FILE_R, fileName);
}
EXPORT(bool) _Clb_File_locateForWriting(int teamId, char* fileName) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetValue(AIVAL_LOCATE_FILE_W, fileName);
}



EXPORT(const char*) _Clb_Mod_getName(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetModName();
}

//########### BEGINN Map
EXPORT(bool) _Clb_Map_isPosInCamera(int teamId, SAIFloat3 pos, float radius) {
	IAICallback* clb = team_callback[teamId];
	return clb->PosInCamera(SAIFloat3(pos), radius);
}
EXPORT(unsigned int) _Clb_Map_getChecksum(int teamId) {
	unsigned int checksum;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_MAP_CHECKSUM, &checksum);
	if (!fetchOk) {
		checksum = -1;
	}
	return checksum;
}
EXPORT(int) _Clb_Map_getWidth(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMapWidth();
}

EXPORT(int) _Clb_Map_getHeight(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMapHeight();
}

EXPORT(int) _Clb_Map_0ARRAY1SIZE0getHeightMap(int teamId) {
	return gs->mapx * gs->mapy;
}
EXPORT(int) _Clb_Map_0ARRAY1VALS0getHeightMap(int teamId, float heights[],
		int heights_max) {

	IAICallback* clb = team_callback[teamId];
	const float* tmpMap = clb->GetHeightMap();
	int size = _Clb_Map_0ARRAY1SIZE0getHeightMap(teamId);
	size = min(size, heights_max);
	int i;
	for (i=0; i < size; ++i) {
		heights[i] = tmpMap[i];
	}
	return size;
}

EXPORT(float) _Clb_Map_getMinHeight(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMinHeight();
}

EXPORT(float) _Clb_Map_getMaxHeight(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMaxHeight();
}

EXPORT(int) _Clb_Map_0ARRAY1SIZE0getSlopeMap(int teamId) {
	return gs->hmapx * gs->hmapy;
}
EXPORT(int) _Clb_Map_0ARRAY1VALS0getSlopeMap(int teamId, float slopes[],
		int slopes_max) {

	IAICallback* clb = team_callback[teamId];
	const float* tmpMap = clb->GetSlopeMap();
	int size = _Clb_Map_0ARRAY1SIZE0getSlopeMap(teamId);
	size = min(size, slopes_max);
	int i;
	for (i=0; i < size; ++i) {
		slopes[i] = tmpMap[i];
	}
	return size;
}

EXPORT(int) _Clb_Map_0ARRAY1SIZE0getLosMap(int teamId) {
	return loshandler->losSizeX * loshandler->losSizeY;
}
EXPORT(int) _Clb_Map_0ARRAY1VALS0getLosMap(int teamId,
		unsigned short losValues[], int losValues_max) {

	IAICallback* clb = team_callback[teamId];
	const unsigned short* tmpMap = clb->GetLosMap();
	int size = _Clb_Map_0ARRAY1SIZE0getLosMap(teamId);
	size = min(size, losValues_max);
	int i;
	for (i=0; i < size; ++i) {
		losValues[i] = tmpMap[i];
	}
	return size;
}

EXPORT(int) _Clb_Map_0ARRAY1SIZE0getRadarMap(int teamId) {
	return radarhandler->xsize * radarhandler->zsize;
}
EXPORT(int) _Clb_Map_0ARRAY1VALS0getRadarMap(int teamId,
		unsigned short radarValues[], int radarValues_max) {

	IAICallback* clb = team_callback[teamId];
	const unsigned short* tmpMap = clb->GetRadarMap();
	int size = _Clb_Map_0ARRAY1SIZE0getRadarMap(teamId);
	size = min(size, radarValues_max);
	int i;
	for (i=0; i < size; ++i) {
		radarValues[i] = tmpMap[i];
	}
	return size;
}

EXPORT(int) _Clb_Map_0ARRAY1SIZE0getJammerMap(int teamId) {
	// Yes, it is correct, jammer-map has the same size as the radar map
	return radarhandler->xsize * radarhandler->zsize;
}
EXPORT(int) _Clb_Map_0ARRAY1VALS0getJammerMap(int teamId,
		unsigned short jammerValues[], int jammerValues_max) {

	IAICallback* clb = team_callback[teamId];
	const unsigned short* tmpMap = clb->GetJammerMap();
	int size = _Clb_Map_0ARRAY1SIZE0getJammerMap(teamId);
	size = min(size, jammerValues_max);
	int i;
	for (i=0; i < size; ++i) {
		jammerValues[i] = tmpMap[i];
	}
	return size;
}

EXPORT(const unsigned char*) _Clb_Map_0REF1Resource2resourceId0getResourceMap(
		int teamId, int resourceId) {

	IAICallback* clb = team_callback[teamId];
	if (resourceId == rh->GetMetalId()) {
		return clb->GetMetalMap();
	} else {
		return NULL;
	}
}
EXPORT(int) _Clb_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMap(
		int teamId, int resourceId) {

	if (resourceId == rh->GetMetalId()) {
		return readmap->metalMap->GetSizeX() * readmap->metalMap->GetSizeZ();
	} else {
		return 0;
	}
}
EXPORT(int) _Clb_Map_0ARRAY1VALS0REF1Resource2resourceId0getResourceMap(
		int teamId, int resourceId, unsigned char resources[],
		int resources_max) {

	IAICallback* clb = team_callback[teamId];
	int size = _Clb_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMap(
			teamId, resourceId);
	size = min(size, resources_max);

	const unsigned char* tmpMap;
	if (resourceId == rh->GetMetalId()) {
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

EXPORT(const char*) _Clb_Map_getName(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMapName();
}

EXPORT(float) _Clb_Map_getElevationAt(int teamId, float x, float z) {
	IAICallback* clb = team_callback[teamId]; return clb->GetElevation(x, z);
}

EXPORT(float) _Clb_Map_0REF1Resource2resourceId0getMaxResource(int teamId,
		int resourceId) {

	IAICallback* clb = team_callback[teamId];
	if (resourceId == rh->GetMetalId()) {
		return clb->GetMaxMetal();
	} else {
		return NULL;
	}
}

EXPORT(float) _Clb_Map_0REF1Resource2resourceId0getExtractorRadius(int teamId,
		int resourceId) {

	IAICallback* clb = team_callback[teamId];
	if (resourceId == rh->GetMetalId()) {
		return clb->GetExtractorRadius();
	} else {
		return NULL;
	}
}

EXPORT(float) _Clb_Map_getMinWind(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMinWind();
}

EXPORT(float) _Clb_Map_getMaxWind(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMaxWind();
}

EXPORT(float) _Clb_Map_getTidalStrength(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetTidalStrength();
}

EXPORT(float) _Clb_Map_getGravity(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetGravity();
}

EXPORT(bool) _Clb_Map_isPossibleToBuildAt(int teamId, int unitDefId,
		SAIFloat3 pos, int facing) {
	IAICallback* clb = team_callback[teamId];
	const UnitDef* unitDef = getUnitDefById(teamId, unitDefId);
	return clb->CanBuildAt(unitDef, pos, facing);
}

EXPORT(SAIFloat3) _Clb_Map_findClosestBuildSite(int teamId, int unitDefId,
		SAIFloat3 pos, float searchRadius, int minDist, int facing) {
	IAICallback* clb = team_callback[teamId];
	const UnitDef* unitDef = getUnitDefById(teamId, unitDefId);
	return clb->ClosestBuildSite(unitDef, pos, searchRadius, minDist, facing)
			.toSAIFloat3();
}

EXPORT(int) _Clb_Map_0MULTI1SIZE0Point(int teamId) {
	return team_callback[teamId]->GetMapPoints(tmpPointMarkerArr[teamId],
			TMP_ARR_SIZE);
}
EXPORT(struct SAIFloat3) _Clb_Map_Point_getPosition(int teamId, int pointId) {
	return tmpPointMarkerArr[teamId][pointId].pos.toSAIFloat3();
}
EXPORT(struct SAIFloat3) _Clb_Map_Point_getColor(int teamId, int pointId) {

	unsigned char* color = tmpPointMarkerArr[teamId][pointId].color;
	SAIFloat3 f3color = {color[0], color[1], color[2]};
	return f3color;
}
EXPORT(const char*) _Clb_Map_Point_getLabel(int teamId, int pointId) {
	return tmpPointMarkerArr[teamId][pointId].label;
}

EXPORT(int) _Clb_Map_0MULTI1SIZE0Line(int teamId) {
	return team_callback[teamId]->GetMapLines(tmpLineMarkerArr[teamId],
			TMP_ARR_SIZE);
}
EXPORT(struct SAIFloat3) _Clb_Map_Line_getFirstPosition(int teamId, int lineId) {
	return tmpLineMarkerArr[teamId][lineId].pos.toSAIFloat3();
}
EXPORT(struct SAIFloat3) _Clb_Map_Line_getSecondPosition(int teamId, int lineId) {
	return tmpLineMarkerArr[teamId][lineId].pos2.toSAIFloat3();
}
EXPORT(struct SAIFloat3) _Clb_Map_Line_getColor(int teamId, int lineId) {

	unsigned char* color = tmpLineMarkerArr[teamId][lineId].color;
	SAIFloat3 f3color = {color[0], color[1], color[2]};
	return f3color;
}
EXPORT(SAIFloat3) _Clb_Map_getStartPos(int teamId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetStartPos()->toSAIFloat3();
}
EXPORT(SAIFloat3) _Clb_Map_getMousePos(int teamId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetMousePos().toSAIFloat3();
}
//########### END Map


// DEPRECATED
/*
EXPORT(bool) _Clb_getProperty(int teamId, int id, int property, void* dst) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetProperty(id, property, dst);
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetProperty(id, property, dst);
	} else {
		return team_callback[teamId]->GetProperty(id, property, dst);
	}
}

EXPORT(bool) _Clb_getValue(int teamId, int id, void* dst) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetValue(id, dst);
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetValue(id, dst);
	} else {
		return team_callback[teamId]->GetValue(id, dst);
	}
}
*/

EXPORT(int) _Clb_File_getSize(int teamId, const char* fileName) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFileSize(fileName);
}

EXPORT(bool) _Clb_File_getContent(int teamId, const char* fileName, void* buffer,
		int bufferLen) {
	IAICallback* clb = team_callback[teamId]; return clb->ReadFile(fileName,
			buffer, bufferLen);
}





// BEGINN OBJECT Resource
EXPORT(int) _Clb_0MULTI1SIZE0Resource(int teamId) {
	return rh->GetNumResources();
}
EXPORT(int) _Clb_0MULTI1FETCH3ResourceByName0Resource(int teamId,
		const char* resourceName) {
	return rh->GetResourceId(resourceName);
}
EXPORT(const char*) _Clb_Resource_getName(int teamId, int resourceId) {
	return rh->GetResource(resourceId)->name.c_str();
}
EXPORT(float) _Clb_Resource_getOptimum(int teamId, int resourceId) {
	return rh->GetResource(resourceId)->optimum;
}
EXPORT(float) _Clb_Economy_0REF1Resource2resourceId0getCurrent(int teamId,
		int resourceId) {

	IAICallback* clb = team_callback[teamId];
	if (resourceId == rh->GetMetalId()) {
		return clb->GetMetal();
	} else if (resourceId == rh->GetEnergyId()) {
		return clb->GetEnergy();
	} else {
		return -1.0f;
	}
}

EXPORT(float) _Clb_Economy_0REF1Resource2resourceId0getIncome(int teamId,
		int resourceId) {

	IAICallback* clb = team_callback[teamId];
	if (resourceId == rh->GetMetalId()) {
		return clb->GetMetalIncome();
	} else if (resourceId == rh->GetEnergyId()) {
		return clb->GetEnergyIncome();
	} else {
		return -1.0f;
	}
}

EXPORT(float) _Clb_Economy_0REF1Resource2resourceId0getUsage(int teamId,
		int resourceId) {

	IAICallback* clb = team_callback[teamId];
	if (resourceId == rh->GetMetalId()) {
		return clb->GetMetalUsage();
	} else if (resourceId == rh->GetEnergyId()) {
		return clb->GetEnergyUsage();
	} else {
		return -1.0f;
	}
}

EXPORT(float) _Clb_Economy_0REF1Resource2resourceId0getStorage(int teamId,
		int resourceId) {

	IAICallback* clb = team_callback[teamId];
	if (resourceId == rh->GetMetalId()) {
		return clb->GetMetalStorage();
	} else if (resourceId == rh->GetEnergyId()) {
		return clb->GetEnergyStorage();
	} else {
		return -1.0f;
	}
}

EXPORT(const char*) _Clb_Game_getSetupScript(int teamId) {
	const char* setupScript;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_SCRIPT, &setupScript);
	if (!fetchOk) {
		return NULL;
	}
	return setupScript;
}







//########### BEGINN UnitDef
EXPORT(int) _Clb_0MULTI1SIZE0UnitDef(int teamId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetNumUnitDefs();
}
EXPORT(int) _Clb_0MULTI1VALS0UnitDef(int teamId, int unitDefIds[],
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

	return size;
}
EXPORT(int) _Clb_0MULTI1FETCH3UnitDefByName0UnitDef(int teamId,
		const char* unitName) {

	int unitDefId = -1;

	IAICallback* clb = team_callback[teamId];
	const UnitDef* ud = clb->GetUnitDef(unitName);
	if (ud != NULL) {
		unitDefId = ud->id;
	}

	return unitDefId;
}

EXPORT(float) _Clb_UnitDef_getHeight(int teamId, int unitDefId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetUnitDefHeight(unitDefId);
}
EXPORT(float) _Clb_UnitDef_getRadius(int teamId, int unitDefId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetUnitDefRadius(unitDefId);
}

EXPORT(bool) _Clb_UnitDef_isValid(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->valid;
}
EXPORT(const char*) _Clb_UnitDef_getName(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->name.c_str();
}
EXPORT(const char*) _Clb_UnitDef_getHumanName(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->humanName.c_str();
}
//EXPORT(const char*) _Clb_UnitDef_getFileName(int teamId, int unitDefId) {
//	return getUnitDefById(teamId, unitDefId)->filename.c_str();
//}
//EXPORT(int) _Clb_UnitDef_getId(int teamId, int unitDefId) {
//	return getUnitDefById(teamId, unitDefId)->id;
//}
EXPORT(int) _Clb_UnitDef_getAiHint(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->aihint;
}
EXPORT(int) _Clb_UnitDef_getCobId(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->cobID;
}
EXPORT(int) _Clb_UnitDef_getTechLevel(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->techLevel;
}
EXPORT(const char*) _Clb_UnitDef_getGaia(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->gaia.c_str();
}
EXPORT(float) _Clb_UnitDef_0REF1Resource2resourceId0getUpkeep(int teamId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == rh->GetMetalId()) {
		return ud->metalUpkeep;
	} else if (resourceId == rh->GetEnergyId()) {
		return ud->energyUpkeep;
	} else {
		return 0.0f;
	}
}
EXPORT(float) _Clb_UnitDef_0REF1Resource2resourceId0getResourceMake(int teamId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == rh->GetMetalId()) {
		return ud->metalMake;
	} else if (resourceId == rh->GetEnergyId()) {
		return ud->energyMake;
	} else {
		return 0.0f;
	}
}
EXPORT(float) _Clb_UnitDef_0REF1Resource2resourceId0getMakesResource(int teamId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == rh->GetMetalId()) {
		return ud->makesMetal;
	} else {
		return 0.0f;
	}
}
EXPORT(float) _Clb_UnitDef_0REF1Resource2resourceId0getCost(int teamId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == rh->GetMetalId()) {
		return ud->metalCost;
	} else if (resourceId == rh->GetEnergyId()) {
		return ud->energyCost;
	} else {
		return 0.0f;
	}
}
EXPORT(float) _Clb_UnitDef_0REF1Resource2resourceId0getExtractsResource(
		int teamId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == rh->GetMetalId()) {
		return ud->extractsMetal;
	} else {
		return 0.0f;
	}
}
EXPORT(float) _Clb_UnitDef_0REF1Resource2resourceId0getResourceExtractorRange(
		int teamId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == rh->GetMetalId()) {
		return ud->extractRange;
	} else {
		return 0.0f;
	}
}
EXPORT(float) _Clb_UnitDef_0REF1Resource2resourceId0getWindResourceGenerator(
		int teamId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == rh->GetEnergyId()) {
		return ud->windGenerator;
	} else {
		return 0.0f;
	}
}
EXPORT(float) _Clb_UnitDef_0REF1Resource2resourceId0getTidalResourceGenerator(
		int teamId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == rh->GetEnergyId()) {
		return ud->tidalGenerator;
	} else {
		return 0.0f;
	}
}
EXPORT(float) _Clb_UnitDef_0REF1Resource2resourceId0getStorage(int teamId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == rh->GetMetalId()) {
		return ud->metalStorage;
	} else if (resourceId == rh->GetEnergyId()) {
		return ud->energyStorage;
	} else {
		return 0.0f;
	}
}
EXPORT(bool) _Clb_UnitDef_0REF1Resource2resourceId0isSquareResourceExtractor(
		int teamId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == rh->GetMetalId()) {
		return ud->extractSquare;
	} else {
		return false;
	}
}
EXPORT(float) _Clb_UnitDef_getBuildTime(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildTime;}
EXPORT(float) _Clb_UnitDef_getAutoHeal(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->autoHeal;}
EXPORT(float) _Clb_UnitDef_getIdleAutoHeal(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->idleAutoHeal;}
EXPORT(int) _Clb_UnitDef_getIdleTime(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->idleTime;}
EXPORT(float) _Clb_UnitDef_getPower(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->power;}
EXPORT(float) _Clb_UnitDef_getHealth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->health;}
EXPORT(unsigned int) _Clb_UnitDef_getCategory(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->category;}
EXPORT(float) _Clb_UnitDef_getSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->speed;}
EXPORT(float) _Clb_UnitDef_getTurnRate(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->turnRate;}
EXPORT(bool) _Clb_UnitDef_isTurnInPlace(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->turnInPlace;}
EXPORT(int) _Clb_UnitDef_getMoveType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->moveType;}
EXPORT(bool) _Clb_UnitDef_isUpright(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->upright;}
EXPORT(bool) _Clb_UnitDef_isCollide(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->collide;}
EXPORT(float) _Clb_UnitDef_getControlRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->controlRadius;}
EXPORT(float) _Clb_UnitDef_getLosRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->losRadius;}
EXPORT(float) _Clb_UnitDef_getAirLosRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->airLosRadius;}
EXPORT(float) _Clb_UnitDef_getLosHeight(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->losHeight;}
EXPORT(int) _Clb_UnitDef_getRadarRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->radarRadius;}
EXPORT(int) _Clb_UnitDef_getSonarRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->sonarRadius;}
EXPORT(int) _Clb_UnitDef_getJammerRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->jammerRadius;}
EXPORT(int) _Clb_UnitDef_getSonarJamRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->sonarJamRadius;}
EXPORT(int) _Clb_UnitDef_getSeismicRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->seismicRadius;}
EXPORT(float) _Clb_UnitDef_getSeismicSignature(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->seismicSignature;}
EXPORT(bool) _Clb_UnitDef_isStealth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->stealth;}
EXPORT(bool) _Clb_UnitDef_isSonarStealth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->sonarStealth;}
EXPORT(bool) _Clb_UnitDef_isBuildRange3D(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildRange3D;}
EXPORT(float) _Clb_UnitDef_getBuildDistance(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildDistance;}
EXPORT(float) _Clb_UnitDef_getBuildSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildSpeed;}
EXPORT(float) _Clb_UnitDef_getReclaimSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->reclaimSpeed;}
EXPORT(float) _Clb_UnitDef_getRepairSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->repairSpeed;}
EXPORT(float) _Clb_UnitDef_getMaxRepairSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxRepairSpeed;}
EXPORT(float) _Clb_UnitDef_getResurrectSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->resurrectSpeed;}
EXPORT(float) _Clb_UnitDef_getCaptureSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->captureSpeed;}
EXPORT(float) _Clb_UnitDef_getTerraformSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->terraformSpeed;}
EXPORT(float) _Clb_UnitDef_getMass(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->mass;}
EXPORT(bool) _Clb_UnitDef_isPushResistant(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->pushResistant;}
EXPORT(bool) _Clb_UnitDef_isStrafeToAttack(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->strafeToAttack;}
EXPORT(float) _Clb_UnitDef_getMinCollisionSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->minCollisionSpeed;}
EXPORT(float) _Clb_UnitDef_getSlideTolerance(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->slideTolerance;}
EXPORT(float) _Clb_UnitDef_getMaxSlope(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxSlope;}
EXPORT(float) _Clb_UnitDef_getMaxHeightDif(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxHeightDif;}
EXPORT(float) _Clb_UnitDef_getMinWaterDepth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->minWaterDepth;}
EXPORT(float) _Clb_UnitDef_getWaterline(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->waterline;}
EXPORT(float) _Clb_UnitDef_getMaxWaterDepth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxWaterDepth;}
EXPORT(float) _Clb_UnitDef_getArmoredMultiple(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->armoredMultiple;}
EXPORT(int) _Clb_UnitDef_getArmorType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->armorType;}
EXPORT(int) _Clb_UnitDef_FlankingBonus_getMode(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flankingBonusMode;}
EXPORT(SAIFloat3) _Clb_UnitDef_FlankingBonus_getDir(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flankingBonusDir.toSAIFloat3();}
EXPORT(float) _Clb_UnitDef_FlankingBonus_getMax(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flankingBonusMax;}
EXPORT(float) _Clb_UnitDef_FlankingBonus_getMin(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flankingBonusMin;}
EXPORT(float) _Clb_UnitDef_FlankingBonus_getMobilityAdd(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flankingBonusMobilityAdd;}
EXPORT(const char*) _Clb_UnitDef_CollisionVolume_getType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->collisionVolumeType.c_str();}
EXPORT(SAIFloat3) _Clb_UnitDef_CollisionVolume_getScales(int teamId,
		int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->collisionVolumeScales
			.toSAIFloat3();
}
EXPORT(SAIFloat3) _Clb_UnitDef_CollisionVolume_getOffsets(int teamId,
		int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->collisionVolumeOffsets
			.toSAIFloat3();
}
EXPORT(int) _Clb_UnitDef_CollisionVolume_getTest(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->collisionVolumeTest;}
EXPORT(float) _Clb_UnitDef_getMaxWeaponRange(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxWeaponRange;}
EXPORT(const char*) _Clb_UnitDef_getType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->type.c_str();}
EXPORT(const char*) _Clb_UnitDef_getTooltip(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->tooltip.c_str();}
EXPORT(const char*) _Clb_UnitDef_getWreckName(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->wreckName.c_str();}
EXPORT(const char*) _Clb_UnitDef_getDeathExplosion(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->deathExplosion.c_str();}
EXPORT(const char*) _Clb_UnitDef_getSelfDExplosion(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->selfDExplosion.c_str();}
EXPORT(const char*) _Clb_UnitDef_getTedClassString(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->TEDClassString.c_str();}
EXPORT(const char*) _Clb_UnitDef_getCategoryString(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->categoryString.c_str();}
EXPORT(bool) _Clb_UnitDef_isAbleToSelfD(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canSelfD;}
EXPORT(int) _Clb_UnitDef_getSelfDCountdown(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->selfDCountdown;}
EXPORT(bool) _Clb_UnitDef_isAbleToSubmerge(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canSubmerge;}
EXPORT(bool) _Clb_UnitDef_isAbleToFly(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canfly;}
EXPORT(bool) _Clb_UnitDef_isAbleToMove(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canmove;}
EXPORT(bool) _Clb_UnitDef_isAbleToHover(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canhover;}
EXPORT(bool) _Clb_UnitDef_isFloater(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->floater;}
EXPORT(bool) _Clb_UnitDef_isBuilder(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->builder;}
EXPORT(bool) _Clb_UnitDef_isActivateWhenBuilt(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->activateWhenBuilt;}
EXPORT(bool) _Clb_UnitDef_isOnOffable(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->onoffable;}
EXPORT(bool) _Clb_UnitDef_isFullHealthFactory(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->fullHealthFactory;}
EXPORT(bool) _Clb_UnitDef_isFactoryHeadingTakeoff(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->factoryHeadingTakeoff;}
EXPORT(bool) _Clb_UnitDef_isReclaimable(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->reclaimable;}
EXPORT(bool) _Clb_UnitDef_isCapturable(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->capturable;}
EXPORT(bool) _Clb_UnitDef_isAbleToRestore(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canRestore;}
EXPORT(bool) _Clb_UnitDef_isAbleToRepair(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canRepair;}
EXPORT(bool) _Clb_UnitDef_isAbleToSelfRepair(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canSelfRepair;}
EXPORT(bool) _Clb_UnitDef_isAbleToReclaim(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canReclaim;}
EXPORT(bool) _Clb_UnitDef_isAbleToAttack(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canAttack;}
EXPORT(bool) _Clb_UnitDef_isAbleToPatrol(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canPatrol;}
EXPORT(bool) _Clb_UnitDef_isAbleToFight(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canFight;}
EXPORT(bool) _Clb_UnitDef_isAbleToGuard(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canGuard;}
EXPORT(bool) _Clb_UnitDef_isAbleToBuild(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canBuild;}
EXPORT(bool) _Clb_UnitDef_isAbleToAssist(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canAssist;}
EXPORT(bool) _Clb_UnitDef_isAssistable(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canBeAssisted;}
EXPORT(bool) _Clb_UnitDef_isAbleToRepeat(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canRepeat;}
EXPORT(bool) _Clb_UnitDef_isAbleToFireControl(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canFireControl;}
EXPORT(int) _Clb_UnitDef_getFireState(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->fireState;}
EXPORT(int) _Clb_UnitDef_getMoveState(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->moveState;}
EXPORT(float) _Clb_UnitDef_getWingDrag(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->wingDrag;}
EXPORT(float) _Clb_UnitDef_getWingAngle(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->wingAngle;}
EXPORT(float) _Clb_UnitDef_getDrag(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->drag;}
EXPORT(float) _Clb_UnitDef_getFrontToSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->frontToSpeed;}
EXPORT(float) _Clb_UnitDef_getSpeedToFront(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->speedToFront;}
EXPORT(float) _Clb_UnitDef_getMyGravity(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->myGravity;}
EXPORT(float) _Clb_UnitDef_getMaxBank(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxBank;}
EXPORT(float) _Clb_UnitDef_getMaxPitch(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxPitch;}
EXPORT(float) _Clb_UnitDef_getTurnRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->turnRadius;}
EXPORT(float) _Clb_UnitDef_getWantedHeight(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->wantedHeight;}
EXPORT(float) _Clb_UnitDef_getVerticalSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->verticalSpeed;}
EXPORT(bool) _Clb_UnitDef_isAbleToCrash(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canCrash;}
EXPORT(bool) _Clb_UnitDef_isHoverAttack(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->hoverAttack;}
EXPORT(bool) _Clb_UnitDef_isAirStrafe(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->airStrafe;}
EXPORT(float) _Clb_UnitDef_getDlHoverFactor(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->dlHoverFactor;}
EXPORT(float) _Clb_UnitDef_getMaxAcceleration(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxAcc;}
EXPORT(float) _Clb_UnitDef_getMaxDeceleration(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxDec;}
EXPORT(float) _Clb_UnitDef_getMaxAileron(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxAileron;}
EXPORT(float) _Clb_UnitDef_getMaxElevator(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxElevator;}
EXPORT(float) _Clb_UnitDef_getMaxRudder(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxRudder;}
//const unsigned char** _UnitDef_getYardMaps(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->yardmaps;}
EXPORT(int) _Clb_UnitDef_getXSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->xsize;}
EXPORT(int) _Clb_UnitDef_getZSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->zsize;}
EXPORT(int) _Clb_UnitDef_getBuildAngle(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildangle;}
EXPORT(float) _Clb_UnitDef_getLoadingRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->loadingRadius;}
EXPORT(float) _Clb_UnitDef_getUnloadSpread(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->unloadSpread;}
EXPORT(int) _Clb_UnitDef_getTransportCapacity(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->transportCapacity;}
EXPORT(int) _Clb_UnitDef_getTransportSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->transportSize;}
EXPORT(int) _Clb_UnitDef_getMinTransportSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->minTransportSize;}
EXPORT(bool) _Clb_UnitDef_isAirBase(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->isAirBase;}
EXPORT(float) _Clb_UnitDef_getTransportMass(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->transportMass;}
EXPORT(float) _Clb_UnitDef_getMinTransportMass(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->minTransportMass;}
EXPORT(bool) _Clb_UnitDef_isHoldSteady(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->holdSteady;}
EXPORT(bool) _Clb_UnitDef_isReleaseHeld(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->releaseHeld;}
EXPORT(bool) _Clb_UnitDef_isNotTransportable(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->cantBeTransported;}
EXPORT(bool) _Clb_UnitDef_isTransportByEnemy(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->transportByEnemy;}
EXPORT(int) _Clb_UnitDef_getTransportUnloadMethod(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->transportUnloadMethod;}
EXPORT(float) _Clb_UnitDef_getFallSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->fallSpeed;}
EXPORT(float) _Clb_UnitDef_getUnitFallSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->unitFallSpeed;}
EXPORT(bool) _Clb_UnitDef_isAbleToCloak(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canCloak;}
EXPORT(bool) _Clb_UnitDef_isStartCloaked(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->startCloaked;}
EXPORT(float) _Clb_UnitDef_getCloakCost(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->cloakCost;}
EXPORT(float) _Clb_UnitDef_getCloakCostMoving(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->cloakCostMoving;}
EXPORT(float) _Clb_UnitDef_getDecloakDistance(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->decloakDistance;}
EXPORT(bool) _Clb_UnitDef_isDecloakSpherical(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->decloakSpherical;}
EXPORT(bool) _Clb_UnitDef_isDecloakOnFire(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->decloakOnFire;}
EXPORT(bool) _Clb_UnitDef_isAbleToKamikaze(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canKamikaze;}
EXPORT(float) _Clb_UnitDef_getKamikazeDist(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->kamikazeDist;}
EXPORT(bool) _Clb_UnitDef_isTargetingFacility(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->targfac;}
EXPORT(bool) _Clb_UnitDef_isAbleToDGun(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canDGun;}
EXPORT(bool) _Clb_UnitDef_isNeedGeo(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->needGeo;}
EXPORT(bool) _Clb_UnitDef_isFeature(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->isFeature;}
EXPORT(bool) _Clb_UnitDef_isHideDamage(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->hideDamage;}
EXPORT(bool) _Clb_UnitDef_isCommander(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->isCommander;}
EXPORT(bool) _Clb_UnitDef_isShowPlayerName(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->showPlayerName;}
EXPORT(bool) _Clb_UnitDef_isAbleToResurrect(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canResurrect;}
EXPORT(bool) _Clb_UnitDef_isAbleToCapture(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canCapture;}
EXPORT(int) _Clb_UnitDef_getHighTrajectoryType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->highTrajectoryType;}
EXPORT(unsigned int) _Clb_UnitDef_getNoChaseCategory(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->noChaseCategory;}
EXPORT(bool) _Clb_UnitDef_isLeaveTracks(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->leaveTracks;}
EXPORT(float) _Clb_UnitDef_getTrackWidth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->trackWidth;}
EXPORT(float) _Clb_UnitDef_getTrackOffset(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->trackOffset;}
EXPORT(float) _Clb_UnitDef_getTrackStrength(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->trackStrength;}
EXPORT(float) _Clb_UnitDef_getTrackStretch(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->trackStretch;}
EXPORT(int) _Clb_UnitDef_getTrackType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->trackType;}
EXPORT(bool) _Clb_UnitDef_isAbleToDropFlare(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canDropFlare;}
EXPORT(float) _Clb_UnitDef_getFlareReloadTime(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareReloadTime;}
EXPORT(float) _Clb_UnitDef_getFlareEfficiency(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareEfficiency;}
EXPORT(float) _Clb_UnitDef_getFlareDelay(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareDelay;}
EXPORT(SAIFloat3) _Clb_UnitDef_getFlareDropVector(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareDropVector.toSAIFloat3();}
EXPORT(int) _Clb_UnitDef_getFlareTime(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareTime;}
EXPORT(int) _Clb_UnitDef_getFlareSalvoSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareSalvoSize;}
EXPORT(int) _Clb_UnitDef_getFlareSalvoDelay(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareSalvoDelay;}
//EXPORT(bool) _Clb_UnitDef_isSmoothAnim(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->smoothAnim;}
EXPORT(bool) _Clb_UnitDef_0REF1Resource2resourceId0isResourceMaker(int teamId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(teamId, unitDefId);
	if (resourceId == rh->GetMetalId()) {
		return ud->isMetalMaker;
	} else {
		return false;
	}
}
EXPORT(bool) _Clb_UnitDef_isAbleToLoopbackAttack(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canLoopbackAttack;}
EXPORT(bool) _Clb_UnitDef_isLevelGround(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->levelGround;}
EXPORT(bool) _Clb_UnitDef_isUseBuildingGroundDecal(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->useBuildingGroundDecal;}
EXPORT(int) _Clb_UnitDef_getBuildingDecalType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildingDecalType;}
EXPORT(int) _Clb_UnitDef_getBuildingDecalSizeX(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildingDecalSizeX;}
EXPORT(int) _Clb_UnitDef_getBuildingDecalSizeY(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildingDecalSizeY;}
EXPORT(float) _Clb_UnitDef_getBuildingDecalDecaySpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildingDecalDecaySpeed;}
EXPORT(bool) _Clb_UnitDef_isFirePlatform(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->isFirePlatform;}
EXPORT(float) _Clb_UnitDef_getMaxFuel(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxFuel;}
EXPORT(float) _Clb_UnitDef_getRefuelTime(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->refuelTime;}
EXPORT(float) _Clb_UnitDef_getMinAirBasePower(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->minAirBasePower;}
EXPORT(int) _Clb_UnitDef_getMaxThisUnit(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxThisUnit;}
EXPORT(int) _Clb_UnitDef_0SINGLE1FETCH2UnitDef0getDecoyDef(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->decoyDef->id;}
EXPORT(bool) _Clb_UnitDef_isDontLand(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->DontLand();}
EXPORT(int) _Clb_UnitDef_0SINGLE1FETCH2WeaponDef0getShieldDef(int teamId,
		int unitDefId) {
	const WeaponDef* wd = getUnitDefById(teamId, unitDefId)->shieldWeaponDef;
	if (wd == NULL)
		return -1;
	else
		return wd->id;
}
EXPORT(int) _Clb_UnitDef_0SINGLE1FETCH2WeaponDef0getStockpileDef(int teamId,
		int unitDefId) {
	const WeaponDef* wd = getUnitDefById(teamId, unitDefId)->stockpileWeaponDef;
	if (wd == NULL)
		return -1;
	else
		return wd->id;
}
EXPORT(int) _Clb_UnitDef_0ARRAY1SIZE1UnitDef0getBuildOptions(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildOptions.size();}
EXPORT(int) _Clb_UnitDef_0ARRAY1VALS1UnitDef0getBuildOptions(int teamId,
		int unitDefId, int* unitDefIds, int unitDefIds_max) {
	const std::map<int,std::string>& bo = getUnitDefById(teamId, unitDefId)->buildOptions;
	std::map<int,std::string>::const_iterator bb;
	int b;
	for (b=0, bb = bo.begin(); bb != bo.end() && b < unitDefIds_max; ++b, ++bb) {
		unitDefIds[b] = _Clb_0MULTI1FETCH3UnitDefByName0UnitDef(teamId, bb->second.c_str());
	}
	return b;
}
EXPORT(int) _Clb_UnitDef_0MAP1SIZE0getCustomParams(int teamId, int unitDefId) {

	tmpSize[teamId] =
			fillCMap(&(getUnitDefById(teamId, unitDefId)->customParams),
					tmpKeysArr[teamId], tmpValuesArr[teamId]);
	return tmpSize[teamId];
}
EXPORT(void) _Clb_UnitDef_0MAP1KEYS0getCustomParams(int teamId, int unitDefId,
		const char* keys[]) {
	copyStringArr(keys, tmpKeysArr[teamId], tmpSize[teamId]);
}
EXPORT(void) _Clb_UnitDef_0MAP1VALS0getCustomParams(int teamId, int unitDefId,
		const char* values[]) {
	copyStringArr(values, tmpValuesArr[teamId], tmpSize[teamId]);
}
EXPORT(bool) _Clb_UnitDef_0AVAILABLE0MoveData(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata != NULL;}
EXPORT(int) _Clb_UnitDef_MoveData_getMoveType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->moveType;}
EXPORT(int) _Clb_UnitDef_MoveData_getMoveFamily(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->moveFamily;}
EXPORT(int) _Clb_UnitDef_MoveData_getSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->size;}
EXPORT(float) _Clb_UnitDef_MoveData_getDepth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->depth;}
EXPORT(float) _Clb_UnitDef_MoveData_getMaxSlope(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->maxSlope;}
EXPORT(float) _Clb_UnitDef_MoveData_getSlopeMod(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->slopeMod;}
EXPORT(float) _Clb_UnitDef_MoveData_getDepthMod(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->depthMod;}
EXPORT(int) _Clb_UnitDef_MoveData_getPathType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->pathType;}
EXPORT(float) _Clb_UnitDef_MoveData_getCrushStrength(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->crushStrength;}
EXPORT(float) _Clb_UnitDef_MoveData_getMaxSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->maxSpeed;}
EXPORT(short) _Clb_UnitDef_MoveData_getMaxTurnRate(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->maxTurnRate;}
EXPORT(float) _Clb_UnitDef_MoveData_getMaxAcceleration(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->maxAcceleration;}
EXPORT(float) _Clb_UnitDef_MoveData_getMaxBreaking(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->maxBreaking;}
EXPORT(bool) _Clb_UnitDef_MoveData_isSubMarine(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->subMarine;}
EXPORT(int) _Clb_UnitDef_0MULTI1SIZE0WeaponMount(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->weapons.size();}
EXPORT(const char*) _Clb_UnitDef_WeaponMount_getName(int teamId, int unitDefId, int weaponMountId) {
	return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).name.c_str();
}
EXPORT(int) _Clb_UnitDef_WeaponMount_0SINGLE1FETCH2WeaponDef0getWeaponDef(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).def->id;}
EXPORT(int) _Clb_UnitDef_WeaponMount_getSlavedTo(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).slavedTo;}
EXPORT(SAIFloat3) _Clb_UnitDef_WeaponMount_getMainDir(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).mainDir.toSAIFloat3();}
EXPORT(float) _Clb_UnitDef_WeaponMount_getMaxAngleDif(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).maxAngleDif;}
EXPORT(float) _Clb_UnitDef_WeaponMount_getFuelUsage(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).fuelUsage;}
EXPORT(unsigned int) _Clb_UnitDef_WeaponMount_getBadTargetCategory(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).badTargetCat;}
EXPORT(unsigned int) _Clb_UnitDef_WeaponMount_getOnlyTargetCategory(int teamId, int unitDefId, int weaponMountId) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponMountId).onlyTargetCat;}
//########### END UnitDef





//########### BEGINN Unit
EXPORT(int) _Clb_Unit_0STATIC0getLimit(int teamId) {
	int unitLimit;
	IAICallback* clb = team_callback[teamId];
	bool fetchOk = clb->GetValue(AIVAL_UNIT_LIMIT, &unitLimit);
	if (!fetchOk) {
		unitLimit = -1;
	}
	return unitLimit;
}
EXPORT(int) _Clb_Unit_0SINGLE1FETCH2UnitDef0getDef(int teamId, int unitId) {
	const UnitDef* unitDef;
	if (_Clb_Cheats_isEnabled(teamId)) {
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


EXPORT(int) _Clb_Unit_0MULTI1SIZE0ModParam(int teamId, int unitId) {

	const CUnit* unit = getUnit(unitId);
	if (unit && /*(_Clb_Cheats_isEnabled(teamId) || */isAlliedUnit(teamId, unit)/*)*/) {
		return unit->modParams.size();
	} else {
		return 0;
	}
}
EXPORT(const char*) _Clb_Unit_ModParam_getName(int teamId, int unitId,
		int modParamId)
{
	const CUnit* unit = getUnit(unitId);
	if (unit && /*(_Clb_Cheats_isEnabled(teamId) || */isAlliedUnit(teamId, unit)/*)*/
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
EXPORT(float) _Clb_Unit_ModParam_getValue(int teamId, int unitId,
		int modParamId)
{
	const CUnit* unit = getUnit(unitId);
	if (unit && /*(_Clb_Cheats_isEnabled(teamId) || */isAlliedUnit(teamId, unit)/*)*/
			&& modParamId >= 0
			&& (unsigned int)modParamId < unit->modParams.size()) {
		return unit->modParams[modParamId];
	} else {
		return 0.0f;
	}
}

EXPORT(int) _Clb_Unit_getTeam(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetUnitTeam(unitId);
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitTeam(unitId);
	} else {
		return team_callback[teamId]->GetUnitTeam(unitId);
	}
}
EXPORT(int) _Clb_Unit_getAllyTeam(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetUnitAllyTeam(unitId);
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitAllyTeam(unitId);
	} else {
		return team_callback[teamId]->GetUnitAllyTeam(unitId);
	}
}
EXPORT(int) _Clb_Unit_getLineage(int teamId, int unitId) {

	const CUnit* unit = getUnit(unitId);
	if (unit == NULL) {
		return -1;
	}
	return unit->lineage;
}
EXPORT(int) _Clb_Unit_getAiHint(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetUnitAiHint(unitId);
}

EXPORT(int) _Clb_Unit_0MULTI1SIZE0SupportedCommand(int teamId, int unitId) {
	return team_callback[teamId]->GetUnitCommands(unitId)->size();
}
EXPORT(int) _Clb_Unit_SupportedCommand_getId(int teamId, int unitId, int commandId) {
	return team_callback[teamId]->GetUnitCommands(unitId)->at(commandId).id;
}
EXPORT(const char*) _Clb_Unit_SupportedCommand_getName(int teamId, int unitId, int commandId) {
	return team_callback[teamId]->GetUnitCommands(unitId)->at(commandId).name.c_str();
}
EXPORT(const char*) _Clb_Unit_SupportedCommand_getToolTip(int teamId, int unitId, int commandId) {
	return team_callback[teamId]->GetUnitCommands(unitId)->at(commandId).tooltip.c_str();
}
EXPORT(bool) _Clb_Unit_SupportedCommand_isShowUnique(int teamId, int unitId, int commandId) {
	return team_callback[teamId]->GetUnitCommands(unitId)->at(commandId).showUnique;
}
EXPORT(bool) _Clb_Unit_SupportedCommand_isDisabled(int teamId, int unitId, int commandId) {
	return team_callback[teamId]->GetUnitCommands(unitId)->at(commandId).disabled;
}
EXPORT(int) _Clb_Unit_SupportedCommand_0ARRAY1SIZE0getParams(int teamId, int unitId, int commandId) {
	return team_callback[teamId]->GetUnitCommands(unitId)->at(commandId).params.size();
}
EXPORT(int) _Clb_Unit_SupportedCommand_0ARRAY1VALS0getParams(int teamId,
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

EXPORT(int) _Clb_Unit_getStockpile(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId];
	int stockpile;
	bool fetchOk = clb->GetProperty(AIVAL_STOCKPILED, unitId, &stockpile);
	if (!fetchOk) {
		stockpile = -1;
	}
	return stockpile;
}
EXPORT(int) _Clb_Unit_getStockpileQueued(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId];
	int stockpileQueue;
	bool fetchOk = clb->GetProperty(AIVAL_STOCKPILE_QUED, unitId,
			&stockpileQueue);
	if (!fetchOk) {
		stockpileQueue = -1;
	}
	return stockpileQueue;
}
EXPORT(float) _Clb_Unit_getCurrentFuel(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId];
	float currentFuel;
	bool fetchOk = clb->GetProperty(AIVAL_CURRENT_FUEL, unitId, &currentFuel);
	if (!fetchOk) {
		currentFuel = -1.0f;
	}
	return currentFuel;
}
EXPORT(float) _Clb_Unit_getMaxSpeed(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId];
	float maxSpeed;
	bool fetchOk = clb->GetProperty(AIVAL_UNIT_MAXSPEED, unitId, &maxSpeed);
	if (!fetchOk) {
		maxSpeed = -1.0f;
	}
	return maxSpeed;
}

EXPORT(float) _Clb_Unit_getMaxRange(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetUnitMaxRange(unitId);
}

EXPORT(float) _Clb_Unit_getMaxHealth(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetUnitMaxHealth(unitId);
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitMaxHealth(unitId);
	} else {
		return team_callback[teamId]->GetUnitMaxHealth(unitId);
	}
}

EXPORT(int) _Clb_Unit_0MULTI1SIZE1Command0CurrentCommand(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId];
//	return clb->GetCurrentUnitCommands(unitId)->size();
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId)->size();
	} else {
		return team_callback[teamId]->GetCurrentUnitCommands(unitId)->size();
	}
}
EXPORT(int) _Clb_Unit_CurrentCommand_0STATIC0getType(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId];
//	CCommandQueue::QueueType qt = clb->GetCurrentUnitCommands(unitId)->GetType();
//	return qt;
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId)->GetType();
	} else {
		return team_callback[teamId]->GetCurrentUnitCommands(unitId)->GetType();
	}
}
EXPORT(int) _Clb_Unit_CurrentCommand_getId(int teamId, int unitId, int commandId) {

	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId)->at(commandId).id;
	} else {
		return team_callback[teamId]->GetCurrentUnitCommands(unitId)->at(commandId).id;
	}
}
EXPORT(unsigned char) _Clb_Unit_CurrentCommand_getOptions(int teamId, int unitId, int commandId) {

	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId)->at(commandId).options;
	} else {
		return team_callback[teamId]->GetCurrentUnitCommands(unitId)->at(commandId).options;
	}
}
EXPORT(unsigned int) _Clb_Unit_CurrentCommand_getTag(int teamId, int unitId, int commandId) {

	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId)->at(commandId).tag;
	} else {
		return team_callback[teamId]->GetCurrentUnitCommands(unitId)->at(commandId).tag;
	}
}
EXPORT(int) _Clb_Unit_CurrentCommand_getTimeOut(int teamId, int unitId, int commandId) {

	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId)->at(commandId).timeOut;
	} else {
		return team_callback[teamId]->GetCurrentUnitCommands(unitId)->at(commandId).timeOut;
	}
}
EXPORT(int) _Clb_Unit_CurrentCommand_0ARRAY1SIZE0getParams(int teamId, int unitId, int commandId) {

	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId)->at(commandId).params.size();
	} else {
		return team_callback[teamId]->GetCurrentUnitCommands(unitId)->at(commandId).params.size();
	}
}
EXPORT(int) _Clb_Unit_CurrentCommand_0ARRAY1VALS0getParams(int teamId,
		int unitId, int commandId, float* params, int params_max) {

	const CCommandQueue* cc = NULL;
	if (_Clb_Cheats_isEnabled(teamId)) {
		cc = team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId);
	} else {
		cc = team_callback[teamId]->GetCurrentUnitCommands(unitId);
	}

	if (commandId >= (int)cc->size()) {
		return -1;
	}

	const std::vector<float> ps = cc->at(commandId).params;
	int numParams = ps.size();
	if (params_max < numParams) {
		numParams = params_max;
	}

	int p;
	for (p=0; p < numParams; p++) {
		params[p] = ps.at(p);
	}

	return numParams;
}

EXPORT(float) _Clb_Unit_getExperience(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetUnitExperience(unitId);
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitExperience(unitId);
	} else {
		return team_callback[teamId]->GetUnitExperience(unitId);
	}
}
EXPORT(int) _Clb_Unit_getGroup(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetUnitGroup(unitId);
}

EXPORT(float) _Clb_Unit_getHealth(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetUnitHealth(unitId);
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitHealth(unitId);
	} else {
		return team_callback[teamId]->GetUnitHealth(unitId);
	}
}
EXPORT(float) _Clb_Unit_getSpeed(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetUnitSpeed(unitId);
}
EXPORT(float) _Clb_Unit_getPower(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId];return clb->GetUnitPower(unitId);
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitPower(unitId);
	} else {
		return team_callback[teamId]->GetUnitPower(unitId);
	}
}
EXPORT(SAIFloat3) _Clb_Unit_getPos(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId];
//	return clb->GetUnitPos(unitId).toSAIFloat3();
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitPos(unitId).toSAIFloat3();
	} else {
		return team_callback[teamId]->GetUnitPos(unitId).toSAIFloat3();
	}
}
EXPORT(int) _Clb_Unit_0MULTI1SIZE0ResourceInfo(int teamId, int unitId) {
	return _Clb_0MULTI1SIZE0Resource(teamId);
}
EXPORT(float) _Clb_Unit_0REF1Resource2resourceId0getResourceUse(int teamId,
		int unitId, int resourceId) {

	int res = -1.0F;
	UnitResourceInfo resourceInfo;

	bool fetchOk;
	if (_Clb_Cheats_isEnabled(teamId)) {
		fetchOk = team_cheatCallback[teamId]->GetUnitResourceInfo(unitId,
				&resourceInfo);
	} else {
		fetchOk = team_callback[teamId]->GetUnitResourceInfo(unitId,
				&resourceInfo);
	}
	if (fetchOk) {
		if (resourceId == rh->GetMetalId()) {
			res = resourceInfo.metalUse;
		} else if (resourceId == rh->GetEnergyId()) {
			res = resourceInfo.energyUse;
		}
	}

	return res;
}
EXPORT(float) _Clb_Unit_0REF1Resource2resourceId0getResourceMake(int teamId,
		int unitId, int resourceId) {

	int res = -1.0F;
	UnitResourceInfo resourceInfo;

	bool fetchOk;
	if (_Clb_Cheats_isEnabled(teamId)) {
		fetchOk = team_cheatCallback[teamId]->GetUnitResourceInfo(unitId,
				&resourceInfo);
	} else {
		fetchOk = team_callback[teamId]->GetUnitResourceInfo(unitId,
				&resourceInfo);
	}
	if (fetchOk) {
		if (resourceId == rh->GetMetalId()) {
			res = resourceInfo.metalMake;
		} else if (resourceId == rh->GetEnergyId()) {
			res = resourceInfo.energyMake;
		}
	}

	return res;
}
EXPORT(bool) _Clb_Unit_isActivated(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->IsUnitActivated(unitId);
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->IsUnitActivated(unitId);
	} else {
		return team_callback[teamId]->IsUnitActivated(unitId);
	}
}
EXPORT(bool) _Clb_Unit_isBeingBuilt(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->UnitBeingBuilt(unitId);
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->UnitBeingBuilt(unitId);
	} else {
		return team_callback[teamId]->UnitBeingBuilt(unitId);
	}
}
EXPORT(bool) _Clb_Unit_isCloaked(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->IsUnitCloaked(unitId);
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->IsUnitCloaked(unitId);
	} else {
		return team_callback[teamId]->IsUnitCloaked(unitId);
	}
}
EXPORT(bool) _Clb_Unit_isParalyzed(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->IsUnitParalyzed(unitId);
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->IsUnitParalyzed(unitId);
	} else {
		return team_callback[teamId]->IsUnitParalyzed(unitId);
	}
}
EXPORT(bool) _Clb_Unit_isNeutral(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->IsUnitNeutral(unitId);
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->IsUnitNeutral(unitId);
	} else {
		return team_callback[teamId]->IsUnitNeutral(unitId);
	}
}
EXPORT(int) _Clb_Unit_getBuildingFacing(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetBuildingFacing(unitId);
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetBuildingFacing(unitId);
	} else {
		return team_callback[teamId]->GetBuildingFacing(unitId);
	}
}
EXPORT(int) _Clb_Unit_getLastUserOrderFrame(int teamId, int unitId) {

	if (!isControlledByLocalPlayer(teamId)) return -1;

	return uh->units[unitId]->commandAI->lastUserCommand;
}
//########### END Unit

EXPORT(int) _Clb_0MULTI1SIZE3EnemyUnits0Unit(int teamId) {
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetEnemyUnits(tmpIntArr[teamId]);
	} else {
		return team_callback[teamId]->GetEnemyUnits(tmpIntArr[teamId]);
	}
}
EXPORT(int) _Clb_0MULTI1VALS3EnemyUnits0Unit(int teamId, int* unitIds, int unitIds_max) {
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetEnemyUnits(unitIds, unitIds_max);
	} else {
		return team_callback[teamId]->GetEnemyUnits(unitIds, unitIds_max);
	}
}

EXPORT(int) _Clb_0MULTI1SIZE3EnemyUnitsIn0Unit(int teamId, SAIFloat3 pos, float radius) {
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetEnemyUnits(tmpIntArr[teamId], float3(pos), radius);
	} else {
		return team_callback[teamId]->GetEnemyUnits(tmpIntArr[teamId], float3(pos), radius);
	}
}
EXPORT(int) _Clb_0MULTI1VALS3EnemyUnitsIn0Unit(int teamId, SAIFloat3 pos, float radius, int* unitIds, int unitIds_max) {
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetEnemyUnits(unitIds, float3(pos), radius, unitIds_max);
	} else {
		return team_callback[teamId]->GetEnemyUnits(unitIds, float3(pos), radius, unitIds_max);
	}
}

EXPORT(int) _Clb_0MULTI1SIZE3EnemyUnitsInRadarAndLos0Unit(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetEnemyUnitsInRadarAndLos(tmpIntArr[teamId]);
}
EXPORT(int) _Clb_0MULTI1VALS3EnemyUnitsInRadarAndLos0Unit(int teamId, int* unitIds, int unitIds_max) {
	IAICallback* clb = team_callback[teamId]; return clb->GetEnemyUnitsInRadarAndLos(unitIds, unitIds_max);
}

EXPORT(int) _Clb_0MULTI1SIZE3FriendlyUnits0Unit(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFriendlyUnits(tmpIntArr[teamId]);
}
EXPORT(int) _Clb_0MULTI1VALS3FriendlyUnits0Unit(int teamId, int* unitIds, int unitIds_max) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFriendlyUnits(unitIds);
}

EXPORT(int) _Clb_0MULTI1SIZE3FriendlyUnitsIn0Unit(int teamId, SAIFloat3 pos, float radius) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFriendlyUnits(tmpIntArr[teamId], float3(pos), radius);
}
EXPORT(int) _Clb_0MULTI1VALS3FriendlyUnitsIn0Unit(int teamId, SAIFloat3 pos, float radius, int* unitIds, int unitIds_max) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFriendlyUnits(unitIds, float3(pos), radius, unitIds_max);
}

EXPORT(int) _Clb_0MULTI1SIZE3NeutralUnits0Unit(int teamId) {
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetNeutralUnits(tmpIntArr[teamId]);
	} else {
		return team_callback[teamId]->GetNeutralUnits(tmpIntArr[teamId]);
	}
}
EXPORT(int) _Clb_0MULTI1VALS3NeutralUnits0Unit(int teamId, int* unitIds, int unitIds_max) {
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetNeutralUnits(unitIds, unitIds_max);
	} else {
		return team_callback[teamId]->GetNeutralUnits(unitIds, unitIds_max);
	}
}

EXPORT(int) _Clb_0MULTI1SIZE3NeutralUnitsIn0Unit(int teamId, SAIFloat3 pos, float radius) {
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetNeutralUnits(tmpIntArr[teamId], float3(pos), radius);
	} else {
		return team_callback[teamId]->GetNeutralUnits(tmpIntArr[teamId], float3(pos), radius);
	}
}
EXPORT(int) _Clb_0MULTI1VALS3NeutralUnitsIn0Unit(int teamId, SAIFloat3 pos, float radius, int* unitIds, int unitIds_max) {
	if (_Clb_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetNeutralUnits(unitIds, float3(pos), radius, unitIds_max);
	} else {
		return team_callback[teamId]->GetNeutralUnits(unitIds, float3(pos), radius, unitIds_max);
	}
}

EXPORT(int) _Clb_0MULTI1SIZE3SelectedUnits0Unit(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetSelectedUnits(tmpIntArr[teamId]);
}
EXPORT(int) _Clb_0MULTI1VALS3SelectedUnits0Unit(int teamId, int* unitIds, int unitIds_max) {
	IAICallback* clb = team_callback[teamId]; return clb->GetSelectedUnits(unitIds, unitIds_max);
}

EXPORT(int) _Clb_0MULTI1SIZE3TeamUnits0Unit(int teamId) {

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
EXPORT(int) _Clb_0MULTI1VALS3TeamUnits0Unit(int teamId, int* unitIds, int unitIds_max) {

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
EXPORT(int) _Clb_0MULTI1SIZE0FeatureDef(int teamId) {
	return featureHandler->GetFeatureDefs().size();
}
EXPORT(int) _Clb_0MULTI1VALS0FeatureDef(int teamId, int featureDefIds[], int featureDefIds_max) {

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

EXPORT(const char*) _Clb_FeatureDef_getName(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->myName.c_str();}
EXPORT(const char*) _Clb_FeatureDef_getDescription(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->description.c_str();}
//EXPORT(const char*) _Clb_FeatureDef_getFileName(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->filename.c_str();}
//EXPORT(int) _Clb_FeatureDef_getId(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->id;}
EXPORT(float) _Clb_FeatureDef_0REF1Resource2resourceId0getContainedResource(int teamId, int featureDefId, int resourceId) {

	const FeatureDef* fd = getFeatureDefById(teamId, featureDefId);
	if (resourceId == rh->GetMetalId()) {
		return fd->metal;
	} else if (resourceId == rh->GetEnergyId()) {
		return fd->energy;
	} else {
		return 0.0f;
	}
}
EXPORT(float) _Clb_FeatureDef_getMaxHealth(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->maxHealth;}
EXPORT(float) _Clb_FeatureDef_getReclaimTime(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->reclaimTime;}
EXPORT(float) _Clb_FeatureDef_getMass(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->mass;}
EXPORT(const char*) _Clb_FeatureDef_CollisionVolume_getType(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->collisionVolumeType.c_str();}
EXPORT(SAIFloat3) _Clb_FeatureDef_CollisionVolume_getScales(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->collisionVolumeScales.toSAIFloat3();}
EXPORT(SAIFloat3) _Clb_FeatureDef_CollisionVolume_getOffsets(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->collisionVolumeOffsets.toSAIFloat3();}
EXPORT(int) _Clb_FeatureDef_CollisionVolume_getTest(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->collisionVolumeTest;}
EXPORT(bool) _Clb_FeatureDef_isUpright(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->upright;}
EXPORT(int) _Clb_FeatureDef_getDrawType(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->drawType;}
EXPORT(const char*) _Clb_FeatureDef_getModelName(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->modelname.c_str();}
EXPORT(int) _Clb_FeatureDef_getModelType(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->modelType;}
EXPORT(int) _Clb_FeatureDef_getResurrectable(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->resurrectable;}
EXPORT(int) _Clb_FeatureDef_getSmokeTime(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->smokeTime;}
EXPORT(bool) _Clb_FeatureDef_isDestructable(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->destructable;}
EXPORT(bool) _Clb_FeatureDef_isReclaimable(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->reclaimable;}
EXPORT(bool) _Clb_FeatureDef_isBlocking(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->blocking;}
EXPORT(bool) _Clb_FeatureDef_isBurnable(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->burnable;}
EXPORT(bool) _Clb_FeatureDef_isFloating(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->floating;}
EXPORT(bool) _Clb_FeatureDef_isNoSelect(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->noSelect;}
EXPORT(bool) _Clb_FeatureDef_isGeoThermal(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->geoThermal;}
EXPORT(const char*) _Clb_FeatureDef_getDeathFeature(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->deathFeature.c_str();}
EXPORT(int) _Clb_FeatureDef_getXSize(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->xsize;}
EXPORT(int) _Clb_FeatureDef_getZSize(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->zsize;}
EXPORT(int) _Clb_FeatureDef_0MAP1SIZE0getCustomParams(int teamId, int featureDefId) {

	tmpSize[teamId] = fillCMap(&(getFeatureDefById(teamId, featureDefId)->customParams), tmpKeysArr[teamId], tmpValuesArr[teamId]);
	return tmpSize[teamId];
}
EXPORT(void) _Clb_FeatureDef_0MAP1KEYS0getCustomParams(int teamId, int featureDefId, const char* keys[]) {
	copyStringArr(keys, tmpKeysArr[teamId], tmpSize[teamId]);
}
EXPORT(void) _Clb_FeatureDef_0MAP1VALS0getCustomParams(int teamId, int featureDefId, const char* values[]) {
	copyStringArr(values, tmpValuesArr[teamId], tmpSize[teamId]);
}
//########### END FeatureDef


EXPORT(int) _Clb_0MULTI1SIZE0Feature(int teamId) {

	tmpSize[teamId] = team_callback[teamId]->GetFeatures(tmpIntArr[teamId], TMP_ARR_SIZE);
	return tmpSize[teamId];
}
EXPORT(int) _Clb_0MULTI1VALS0Feature(int teamId, int* featureIds, int featureIds_max) {
	return copyIntArr(featureIds, tmpIntArr[teamId], min(tmpSize[teamId], featureIds_max));
}

EXPORT(int) _Clb_0MULTI1SIZE3FeaturesIn0Feature(int teamId, SAIFloat3 pos, float radius) {

	tmpSize[teamId] = team_callback[teamId]->GetFeatures(tmpIntArr[teamId], TMP_ARR_SIZE, pos, radius);
	return tmpSize[teamId];
}
EXPORT(int) _Clb_0MULTI1VALS3FeaturesIn0Feature(int teamId, SAIFloat3 pos, float radius, int* featureIds, int featureIds_max) {
	return copyIntArr(featureIds, tmpIntArr[teamId], min(tmpSize[teamId], featureIds_max));
}

EXPORT(int) _Clb_Feature_0SINGLE1FETCH2FeatureDef0getDef(int teamId, int featureId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetFeatureDef(featureId)->id;
}

EXPORT(float) _Clb_Feature_getHealth(int teamId, int featureId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFeatureHealth(featureId);
}

EXPORT(float) _Clb_Feature_getReclaimLeft(int teamId, int featureId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFeatureReclaimLeft(featureId);
}

EXPORT(SAIFloat3) _Clb_Feature_getPosition(int teamId, int featureId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFeaturePos(featureId).toSAIFloat3();
}


//########### BEGINN WeaponDef
EXPORT(int) _Clb_0MULTI1SIZE0WeaponDef(int teamId) {
	return weaponDefHandler->numWeaponDefs;
}
EXPORT(int) _Clb_0MULTI1FETCH3WeaponDefByName0WeaponDef(int teamId, const char* weaponDefName) {

	int weaponDefId = -1;

	IAICallback* clb = team_callback[teamId];
	const WeaponDef* wd = clb->GetWeapon(weaponDefName);
	if (wd != NULL) {
		weaponDefId = wd->id;
	}

	return weaponDefId;
}
//EXPORT(int) _Clb_WeaponDef_getId(int teamId, int weaponDefId) {
//	return getWeaponDefById(teamId, weaponDefId)->id;
//}
EXPORT(const char*) _Clb_WeaponDef_getName(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->name.c_str();
}
EXPORT(const char*) _Clb_WeaponDef_getType(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->type.c_str();
}
EXPORT(const char*) _Clb_WeaponDef_getDescription(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->description.c_str();}
//EXPORT(const char*) _Clb_WeaponDef_getFileName(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->filename.c_str();}
//EXPORT(const char*) _Clb_WeaponDef_getCegTag(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->cegTag.c_str();}
EXPORT(float) _Clb_WeaponDef_getRange(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->range;}
EXPORT(float) _Clb_WeaponDef_getHeightMod(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->heightmod;}
EXPORT(float) _Clb_WeaponDef_getAccuracy(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->accuracy;}
EXPORT(float) _Clb_WeaponDef_getSprayAngle(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->sprayAngle;}
EXPORT(float) _Clb_WeaponDef_getMovingAccuracy(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->movingAccuracy;}
EXPORT(float) _Clb_WeaponDef_getTargetMoveError(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->targetMoveError;}
EXPORT(float) _Clb_WeaponDef_getLeadLimit(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->leadLimit;}
EXPORT(float) _Clb_WeaponDef_getLeadBonus(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->leadBonus;}
EXPORT(float) _Clb_WeaponDef_getPredictBoost(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->predictBoost;}
EXPORT(int) _Clb_WeaponDef_Damage_getParalyzeDamageTime(int teamId, int weaponDefId) {
	DamageArray da = getWeaponDefById(teamId, weaponDefId)->damages;
	return da.paralyzeDamageTime;
}
EXPORT(float) _Clb_WeaponDef_Damage_getImpulseFactor(int teamId, int weaponDefId) {
	DamageArray da = getWeaponDefById(teamId, weaponDefId)->damages;
	return da.impulseFactor;
}
EXPORT(float) _Clb_WeaponDef_Damage_getImpulseBoost(int teamId, int weaponDefId) {
	DamageArray da = getWeaponDefById(teamId, weaponDefId)->damages;
	return da.impulseBoost;
}
EXPORT(float) _Clb_WeaponDef_Damage_getCraterMult(int teamId, int weaponDefId) {
	DamageArray da = getWeaponDefById(teamId, weaponDefId)->damages;
	return da.craterMult;
}
EXPORT(float) _Clb_WeaponDef_Damage_getCraterBoost(int teamId, int weaponDefId) {
	DamageArray da = getWeaponDefById(teamId, weaponDefId)->damages;
	return da.craterBoost;
}
EXPORT(int) _Clb_WeaponDef_Damage_0ARRAY1SIZE0getTypes(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->damages.GetNumTypes();
}
EXPORT(int) _Clb_WeaponDef_Damage_0ARRAY1VALS0getTypes(int teamId, int weaponDefId, float* types, int types_max) {

	const WeaponDef* weaponDef = getWeaponDefById(teamId, weaponDefId);
	int types_size = min(weaponDef->damages.GetNumTypes(), types_max);

	for (int i=0; i < types_size; ++i) {
		types[i] = weaponDef->damages[i];
	}

	return types_size;
}
EXPORT(float) _Clb_WeaponDef_getAreaOfEffect(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->areaOfEffect;}
EXPORT(bool) _Clb_WeaponDef_isNoSelfDamage(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->noSelfDamage;}
EXPORT(float) _Clb_WeaponDef_getFireStarter(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->fireStarter;}
EXPORT(float) _Clb_WeaponDef_getEdgeEffectiveness(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->edgeEffectiveness;}
EXPORT(float) _Clb_WeaponDef_getSize(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->size;}
EXPORT(float) _Clb_WeaponDef_getSizeGrowth(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->sizeGrowth;}
EXPORT(float) _Clb_WeaponDef_getCollisionSize(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->collisionSize;}
EXPORT(int) _Clb_WeaponDef_getSalvoSize(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->salvosize;}
EXPORT(float) _Clb_WeaponDef_getSalvoDelay(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->salvodelay;}
EXPORT(float) _Clb_WeaponDef_getReload(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->reload;}
EXPORT(float) _Clb_WeaponDef_getBeamTime(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->beamtime;}
EXPORT(bool) _Clb_WeaponDef_isBeamBurst(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->beamburst;}
EXPORT(bool) _Clb_WeaponDef_isWaterBounce(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->waterBounce;}
EXPORT(bool) _Clb_WeaponDef_isGroundBounce(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->groundBounce;}
EXPORT(float) _Clb_WeaponDef_getBounceRebound(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->bounceRebound;}
EXPORT(float) _Clb_WeaponDef_getBounceSlip(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->bounceSlip;}
EXPORT(int) _Clb_WeaponDef_getNumBounce(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->numBounce;}
EXPORT(float) _Clb_WeaponDef_getMaxAngle(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->maxAngle;}
EXPORT(float) _Clb_WeaponDef_getRestTime(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->restTime;}
EXPORT(float) _Clb_WeaponDef_getUpTime(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->uptime;}
EXPORT(int) _Clb_WeaponDef_getFlightTime(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->flighttime;}
EXPORT(float) _Clb_WeaponDef_0REF1Resource2resourceId0getCost(int teamId, int weaponDefId, int resourceId) {

	const WeaponDef* wd = getWeaponDefById(teamId, weaponDefId);
	if (resourceId == rh->GetMetalId()) {
		return wd->metalcost;
	} else if (resourceId == rh->GetEnergyId()) {
		return wd->energycost;
	} else {
		return 0.0f;
	}
}
EXPORT(float) _Clb_WeaponDef_getSupplyCost(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->supplycost;}
EXPORT(int) _Clb_WeaponDef_getProjectilesPerShot(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->projectilespershot;}
//EXPORT(int) _Clb_WeaponDef_getTdfId(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->tdfId;}
EXPORT(bool) _Clb_WeaponDef_isTurret(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->turret;}
EXPORT(bool) _Clb_WeaponDef_isOnlyForward(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->onlyForward;}
EXPORT(bool) _Clb_WeaponDef_isFixedLauncher(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->fixedLauncher;}
EXPORT(bool) _Clb_WeaponDef_isWaterWeapon(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->waterweapon;}
EXPORT(bool) _Clb_WeaponDef_isFireSubmersed(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->fireSubmersed;}
EXPORT(bool) _Clb_WeaponDef_isSubMissile(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->submissile;}
EXPORT(bool) _Clb_WeaponDef_isTracks(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->tracks;}
EXPORT(bool) _Clb_WeaponDef_isDropped(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dropped;}
EXPORT(bool) _Clb_WeaponDef_isParalyzer(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->paralyzer;}
EXPORT(bool) _Clb_WeaponDef_isImpactOnly(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->impactOnly;}
EXPORT(bool) _Clb_WeaponDef_isNoAutoTarget(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->noAutoTarget;}
EXPORT(bool) _Clb_WeaponDef_isManualFire(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->manualfire;}
EXPORT(int) _Clb_WeaponDef_getInterceptor(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->interceptor;}
EXPORT(int) _Clb_WeaponDef_getTargetable(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->targetable;}
EXPORT(bool) _Clb_WeaponDef_isStockpileable(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->stockpile;}
EXPORT(float) _Clb_WeaponDef_getCoverageRange(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->coverageRange;}
EXPORT(float) _Clb_WeaponDef_getStockpileTime(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->stockpileTime;}
EXPORT(float) _Clb_WeaponDef_getIntensity(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->intensity;}
EXPORT(float) _Clb_WeaponDef_getThickness(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->thickness;}
EXPORT(float) _Clb_WeaponDef_getLaserFlareSize(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->laserflaresize;}
EXPORT(float) _Clb_WeaponDef_getCoreThickness(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->corethickness;}
EXPORT(float) _Clb_WeaponDef_getDuration(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->duration;}
EXPORT(int) _Clb_WeaponDef_getLodDistance(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->lodDistance;}
EXPORT(float) _Clb_WeaponDef_getFalloffRate(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->falloffRate;}
EXPORT(int) _Clb_WeaponDef_getGraphicsType(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->graphicsType;}
EXPORT(bool) _Clb_WeaponDef_isSoundTrigger(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->soundTrigger;}
EXPORT(bool) _Clb_WeaponDef_isSelfExplode(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->selfExplode;}
EXPORT(bool) _Clb_WeaponDef_isGravityAffected(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->gravityAffected;}
EXPORT(int) _Clb_WeaponDef_getHighTrajectory(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->highTrajectory;}
EXPORT(float) _Clb_WeaponDef_getMyGravity(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->myGravity;}
EXPORT(bool) _Clb_WeaponDef_isNoExplode(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->noExplode;}
EXPORT(float) _Clb_WeaponDef_getStartVelocity(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->startvelocity;}
EXPORT(float) _Clb_WeaponDef_getWeaponAcceleration(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->weaponacceleration;}
EXPORT(float) _Clb_WeaponDef_getTurnRate(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->turnrate;}
EXPORT(float) _Clb_WeaponDef_getMaxVelocity(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->maxvelocity;}
EXPORT(float) _Clb_WeaponDef_getProjectileSpeed(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->projectilespeed;}
EXPORT(float) _Clb_WeaponDef_getExplosionSpeed(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->explosionSpeed;}
EXPORT(unsigned int) _Clb_WeaponDef_getOnlyTargetCategory(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->onlyTargetCategory;}
EXPORT(float) _Clb_WeaponDef_getWobble(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->wobble;}
EXPORT(float) _Clb_WeaponDef_getDance(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dance;}
EXPORT(float) _Clb_WeaponDef_getTrajectoryHeight(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->trajectoryHeight;}
EXPORT(bool) _Clb_WeaponDef_isLargeBeamLaser(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->largeBeamLaser;}
EXPORT(bool) _Clb_WeaponDef_isShield(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->isShield;}
EXPORT(bool) _Clb_WeaponDef_isShieldRepulser(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldRepulser;}
EXPORT(bool) _Clb_WeaponDef_isSmartShield(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->smartShield;}
EXPORT(bool) _Clb_WeaponDef_isExteriorShield(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->exteriorShield;}
EXPORT(bool) _Clb_WeaponDef_isVisibleShield(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->visibleShield;}
EXPORT(bool) _Clb_WeaponDef_isVisibleShieldRepulse(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->visibleShieldRepulse;}
EXPORT(int) _Clb_WeaponDef_getVisibleShieldHitFrames(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->visibleShieldHitFrames;}
EXPORT(float) _Clb_WeaponDef_Shield_0REF1Resource2resourceId0getResourceUse(int teamId, int weaponDefId, int resourceId) {

	const WeaponDef* wd = getWeaponDefById(teamId, weaponDefId);
	if (resourceId == rh->GetEnergyId()) {
		return wd->shieldEnergyUse;
	} else {
		return 0.0f;
	}
}
EXPORT(float) _Clb_WeaponDef_Shield_getRadius(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldRadius;}
EXPORT(float) _Clb_WeaponDef_Shield_getForce(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldForce;}
EXPORT(float) _Clb_WeaponDef_Shield_getMaxSpeed(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldMaxSpeed;}
EXPORT(float) _Clb_WeaponDef_Shield_getPower(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldPower;}
EXPORT(float) _Clb_WeaponDef_Shield_getPowerRegen(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldPowerRegen;}
EXPORT(float) _Clb_WeaponDef_Shield_0REF1Resource2resourceId0getPowerRegenResource(int teamId, int weaponDefId, int resourceId) {

	const WeaponDef* wd = getWeaponDefById(teamId, weaponDefId);
	if (resourceId == rh->GetEnergyId()) {
		return wd->shieldPowerRegenEnergy;
	} else {
		return 0.0f;
	}
}
EXPORT(float) _Clb_WeaponDef_Shield_getStartingPower(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldStartingPower;}
EXPORT(int) _Clb_WeaponDef_Shield_getRechargeDelay(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldRechargeDelay;}
EXPORT(struct SAIFloat3) _Clb_WeaponDef_Shield_getGoodColor(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldGoodColor.toSAIFloat3();}
EXPORT(struct SAIFloat3) _Clb_WeaponDef_Shield_getBadColor(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldBadColor.toSAIFloat3();}
EXPORT(float) _Clb_WeaponDef_Shield_getAlpha(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldAlpha;}
EXPORT(unsigned int) _Clb_WeaponDef_Shield_getInterceptType(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldInterceptType;}
EXPORT(unsigned int) _Clb_WeaponDef_getInterceptedByShieldType(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->interceptedByShieldType;}
EXPORT(bool) _Clb_WeaponDef_isAvoidFriendly(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->avoidFriendly;}
EXPORT(bool) _Clb_WeaponDef_isAvoidFeature(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->avoidFeature;}
EXPORT(bool) _Clb_WeaponDef_isAvoidNeutral(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->avoidNeutral;}
EXPORT(float) _Clb_WeaponDef_getTargetBorder(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->targetBorder;}
EXPORT(float) _Clb_WeaponDef_getCylinderTargetting(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->cylinderTargetting;}
EXPORT(float) _Clb_WeaponDef_getMinIntensity(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->minIntensity;}
EXPORT(float) _Clb_WeaponDef_getHeightBoostFactor(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->heightBoostFactor;}
EXPORT(float) _Clb_WeaponDef_getProximityPriority(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->proximityPriority;}
EXPORT(unsigned int) _Clb_WeaponDef_getCollisionFlags(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->collisionFlags;}
EXPORT(bool) _Clb_WeaponDef_isSweepFire(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->sweepFire;}
EXPORT(bool) _Clb_WeaponDef_isAbleToAttackGround(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->canAttackGround;}
EXPORT(float) _Clb_WeaponDef_getCameraShake(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->cameraShake;}
EXPORT(float) _Clb_WeaponDef_getDynDamageExp(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dynDamageExp;}
EXPORT(float) _Clb_WeaponDef_getDynDamageMin(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dynDamageMin;}
EXPORT(float) _Clb_WeaponDef_getDynDamageRange(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dynDamageRange;}
EXPORT(bool) _Clb_WeaponDef_isDynDamageInverted(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dynDamageInverted;}
EXPORT(int) _Clb_WeaponDef_0MAP1SIZE0getCustomParams(int teamId, int weaponDefId) {

	tmpSize[teamId] = fillCMap(&(getWeaponDefById(teamId, weaponDefId)->customParams), tmpKeysArr[teamId], tmpValuesArr[teamId]);
	return tmpSize[teamId];
}
EXPORT(void) _Clb_WeaponDef_0MAP1KEYS0getCustomParams(int teamId, int weaponDefId, const char* keys[]) {
	copyStringArr(keys, tmpKeysArr[teamId], tmpSize[teamId]);
}
EXPORT(void) _Clb_WeaponDef_0MAP1VALS0getCustomParams(int teamId, int weaponDefId, const char* values[]) {
	copyStringArr(values, tmpValuesArr[teamId], tmpSize[teamId]);
}
//########### END WeaponDef


EXPORT(int) _Clb_0MULTI1SIZE0Group(int teamId) {
	return grouphandlers[teamId]->groups.size();
}
EXPORT(int) _Clb_0MULTI1VALS0Group(int teamId, int groupIds[], int groupIds_max) {

	int size = grouphandlers[teamId]->groups.size();

	size = min(size, groupIds_max);
	int i;
	for (i=0; i < size; ++i) {
		groupIds[i] = grouphandlers[teamId]->groups[i]->id;
	}

	return size;
}

EXPORT(int) _Clb_Group_0MULTI1SIZE0SupportedCommand(int teamId, int groupId) {
	return team_callback[teamId]->GetGroupCommands(groupId)->size();
}
EXPORT(int) _Clb_Group_SupportedCommand_getId(int teamId, int groupId, int commandId) {
	return team_callback[teamId]->GetGroupCommands(groupId)->at(commandId).id;
}
EXPORT(const char*) _Clb_Group_SupportedCommand_getName(int teamId, int groupId, int commandId) {
	return team_callback[teamId]->GetGroupCommands(groupId)->at(commandId).name.c_str();
}
EXPORT(const char*) _Clb_Group_SupportedCommand_getToolTip(int teamId, int groupId, int commandId) {
	return team_callback[teamId]->GetGroupCommands(groupId)->at(commandId).tooltip.c_str();
}
EXPORT(bool) _Clb_Group_SupportedCommand_isShowUnique(int teamId, int groupId, int commandId) {
	return team_callback[teamId]->GetGroupCommands(groupId)->at(commandId).showUnique;
}
EXPORT(bool) _Clb_Group_SupportedCommand_isDisabled(int teamId, int groupId, int commandId) {
	return team_callback[teamId]->GetGroupCommands(groupId)->at(commandId).disabled;
}
EXPORT(int) _Clb_Group_SupportedCommand_0ARRAY1SIZE0getParams(int teamId, int groupId, int commandId) {
	return team_callback[teamId]->GetGroupCommands(groupId)->at(commandId).params.size();
}
EXPORT(int) _Clb_Group_SupportedCommand_0ARRAY1VALS0getParams(int teamId,
		int groupId, int commandId, const char** params, int params_max) {

	const std::vector<std::string> ps
			= team_callback[teamId]->GetGroupCommands(groupId)->at(commandId).params;
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

EXPORT(int) _Clb_Group_OrderPreview_getId(int teamId, int groupId) {

	if (!isControlledByLocalPlayer(teamId)) return -1;

	//TODO: need to add support for new gui
	Command tmpCmd = guihandler->GetOrderPreview();
	return tmpCmd.id;
}
EXPORT(unsigned char) _Clb_Group_OrderPreview_getOptions(int teamId, int groupId) {

	if (!isControlledByLocalPlayer(teamId)) return '\0';

	//TODO: need to add support for new gui
	Command tmpCmd = guihandler->GetOrderPreview();
	return tmpCmd.options;
}
EXPORT(unsigned int) _Clb_Group_OrderPreview_getTag(int teamId, int groupId) {

	if (!isControlledByLocalPlayer(teamId)) return 0;

	//TODO: need to add support for new gui
	Command tmpCmd = guihandler->GetOrderPreview();
	return tmpCmd.tag;
}
EXPORT(int) _Clb_Group_OrderPreview_getTimeOut(int teamId, int groupId) {

	if (!isControlledByLocalPlayer(teamId)) return -1;

	//TODO: need to add support for new gui
	Command tmpCmd = guihandler->GetOrderPreview();
	return tmpCmd.timeOut;
}
EXPORT(int) _Clb_Group_OrderPreview_0ARRAY1SIZE0getParams(int teamId,
		int groupId) {

	if (!isControlledByLocalPlayer(teamId)) return 0;

	//TODO: need to add support for new gui
	return guihandler->GetOrderPreview().params.size();
}
EXPORT(int) _Clb_Group_OrderPreview_0ARRAY1VALS0getParams(int teamId,
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

EXPORT(bool) _Clb_Group_isSelected(int teamId, int groupId) {

	if (!isControlledByLocalPlayer(teamId)) return false;

	return selectedUnits.selectedGroup == groupId;
}
//##############################################################################




SAICallback* initSAICallback(int teamId, IGlobalAICallback* aiGlobalCallback) {

	SAICallback* sAICallback = new SAICallback();
	// INSERT AUTO GENERATED PART FROM HERE ...
	sAICallback->Clb_handleCommand = _Clb_handleCommand;
	sAICallback->Clb_Game_getCurrentFrame = _Clb_Game_getCurrentFrame;
	sAICallback->Clb_Game_getAiInterfaceVersion = _Clb_Game_getAiInterfaceVersion;
	sAICallback->Clb_Game_getMyTeam = _Clb_Game_getMyTeam;
	sAICallback->Clb_Game_getMyAllyTeam = _Clb_Game_getMyAllyTeam;
	sAICallback->Clb_Game_getPlayerTeam = _Clb_Game_getPlayerTeam;
	sAICallback->Clb_Game_getTeamSide = _Clb_Game_getTeamSide;
	sAICallback->Clb_Game_isExceptionHandlingEnabled = _Clb_Game_isExceptionHandlingEnabled;
	sAICallback->Clb_Game_isDebugModeEnabled = _Clb_Game_isDebugModeEnabled;
	sAICallback->Clb_Game_getMode = _Clb_Game_getMode;
	sAICallback->Clb_Game_isPaused = _Clb_Game_isPaused;
	sAICallback->Clb_Game_getSpeedFactor = _Clb_Game_getSpeedFactor;
	sAICallback->Clb_Game_getSetupScript = _Clb_Game_getSetupScript;
	sAICallback->Clb_Gui_getViewRange = _Clb_Gui_getViewRange;
	sAICallback->Clb_Gui_getScreenX = _Clb_Gui_getScreenX;
	sAICallback->Clb_Gui_getScreenY = _Clb_Gui_getScreenY;
	sAICallback->Clb_Gui_Camera_getDirection = _Clb_Gui_Camera_getDirection;
	sAICallback->Clb_Gui_Camera_getPosition = _Clb_Gui_Camera_getPosition;
	sAICallback->Clb_Cheats_isEnabled = _Clb_Cheats_isEnabled;
	sAICallback->Clb_Cheats_setEnabled = _Clb_Cheats_setEnabled;
	sAICallback->Clb_Cheats_setEventsEnabled = _Clb_Cheats_setEventsEnabled;
	sAICallback->Clb_Cheats_isOnlyPassive = _Clb_Cheats_isOnlyPassive;
	sAICallback->Clb_0MULTI1SIZE0Resource = _Clb_0MULTI1SIZE0Resource;
	sAICallback->Clb_0MULTI1FETCH3ResourceByName0Resource = _Clb_0MULTI1FETCH3ResourceByName0Resource;
	sAICallback->Clb_Resource_getName = _Clb_Resource_getName;
	sAICallback->Clb_Resource_getOptimum = _Clb_Resource_getOptimum;
	sAICallback->Clb_Economy_0REF1Resource2resourceId0getCurrent = _Clb_Economy_0REF1Resource2resourceId0getCurrent;
	sAICallback->Clb_Economy_0REF1Resource2resourceId0getIncome = _Clb_Economy_0REF1Resource2resourceId0getIncome;
	sAICallback->Clb_Economy_0REF1Resource2resourceId0getUsage = _Clb_Economy_0REF1Resource2resourceId0getUsage;
	sAICallback->Clb_Economy_0REF1Resource2resourceId0getStorage = _Clb_Economy_0REF1Resource2resourceId0getStorage;
	sAICallback->Clb_File_getSize = _Clb_File_getSize;
	sAICallback->Clb_File_getContent = _Clb_File_getContent;
	sAICallback->Clb_File_locateForReading = _Clb_File_locateForReading;
	sAICallback->Clb_File_locateForWriting = _Clb_File_locateForWriting;
	sAICallback->Clb_0MULTI1SIZE0UnitDef = _Clb_0MULTI1SIZE0UnitDef;
	sAICallback->Clb_0MULTI1VALS0UnitDef = _Clb_0MULTI1VALS0UnitDef;
	sAICallback->Clb_0MULTI1FETCH3UnitDefByName0UnitDef = _Clb_0MULTI1FETCH3UnitDefByName0UnitDef;
	sAICallback->Clb_UnitDef_getHeight = _Clb_UnitDef_getHeight;
	sAICallback->Clb_UnitDef_getRadius = _Clb_UnitDef_getRadius;
	sAICallback->Clb_UnitDef_isValid = _Clb_UnitDef_isValid;
	sAICallback->Clb_UnitDef_getName = _Clb_UnitDef_getName;
	sAICallback->Clb_UnitDef_getHumanName = _Clb_UnitDef_getHumanName;
//	sAICallback->Clb_UnitDef_getFileName = _Clb_UnitDef_getFileName;
	sAICallback->Clb_UnitDef_getAiHint = _Clb_UnitDef_getAiHint;
	sAICallback->Clb_UnitDef_getCobId = _Clb_UnitDef_getCobId;
	sAICallback->Clb_UnitDef_getTechLevel = _Clb_UnitDef_getTechLevel;
	sAICallback->Clb_UnitDef_getGaia = _Clb_UnitDef_getGaia;
	sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getUpkeep = _Clb_UnitDef_0REF1Resource2resourceId0getUpkeep;
	sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getResourceMake = _Clb_UnitDef_0REF1Resource2resourceId0getResourceMake;
	sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getMakesResource = _Clb_UnitDef_0REF1Resource2resourceId0getMakesResource;
	sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getCost = _Clb_UnitDef_0REF1Resource2resourceId0getCost;
	sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getExtractsResource = _Clb_UnitDef_0REF1Resource2resourceId0getExtractsResource;
	sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getResourceExtractorRange = _Clb_UnitDef_0REF1Resource2resourceId0getResourceExtractorRange;
	sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getWindResourceGenerator = _Clb_UnitDef_0REF1Resource2resourceId0getWindResourceGenerator;
	sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getTidalResourceGenerator = _Clb_UnitDef_0REF1Resource2resourceId0getTidalResourceGenerator;
	sAICallback->Clb_UnitDef_0REF1Resource2resourceId0getStorage = _Clb_UnitDef_0REF1Resource2resourceId0getStorage;
	sAICallback->Clb_UnitDef_0REF1Resource2resourceId0isSquareResourceExtractor = _Clb_UnitDef_0REF1Resource2resourceId0isSquareResourceExtractor;
	sAICallback->Clb_UnitDef_0REF1Resource2resourceId0isResourceMaker = _Clb_UnitDef_0REF1Resource2resourceId0isResourceMaker;
	sAICallback->Clb_UnitDef_getBuildTime = _Clb_UnitDef_getBuildTime;
	sAICallback->Clb_UnitDef_getAutoHeal = _Clb_UnitDef_getAutoHeal;
	sAICallback->Clb_UnitDef_getIdleAutoHeal = _Clb_UnitDef_getIdleAutoHeal;
	sAICallback->Clb_UnitDef_getIdleTime = _Clb_UnitDef_getIdleTime;
	sAICallback->Clb_UnitDef_getPower = _Clb_UnitDef_getPower;
	sAICallback->Clb_UnitDef_getHealth = _Clb_UnitDef_getHealth;
	sAICallback->Clb_UnitDef_getCategory = _Clb_UnitDef_getCategory;
	sAICallback->Clb_UnitDef_getSpeed = _Clb_UnitDef_getSpeed;
	sAICallback->Clb_UnitDef_getTurnRate = _Clb_UnitDef_getTurnRate;
	sAICallback->Clb_UnitDef_isTurnInPlace = _Clb_UnitDef_isTurnInPlace;
	sAICallback->Clb_UnitDef_getMoveType = _Clb_UnitDef_getMoveType;
	sAICallback->Clb_UnitDef_isUpright = _Clb_UnitDef_isUpright;
	sAICallback->Clb_UnitDef_isCollide = _Clb_UnitDef_isCollide;
	sAICallback->Clb_UnitDef_getControlRadius = _Clb_UnitDef_getControlRadius;
	sAICallback->Clb_UnitDef_getLosRadius = _Clb_UnitDef_getLosRadius;
	sAICallback->Clb_UnitDef_getAirLosRadius = _Clb_UnitDef_getAirLosRadius;
	sAICallback->Clb_UnitDef_getLosHeight = _Clb_UnitDef_getLosHeight;
	sAICallback->Clb_UnitDef_getRadarRadius = _Clb_UnitDef_getRadarRadius;
	sAICallback->Clb_UnitDef_getSonarRadius = _Clb_UnitDef_getSonarRadius;
	sAICallback->Clb_UnitDef_getJammerRadius = _Clb_UnitDef_getJammerRadius;
	sAICallback->Clb_UnitDef_getSonarJamRadius = _Clb_UnitDef_getSonarJamRadius;
	sAICallback->Clb_UnitDef_getSeismicRadius = _Clb_UnitDef_getSeismicRadius;
	sAICallback->Clb_UnitDef_getSeismicSignature = _Clb_UnitDef_getSeismicSignature;
	sAICallback->Clb_UnitDef_isStealth = _Clb_UnitDef_isStealth;
	sAICallback->Clb_UnitDef_isSonarStealth = _Clb_UnitDef_isSonarStealth;
	sAICallback->Clb_UnitDef_isBuildRange3D = _Clb_UnitDef_isBuildRange3D;
	sAICallback->Clb_UnitDef_getBuildDistance = _Clb_UnitDef_getBuildDistance;
	sAICallback->Clb_UnitDef_getBuildSpeed = _Clb_UnitDef_getBuildSpeed;
	sAICallback->Clb_UnitDef_getReclaimSpeed = _Clb_UnitDef_getReclaimSpeed;
	sAICallback->Clb_UnitDef_getRepairSpeed = _Clb_UnitDef_getRepairSpeed;
	sAICallback->Clb_UnitDef_getMaxRepairSpeed = _Clb_UnitDef_getMaxRepairSpeed;
	sAICallback->Clb_UnitDef_getResurrectSpeed = _Clb_UnitDef_getResurrectSpeed;
	sAICallback->Clb_UnitDef_getCaptureSpeed = _Clb_UnitDef_getCaptureSpeed;
	sAICallback->Clb_UnitDef_getTerraformSpeed = _Clb_UnitDef_getTerraformSpeed;
	sAICallback->Clb_UnitDef_getMass = _Clb_UnitDef_getMass;
	sAICallback->Clb_UnitDef_isPushResistant = _Clb_UnitDef_isPushResistant;
	sAICallback->Clb_UnitDef_isStrafeToAttack = _Clb_UnitDef_isStrafeToAttack;
	sAICallback->Clb_UnitDef_getMinCollisionSpeed = _Clb_UnitDef_getMinCollisionSpeed;
	sAICallback->Clb_UnitDef_getSlideTolerance = _Clb_UnitDef_getSlideTolerance;
	sAICallback->Clb_UnitDef_getMaxSlope = _Clb_UnitDef_getMaxSlope;
	sAICallback->Clb_UnitDef_getMaxHeightDif = _Clb_UnitDef_getMaxHeightDif;
	sAICallback->Clb_UnitDef_getMinWaterDepth = _Clb_UnitDef_getMinWaterDepth;
	sAICallback->Clb_UnitDef_getWaterline = _Clb_UnitDef_getWaterline;
	sAICallback->Clb_UnitDef_getMaxWaterDepth = _Clb_UnitDef_getMaxWaterDepth;
	sAICallback->Clb_UnitDef_getArmoredMultiple = _Clb_UnitDef_getArmoredMultiple;
	sAICallback->Clb_UnitDef_getArmorType = _Clb_UnitDef_getArmorType;
	sAICallback->Clb_UnitDef_FlankingBonus_getMode = _Clb_UnitDef_FlankingBonus_getMode;
	sAICallback->Clb_UnitDef_FlankingBonus_getDir = _Clb_UnitDef_FlankingBonus_getDir;
	sAICallback->Clb_UnitDef_FlankingBonus_getMax = _Clb_UnitDef_FlankingBonus_getMax;
	sAICallback->Clb_UnitDef_FlankingBonus_getMin = _Clb_UnitDef_FlankingBonus_getMin;
	sAICallback->Clb_UnitDef_FlankingBonus_getMobilityAdd = _Clb_UnitDef_FlankingBonus_getMobilityAdd;
	sAICallback->Clb_UnitDef_CollisionVolume_getType = _Clb_UnitDef_CollisionVolume_getType;
	sAICallback->Clb_UnitDef_CollisionVolume_getScales = _Clb_UnitDef_CollisionVolume_getScales;
	sAICallback->Clb_UnitDef_CollisionVolume_getOffsets = _Clb_UnitDef_CollisionVolume_getOffsets;
	sAICallback->Clb_UnitDef_CollisionVolume_getTest = _Clb_UnitDef_CollisionVolume_getTest;
	sAICallback->Clb_UnitDef_getMaxWeaponRange = _Clb_UnitDef_getMaxWeaponRange;
	sAICallback->Clb_UnitDef_getType = _Clb_UnitDef_getType;
	sAICallback->Clb_UnitDef_getTooltip = _Clb_UnitDef_getTooltip;
	sAICallback->Clb_UnitDef_getWreckName = _Clb_UnitDef_getWreckName;
	sAICallback->Clb_UnitDef_getDeathExplosion = _Clb_UnitDef_getDeathExplosion;
	sAICallback->Clb_UnitDef_getSelfDExplosion = _Clb_UnitDef_getSelfDExplosion;
	sAICallback->Clb_UnitDef_getTedClassString = _Clb_UnitDef_getTedClassString;
	sAICallback->Clb_UnitDef_getCategoryString = _Clb_UnitDef_getCategoryString;
	sAICallback->Clb_UnitDef_isAbleToSelfD = _Clb_UnitDef_isAbleToSelfD;
	sAICallback->Clb_UnitDef_getSelfDCountdown = _Clb_UnitDef_getSelfDCountdown;
	sAICallback->Clb_UnitDef_isAbleToSubmerge = _Clb_UnitDef_isAbleToSubmerge;
	sAICallback->Clb_UnitDef_isAbleToFly = _Clb_UnitDef_isAbleToFly;
	sAICallback->Clb_UnitDef_isAbleToMove = _Clb_UnitDef_isAbleToMove;
	sAICallback->Clb_UnitDef_isAbleToHover = _Clb_UnitDef_isAbleToHover;
	sAICallback->Clb_UnitDef_isFloater = _Clb_UnitDef_isFloater;
	sAICallback->Clb_UnitDef_isBuilder = _Clb_UnitDef_isBuilder;
	sAICallback->Clb_UnitDef_isActivateWhenBuilt = _Clb_UnitDef_isActivateWhenBuilt;
	sAICallback->Clb_UnitDef_isOnOffable = _Clb_UnitDef_isOnOffable;
	sAICallback->Clb_UnitDef_isFullHealthFactory = _Clb_UnitDef_isFullHealthFactory;
	sAICallback->Clb_UnitDef_isFactoryHeadingTakeoff = _Clb_UnitDef_isFactoryHeadingTakeoff;
	sAICallback->Clb_UnitDef_isReclaimable = _Clb_UnitDef_isReclaimable;
	sAICallback->Clb_UnitDef_isCapturable = _Clb_UnitDef_isCapturable;
	sAICallback->Clb_UnitDef_isAbleToRestore = _Clb_UnitDef_isAbleToRestore;
	sAICallback->Clb_UnitDef_isAbleToRepair = _Clb_UnitDef_isAbleToRepair;
	sAICallback->Clb_UnitDef_isAbleToSelfRepair = _Clb_UnitDef_isAbleToSelfRepair;
	sAICallback->Clb_UnitDef_isAbleToReclaim = _Clb_UnitDef_isAbleToReclaim;
	sAICallback->Clb_UnitDef_isAbleToAttack = _Clb_UnitDef_isAbleToAttack;
	sAICallback->Clb_UnitDef_isAbleToPatrol = _Clb_UnitDef_isAbleToPatrol;
	sAICallback->Clb_UnitDef_isAbleToFight = _Clb_UnitDef_isAbleToFight;
	sAICallback->Clb_UnitDef_isAbleToGuard = _Clb_UnitDef_isAbleToGuard;
	sAICallback->Clb_UnitDef_isAbleToBuild = _Clb_UnitDef_isAbleToBuild;
	sAICallback->Clb_UnitDef_isAbleToAssist = _Clb_UnitDef_isAbleToAssist;
	sAICallback->Clb_UnitDef_isAssistable = _Clb_UnitDef_isAssistable;
	sAICallback->Clb_UnitDef_isAbleToRepeat = _Clb_UnitDef_isAbleToRepeat;
	sAICallback->Clb_UnitDef_isAbleToFireControl = _Clb_UnitDef_isAbleToFireControl;
	sAICallback->Clb_UnitDef_getFireState = _Clb_UnitDef_getFireState;
	sAICallback->Clb_UnitDef_getMoveState = _Clb_UnitDef_getMoveState;
	sAICallback->Clb_UnitDef_getWingDrag = _Clb_UnitDef_getWingDrag;
	sAICallback->Clb_UnitDef_getWingAngle = _Clb_UnitDef_getWingAngle;
	sAICallback->Clb_UnitDef_getDrag = _Clb_UnitDef_getDrag;
	sAICallback->Clb_UnitDef_getFrontToSpeed = _Clb_UnitDef_getFrontToSpeed;
	sAICallback->Clb_UnitDef_getSpeedToFront = _Clb_UnitDef_getSpeedToFront;
	sAICallback->Clb_UnitDef_getMyGravity = _Clb_UnitDef_getMyGravity;
	sAICallback->Clb_UnitDef_getMaxBank = _Clb_UnitDef_getMaxBank;
	sAICallback->Clb_UnitDef_getMaxPitch = _Clb_UnitDef_getMaxPitch;
	sAICallback->Clb_UnitDef_getTurnRadius = _Clb_UnitDef_getTurnRadius;
	sAICallback->Clb_UnitDef_getWantedHeight = _Clb_UnitDef_getWantedHeight;
	sAICallback->Clb_UnitDef_getVerticalSpeed = _Clb_UnitDef_getVerticalSpeed;
	sAICallback->Clb_UnitDef_isAbleToCrash = _Clb_UnitDef_isAbleToCrash;
	sAICallback->Clb_UnitDef_isHoverAttack = _Clb_UnitDef_isHoverAttack;
	sAICallback->Clb_UnitDef_isAirStrafe = _Clb_UnitDef_isAirStrafe;
	sAICallback->Clb_UnitDef_getDlHoverFactor = _Clb_UnitDef_getDlHoverFactor;
	sAICallback->Clb_UnitDef_getMaxAcceleration = _Clb_UnitDef_getMaxAcceleration;
	sAICallback->Clb_UnitDef_getMaxDeceleration = _Clb_UnitDef_getMaxDeceleration;
	sAICallback->Clb_UnitDef_getMaxAileron = _Clb_UnitDef_getMaxAileron;
	sAICallback->Clb_UnitDef_getMaxElevator = _Clb_UnitDef_getMaxElevator;
	sAICallback->Clb_UnitDef_getMaxRudder = _Clb_UnitDef_getMaxRudder;
	sAICallback->Clb_UnitDef_getXSize = _Clb_UnitDef_getXSize;
	sAICallback->Clb_UnitDef_getZSize = _Clb_UnitDef_getZSize;
	sAICallback->Clb_UnitDef_getBuildAngle = _Clb_UnitDef_getBuildAngle;
	sAICallback->Clb_UnitDef_getLoadingRadius = _Clb_UnitDef_getLoadingRadius;
	sAICallback->Clb_UnitDef_getUnloadSpread = _Clb_UnitDef_getUnloadSpread;
	sAICallback->Clb_UnitDef_getTransportCapacity = _Clb_UnitDef_getTransportCapacity;
	sAICallback->Clb_UnitDef_getTransportSize = _Clb_UnitDef_getTransportSize;
	sAICallback->Clb_UnitDef_getMinTransportSize = _Clb_UnitDef_getMinTransportSize;
	sAICallback->Clb_UnitDef_isAirBase = _Clb_UnitDef_isAirBase;
	sAICallback->Clb_UnitDef_isFirePlatform = _Clb_UnitDef_isFirePlatform;
	sAICallback->Clb_UnitDef_getTransportMass = _Clb_UnitDef_getTransportMass;
	sAICallback->Clb_UnitDef_getMinTransportMass = _Clb_UnitDef_getMinTransportMass;
	sAICallback->Clb_UnitDef_isHoldSteady = _Clb_UnitDef_isHoldSteady;
	sAICallback->Clb_UnitDef_isReleaseHeld = _Clb_UnitDef_isReleaseHeld;
	sAICallback->Clb_UnitDef_isNotTransportable = _Clb_UnitDef_isNotTransportable;
	sAICallback->Clb_UnitDef_isTransportByEnemy = _Clb_UnitDef_isTransportByEnemy;
	sAICallback->Clb_UnitDef_getTransportUnloadMethod = _Clb_UnitDef_getTransportUnloadMethod;
	sAICallback->Clb_UnitDef_getFallSpeed = _Clb_UnitDef_getFallSpeed;
	sAICallback->Clb_UnitDef_getUnitFallSpeed = _Clb_UnitDef_getUnitFallSpeed;
	sAICallback->Clb_UnitDef_isAbleToCloak = _Clb_UnitDef_isAbleToCloak;
	sAICallback->Clb_UnitDef_isStartCloaked = _Clb_UnitDef_isStartCloaked;
	sAICallback->Clb_UnitDef_getCloakCost = _Clb_UnitDef_getCloakCost;
	sAICallback->Clb_UnitDef_getCloakCostMoving = _Clb_UnitDef_getCloakCostMoving;
	sAICallback->Clb_UnitDef_getDecloakDistance = _Clb_UnitDef_getDecloakDistance;
	sAICallback->Clb_UnitDef_isDecloakSpherical = _Clb_UnitDef_isDecloakSpherical;
	sAICallback->Clb_UnitDef_isDecloakOnFire = _Clb_UnitDef_isDecloakOnFire;
	sAICallback->Clb_UnitDef_isAbleToKamikaze = _Clb_UnitDef_isAbleToKamikaze;
	sAICallback->Clb_UnitDef_getKamikazeDist = _Clb_UnitDef_getKamikazeDist;
	sAICallback->Clb_UnitDef_isTargetingFacility = _Clb_UnitDef_isTargetingFacility;
	sAICallback->Clb_UnitDef_isAbleToDGun = _Clb_UnitDef_isAbleToDGun;
	sAICallback->Clb_UnitDef_isNeedGeo = _Clb_UnitDef_isNeedGeo;
	sAICallback->Clb_UnitDef_isFeature = _Clb_UnitDef_isFeature;
	sAICallback->Clb_UnitDef_isHideDamage = _Clb_UnitDef_isHideDamage;
	sAICallback->Clb_UnitDef_isCommander = _Clb_UnitDef_isCommander;
	sAICallback->Clb_UnitDef_isShowPlayerName = _Clb_UnitDef_isShowPlayerName;
	sAICallback->Clb_UnitDef_isAbleToResurrect = _Clb_UnitDef_isAbleToResurrect;
	sAICallback->Clb_UnitDef_isAbleToCapture = _Clb_UnitDef_isAbleToCapture;
	sAICallback->Clb_UnitDef_getHighTrajectoryType = _Clb_UnitDef_getHighTrajectoryType;
	sAICallback->Clb_UnitDef_getNoChaseCategory = _Clb_UnitDef_getNoChaseCategory;
	sAICallback->Clb_UnitDef_isLeaveTracks = _Clb_UnitDef_isLeaveTracks;
	sAICallback->Clb_UnitDef_getTrackWidth = _Clb_UnitDef_getTrackWidth;
	sAICallback->Clb_UnitDef_getTrackOffset = _Clb_UnitDef_getTrackOffset;
	sAICallback->Clb_UnitDef_getTrackStrength = _Clb_UnitDef_getTrackStrength;
	sAICallback->Clb_UnitDef_getTrackStretch = _Clb_UnitDef_getTrackStretch;
	sAICallback->Clb_UnitDef_getTrackType = _Clb_UnitDef_getTrackType;
	sAICallback->Clb_UnitDef_isAbleToDropFlare = _Clb_UnitDef_isAbleToDropFlare;
	sAICallback->Clb_UnitDef_getFlareReloadTime = _Clb_UnitDef_getFlareReloadTime;
	sAICallback->Clb_UnitDef_getFlareEfficiency = _Clb_UnitDef_getFlareEfficiency;
	sAICallback->Clb_UnitDef_getFlareDelay = _Clb_UnitDef_getFlareDelay;
	sAICallback->Clb_UnitDef_getFlareDropVector = _Clb_UnitDef_getFlareDropVector;
	sAICallback->Clb_UnitDef_getFlareTime = _Clb_UnitDef_getFlareTime;
	sAICallback->Clb_UnitDef_getFlareSalvoSize = _Clb_UnitDef_getFlareSalvoSize;
	sAICallback->Clb_UnitDef_getFlareSalvoDelay = _Clb_UnitDef_getFlareSalvoDelay;
	sAICallback->Clb_UnitDef_isAbleToLoopbackAttack = _Clb_UnitDef_isAbleToLoopbackAttack;
	sAICallback->Clb_UnitDef_isLevelGround = _Clb_UnitDef_isLevelGround;
	sAICallback->Clb_UnitDef_isUseBuildingGroundDecal = _Clb_UnitDef_isUseBuildingGroundDecal;
	sAICallback->Clb_UnitDef_getBuildingDecalType = _Clb_UnitDef_getBuildingDecalType;
	sAICallback->Clb_UnitDef_getBuildingDecalSizeX = _Clb_UnitDef_getBuildingDecalSizeX;
	sAICallback->Clb_UnitDef_getBuildingDecalSizeY = _Clb_UnitDef_getBuildingDecalSizeY;
	sAICallback->Clb_UnitDef_getBuildingDecalDecaySpeed = _Clb_UnitDef_getBuildingDecalDecaySpeed;
	sAICallback->Clb_UnitDef_getMaxFuel = _Clb_UnitDef_getMaxFuel;
	sAICallback->Clb_UnitDef_getRefuelTime = _Clb_UnitDef_getRefuelTime;
	sAICallback->Clb_UnitDef_getMinAirBasePower = _Clb_UnitDef_getMinAirBasePower;
	sAICallback->Clb_UnitDef_getMaxThisUnit = _Clb_UnitDef_getMaxThisUnit;
	sAICallback->Clb_UnitDef_0SINGLE1FETCH2UnitDef0getDecoyDef = _Clb_UnitDef_0SINGLE1FETCH2UnitDef0getDecoyDef;
	sAICallback->Clb_UnitDef_isDontLand = _Clb_UnitDef_isDontLand;
	sAICallback->Clb_UnitDef_0SINGLE1FETCH2WeaponDef0getShieldDef = _Clb_UnitDef_0SINGLE1FETCH2WeaponDef0getShieldDef;
	sAICallback->Clb_UnitDef_0SINGLE1FETCH2WeaponDef0getStockpileDef = _Clb_UnitDef_0SINGLE1FETCH2WeaponDef0getStockpileDef;
	sAICallback->Clb_UnitDef_0ARRAY1SIZE1UnitDef0getBuildOptions = _Clb_UnitDef_0ARRAY1SIZE1UnitDef0getBuildOptions;
	sAICallback->Clb_UnitDef_0ARRAY1VALS1UnitDef0getBuildOptions = _Clb_UnitDef_0ARRAY1VALS1UnitDef0getBuildOptions;
	sAICallback->Clb_UnitDef_0MAP1SIZE0getCustomParams = _Clb_UnitDef_0MAP1SIZE0getCustomParams;
	sAICallback->Clb_UnitDef_0MAP1KEYS0getCustomParams = _Clb_UnitDef_0MAP1KEYS0getCustomParams;
	sAICallback->Clb_UnitDef_0MAP1VALS0getCustomParams = _Clb_UnitDef_0MAP1VALS0getCustomParams;
	sAICallback->Clb_UnitDef_0AVAILABLE0MoveData = _Clb_UnitDef_0AVAILABLE0MoveData;
	sAICallback->Clb_UnitDef_MoveData_getMoveType = _Clb_UnitDef_MoveData_getMoveType;
	sAICallback->Clb_UnitDef_MoveData_getMoveFamily = _Clb_UnitDef_MoveData_getMoveFamily;
	sAICallback->Clb_UnitDef_MoveData_getSize = _Clb_UnitDef_MoveData_getSize;
	sAICallback->Clb_UnitDef_MoveData_getDepth = _Clb_UnitDef_MoveData_getDepth;
	sAICallback->Clb_UnitDef_MoveData_getMaxSlope = _Clb_UnitDef_MoveData_getMaxSlope;
	sAICallback->Clb_UnitDef_MoveData_getSlopeMod = _Clb_UnitDef_MoveData_getSlopeMod;
	sAICallback->Clb_UnitDef_MoveData_getDepthMod = _Clb_UnitDef_MoveData_getDepthMod;
	sAICallback->Clb_UnitDef_MoveData_getPathType = _Clb_UnitDef_MoveData_getPathType;
	sAICallback->Clb_UnitDef_MoveData_getCrushStrength = _Clb_UnitDef_MoveData_getCrushStrength;
	sAICallback->Clb_UnitDef_MoveData_getMaxSpeed = _Clb_UnitDef_MoveData_getMaxSpeed;
	sAICallback->Clb_UnitDef_MoveData_getMaxTurnRate = _Clb_UnitDef_MoveData_getMaxTurnRate;
	sAICallback->Clb_UnitDef_MoveData_getMaxAcceleration = _Clb_UnitDef_MoveData_getMaxAcceleration;
	sAICallback->Clb_UnitDef_MoveData_getMaxBreaking = _Clb_UnitDef_MoveData_getMaxBreaking;
	sAICallback->Clb_UnitDef_MoveData_isSubMarine = _Clb_UnitDef_MoveData_isSubMarine;
	sAICallback->Clb_UnitDef_0MULTI1SIZE0WeaponMount = _Clb_UnitDef_0MULTI1SIZE0WeaponMount;
	sAICallback->Clb_UnitDef_WeaponMount_getName = _Clb_UnitDef_WeaponMount_getName;
	sAICallback->Clb_UnitDef_WeaponMount_0SINGLE1FETCH2WeaponDef0getWeaponDef = _Clb_UnitDef_WeaponMount_0SINGLE1FETCH2WeaponDef0getWeaponDef;
	sAICallback->Clb_UnitDef_WeaponMount_getSlavedTo = _Clb_UnitDef_WeaponMount_getSlavedTo;
	sAICallback->Clb_UnitDef_WeaponMount_getMainDir = _Clb_UnitDef_WeaponMount_getMainDir;
	sAICallback->Clb_UnitDef_WeaponMount_getMaxAngleDif = _Clb_UnitDef_WeaponMount_getMaxAngleDif;
	sAICallback->Clb_UnitDef_WeaponMount_getFuelUsage = _Clb_UnitDef_WeaponMount_getFuelUsage;
	sAICallback->Clb_UnitDef_WeaponMount_getBadTargetCategory = _Clb_UnitDef_WeaponMount_getBadTargetCategory;
	sAICallback->Clb_UnitDef_WeaponMount_getOnlyTargetCategory = _Clb_UnitDef_WeaponMount_getOnlyTargetCategory;
	sAICallback->Clb_Unit_0STATIC0getLimit = _Clb_Unit_0STATIC0getLimit;
	sAICallback->Clb_0MULTI1SIZE3EnemyUnits0Unit = _Clb_0MULTI1SIZE3EnemyUnits0Unit;
	sAICallback->Clb_0MULTI1VALS3EnemyUnits0Unit = _Clb_0MULTI1VALS3EnemyUnits0Unit;
	sAICallback->Clb_0MULTI1SIZE3EnemyUnitsIn0Unit = _Clb_0MULTI1SIZE3EnemyUnitsIn0Unit;
	sAICallback->Clb_0MULTI1VALS3EnemyUnitsIn0Unit = _Clb_0MULTI1VALS3EnemyUnitsIn0Unit;
	sAICallback->Clb_0MULTI1SIZE3EnemyUnitsInRadarAndLos0Unit = _Clb_0MULTI1SIZE3EnemyUnitsInRadarAndLos0Unit;
	sAICallback->Clb_0MULTI1VALS3EnemyUnitsInRadarAndLos0Unit = _Clb_0MULTI1VALS3EnemyUnitsInRadarAndLos0Unit;
	sAICallback->Clb_0MULTI1SIZE3FriendlyUnits0Unit = _Clb_0MULTI1SIZE3FriendlyUnits0Unit;
	sAICallback->Clb_0MULTI1VALS3FriendlyUnits0Unit = _Clb_0MULTI1VALS3FriendlyUnits0Unit;
	sAICallback->Clb_0MULTI1SIZE3FriendlyUnitsIn0Unit = _Clb_0MULTI1SIZE3FriendlyUnitsIn0Unit;
	sAICallback->Clb_0MULTI1VALS3FriendlyUnitsIn0Unit = _Clb_0MULTI1VALS3FriendlyUnitsIn0Unit;
	sAICallback->Clb_0MULTI1SIZE3NeutralUnits0Unit = _Clb_0MULTI1SIZE3NeutralUnits0Unit;
	sAICallback->Clb_0MULTI1VALS3NeutralUnits0Unit = _Clb_0MULTI1VALS3NeutralUnits0Unit;
	sAICallback->Clb_0MULTI1SIZE3NeutralUnitsIn0Unit = _Clb_0MULTI1SIZE3NeutralUnitsIn0Unit;
	sAICallback->Clb_0MULTI1VALS3NeutralUnitsIn0Unit = _Clb_0MULTI1VALS3NeutralUnitsIn0Unit;
	sAICallback->Clb_0MULTI1SIZE3TeamUnits0Unit = _Clb_0MULTI1SIZE3TeamUnits0Unit;
	sAICallback->Clb_0MULTI1VALS3TeamUnits0Unit = _Clb_0MULTI1VALS3TeamUnits0Unit;
	sAICallback->Clb_0MULTI1SIZE3SelectedUnits0Unit = _Clb_0MULTI1SIZE3SelectedUnits0Unit;
	sAICallback->Clb_0MULTI1VALS3SelectedUnits0Unit = _Clb_0MULTI1VALS3SelectedUnits0Unit;
	sAICallback->Clb_Unit_0SINGLE1FETCH2UnitDef0getDef = _Clb_Unit_0SINGLE1FETCH2UnitDef0getDef;
	sAICallback->Clb_Unit_0MULTI1SIZE0ModParam = _Clb_Unit_0MULTI1SIZE0ModParam;
	sAICallback->Clb_Unit_ModParam_getName = _Clb_Unit_ModParam_getName;
	sAICallback->Clb_Unit_ModParam_getValue = _Clb_Unit_ModParam_getValue;
	sAICallback->Clb_Unit_getTeam = _Clb_Unit_getTeam;
	sAICallback->Clb_Unit_getAllyTeam = _Clb_Unit_getAllyTeam;
	sAICallback->Clb_Unit_getLineage = _Clb_Unit_getLineage;
	sAICallback->Clb_Unit_getAiHint = _Clb_Unit_getAiHint;
	sAICallback->Clb_Unit_getStockpile = _Clb_Unit_getStockpile;
	sAICallback->Clb_Unit_getStockpileQueued = _Clb_Unit_getStockpileQueued;
	sAICallback->Clb_Unit_getCurrentFuel = _Clb_Unit_getCurrentFuel;
	sAICallback->Clb_Unit_getMaxSpeed = _Clb_Unit_getMaxSpeed;
	sAICallback->Clb_Unit_getMaxRange = _Clb_Unit_getMaxRange;
	sAICallback->Clb_Unit_getMaxHealth = _Clb_Unit_getMaxHealth;
	sAICallback->Clb_Unit_getExperience = _Clb_Unit_getExperience;
	sAICallback->Clb_Unit_getGroup = _Clb_Unit_getGroup;
	sAICallback->Clb_Unit_0MULTI1SIZE1Command0CurrentCommand = _Clb_Unit_0MULTI1SIZE1Command0CurrentCommand;
	sAICallback->Clb_Unit_CurrentCommand_0STATIC0getType = _Clb_Unit_CurrentCommand_0STATIC0getType;
	sAICallback->Clb_Unit_CurrentCommand_getId = _Clb_Unit_CurrentCommand_getId;
	sAICallback->Clb_Unit_CurrentCommand_getOptions = _Clb_Unit_CurrentCommand_getOptions;
	sAICallback->Clb_Unit_CurrentCommand_getTag = _Clb_Unit_CurrentCommand_getTag;
	sAICallback->Clb_Unit_CurrentCommand_getTimeOut = _Clb_Unit_CurrentCommand_getTimeOut;
	sAICallback->Clb_Unit_CurrentCommand_0ARRAY1SIZE0getParams = _Clb_Unit_CurrentCommand_0ARRAY1SIZE0getParams;
	sAICallback->Clb_Unit_CurrentCommand_0ARRAY1VALS0getParams = _Clb_Unit_CurrentCommand_0ARRAY1VALS0getParams;
	sAICallback->Clb_Unit_0MULTI1SIZE0SupportedCommand = _Clb_Unit_0MULTI1SIZE0SupportedCommand;
	sAICallback->Clb_Unit_SupportedCommand_getId = _Clb_Unit_SupportedCommand_getId;
	sAICallback->Clb_Unit_SupportedCommand_getName = _Clb_Unit_SupportedCommand_getName;
	sAICallback->Clb_Unit_SupportedCommand_getToolTip = _Clb_Unit_SupportedCommand_getToolTip;
	sAICallback->Clb_Unit_SupportedCommand_isShowUnique = _Clb_Unit_SupportedCommand_isShowUnique;
	sAICallback->Clb_Unit_SupportedCommand_isDisabled = _Clb_Unit_SupportedCommand_isDisabled;
	sAICallback->Clb_Unit_SupportedCommand_0ARRAY1SIZE0getParams = _Clb_Unit_SupportedCommand_0ARRAY1SIZE0getParams;
	sAICallback->Clb_Unit_SupportedCommand_0ARRAY1VALS0getParams = _Clb_Unit_SupportedCommand_0ARRAY1VALS0getParams;
	sAICallback->Clb_Unit_getHealth = _Clb_Unit_getHealth;
	sAICallback->Clb_Unit_getSpeed = _Clb_Unit_getSpeed;
	sAICallback->Clb_Unit_getPower = _Clb_Unit_getPower;
	sAICallback->Clb_Unit_0MULTI1SIZE0ResourceInfo = _Clb_Unit_0MULTI1SIZE0ResourceInfo;
	sAICallback->Clb_Unit_0REF1Resource2resourceId0getResourceUse = _Clb_Unit_0REF1Resource2resourceId0getResourceUse;
	sAICallback->Clb_Unit_0REF1Resource2resourceId0getResourceMake = _Clb_Unit_0REF1Resource2resourceId0getResourceMake;
	sAICallback->Clb_Unit_getPos = _Clb_Unit_getPos;
	sAICallback->Clb_Unit_isActivated = _Clb_Unit_isActivated;
	sAICallback->Clb_Unit_isBeingBuilt = _Clb_Unit_isBeingBuilt;
	sAICallback->Clb_Unit_isCloaked = _Clb_Unit_isCloaked;
	sAICallback->Clb_Unit_isParalyzed = _Clb_Unit_isParalyzed;
	sAICallback->Clb_Unit_isNeutral = _Clb_Unit_isNeutral;
	sAICallback->Clb_Unit_getBuildingFacing = _Clb_Unit_getBuildingFacing;
	sAICallback->Clb_Unit_getLastUserOrderFrame = _Clb_Unit_getLastUserOrderFrame;
	sAICallback->Clb_0MULTI1SIZE0Group = _Clb_0MULTI1SIZE0Group;
	sAICallback->Clb_0MULTI1VALS0Group = _Clb_0MULTI1VALS0Group;
	sAICallback->Clb_Group_0MULTI1SIZE0SupportedCommand = _Clb_Group_0MULTI1SIZE0SupportedCommand;
	sAICallback->Clb_Group_SupportedCommand_getId = _Clb_Group_SupportedCommand_getId;
	sAICallback->Clb_Group_SupportedCommand_getName = _Clb_Group_SupportedCommand_getName;
	sAICallback->Clb_Group_SupportedCommand_getToolTip = _Clb_Group_SupportedCommand_getToolTip;
	sAICallback->Clb_Group_SupportedCommand_isShowUnique = _Clb_Group_SupportedCommand_isShowUnique;
	sAICallback->Clb_Group_SupportedCommand_isDisabled = _Clb_Group_SupportedCommand_isDisabled;
	sAICallback->Clb_Group_SupportedCommand_0ARRAY1SIZE0getParams = _Clb_Group_SupportedCommand_0ARRAY1SIZE0getParams;
	sAICallback->Clb_Group_SupportedCommand_0ARRAY1VALS0getParams = _Clb_Group_SupportedCommand_0ARRAY1VALS0getParams;
	sAICallback->Clb_Group_OrderPreview_getId = _Clb_Group_OrderPreview_getId;
	sAICallback->Clb_Group_OrderPreview_getOptions = _Clb_Group_OrderPreview_getOptions;
	sAICallback->Clb_Group_OrderPreview_getTag = _Clb_Group_OrderPreview_getTag;
	sAICallback->Clb_Group_OrderPreview_getTimeOut = _Clb_Group_OrderPreview_getTimeOut;
	sAICallback->Clb_Group_OrderPreview_0ARRAY1SIZE0getParams = _Clb_Group_OrderPreview_0ARRAY1SIZE0getParams;
	sAICallback->Clb_Group_OrderPreview_0ARRAY1VALS0getParams = _Clb_Group_OrderPreview_0ARRAY1VALS0getParams;
	sAICallback->Clb_Group_isSelected = _Clb_Group_isSelected;
	sAICallback->Clb_Mod_getName = _Clb_Mod_getName;
	sAICallback->Clb_Map_getChecksum = _Clb_Map_getChecksum;
	sAICallback->Clb_Map_getStartPos = _Clb_Map_getStartPos;
	sAICallback->Clb_Map_getMousePos = _Clb_Map_getMousePos;
	sAICallback->Clb_Map_isPosInCamera = _Clb_Map_isPosInCamera;
	sAICallback->Clb_Map_getWidth = _Clb_Map_getWidth;
	sAICallback->Clb_Map_getHeight = _Clb_Map_getHeight;
	sAICallback->Clb_Map_0ARRAY1SIZE0getHeightMap = _Clb_Map_0ARRAY1SIZE0getHeightMap;
	sAICallback->Clb_Map_0ARRAY1VALS0getHeightMap = _Clb_Map_0ARRAY1VALS0getHeightMap;
	sAICallback->Clb_Map_getMinHeight = _Clb_Map_getMinHeight;
	sAICallback->Clb_Map_getMaxHeight = _Clb_Map_getMaxHeight;
	sAICallback->Clb_Map_0ARRAY1SIZE0getSlopeMap = _Clb_Map_0ARRAY1SIZE0getSlopeMap;
	sAICallback->Clb_Map_0ARRAY1VALS0getSlopeMap = _Clb_Map_0ARRAY1VALS0getSlopeMap;
	sAICallback->Clb_Map_0ARRAY1SIZE0getLosMap = _Clb_Map_0ARRAY1SIZE0getLosMap;
	sAICallback->Clb_Map_0ARRAY1VALS0getLosMap = _Clb_Map_0ARRAY1VALS0getLosMap;
	sAICallback->Clb_Map_0ARRAY1SIZE0getRadarMap = _Clb_Map_0ARRAY1SIZE0getRadarMap;
	sAICallback->Clb_Map_0ARRAY1VALS0getRadarMap = _Clb_Map_0ARRAY1VALS0getRadarMap;
	sAICallback->Clb_Map_0ARRAY1SIZE0getJammerMap = _Clb_Map_0ARRAY1SIZE0getJammerMap;
	sAICallback->Clb_Map_0ARRAY1VALS0getJammerMap = _Clb_Map_0ARRAY1VALS0getJammerMap;
	sAICallback->Clb_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMap = _Clb_Map_0ARRAY1SIZE0REF1Resource2resourceId0getResourceMap;
	sAICallback->Clb_Map_0ARRAY1VALS0REF1Resource2resourceId0getResourceMap = _Clb_Map_0ARRAY1VALS0REF1Resource2resourceId0getResourceMap;
	sAICallback->Clb_Map_getName = _Clb_Map_getName;
	sAICallback->Clb_Map_getElevationAt = _Clb_Map_getElevationAt;
	sAICallback->Clb_Map_0REF1Resource2resourceId0getMaxResource = _Clb_Map_0REF1Resource2resourceId0getMaxResource;
	sAICallback->Clb_Map_0REF1Resource2resourceId0getExtractorRadius = _Clb_Map_0REF1Resource2resourceId0getExtractorRadius;
	sAICallback->Clb_Map_getMinWind = _Clb_Map_getMinWind;
	sAICallback->Clb_Map_getMaxWind = _Clb_Map_getMaxWind;
	sAICallback->Clb_Map_getTidalStrength = _Clb_Map_getTidalStrength;
	sAICallback->Clb_Map_getGravity = _Clb_Map_getGravity;
	sAICallback->Clb_Map_0MULTI1SIZE0Point = _Clb_Map_0MULTI1SIZE0Point;
	sAICallback->Clb_Map_Point_getPosition = _Clb_Map_Point_getPosition;
	sAICallback->Clb_Map_Point_getColor = _Clb_Map_Point_getColor;
	sAICallback->Clb_Map_Point_getLabel = _Clb_Map_Point_getLabel;
	sAICallback->Clb_Map_0MULTI1SIZE0Line = _Clb_Map_0MULTI1SIZE0Line;
	sAICallback->Clb_Map_Line_getFirstPosition = _Clb_Map_Line_getFirstPosition;
	sAICallback->Clb_Map_Line_getSecondPosition = _Clb_Map_Line_getSecondPosition;
	sAICallback->Clb_Map_Line_getColor = _Clb_Map_Line_getColor;
	sAICallback->Clb_Map_isPossibleToBuildAt = _Clb_Map_isPossibleToBuildAt;
	sAICallback->Clb_Map_findClosestBuildSite = _Clb_Map_findClosestBuildSite;
	sAICallback->Clb_0MULTI1SIZE0FeatureDef = _Clb_0MULTI1SIZE0FeatureDef;
	sAICallback->Clb_0MULTI1VALS0FeatureDef = _Clb_0MULTI1VALS0FeatureDef;
//	sAICallback->Clb_FeatureDef_getId = _Clb_FeatureDef_getId;
	sAICallback->Clb_FeatureDef_getName = _Clb_FeatureDef_getName;
	sAICallback->Clb_FeatureDef_getDescription = _Clb_FeatureDef_getDescription;
//	sAICallback->Clb_FeatureDef_getFileName = _Clb_FeatureDef_getFileName;
	sAICallback->Clb_FeatureDef_0REF1Resource2resourceId0getContainedResource = _Clb_FeatureDef_0REF1Resource2resourceId0getContainedResource;
	sAICallback->Clb_FeatureDef_getMaxHealth = _Clb_FeatureDef_getMaxHealth;
	sAICallback->Clb_FeatureDef_getReclaimTime = _Clb_FeatureDef_getReclaimTime;
	sAICallback->Clb_FeatureDef_getMass = _Clb_FeatureDef_getMass;
	sAICallback->Clb_FeatureDef_CollisionVolume_getType = _Clb_FeatureDef_CollisionVolume_getType;
	sAICallback->Clb_FeatureDef_CollisionVolume_getScales = _Clb_FeatureDef_CollisionVolume_getScales;
	sAICallback->Clb_FeatureDef_CollisionVolume_getOffsets = _Clb_FeatureDef_CollisionVolume_getOffsets;
	sAICallback->Clb_FeatureDef_CollisionVolume_getTest = _Clb_FeatureDef_CollisionVolume_getTest;
	sAICallback->Clb_FeatureDef_isUpright = _Clb_FeatureDef_isUpright;
	sAICallback->Clb_FeatureDef_getDrawType = _Clb_FeatureDef_getDrawType;
	sAICallback->Clb_FeatureDef_getModelName = _Clb_FeatureDef_getModelName;
	sAICallback->Clb_FeatureDef_getModelType = _Clb_FeatureDef_getModelType;
	sAICallback->Clb_FeatureDef_getResurrectable = _Clb_FeatureDef_getResurrectable;
	sAICallback->Clb_FeatureDef_getSmokeTime = _Clb_FeatureDef_getSmokeTime;
	sAICallback->Clb_FeatureDef_isDestructable = _Clb_FeatureDef_isDestructable;
	sAICallback->Clb_FeatureDef_isReclaimable = _Clb_FeatureDef_isReclaimable;
	sAICallback->Clb_FeatureDef_isBlocking = _Clb_FeatureDef_isBlocking;
	sAICallback->Clb_FeatureDef_isBurnable = _Clb_FeatureDef_isBurnable;
	sAICallback->Clb_FeatureDef_isFloating = _Clb_FeatureDef_isFloating;
	sAICallback->Clb_FeatureDef_isNoSelect = _Clb_FeatureDef_isNoSelect;
	sAICallback->Clb_FeatureDef_isGeoThermal = _Clb_FeatureDef_isGeoThermal;
	sAICallback->Clb_FeatureDef_getDeathFeature = _Clb_FeatureDef_getDeathFeature;
	sAICallback->Clb_FeatureDef_getXSize = _Clb_FeatureDef_getXSize;
	sAICallback->Clb_FeatureDef_getZSize = _Clb_FeatureDef_getZSize;
	sAICallback->Clb_FeatureDef_0MAP1SIZE0getCustomParams = _Clb_FeatureDef_0MAP1SIZE0getCustomParams;
	sAICallback->Clb_FeatureDef_0MAP1KEYS0getCustomParams = _Clb_FeatureDef_0MAP1KEYS0getCustomParams;
	sAICallback->Clb_FeatureDef_0MAP1VALS0getCustomParams = _Clb_FeatureDef_0MAP1VALS0getCustomParams;
	sAICallback->Clb_0MULTI1SIZE0Feature = _Clb_0MULTI1SIZE0Feature;
	sAICallback->Clb_0MULTI1VALS0Feature = _Clb_0MULTI1VALS0Feature;
	sAICallback->Clb_0MULTI1SIZE3FeaturesIn0Feature = _Clb_0MULTI1SIZE3FeaturesIn0Feature;
	sAICallback->Clb_0MULTI1VALS3FeaturesIn0Feature = _Clb_0MULTI1VALS3FeaturesIn0Feature;
	sAICallback->Clb_Feature_0SINGLE1FETCH2FeatureDef0getDef = _Clb_Feature_0SINGLE1FETCH2FeatureDef0getDef;
	sAICallback->Clb_Feature_getHealth = _Clb_Feature_getHealth;
	sAICallback->Clb_Feature_getReclaimLeft = _Clb_Feature_getReclaimLeft;
	sAICallback->Clb_Feature_getPosition = _Clb_Feature_getPosition;
	sAICallback->Clb_0MULTI1SIZE0WeaponDef = _Clb_0MULTI1SIZE0WeaponDef;
	sAICallback->Clb_0MULTI1FETCH3WeaponDefByName0WeaponDef = _Clb_0MULTI1FETCH3WeaponDefByName0WeaponDef;
	sAICallback->Clb_WeaponDef_getName = _Clb_WeaponDef_getName;
	sAICallback->Clb_WeaponDef_getType = _Clb_WeaponDef_getType;
	sAICallback->Clb_WeaponDef_getDescription = _Clb_WeaponDef_getDescription;
//	sAICallback->Clb_WeaponDef_getFileName = _Clb_WeaponDef_getFileName;
	sAICallback->Clb_WeaponDef_getRange = _Clb_WeaponDef_getRange;
	sAICallback->Clb_WeaponDef_getHeightMod = _Clb_WeaponDef_getHeightMod;
	sAICallback->Clb_WeaponDef_getAccuracy = _Clb_WeaponDef_getAccuracy;
	sAICallback->Clb_WeaponDef_getSprayAngle = _Clb_WeaponDef_getSprayAngle;
	sAICallback->Clb_WeaponDef_getMovingAccuracy = _Clb_WeaponDef_getMovingAccuracy;
	sAICallback->Clb_WeaponDef_getTargetMoveError = _Clb_WeaponDef_getTargetMoveError;
	sAICallback->Clb_WeaponDef_getLeadLimit = _Clb_WeaponDef_getLeadLimit;
	sAICallback->Clb_WeaponDef_getLeadBonus = _Clb_WeaponDef_getLeadBonus;
	sAICallback->Clb_WeaponDef_getPredictBoost = _Clb_WeaponDef_getPredictBoost;
	sAICallback->Clb_WeaponDef_0STATIC0getNumDamageTypes = _Clb_WeaponDef_0STATIC0getNumDamageTypes;
	sAICallback->Clb_WeaponDef_Damage_getParalyzeDamageTime = _Clb_WeaponDef_Damage_getParalyzeDamageTime;
	sAICallback->Clb_WeaponDef_Damage_getImpulseFactor = _Clb_WeaponDef_Damage_getImpulseFactor;
	sAICallback->Clb_WeaponDef_Damage_getImpulseBoost = _Clb_WeaponDef_Damage_getImpulseBoost;
	sAICallback->Clb_WeaponDef_Damage_getCraterMult = _Clb_WeaponDef_Damage_getCraterMult;
	sAICallback->Clb_WeaponDef_Damage_getCraterBoost = _Clb_WeaponDef_Damage_getCraterBoost;
	sAICallback->Clb_WeaponDef_Damage_0ARRAY1SIZE0getTypes = _Clb_WeaponDef_Damage_0ARRAY1SIZE0getTypes;
	sAICallback->Clb_WeaponDef_Damage_0ARRAY1VALS0getTypes = _Clb_WeaponDef_Damage_0ARRAY1VALS0getTypes;
//	sAICallback->Clb_WeaponDef_getId = _Clb_WeaponDef_getId;
	sAICallback->Clb_WeaponDef_getAreaOfEffect = _Clb_WeaponDef_getAreaOfEffect;
	sAICallback->Clb_WeaponDef_isNoSelfDamage = _Clb_WeaponDef_isNoSelfDamage;
	sAICallback->Clb_WeaponDef_getFireStarter = _Clb_WeaponDef_getFireStarter;
	sAICallback->Clb_WeaponDef_getEdgeEffectiveness = _Clb_WeaponDef_getEdgeEffectiveness;
	sAICallback->Clb_WeaponDef_getSize = _Clb_WeaponDef_getSize;
	sAICallback->Clb_WeaponDef_getSizeGrowth = _Clb_WeaponDef_getSizeGrowth;
	sAICallback->Clb_WeaponDef_getCollisionSize = _Clb_WeaponDef_getCollisionSize;
	sAICallback->Clb_WeaponDef_getSalvoSize = _Clb_WeaponDef_getSalvoSize;
	sAICallback->Clb_WeaponDef_getSalvoDelay = _Clb_WeaponDef_getSalvoDelay;
	sAICallback->Clb_WeaponDef_getReload = _Clb_WeaponDef_getReload;
	sAICallback->Clb_WeaponDef_getBeamTime = _Clb_WeaponDef_getBeamTime;
	sAICallback->Clb_WeaponDef_isBeamBurst = _Clb_WeaponDef_isBeamBurst;
	sAICallback->Clb_WeaponDef_isWaterBounce = _Clb_WeaponDef_isWaterBounce;
	sAICallback->Clb_WeaponDef_isGroundBounce = _Clb_WeaponDef_isGroundBounce;
	sAICallback->Clb_WeaponDef_getBounceRebound = _Clb_WeaponDef_getBounceRebound;
	sAICallback->Clb_WeaponDef_getBounceSlip = _Clb_WeaponDef_getBounceSlip;
	sAICallback->Clb_WeaponDef_getNumBounce = _Clb_WeaponDef_getNumBounce;
	sAICallback->Clb_WeaponDef_getMaxAngle = _Clb_WeaponDef_getMaxAngle;
	sAICallback->Clb_WeaponDef_getRestTime = _Clb_WeaponDef_getRestTime;
	sAICallback->Clb_WeaponDef_getUpTime = _Clb_WeaponDef_getUpTime;
	sAICallback->Clb_WeaponDef_getFlightTime = _Clb_WeaponDef_getFlightTime;
	sAICallback->Clb_WeaponDef_0REF1Resource2resourceId0getCost = _Clb_WeaponDef_0REF1Resource2resourceId0getCost;
	sAICallback->Clb_WeaponDef_getSupplyCost = _Clb_WeaponDef_getSupplyCost;
	sAICallback->Clb_WeaponDef_getProjectilesPerShot = _Clb_WeaponDef_getProjectilesPerShot;
	sAICallback->Clb_WeaponDef_isTurret = _Clb_WeaponDef_isTurret;
	sAICallback->Clb_WeaponDef_isOnlyForward = _Clb_WeaponDef_isOnlyForward;
	sAICallback->Clb_WeaponDef_isFixedLauncher = _Clb_WeaponDef_isFixedLauncher;
	sAICallback->Clb_WeaponDef_isWaterWeapon = _Clb_WeaponDef_isWaterWeapon;
	sAICallback->Clb_WeaponDef_isFireSubmersed = _Clb_WeaponDef_isFireSubmersed;
	sAICallback->Clb_WeaponDef_isSubMissile = _Clb_WeaponDef_isSubMissile;
	sAICallback->Clb_WeaponDef_isTracks = _Clb_WeaponDef_isTracks;
	sAICallback->Clb_WeaponDef_isDropped = _Clb_WeaponDef_isDropped;
	sAICallback->Clb_WeaponDef_isParalyzer = _Clb_WeaponDef_isParalyzer;
	sAICallback->Clb_WeaponDef_isImpactOnly = _Clb_WeaponDef_isImpactOnly;
	sAICallback->Clb_WeaponDef_isNoAutoTarget = _Clb_WeaponDef_isNoAutoTarget;
	sAICallback->Clb_WeaponDef_isManualFire = _Clb_WeaponDef_isManualFire;
	sAICallback->Clb_WeaponDef_getInterceptor = _Clb_WeaponDef_getInterceptor;
	sAICallback->Clb_WeaponDef_getTargetable = _Clb_WeaponDef_getTargetable;
	sAICallback->Clb_WeaponDef_isStockpileable = _Clb_WeaponDef_isStockpileable;
	sAICallback->Clb_WeaponDef_getCoverageRange = _Clb_WeaponDef_getCoverageRange;
	sAICallback->Clb_WeaponDef_getStockpileTime = _Clb_WeaponDef_getStockpileTime;
	sAICallback->Clb_WeaponDef_getIntensity = _Clb_WeaponDef_getIntensity;
	sAICallback->Clb_WeaponDef_getThickness = _Clb_WeaponDef_getThickness;
	sAICallback->Clb_WeaponDef_getLaserFlareSize = _Clb_WeaponDef_getLaserFlareSize;
	sAICallback->Clb_WeaponDef_getCoreThickness = _Clb_WeaponDef_getCoreThickness;
	sAICallback->Clb_WeaponDef_getDuration = _Clb_WeaponDef_getDuration;
	sAICallback->Clb_WeaponDef_getLodDistance = _Clb_WeaponDef_getLodDistance;
	sAICallback->Clb_WeaponDef_getFalloffRate = _Clb_WeaponDef_getFalloffRate;
	sAICallback->Clb_WeaponDef_getGraphicsType = _Clb_WeaponDef_getGraphicsType;
	sAICallback->Clb_WeaponDef_isSoundTrigger = _Clb_WeaponDef_isSoundTrigger;
	sAICallback->Clb_WeaponDef_isSelfExplode = _Clb_WeaponDef_isSelfExplode;
	sAICallback->Clb_WeaponDef_isGravityAffected = _Clb_WeaponDef_isGravityAffected;
	sAICallback->Clb_WeaponDef_getHighTrajectory = _Clb_WeaponDef_getHighTrajectory;
	sAICallback->Clb_WeaponDef_getMyGravity = _Clb_WeaponDef_getMyGravity;
	sAICallback->Clb_WeaponDef_isNoExplode = _Clb_WeaponDef_isNoExplode;
	sAICallback->Clb_WeaponDef_getStartVelocity = _Clb_WeaponDef_getStartVelocity;
	sAICallback->Clb_WeaponDef_getWeaponAcceleration = _Clb_WeaponDef_getWeaponAcceleration;
	sAICallback->Clb_WeaponDef_getTurnRate = _Clb_WeaponDef_getTurnRate;
	sAICallback->Clb_WeaponDef_getMaxVelocity = _Clb_WeaponDef_getMaxVelocity;
	sAICallback->Clb_WeaponDef_getProjectileSpeed = _Clb_WeaponDef_getProjectileSpeed;
	sAICallback->Clb_WeaponDef_getExplosionSpeed = _Clb_WeaponDef_getExplosionSpeed;
	sAICallback->Clb_WeaponDef_getOnlyTargetCategory = _Clb_WeaponDef_getOnlyTargetCategory;
	sAICallback->Clb_WeaponDef_getWobble = _Clb_WeaponDef_getWobble;
	sAICallback->Clb_WeaponDef_getDance = _Clb_WeaponDef_getDance;
	sAICallback->Clb_WeaponDef_getTrajectoryHeight = _Clb_WeaponDef_getTrajectoryHeight;
	sAICallback->Clb_WeaponDef_isLargeBeamLaser = _Clb_WeaponDef_isLargeBeamLaser;
	sAICallback->Clb_WeaponDef_isShield = _Clb_WeaponDef_isShield;
	sAICallback->Clb_WeaponDef_isShieldRepulser = _Clb_WeaponDef_isShieldRepulser;
	sAICallback->Clb_WeaponDef_isSmartShield = _Clb_WeaponDef_isSmartShield;
	sAICallback->Clb_WeaponDef_isExteriorShield = _Clb_WeaponDef_isExteriorShield;
	sAICallback->Clb_WeaponDef_isVisibleShield = _Clb_WeaponDef_isVisibleShield;
	sAICallback->Clb_WeaponDef_isVisibleShieldRepulse = _Clb_WeaponDef_isVisibleShieldRepulse;
	sAICallback->Clb_WeaponDef_getVisibleShieldHitFrames = _Clb_WeaponDef_getVisibleShieldHitFrames;
	sAICallback->Clb_WeaponDef_Shield_0REF1Resource2resourceId0getResourceUse = _Clb_WeaponDef_Shield_0REF1Resource2resourceId0getResourceUse;
	sAICallback->Clb_WeaponDef_Shield_getRadius = _Clb_WeaponDef_Shield_getRadius;
	sAICallback->Clb_WeaponDef_Shield_getForce = _Clb_WeaponDef_Shield_getForce;
	sAICallback->Clb_WeaponDef_Shield_getMaxSpeed = _Clb_WeaponDef_Shield_getMaxSpeed;
	sAICallback->Clb_WeaponDef_Shield_getPower = _Clb_WeaponDef_Shield_getPower;
	sAICallback->Clb_WeaponDef_Shield_getPowerRegen = _Clb_WeaponDef_Shield_getPowerRegen;
	sAICallback->Clb_WeaponDef_Shield_0REF1Resource2resourceId0getPowerRegenResource = _Clb_WeaponDef_Shield_0REF1Resource2resourceId0getPowerRegenResource;
	sAICallback->Clb_WeaponDef_Shield_getStartingPower = _Clb_WeaponDef_Shield_getStartingPower;
	sAICallback->Clb_WeaponDef_Shield_getRechargeDelay = _Clb_WeaponDef_Shield_getRechargeDelay;
	sAICallback->Clb_WeaponDef_Shield_getGoodColor = _Clb_WeaponDef_Shield_getGoodColor;
	sAICallback->Clb_WeaponDef_Shield_getBadColor = _Clb_WeaponDef_Shield_getBadColor;
	sAICallback->Clb_WeaponDef_Shield_getAlpha = _Clb_WeaponDef_Shield_getAlpha;
	sAICallback->Clb_WeaponDef_Shield_getInterceptType = _Clb_WeaponDef_Shield_getInterceptType;
	sAICallback->Clb_WeaponDef_getInterceptedByShieldType = _Clb_WeaponDef_getInterceptedByShieldType;
	sAICallback->Clb_WeaponDef_isAvoidFriendly = _Clb_WeaponDef_isAvoidFriendly;
	sAICallback->Clb_WeaponDef_isAvoidFeature = _Clb_WeaponDef_isAvoidFeature;
	sAICallback->Clb_WeaponDef_isAvoidNeutral = _Clb_WeaponDef_isAvoidNeutral;
	sAICallback->Clb_WeaponDef_getTargetBorder = _Clb_WeaponDef_getTargetBorder;
	sAICallback->Clb_WeaponDef_getCylinderTargetting = _Clb_WeaponDef_getCylinderTargetting;
	sAICallback->Clb_WeaponDef_getMinIntensity = _Clb_WeaponDef_getMinIntensity;
	sAICallback->Clb_WeaponDef_getHeightBoostFactor = _Clb_WeaponDef_getHeightBoostFactor;
	sAICallback->Clb_WeaponDef_getProximityPriority = _Clb_WeaponDef_getProximityPriority;
	sAICallback->Clb_WeaponDef_getCollisionFlags = _Clb_WeaponDef_getCollisionFlags;
	sAICallback->Clb_WeaponDef_isSweepFire = _Clb_WeaponDef_isSweepFire;
	sAICallback->Clb_WeaponDef_isAbleToAttackGround = _Clb_WeaponDef_isAbleToAttackGround;
	sAICallback->Clb_WeaponDef_getCameraShake = _Clb_WeaponDef_getCameraShake;
	sAICallback->Clb_WeaponDef_getDynDamageExp = _Clb_WeaponDef_getDynDamageExp;
	sAICallback->Clb_WeaponDef_getDynDamageMin = _Clb_WeaponDef_getDynDamageMin;
	sAICallback->Clb_WeaponDef_getDynDamageRange = _Clb_WeaponDef_getDynDamageRange;
	sAICallback->Clb_WeaponDef_isDynDamageInverted = _Clb_WeaponDef_isDynDamageInverted;
	sAICallback->Clb_WeaponDef_0MAP1SIZE0getCustomParams = _Clb_WeaponDef_0MAP1SIZE0getCustomParams;
	sAICallback->Clb_WeaponDef_0MAP1KEYS0getCustomParams = _Clb_WeaponDef_0MAP1KEYS0getCustomParams;
	sAICallback->Clb_WeaponDef_0MAP1VALS0getCustomParams = _Clb_WeaponDef_0MAP1VALS0getCustomParams;
	// ... TILL HERE

	team_globalCallback[teamId] = aiGlobalCallback;
//	team_callback[teamId] = aiCallback;
	team_callback[teamId] = aiGlobalCallback->GetAICallback();

	return sAICallback;
}
