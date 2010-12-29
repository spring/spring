/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ExternalAI/AICallback.h"
#include "ExternalAI/AICheats.h"
#include "ExternalAI/IAILibraryManager.h"
#include "ExternalAI/SSkirmishAICallbackImpl.h"
#include "ExternalAI/SkirmishAILibraryInfo.h"
#include "ExternalAI/SAIInterfaceCallbackImpl.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "ExternalAI/Interface/AISCommands.h"
#include "ExternalAI/Interface/SSkirmishAICallback.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Units/Groups/GroupHandler.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Misc/CategoryHandler.h"
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
#include "Map/MapInfo.h"
#include "Game/SelectedUnits.h"
#include "Game/UI/GuiHandler.h" //TODO: fix some switch for new gui
#include "Game/GameSetup.h"
#include "Game/GameVersion.h"
#include "Lua/LuaRulesParams.h"
#include "FileSystem/ArchiveScanner.h"
#include "GlobalUnsynced.h" // for myTeam
#include "LogOutput.h"


static const char* SKIRMISH_AIS_VERSION_COMMON = "common";

static std::map<int, CAICallback*>         skirmishAIId_callback;
static std::map<int, CAICheats*>           skirmishAIId_cheatCallback;
static std::map<int, SSkirmishAICallback*> skirmishAIId_cCallback;
static std::map<int, bool>                 skirmishAIId_cheatingEnabled;
static std::map<int, bool>                 skirmishAIId_usesCheats;
static std::map<int, int>                  skirmishAIId_teamId;

static const size_t TMP_ARR_SIZE = 16384;
// FIXME: the following lines are relatively memory intensive (~1MB per line)
// this memory is only freed at exit of the application
// There is quite some CPU and Memory performance waste present
// in them and their use.
static int         tmpIntArr[MAX_SKIRMISH_AIS][TMP_ARR_SIZE];
//static PointMarker tmpPointMarkerArr[MAX_SKIRMISH_AIS][TMP_ARR_SIZE];
//static LineMarker  tmpLineMarkerArr[MAX_SKIRMISH_AIS][TMP_ARR_SIZE];

/*
 * FIXME: get rid of this and replace with std::vectors that are
 *        automatically grown to the needed size?
 *
 * GCC 4.4.1 on Ubuntu 9.10 segfaults during static initialization of the
 * tmpPointMarkerArr and tmpLineMarkerArr above. This seems to have to do
 * with the size of these arrays. (Bigger crashes more often, but even
 * TMP_ARR_SIZE=1024 still crashes occasionally.)
 *
 * As a workaround, we change the vars to be a reference to a 2d array,
 * and allocate the actual array on the heap.
 */
static PointMarker (&tmpPointMarkerArr)[MAX_SKIRMISH_AIS][TMP_ARR_SIZE] =
		*(PointMarker (*)[MAX_SKIRMISH_AIS][TMP_ARR_SIZE]) calloc(MAX_SKIRMISH_AIS * TMP_ARR_SIZE, sizeof(PointMarker));
static LineMarker (&tmpLineMarkerArr)[MAX_SKIRMISH_AIS][TMP_ARR_SIZE] =
		*(LineMarker (*)[MAX_SKIRMISH_AIS][TMP_ARR_SIZE]) calloc(MAX_SKIRMISH_AIS * TMP_ARR_SIZE, sizeof(LineMarker));

static void checkSkirmishAIId(int skirmishAIId) {

	if ((skirmishAIId < 0) || (skirmishAIId_cCallback.find(static_cast<size_t>(skirmishAIId)) == skirmishAIId_cCallback.end())) {
		const static size_t skirmishAIIdError_maxSize = 512;
		char skirmishAIIdError[skirmishAIIdError_maxSize];
		SNPRINTF(skirmishAIIdError, skirmishAIIdError_maxSize,
				"Bad team ID supplied by a Skirmish AI.\n"
				"Is %i, but should be between min %i and max %u.",
				skirmishAIId, 0, MAX_SKIRMISH_AIS);
		// log exception to the engine and die
		skirmishAiCallback_Log_exception(skirmishAIId, skirmishAIIdError, 1, true);
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

static void toFloatArr(const short color[3], const short alpha, float arrColor[4]) {
	arrColor[0] = color[0] / 256.0f;
	arrColor[1] = color[1] / 256.0f;
	arrColor[2] = color[2] / 256.0f;
	arrColor[3] = alpha / 256.0f;
}

static void fillVector(std::vector<int>* vector_unitIds, int* unitIds,
		int numUnitIds) {
	for (int i=0; i < numUnitIds; ++i) {
		vector_unitIds->push_back(unitIds[i]);
	}
}

static bool isControlledByLocalPlayer(int skirmishAIId) {
	return gu->myTeam == skirmishAIId_teamId[skirmishAIId];
}


static const CUnit* getUnit(int unitId) {

	if (unitId < MAX_UNITS) {
		return uh->units[unitId];
	} else {
		return NULL;
	}
}

static bool isAlliedUnit(int skirmishAIId, const CUnit* unit) {
	return teamHandler->AlliedTeams(unit->team, skirmishAIId_teamId[skirmishAIId]);
}

static inline bool unitModParamIsValidId(const CUnit& unit, int modParamId) {
	return ((size_t)modParamId < unit.modParams.size());
}

static bool unitModParamIsVisible(int skirmishAIId, const CUnit& unit,
		int modParamId)
{
	const int teamId = skirmishAIId_teamId[skirmishAIId];

	if (unitModParamIsValidId(unit, modParamId)) {
		const int allyID = teamHandler->AllyTeam(teamId);
		const int& losStatus = unit.losStatus[allyID];

		int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

		if (isAlliedUnit(skirmishAIId, &unit)
				|| skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
			losMask |= LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
		} else if (teamHandler->AlliedTeams(unit.team, teamId)) {
			// ingame alliances
			losMask |= LuaRulesParams::RULESPARAMLOS_ALLIED_MASK;
		} else if (losStatus & LOS_INLOS) {
			losMask |= LuaRulesParams::RULESPARAMLOS_INLOS_MASK;
		} else if (losStatus & LOS_INRADAR) {
			losMask |= LuaRulesParams::RULESPARAMLOS_INRADAR_MASK;
		}

		return ((unit.modParams[modParamId].los & losMask) > 0);
	}

	return false;
}

static inline const UnitDef* getUnitDefById(int skirmishAIId, int unitDefId) {

	const UnitDef* unitDef = unitDefHandler->GetUnitDefByID(unitDefId);
	assert(unitDef != NULL);
	return unitDef;
}

static inline const MoveData* getUnitDefMoveDataById(int skirmishAIId, int unitDefId) {

	const MoveData* moveData = getUnitDefById(skirmishAIId, unitDefId)->movedata;
	assert(moveData != NULL); // There is a callback method to check whether MKoveData is available, use it.
	return moveData;
}

static inline const WeaponDef* getWeaponDefById(int skirmishAIId, int weaponDefId) {

	const WeaponDef* weaponDef = weaponDefHandler->GetWeaponById(weaponDefId);
	assert(weaponDef != NULL);
	return weaponDef;
}

static inline const FeatureDef* getFeatureDefById(int skirmishAIId, int featureDefId) {

	const FeatureDef* featureDef = featureHandler->GetFeatureDefByID(featureDefId);
	assert(featureDef != NULL);
	return featureDef;
}

static int wrapper_HandleCommand(CAICallback* clb, CAICheats* clbCheat,
		int cmdId, void* cmdData) {

	int ret;

	if (clbCheat != NULL) {
		ret = clbCheat->HandleCommand(cmdId, cmdData);
	} else {
		ret = clb->HandleCommand(cmdId, cmdData);
	}

	return ret;
}

EXPORT(int) skirmishAiCallback_Engine_handleCommand(int skirmishAIId, int toId, int commandId,
		int commandTopic, void* commandData) {

	int ret = 0;

	CAICallback* clb = skirmishAIId_callback[skirmishAIId];
	// if this is != NULL, cheating is enabled
	CAICheats* clbCheat = NULL;
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		clbCheat = skirmishAIId_cheatCallback[skirmishAIId];
	}


	switch (commandTopic) {

		case COMMAND_CHEATS_SET_MY_INCOME_MULTIPLIER:
		{
			const SSetMyIncomeMultiplierCheatCommand* cmd =
					(SSetMyIncomeMultiplierCheatCommand*) commandData;
			if (clbCheat != NULL) {
				clbCheat->SetMyIncomeMultiplier(cmd->factor);
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
				cmd->ret_newUnitId = clbCheat->CreateUnit(getUnitDefById(skirmishAIId,
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
					cmd->end_posF3, cmd->pathType, cmd->goalRadius);
			break;
		}
		case COMMAND_PATH_GET_APPROXIMATE_LENGTH:
		{
			SGetApproximateLengthPathCommand* cmd =
					(SGetApproximateLengthPathCommand*) commandData;
			cmd->ret_approximatePathLength =
					clb->GetPathLength(cmd->start_posF3, cmd->end_posF3,
							cmd->pathType, cmd->goalRadius);
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
			// TODO: FIXME: should strcpy() this
			cmd->ret_outData = clb->CallLuaRules(cmd->data, cmd->inSize);
			break;
		}


		case COMMAND_DRAWER_ADD_NOTIFICATION:
		{
			const SAddNotificationDrawerCommand* cmd =
					(SAddNotificationDrawerCommand*) commandData;
			clb->AddNotification(cmd->pos_posF3,
					float3(
						cmd->color_colorS3[0] / 256.0f,
						cmd->color_colorS3[1] / 256.0f,
						cmd->color_colorS3[2] / 256.0f),
					cmd->alpha / 256.0f);
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
					cmd->color_colorS3[0] / 256.0f,
					cmd->color_colorS3[1] / 256.0f,
					cmd->color_colorS3[2] / 256.0f,
					cmd->alpha / 256.0f);
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
			clb->DrawUnit(getUnitDefById(skirmishAIId,
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
				cCmdData->srcUnitId,
				cCmdData->ret_hitUnitId,
				cCmdData->flags
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCTraceRayId, &cppCmdData);

			cCmdData->rayLen = cppCmdData.rayLen;
			cCmdData->ret_hitUnitId = cppCmdData.hitUID;
		} break;

		case COMMAND_TRACE_RAY_FEATURE: {
			SFeatureTraceRayCommand* cCmdData = (SFeatureTraceRayCommand*) commandData;
			AIHCFeatureTraceRay cppCmdData = {
				cCmdData->rayPos_posF3,
				cCmdData->rayDir_posF3,
				cCmdData->rayLen,
				cCmdData->srcUnitId,
				cCmdData->ret_hitFeatureId,
				cCmdData->flags
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCFeatureTraceRayId, &cppCmdData);

			cCmdData->rayLen = cppCmdData.rayLen;
			cCmdData->ret_hitFeatureId = cppCmdData.hitFID;
		} break;

		case COMMAND_PAUSE: {
			const SPauseCommand* cmd = (SPauseCommand*) commandData;
			AIHCPause cppCmdData = {
				cmd->enable,
				cmd->reason
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCPauseId, &cppCmdData);
		} break;

		case COMMAND_DEBUG_DRAWER_GRAPH_LINE_ADD_POINT: {
			SAddPointLineGraphDrawerDebugCommand* cCmdData = (SAddPointLineGraphDrawerDebugCommand*) commandData;
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_ADD_GRAPH_POINT,
				cCmdData->x, cCmdData->y,
				0.0f, 0.0f,
				cCmdData->lineId, 0,
				ZeroVector,
				"",
				0, NULL
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;
		case COMMAND_DEBUG_DRAWER_GRAPH_LINE_DELETE_POINTS: {
			SDeletePointsLineGraphDrawerDebugCommand* cCmdData = (SDeletePointsLineGraphDrawerDebugCommand*) commandData;
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_DEL_GRAPH_POINTS,
				0.0f, 0.0f,
				0.0f, 0.0f,
				cCmdData->lineId, cCmdData->numPoints,
				ZeroVector,
				"",
				0, NULL
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;
		case COMMAND_DEBUG_DRAWER_GRAPH_SET_POS: {
			SSetPositionGraphDrawerDebugCommand* cCmdData = (SSetPositionGraphDrawerDebugCommand*) commandData;
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_GRAPH_POS,
				cCmdData->x, cCmdData->y,
				0.0f, 0.0f,
				0, 0,
				ZeroVector,
				"",
				0, NULL
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;
		case COMMAND_DEBUG_DRAWER_GRAPH_SET_SIZE: {
			SSetSizeGraphDrawerDebugCommand* cCmdData = (SSetSizeGraphDrawerDebugCommand*) commandData;
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_GRAPH_SIZE,
				0.0f, 0.0f,
				cCmdData->w, cCmdData->h,
				0, 0,
				ZeroVector,
				"",
				0, NULL
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;
		case COMMAND_DEBUG_DRAWER_GRAPH_LINE_SET_COLOR: {
			SSetColorLineGraphDrawerDebugCommand* cCmdData = (SSetColorLineGraphDrawerDebugCommand*) commandData;
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_GRAPH_LINE_COLOR,
				0.0f, 0.0f,
				0.0f, 0.0f,
				cCmdData->lineId, 0,
				float3(cCmdData->color_colorS3[0] / 256.0f, cCmdData->color_colorS3[1] / 256.0f, cCmdData->color_colorS3[2] / 256.0f),
				"",
				0, NULL
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;
		case COMMAND_DEBUG_DRAWER_GRAPH_LINE_SET_LABEL: {
			SSetLabelLineGraphDrawerDebugCommand* cCmdData = (SSetLabelLineGraphDrawerDebugCommand*) commandData;
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_GRAPH_LINE_LABEL,
				0.0f, 0.0f,
				0.0f, 0.0f,
				cCmdData->lineId, 0,
				ZeroVector,
				std::string(cCmdData->label),
				0, NULL
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;


		case COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_ADD: {
			SAddOverlayTextureDrawerDebugCommand* cCmdData = (SAddOverlayTextureDrawerDebugCommand*) commandData;
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_ADD_OVERLAY_TEXTURE,
				0.0f, 0.0f,
				cCmdData->w, cCmdData->h,
				0, 0,
				ZeroVector,
				"",
				0, cCmdData->texData
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);

			cCmdData->ret_overlayTextureId = cppCmdData.texHandle;
		} break;
		case COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_UPDATE: {
			SUpdateOverlayTextureDrawerDebugCommand* cCmdData = (SUpdateOverlayTextureDrawerDebugCommand*) commandData;
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_UPDATE_OVERLAY_TEXTURE,
				cCmdData->x, cCmdData->y,
				cCmdData->w, cCmdData->h,
				0, 0,
				ZeroVector,
				"",
				cCmdData->overlayTextureId, cCmdData->texData
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;

		case COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_DELETE: {
			SDeleteOverlayTextureDrawerDebugCommand* cCmdData = (SDeleteOverlayTextureDrawerDebugCommand*) commandData;
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_DEL_OVERLAY_TEXTURE,
				0.0f, 0.0f,
				0.0f, 0.0f,
				0, 0,
				ZeroVector,
				"",
				cCmdData->overlayTextureId, NULL
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;
		case COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_POS: {
			SSetPositionOverlayTextureDrawerDebugCommand* cCmdData = (SSetPositionOverlayTextureDrawerDebugCommand*) commandData;
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_OVERLAY_TEXTURE_POS,
				cCmdData->x, cCmdData->y,
				0.0f, 0.0f,
				0, 0,
				ZeroVector,
				"",
				cCmdData->overlayTextureId, NULL
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;
		case COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_SIZE: {
			SSetSizeOverlayTextureDrawerDebugCommand* cCmdData = (SSetSizeOverlayTextureDrawerDebugCommand*) commandData;
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_OVERLAY_TEXTURE_SIZE,
				0.0f, 0.0f,
				cCmdData->w, cCmdData->h,
				0, 0,
				ZeroVector,
				"",
				cCmdData->overlayTextureId, NULL
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;
		case COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_LABEL: {
			SSetLabelOverlayTextureDrawerDebugCommand* cCmdData = (SSetLabelOverlayTextureDrawerDebugCommand*) commandData;
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_OVERLAY_TEXTURE_LABEL,
				0.0f, 0.0f,
				0.0f, 0.0f,
				0, 0,
				ZeroVector,
				std::string(cCmdData->label),
				cCmdData->overlayTextureId, NULL
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;


		default: {
			// check if it is a unit command
			Command* c = (Command*) newCommand(commandData, commandTopic, uh->MaxUnits());
			if (c != NULL) { // it is a unit command
				c->aiCommandId = commandId;
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


EXPORT(const char*) skirmishAiCallback_Engine_Version_getMajor(int skirmishAIId) {
	return aiInterfaceCallback_Engine_Version_getMajor(-1);
}

EXPORT(const char*) skirmishAiCallback_Engine_Version_getMinor(int skirmishAIId) {
	return aiInterfaceCallback_Engine_Version_getMinor(-1);
}

EXPORT(const char*) skirmishAiCallback_Engine_Version_getPatchset(int skirmishAIId) {
	return aiInterfaceCallback_Engine_Version_getPatchset(-1);
}

EXPORT(const char*) skirmishAiCallback_Engine_Version_getAdditional(int skirmishAIId) {
	return aiInterfaceCallback_Engine_Version_getAdditional(-1);
}

EXPORT(const char*) skirmishAiCallback_Engine_Version_getBuildTime(int skirmishAIId) {
	return aiInterfaceCallback_Engine_Version_getBuildTime(-1);
}

EXPORT(const char*) skirmishAiCallback_Engine_Version_getNormal(int skirmishAIId) {
	return aiInterfaceCallback_Engine_Version_getNormal(-1);
}

EXPORT(const char*) skirmishAiCallback_Engine_Version_getFull(int skirmishAIId) {
	return aiInterfaceCallback_Engine_Version_getFull(-1);
}

EXPORT(int) skirmishAiCallback_Teams_getSize(int skirmishAIId) {
	return teamHandler->ActiveTeams();
}

EXPORT(int) skirmishAiCallback_SkirmishAIs_getSize(int skirmishAIId) {
	return aiInterfaceCallback_SkirmishAIs_getSize(-1);
}

EXPORT(int) skirmishAiCallback_SkirmishAIs_getMax(int skirmishAIId) {
	return aiInterfaceCallback_SkirmishAIs_getMax(-1);
}

EXPORT(int) skirmishAiCallback_SkirmishAI_getTeamId(int skirmishAIId) {
	return skirmishAIId_teamId[skirmishAIId];
}

static inline const CSkirmishAILibraryInfo* getSkirmishAILibraryInfo(int skirmishAIId) {

	const CSkirmishAILibraryInfo* info = NULL;

	const SkirmishAIKey* key = skirmishAIHandler.GetLocalSkirmishAILibraryKey(skirmishAIId);
	assert(key != NULL);
	const IAILibraryManager* libMan = IAILibraryManager::GetInstance();
	IAILibraryManager::T_skirmishAIInfos infs = libMan->GetSkirmishAIInfos();
	IAILibraryManager::T_skirmishAIInfos::const_iterator inf = infs.find(*key);
	if (inf != infs.end()) {
		info = (const CSkirmishAILibraryInfo*) inf->second;
	}

	return info;
}

EXPORT(int) skirmishAiCallback_SkirmishAI_Info_getSize(int skirmishAIId) {

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(skirmishAIId);
	return (int)info->size();
}

EXPORT(const char*) skirmishAiCallback_SkirmishAI_Info_getKey(int skirmishAIId, int infoIndex) {

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(skirmishAIId);
	return info->GetKeyAt(infoIndex).c_str();
}

EXPORT(const char*) skirmishAiCallback_SkirmishAI_Info_getValue(int skirmishAIId, int infoIndex) {

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(skirmishAIId);
	return info->GetValueAt(infoIndex).c_str();
}

EXPORT(const char*) skirmishAiCallback_SkirmishAI_Info_getDescription(int skirmishAIId, int infoIndex) {

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(skirmishAIId);
	return info->GetDescriptionAt(infoIndex).c_str();
}

EXPORT(const char*) skirmishAiCallback_SkirmishAI_Info_getValueByKey(int skirmishAIId, const char* const key) {

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(skirmishAIId);
	return info->GetInfo(key).c_str();
}

static inline bool checkOptionIndex(int optionIndex, const std::vector<std::string>& optionKeys) {
	return ((optionIndex < 0) || ((size_t)optionIndex >= optionKeys.size()));
}

EXPORT(int) skirmishAiCallback_SkirmishAI_OptionValues_getSize(int skirmishAIId) {
	return (int)skirmishAIHandler.GetSkirmishAI(skirmishAIId)->options.size();
}

EXPORT(const char*) skirmishAiCallback_SkirmishAI_OptionValues_getKey(int skirmishAIId, int optionIndex) {

	const std::vector<std::string>& optionKeys = skirmishAIHandler.GetSkirmishAI(skirmishAIId)->optionKeys;
	if (checkOptionIndex(optionIndex, optionKeys)) {
		return NULL;
	} else {
		const std::string& key = *(optionKeys.begin() + optionIndex);
		return key.c_str();
	}
}

EXPORT(const char*) skirmishAiCallback_SkirmishAI_OptionValues_getValue(int skirmishAIId, int optionIndex) {

	const std::vector<std::string>& optionKeys = skirmishAIHandler.GetSkirmishAI(skirmishAIId)->optionKeys;
	if (checkOptionIndex(optionIndex, optionKeys)) {
		return NULL;
	} else {
		const std::map<std::string, std::string>& options = skirmishAIHandler.GetSkirmishAI(skirmishAIId)->options;
		const std::string& key = *(optionKeys.begin() + optionIndex);
		const std::map<std::string, std::string>::const_iterator op = options.find(key);
		if (op == options.end()) {
			return NULL;
		} else {
			return op->second.c_str();
		}
	}
}

EXPORT(const char*) skirmishAiCallback_SkirmishAI_OptionValues_getValueByKey(int skirmishAIId, const char* const key) {

	const std::map<std::string, std::string>& options = skirmishAIHandler.GetSkirmishAI(skirmishAIId)->options;
	const std::map<std::string, std::string>::const_iterator op = options.find(key);
	if (op == options.end()) {
		return NULL;
	} else {
		return op->second.c_str();
	}
}


EXPORT(void) skirmishAiCallback_Log_log(int skirmishAIId, const char* const msg) {

	checkSkirmishAIId(skirmishAIId);

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(skirmishAIId);
	logOutput.Print("Skirmish AI <%s-%s>: %s", info->GetName().c_str(), info->GetVersion().c_str(), msg);
}

EXPORT(void) skirmishAiCallback_Log_exception(int skirmishAIId, const char* const msg, int severety, bool die) {

	checkSkirmishAIId(skirmishAIId);

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(skirmishAIId);
	logOutput.Print("Skirmish AI <%s-%s>: error, severety %i: [%s] %s",
			info->GetName().c_str(), info->GetVersion().c_str(), severety,
			(die ? "AI shutting down" : "AI still running"), msg);
	if (die) {
		skirmishAIHandler.SetLocalSkirmishAIDieing(skirmishAIId, 4 /* = AI crashed */);
	}
}

EXPORT(char) skirmishAiCallback_DataDirs_getPathSeparator(int UNUSED_skirmishAIId) {
	return aiInterfaceCallback_DataDirs_getPathSeparator(-1);
}

EXPORT(int) skirmishAiCallback_DataDirs_Roots_getSize(int UNUSED_skirmishAIId) {
	return aiInterfaceCallback_DataDirs_Roots_getSize(-1);
}

EXPORT(bool) skirmishAiCallback_DataDirs_Roots_getDir(int UNUSED_skirmishAIId, char* path, int path_sizeMax, int dirIndex) {
	return aiInterfaceCallback_DataDirs_Roots_getDir(-1, path, path_sizeMax, dirIndex);
}

EXPORT(bool) skirmishAiCallback_DataDirs_Roots_locatePath(int UNUSED_skirmishAIId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir) {
	return aiInterfaceCallback_DataDirs_Roots_locatePath(-1, path, path_sizeMax, relPath, writeable, create, dir);
}

EXPORT(char*) skirmishAiCallback_DataDirs_Roots_allocatePath(int UNUSED_skirmishAIId, const char* const relPath, bool writeable, bool create, bool dir) {
	return aiInterfaceCallback_DataDirs_Roots_allocatePath(-1, relPath, writeable, create, dir);
}

EXPORT(const char*) skirmishAiCallback_DataDirs_getConfigDir(int skirmishAIId) {

	checkSkirmishAIId(skirmishAIId);

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(skirmishAIId);
	return info->GetDataDir().c_str();
}

EXPORT(bool) skirmishAiCallback_DataDirs_locatePath(int skirmishAIId, char* path, int path_sizeMax, const char* const relPath, bool writeable, bool create, bool dir, bool common) {

	bool exists = false;

	const char ps = skirmishAiCallback_DataDirs_getPathSeparator(skirmishAIId);
	const std::string aiShortName = skirmishAiCallback_SkirmishAI_Info_getValueByKey(skirmishAIId, SKIRMISH_AI_PROPERTY_SHORT_NAME);
	std::string aiVersion;
	if (common) {
		aiVersion = SKIRMISH_AIS_VERSION_COMMON;
	} else {
		aiVersion = skirmishAiCallback_SkirmishAI_Info_getValueByKey(skirmishAIId, SKIRMISH_AI_PROPERTY_VERSION);
	}
	std::string aiRelPath(SKIRMISH_AI_DATA_DIR);
	aiRelPath += ps + aiShortName + ps + aiVersion + ps + relPath;

	exists = skirmishAiCallback_DataDirs_Roots_locatePath(skirmishAIId, path, path_sizeMax, aiRelPath.c_str(), writeable, create, dir);

	return exists;
}

EXPORT(char*) skirmishAiCallback_DataDirs_allocatePath(int skirmishAIId, const char* const relPath, bool writeable, bool create, bool dir, bool common) {

	static const unsigned int path_sizeMax = 2048;

	char* path = (char*) calloc(path_sizeMax, sizeof(char*));
	const bool fetchOk = skirmishAiCallback_DataDirs_locatePath(skirmishAIId, path, path_sizeMax, relPath, writeable, create, dir, common);

	if (!fetchOk) {
		FREE(path);
	}

	return path;
}

static std::vector<std::string> writeableDataDirs;
EXPORT(const char*) skirmishAiCallback_DataDirs_getWriteableDir(int skirmishAIId) {

	checkSkirmishAIId(skirmishAIId);

	// fill up writeableDataDirs until teamId index is in there
	// if it is not yet
	size_t wdd;
	for (wdd=writeableDataDirs.size(); wdd <= (size_t)skirmishAIId; ++wdd) {
		writeableDataDirs.push_back("");
	}
	if (writeableDataDirs[skirmishAIId].empty()) {
		static const unsigned int sizeMax = 1024;
		char tmpRes[sizeMax];
		static const char* const rootPath = "";
		const bool exists = skirmishAiCallback_DataDirs_locatePath(skirmishAIId,
				tmpRes, sizeMax, rootPath, true, true, true, false);
		writeableDataDirs[skirmishAIId] = tmpRes;
		if (!exists) {
			char errorMsg[sizeMax];
			SNPRINTF(errorMsg, sizeMax,
					"Unable to create writable data-dir for Skirmish AI (ID:%i): %s",
					skirmishAIId, tmpRes);
			skirmishAiCallback_Log_exception(skirmishAIId, errorMsg, 1, true);
			return NULL;
		}
	}

	return writeableDataDirs[skirmishAIId].c_str();
}


//##############################################################################
EXPORT(bool) skirmishAiCallback_Cheats_isEnabled(int skirmishAIId) {
	return skirmishAIId_cheatingEnabled[skirmishAIId];
}

EXPORT(bool) skirmishAiCallback_Cheats_setEnabled(int skirmishAIId, bool enabled) {

	skirmishAIId_cheatingEnabled[skirmishAIId] = enabled;
	if (enabled && !skirmishAIId_usesCheats[skirmishAIId]) {
		logOutput.Print("SkirmishAI (ID = %i, team ID = %i) is using cheats!", skirmishAIId, skirmishAIId_teamId[skirmishAIId]);
		skirmishAIId_usesCheats[skirmishAIId] = true;
	}
	return (enabled == skirmishAiCallback_Cheats_isEnabled(skirmishAIId));
}

EXPORT(bool) skirmishAiCallback_Cheats_setEventsEnabled(int skirmishAIId, bool enabled) {

	bool success = false;

	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		skirmishAIId_cheatCallback[skirmishAIId]->EnableCheatEvents(enabled);
		success = true;
	}

	return success;
}

EXPORT(bool) skirmishAiCallback_Cheats_isOnlyPassive(int skirmishAIId) {
	return CAICheats::OnlyPassiveCheats();
}

EXPORT(int) skirmishAiCallback_Game_getAiInterfaceVersion(int skirmishAIId) {
	CAICallback* clb = skirmishAIId_callback[skirmishAIId];
	return wrapper_HandleCommand(clb, NULL, AIHCQuerySubVersionId, NULL);
}

EXPORT(int) skirmishAiCallback_Game_getCurrentFrame(int skirmishAIId) {
	return skirmishAIId_callback[skirmishAIId]->GetCurrentFrame();
}

EXPORT(int) skirmishAiCallback_Game_getMyTeam(int skirmishAIId) {
	return skirmishAIId_callback[skirmishAIId]->GetMyTeam();
}

EXPORT(int) skirmishAiCallback_Game_getMyAllyTeam(int skirmishAIId) {
	return skirmishAIId_callback[skirmishAIId]->GetMyAllyTeam();
}

EXPORT(int) skirmishAiCallback_Game_getPlayerTeam(int skirmishAIId, int player) {
	return skirmishAIId_callback[skirmishAIId]->GetPlayerTeam(player);
}

EXPORT(int) skirmishAiCallback_Game_getTeams(int skirmishAIId) {
	return teamHandler->ActiveTeams();
}

EXPORT(const char*) skirmishAiCallback_Game_getTeamSide(int skirmishAIId, int otherTeamId) {
	return skirmishAIId_callback[skirmishAIId]->GetTeamSide(otherTeamId);
}

EXPORT(void) skirmishAiCallback_Game_getTeamColor(int skirmishAIId, int otherTeamId, short* return_colorS3_out) {

	const unsigned char* color = teamHandler->Team(otherTeamId)->color;
	return_colorS3_out[0] = color[0];
	return_colorS3_out[1] = color[1];
	return_colorS3_out[2] = color[2];
}

EXPORT(float) skirmishAiCallback_Game_getTeamIncomeMultiplier(int skirmishAIId, int otherTeamId) {
	return teamHandler->Team(otherTeamId)->GetIncomeMultiplier();
}

EXPORT(int) skirmishAiCallback_Game_getTeamAllyTeam(int skirmishAIId, int otherTeamId) {
	return teamHandler->AllyTeam(otherTeamId);
}

EXPORT(float) skirmishAiCallback_Game_getTeamResourceCurrent(int skirmishAIId, int otherTeamId, int resourceId) {

	float res = -1.0f;

	const bool fetchOk = teamHandler->AlliedTeams(skirmishAIId_teamId[skirmishAIId], otherTeamId) || skirmishAiCallback_Cheats_isEnabled(skirmishAIId);
	if (fetchOk) {
		if (resourceId == resourceHandler->GetMetalId()) {
			res = teamHandler->Team(otherTeamId)->metal;
		} else if (resourceId == resourceHandler->GetEnergyId()) {
			res = teamHandler->Team(otherTeamId)->energy;
		}
	}

	return res;
}

EXPORT(float) skirmishAiCallback_Game_getTeamResourceIncome(int skirmishAIId, int otherTeamId, int resourceId) {

	float res = -1.0f;

	const bool fetchOk = teamHandler->AlliedTeams(skirmishAIId_teamId[skirmishAIId], otherTeamId) || skirmishAiCallback_Cheats_isEnabled(skirmishAIId);
	if (fetchOk) {
		if (resourceId == resourceHandler->GetMetalId()) {
			res = teamHandler->Team(otherTeamId)->prevMetalIncome;
		} else if (resourceId == resourceHandler->GetEnergyId()) {
			res = teamHandler->Team(otherTeamId)->prevEnergyIncome;
		}
	}

	return res;
}

EXPORT(float) skirmishAiCallback_Game_getTeamResourceUsage(int skirmishAIId, int otherTeamId, int resourceId) {

	float res = -1.0f;

	const bool fetchOk = teamHandler->AlliedTeams(skirmishAIId_teamId[skirmishAIId], otherTeamId) || skirmishAiCallback_Cheats_isEnabled(skirmishAIId);
	if (fetchOk) {
		if (resourceId == resourceHandler->GetMetalId()) {
			res = teamHandler->Team(otherTeamId)->prevMetalExpense;
		} else if (resourceId == resourceHandler->GetEnergyId()) {
			res = teamHandler->Team(otherTeamId)->prevEnergyExpense;
		}
	}

	return res;
}

EXPORT(float) skirmishAiCallback_Game_getTeamResourceStorage(int skirmishAIId, int otherTeamId, int resourceId) {

	float res = -1.0f;

	const bool fetchOk = teamHandler->AlliedTeams(skirmishAIId_teamId[skirmishAIId], otherTeamId) || skirmishAiCallback_Cheats_isEnabled(skirmishAIId);
	if (fetchOk) {
		if (resourceId == resourceHandler->GetMetalId()) {
			res = teamHandler->Team(otherTeamId)->metalStorage;
		} else if (resourceId == resourceHandler->GetEnergyId()) {
			res = teamHandler->Team(otherTeamId)->energyStorage;
		}
	}

	return res;
}

EXPORT(bool) skirmishAiCallback_Game_isAllied(int skirmishAIId, int firstAllyTeamId, int secondAllyTeamId) {
	return teamHandler->Ally(firstAllyTeamId, secondAllyTeamId);
}





EXPORT(int) skirmishAiCallback_WeaponDef_getNumDamageTypes(int skirmishAIId) {

	int numDamageTypes;

	const bool fetchOk = skirmishAIId_callback[skirmishAIId]->GetValue(AIVAL_NUMDAMAGETYPES, &numDamageTypes);
	if (!fetchOk) {
		numDamageTypes = -1;
	}

	return numDamageTypes;
}

EXPORT(bool) skirmishAiCallback_Game_isExceptionHandlingEnabled(int skirmishAIId) {

	bool exceptionHandlingEnabled;

	const bool fetchOk = skirmishAIId_callback[skirmishAIId]->GetValue(AIVAL_EXCEPTION_HANDLING,
			&exceptionHandlingEnabled);
	if (!fetchOk) {
		exceptionHandlingEnabled = false;
	}

	return exceptionHandlingEnabled;
}

EXPORT(bool) skirmishAiCallback_Game_isDebugModeEnabled(int skirmishAIId) {

	bool debugModeEnabled;

	const bool fetchOk = skirmishAIId_callback[skirmishAIId]->GetValue(AIVAL_DEBUG_MODE, &debugModeEnabled);
	if (!fetchOk) {
		debugModeEnabled = false;
	}

	return debugModeEnabled;
}

EXPORT(bool) skirmishAiCallback_Game_isPaused(int skirmishAIId) {

	bool paused;

	const bool fetchOk = skirmishAIId_callback[skirmishAIId]->GetValue(AIVAL_GAME_PAUSED, &paused);
	if (!fetchOk) {
		paused = false;
	}

	return paused;
}

EXPORT(float) skirmishAiCallback_Game_getSpeedFactor(int skirmishAIId) {

	float speedFactor;

	const bool fetchOk = skirmishAIId_callback[skirmishAIId]->GetValue(AIVAL_GAME_SPEED_FACTOR, &speedFactor);
	if (!fetchOk) {
		speedFactor = false;
	}

	return speedFactor;
}

EXPORT(int) skirmishAiCallback_Game_getCategoryFlag(int skirmishAIId, const char* categoryName) {
	return CCategoryHandler::Instance()->GetCategory(categoryName);
}

EXPORT(int) skirmishAiCallback_Game_getCategoriesFlag(int skirmishAIId, const char* categoryNames) {
	return CCategoryHandler::Instance()->GetCategories(categoryNames);
}

EXPORT(void) skirmishAiCallback_Game_getCategoryName(int skirmishAIId, int categoryFlag, char* name, int name_sizeMax) {

	const std::vector<std::string>& names = CCategoryHandler::Instance()->GetCategoryNames(categoryFlag);

	const char* theName = '\0';
	if (!names.empty()) {
		theName = names.begin()->c_str();
	}
	STRCPYS(name, name_sizeMax, theName);
}

EXPORT(float) skirmishAiCallback_Gui_getViewRange(int skirmishAIId) {

	float viewRange;

	const bool fetchOk = skirmishAIId_callback[skirmishAIId]->GetValue(AIVAL_GUI_VIEW_RANGE, &viewRange);
	if (!fetchOk) {
		viewRange = false;
	}

	return viewRange;
}

EXPORT(float) skirmishAiCallback_Gui_getScreenX(int skirmishAIId) {

	float screenX;

	const bool fetchOk = skirmishAIId_callback[skirmishAIId]->GetValue(AIVAL_GUI_SCREENX, &screenX);
	if (!fetchOk) {
		screenX = false;
	}

	return screenX;
}

EXPORT(float) skirmishAiCallback_Gui_getScreenY(int skirmishAIId) {

	float screenY;

	const bool fetchOk = skirmishAIId_callback[skirmishAIId]->GetValue(AIVAL_GUI_SCREENY, &screenY);
	if (!fetchOk) {
		screenY = false;
	}

	return screenY;
}

EXPORT(void) skirmishAiCallback_Gui_Camera_getDirection(int skirmishAIId, float* return_posF3_out) {

	float3 cameraDir;

	const bool fetchOk = skirmishAIId_callback[skirmishAIId]->GetValue(AIVAL_GUI_CAMERA_DIR, &cameraDir);
	if (!fetchOk) {
		cameraDir = float3(1.0f, 0.0f, 0.0f);
	}

	cameraDir.copyInto(return_posF3_out);
}

EXPORT(void) skirmishAiCallback_Gui_Camera_getPosition(int skirmishAIId, float* return_posF3_out) {

	float3 cameraPosition;

	const bool fetchOk = skirmishAIId_callback[skirmishAIId]->GetValue(AIVAL_GUI_CAMERA_POS, &cameraPosition);
	if (!fetchOk) {
		cameraPosition = float3(1.0f, 0.0f, 0.0f);
	}

	cameraPosition.copyInto(return_posF3_out);
}

// EXPORT(bool) skirmishAiCallback_File_locateForReading(int skirmishAIId, char* fileName, int fileName_sizeMax) {
// 	CAICallback* clb = skirmishAIId_callback[skirmishAIId];
// 	return clb->GetValue(AIVAL_LOCATE_FILE_R, fileName, fileName_sizeMax);
// }
// EXPORT(bool) skirmishAiCallback_File_locateForWriting(int skirmishAIId, char* fileName, int fileName_sizeMax) {
// 	CAICallback* clb = skirmishAIId_callback[skirmishAIId];
// 	return clb->GetValue(AIVAL_LOCATE_FILE_W, fileName, fileName_sizeMax);
// }




//########### BEGINN Mod

EXPORT(const char*) skirmishAiCallback_Mod_getFileName(int skirmishAIId) {
	return modInfo.filename.c_str();
}

EXPORT(int) skirmishAiCallback_Mod_getHash(int skirmishAIId) {
	return archiveScanner->GetArchiveCompleteChecksum(modInfo.humanName);
}
EXPORT(const char*) skirmishAiCallback_Mod_getHumanName(int skirmishAIId) {
	return modInfo.humanName.c_str();
}

EXPORT(const char*) skirmishAiCallback_Mod_getShortName(int skirmishAIId) {
	return modInfo.shortName.c_str();
}

EXPORT(const char*) skirmishAiCallback_Mod_getVersion(int skirmishAIId) {
	return modInfo.version.c_str();
}

EXPORT(const char*) skirmishAiCallback_Mod_getMutator(int skirmishAIId) {
	return modInfo.mutator.c_str();
}

EXPORT(const char*) skirmishAiCallback_Mod_getDescription(int skirmishAIId) {
	return modInfo.description.c_str();
}

EXPORT(bool) skirmishAiCallback_Mod_getAllowTeamColors(int skirmishAIId) {
	return modInfo.allowTeamColors;
}

EXPORT(bool) skirmishAiCallback_Mod_getConstructionDecay(int skirmishAIId) {
	return modInfo.constructionDecay;
}

EXPORT(int) skirmishAiCallback_Mod_getConstructionDecayTime(int skirmishAIId) {
	return modInfo.constructionDecayTime;
}

EXPORT(float) skirmishAiCallback_Mod_getConstructionDecaySpeed(int skirmishAIId) {
	return modInfo.constructionDecaySpeed;
}

EXPORT(int) skirmishAiCallback_Mod_getMultiReclaim(int skirmishAIId) {
	return modInfo.multiReclaim;
}

EXPORT(int) skirmishAiCallback_Mod_getReclaimMethod(int skirmishAIId) {
	return modInfo.reclaimMethod;
}

EXPORT(int) skirmishAiCallback_Mod_getReclaimUnitMethod(int skirmishAIId) {
	return modInfo.reclaimUnitMethod;
}

EXPORT(float) skirmishAiCallback_Mod_getReclaimUnitEnergyCostFactor(int skirmishAIId) {
	return modInfo.reclaimUnitEnergyCostFactor;
}

EXPORT(float) skirmishAiCallback_Mod_getReclaimUnitEfficiency(int skirmishAIId) {
	return modInfo.reclaimUnitEfficiency;
}

EXPORT(float) skirmishAiCallback_Mod_getReclaimFeatureEnergyCostFactor(int skirmishAIId) {
	return modInfo.reclaimFeatureEnergyCostFactor;
}

EXPORT(bool) skirmishAiCallback_Mod_getReclaimAllowEnemies(int skirmishAIId) {
	return modInfo.reclaimAllowEnemies;
}

EXPORT(bool) skirmishAiCallback_Mod_getReclaimAllowAllies(int skirmishAIId) {
	return modInfo.reclaimAllowAllies;
}

EXPORT(float) skirmishAiCallback_Mod_getRepairEnergyCostFactor(int skirmishAIId) {
	return modInfo.repairEnergyCostFactor;
}

EXPORT(float) skirmishAiCallback_Mod_getResurrectEnergyCostFactor(int skirmishAIId) {
	return modInfo.resurrectEnergyCostFactor;
}

EXPORT(float) skirmishAiCallback_Mod_getCaptureEnergyCostFactor(int skirmishAIId) {
	return modInfo.captureEnergyCostFactor;
}

EXPORT(int) skirmishAiCallback_Mod_getTransportGround(int skirmishAIId) {
	return modInfo.transportGround;
}

EXPORT(int) skirmishAiCallback_Mod_getTransportHover(int skirmishAIId) {
	return modInfo.transportHover;
}

EXPORT(int) skirmishAiCallback_Mod_getTransportShip(int skirmishAIId) {
	return modInfo.transportShip;
}

EXPORT(int) skirmishAiCallback_Mod_getTransportAir(int skirmishAIId) {
	return modInfo.transportAir;
}

EXPORT(int) skirmishAiCallback_Mod_getFireAtKilled(int skirmishAIId) {
	return modInfo.fireAtKilled;
}

EXPORT(int) skirmishAiCallback_Mod_getFireAtCrashing(int skirmishAIId) {
	return modInfo.fireAtCrashing;
}

EXPORT(int) skirmishAiCallback_Mod_getFlankingBonusModeDefault(int skirmishAIId) {
	return modInfo.flankingBonusModeDefault;
}

EXPORT(int) skirmishAiCallback_Mod_getLosMipLevel(int skirmishAIId) {
	return modInfo.losMipLevel;
}

EXPORT(int) skirmishAiCallback_Mod_getAirMipLevel(int skirmishAIId) {
	return modInfo.airMipLevel;
}

EXPORT(float) skirmishAiCallback_Mod_getLosMul(int skirmishAIId) {
	return modInfo.losMul;
}

EXPORT(float) skirmishAiCallback_Mod_getAirLosMul(int skirmishAIId) {
	return modInfo.airLosMul;
}

EXPORT(bool) skirmishAiCallback_Mod_getRequireSonarUnderWater(int skirmishAIId) {
	return modInfo.requireSonarUnderWater;
}

//########### END Mod



//########### BEGINN Map
EXPORT(bool) skirmishAiCallback_Map_isPosInCamera(int skirmishAIId, float* pos_posF3, float radius) {
	return skirmishAIId_callback[skirmishAIId]->PosInCamera(pos_posF3, radius);
}

EXPORT(int) skirmishAiCallback_Map_getChecksum(int skirmishAIId) {

	unsigned int checksum;

	const bool fetchOk = skirmishAIId_callback[skirmishAIId]->GetValue(AIVAL_MAP_CHECKSUM, &checksum);
	if (!fetchOk) {
		checksum = -1;
	}

	return checksum;
}

EXPORT(int) skirmishAiCallback_Map_getWidth(int skirmishAIId) {
	return skirmishAIId_callback[skirmishAIId]->GetMapWidth();
}

EXPORT(int) skirmishAiCallback_Map_getHeight(int skirmishAIId) {
	return skirmishAIId_callback[skirmishAIId]->GetMapHeight();
}

EXPORT(int) skirmishAiCallback_Map_getHeightMap(int skirmishAIId, float* heights,
		int heights_sizeMax) {

	static const int heights_sizeReal = gs->mapx * gs->mapy;

	int heights_size = heights_sizeReal;

	if (heights != NULL) {
		const float* tmpMap = skirmishAIId_callback[skirmishAIId]->GetHeightMap();
		heights_size = min(heights_sizeReal, heights_sizeMax);
		int i;
		for (i=0; i < heights_size; ++i) {
			heights[i] = tmpMap[i];
		}
	}

	return heights_size;
}

EXPORT(int) skirmishAiCallback_Map_getCornersHeightMap(int skirmishAIId,
		float* cornerHeights, int cornerHeights_sizeMax) {

	static const int cornerHeights_sizeReal = (gs->mapx + 1) * (gs->mapy + 1);

	int cornerHeights_size = cornerHeights_sizeReal;

	if (cornerHeights != NULL) {
		const float* tmpMap =  skirmishAIId_callback[skirmishAIId]->GetCornersHeightMap();
		cornerHeights_size = min(cornerHeights_sizeReal, cornerHeights_sizeMax);
		int i;
		for (i=0; i < cornerHeights_size; ++i) {
			cornerHeights[i] = tmpMap[i];
		}
	}

	return cornerHeights_size;
}

EXPORT(float) skirmishAiCallback_Map_getMinHeight(int skirmishAIId) {
	return skirmishAIId_callback[skirmishAIId]->GetMinHeight();
}

EXPORT(float) skirmishAiCallback_Map_getMaxHeight(int skirmishAIId) {
	return skirmishAIId_callback[skirmishAIId]->GetMaxHeight();
}

EXPORT(int) skirmishAiCallback_Map_getSlopeMap(int skirmishAIId,
		float* slopes, int slopes_sizeMax) {

	static const int slopes_sizeReal = gs->hmapx * gs->hmapy;

	int slopes_size = slopes_sizeReal;

	if (slopes != NULL) {
		const float* tmpMap =  skirmishAIId_callback[skirmishAIId]->GetSlopeMap();
		slopes_size = min(slopes_sizeReal, slopes_sizeMax);
		int i;
		for (i=0; i < slopes_size; ++i) {
			slopes[i] = tmpMap[i];
		}
	}

	return slopes_size;
}

EXPORT(int) skirmishAiCallback_Map_getLosMap(int skirmishAIId,
		int* losValues, int losValues_sizeMax) {

	static const int losValues_sizeReal = loshandler->losSizeX * loshandler->losSizeY;

	int losValues_size = losValues_sizeReal;

	if (losValues != NULL) {
		const unsigned short* tmpMap =  skirmishAIId_callback[skirmishAIId]->GetLosMap();
		losValues_size = min(losValues_sizeReal, losValues_sizeMax);
		int i;
		for (i=0; i < losValues_size; ++i) {
			losValues[i] = tmpMap[i];
		}
	}

	return losValues_size;
}

EXPORT(int) skirmishAiCallback_Map_getRadarMap(int skirmishAIId,
		int* radarValues, int radarValues_sizeMax) {

	static const int radarValues_sizeReal = radarhandler->xsize * radarhandler->zsize;

	int radarValues_size = radarValues_sizeReal;

	if (radarValues != NULL) {
		const unsigned short* tmpMap =  skirmishAIId_callback[skirmishAIId]->GetRadarMap();
		radarValues_size = min(radarValues_sizeReal, radarValues_sizeMax);
		int i;
		for (i=0; i < radarValues_size; ++i) {
			radarValues[i] = tmpMap[i];
		}
	}

	return radarValues_size;
}

EXPORT(int) skirmishAiCallback_Map_getJammerMap(int skirmishAIId,
		int* jammerValues, int jammerValues_sizeMax) {

	// Yes, it is correct, jammer-map has the same size as the radar map
	static const int jammerValues_sizeReal = radarhandler->xsize * radarhandler->zsize;

	int jammerValues_size = jammerValues_sizeReal;

	if (jammerValues != NULL) {
		const unsigned short* tmpMap =  skirmishAIId_callback[skirmishAIId]->GetJammerMap();
		jammerValues_size = min(jammerValues_sizeReal, jammerValues_sizeMax);
		int i;
		for (i=0; i < jammerValues_size; ++i) {
			jammerValues[i] = tmpMap[i];
		}
	}

	return jammerValues_size;
}

EXPORT(int) skirmishAiCallback_Map_getResourceMapRaw(
		int skirmishAIId, int resourceId, short* resources, int resources_sizeMax) {

	int resources_sizeReal = 0;
	if (resourceId == resourceHandler->GetMetalId()) {
		resources_sizeReal = readmap->metalMap->GetSizeX() * readmap->metalMap->GetSizeZ();
	}

	int resources_size = resources_sizeReal;

	if ((resources != NULL) && (resources_sizeReal > 0)) {
		resources_size = min(resources_sizeReal, resources_sizeMax);

		const unsigned char* tmpMap;
		if (resourceId == resourceHandler->GetMetalId()) {
			tmpMap = skirmishAIId_callback[skirmishAIId]->GetMetalMap();
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
		int skirmishAIId, int resourceId, float* spots, int spots_sizeMax) {

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
		int skirmishAIId, int resourceId) {
	return getResourceMapAnalyzer(resourceId)->GetAverageIncome();
}

EXPORT(void) skirmishAiCallback_Map_getResourceMapSpotsNearest(
		int skirmishAIId, int resourceId, float* pos_posF3, float* return_posF3_out) {
	getResourceMapAnalyzer(resourceId)->GetNearestSpot(pos_posF3, skirmishAIId_teamId[skirmishAIId]).copyInto(return_posF3_out);
}

EXPORT(int) skirmishAiCallback_Map_getHash(int skirmishAIId) {
	return archiveScanner->GetArchiveCompleteChecksum(mapInfo->map.name);
}

EXPORT(const char*) skirmishAiCallback_Map_getName(int skirmishAIId) {
	return mapInfo->map.name.c_str();
}

EXPORT(const char*) skirmishAiCallback_Map_getHumanName(int skirmishAIId) {
	return mapInfo->map.description.c_str();
}

EXPORT(float) skirmishAiCallback_Map_getElevationAt(int skirmishAIId, float x, float z) {
	return skirmishAIId_callback[skirmishAIId]->GetElevation(x, z);
}



EXPORT(float) skirmishAiCallback_Map_getMaxResource(
		int skirmishAIId, int resourceId) {

	if (resourceId == resourceHandler->GetMetalId()) {
		return skirmishAIId_callback[skirmishAIId]->GetMaxMetal();
	} else {
		return -1.0f;
	}
}

EXPORT(float) skirmishAiCallback_Map_getExtractorRadius(int skirmishAIId,
		int resourceId) {

	if (resourceId == resourceHandler->GetMetalId()) {
		return skirmishAIId_callback[skirmishAIId]->GetExtractorRadius();
	} else {
		return -1.0f;
	}
}

EXPORT(float) skirmishAiCallback_Map_getMinWind(int skirmishAIId) {
	return skirmishAIId_callback[skirmishAIId]->GetMinWind();
}

EXPORT(float) skirmishAiCallback_Map_getMaxWind(int skirmishAIId) {
	return skirmishAIId_callback[skirmishAIId]->GetMaxWind();
}

EXPORT(float) skirmishAiCallback_Map_getCurWind(int skirmishAIId) {
	return skirmishAIId_callback[skirmishAIId]->GetCurWind();
}

EXPORT(float) skirmishAiCallback_Map_getTidalStrength(int skirmishAIId) {
	return skirmishAIId_callback[skirmishAIId]->GetTidalStrength();
}

EXPORT(float) skirmishAiCallback_Map_getGravity(int skirmishAIId) {
	return skirmishAIId_callback[skirmishAIId]->GetGravity();
}



EXPORT(bool) skirmishAiCallback_Map_isPossibleToBuildAt(int skirmishAIId, int unitDefId,
		float* pos_posF3, int facing) {

	const UnitDef* unitDef = getUnitDefById(skirmishAIId, unitDefId);
	return skirmishAIId_callback[skirmishAIId]->CanBuildAt(unitDef, pos_posF3, facing);
}

EXPORT(void) skirmishAiCallback_Map_findClosestBuildSite(int skirmishAIId, int unitDefId,
		float* pos_posF3, float searchRadius, int minDist, int facing, float* return_posF3_out) {

			const UnitDef* unitDef = getUnitDefById(skirmishAIId, unitDefId);
	skirmishAIId_callback[skirmishAIId]->ClosestBuildSite(unitDef, pos_posF3, searchRadius, minDist, facing)
			.copyInto(return_posF3_out);
}

EXPORT(int) skirmishAiCallback_Map_getPoints(int skirmishAIId, bool includeAllies) {
	return skirmishAIId_callback[skirmishAIId]->GetMapPoints(tmpPointMarkerArr[skirmishAIId],
			TMP_ARR_SIZE, includeAllies);
}

EXPORT(void) skirmishAiCallback_Map_Point_getPosition(int skirmishAIId, int pointId, float* return_posF3_out) {
	tmpPointMarkerArr[skirmishAIId][pointId].pos.copyInto(return_posF3_out);
}

EXPORT(void) skirmishAiCallback_Map_Point_getColor(int skirmishAIId, int pointId, short* return_colorS3_out) {

	const unsigned char* color = tmpPointMarkerArr[skirmishAIId][pointId].color;
	return_colorS3_out[0] = color[0];
	return_colorS3_out[1] = color[1];
	return_colorS3_out[2] = color[2];
}

EXPORT(const char*) skirmishAiCallback_Map_Point_getLabel(int skirmishAIId, int pointId) {
	return tmpPointMarkerArr[skirmishAIId][pointId].label;
}

EXPORT(int) skirmishAiCallback_Map_getLines(int skirmishAIId, bool includeAllies) {
	return skirmishAIId_callback[skirmishAIId]->GetMapLines(tmpLineMarkerArr[skirmishAIId],
			TMP_ARR_SIZE, includeAllies);
}

EXPORT(void) skirmishAiCallback_Map_Line_getFirstPosition(int skirmishAIId, int lineId, float* return_posF3_out) {
	tmpLineMarkerArr[skirmishAIId][lineId].pos.copyInto(return_posF3_out);
}

EXPORT(void) skirmishAiCallback_Map_Line_getSecondPosition(int skirmishAIId, int lineId, float* return_posF3_out) {
	tmpLineMarkerArr[skirmishAIId][lineId].pos2.copyInto(return_posF3_out);
}

EXPORT(void) skirmishAiCallback_Map_Line_getColor(int skirmishAIId, int lineId, short* return_colorS3_out) {

	const unsigned char* color = tmpLineMarkerArr[skirmishAIId][lineId].color;
	return_colorS3_out[0] = color[0];
	return_colorS3_out[1] = color[1];
	return_colorS3_out[2] = color[2];
}

EXPORT(void) skirmishAiCallback_Map_getStartPos(int skirmishAIId, float* return_posF3_out) {
	skirmishAIId_callback[skirmishAIId]->GetStartPos()->copyInto(return_posF3_out);
}

EXPORT(void) skirmishAiCallback_Map_getMousePos(int skirmishAIId, float* return_posF3_out) {
	skirmishAIId_callback[skirmishAIId]->GetMousePos().copyInto(return_posF3_out);
}

//########### END Map


// DEPRECATED
/*
EXPORT(bool) skirmishAiCallback_getProperty(int skirmishAIId, int id, int property, void* dst) {
//	return skirmishAIId_callback[skirmishAIId]->GetProperty(id, property, dst);
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->GetProperty(id, property, dst);
	} else {
		return skirmishAIId_callback[skirmishAIId]->GetProperty(id, property, dst);
	}
}

EXPORT(bool) skirmishAiCallback_getValue(int skirmishAIId, int id, void* dst) {
//	return skirmishAIId_callback[skirmishAIId]->GetValue(id, dst);
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->GetValue(id, dst);
	} else {
		return skirmishAIId_callback[skirmishAIId]->GetValue(id, dst);
	}
}

*/

EXPORT(int) skirmishAiCallback_File_getSize(int skirmishAIId, const char* fileName) {
	return skirmishAIId_callback[skirmishAIId]->GetFileSize(fileName);
}

EXPORT(bool) skirmishAiCallback_File_getContent(int skirmishAIId, const char* fileName, void* buffer, int bufferLen) {
	return skirmishAIId_callback[skirmishAIId]->ReadFile(fileName, buffer, bufferLen);
}





// BEGINN OBJECT Resource
EXPORT(int) skirmishAiCallback_getResources(int skirmishAIId) {
	return resourceHandler->GetNumResources();
}

EXPORT(int) skirmishAiCallback_getResourceByName(int skirmishAIId,
		const char* resourceName) {
	return resourceHandler->GetResourceId(resourceName);
}

EXPORT(const char*) skirmishAiCallback_Resource_getName(int skirmishAIId, int resourceId) {
	return resourceHandler->GetResource(resourceId)->name.c_str();
}

EXPORT(float) skirmishAiCallback_Resource_getOptimum(int skirmishAIId, int resourceId) {
	return resourceHandler->GetResource(resourceId)->optimum;
}

EXPORT(float) skirmishAiCallback_Economy_getCurrent(int skirmishAIId,
		int resourceId) {

	CAICallback* clb = skirmishAIId_callback[skirmishAIId];
	if (resourceId == resourceHandler->GetMetalId()) {
		return clb->GetMetal();
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return clb->GetEnergy();
	} else {
		return -1.0f;
	}
}

EXPORT(float) skirmishAiCallback_Economy_getIncome(int skirmishAIId,
		int resourceId) {

	CAICallback* clb = skirmishAIId_callback[skirmishAIId];
	if (resourceId == resourceHandler->GetMetalId()) {
		return clb->GetMetalIncome();
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return clb->GetEnergyIncome();
	} else {
		return -1.0f;
	}
}

EXPORT(float) skirmishAiCallback_Economy_getUsage(int skirmishAIId,
		int resourceId) {

	CAICallback* clb = skirmishAIId_callback[skirmishAIId];
	if (resourceId == resourceHandler->GetMetalId()) {
		return clb->GetMetalUsage();
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return clb->GetEnergyUsage();
	} else {
		return -1.0f;
	}
}

EXPORT(float) skirmishAiCallback_Economy_getStorage(int skirmishAIId,
		int resourceId) {

	CAICallback* clb = skirmishAIId_callback[skirmishAIId];
	if (resourceId == resourceHandler->GetMetalId()) {
		return clb->GetMetalStorage();
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return clb->GetEnergyStorage();
	} else {
		return -1.0f;
	}
}

EXPORT(const char*) skirmishAiCallback_Game_getSetupScript(int skirmishAIId) {
	const char* setupScript;
	CAICallback* clb = skirmishAIId_callback[skirmishAIId];
	const bool fetchOk = clb->GetValue(AIVAL_SCRIPT, &setupScript);
	if (!fetchOk) {
		return NULL;
	}
	return setupScript;
}







//########### BEGINN UnitDef
EXPORT(int) skirmishAiCallback_getUnitDefs(int skirmishAIId, int* unitDefIds,
		int unitDefIds_sizeMax) {

	const int unitDefIds_sizeReal = skirmishAIId_callback[skirmishAIId]->GetNumUnitDefs();

	int unitDefIds_size = unitDefIds_sizeReal;

	if (unitDefIds != NULL) {
		const UnitDef** defList = (const UnitDef**) new UnitDef*[unitDefIds_sizeReal];
		skirmishAIId_callback[skirmishAIId]->GetUnitDefList(defList);
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

EXPORT(int) skirmishAiCallback_getUnitDefByName(int skirmishAIId,
		const char* unitName) {

	int unitDefId = -1;

	const UnitDef* ud = skirmishAIId_callback[skirmishAIId]->GetUnitDef(unitName);
	if (ud != NULL) {
		unitDefId = ud->id;
	}

	return unitDefId;
}

EXPORT(float) skirmishAiCallback_UnitDef_getHeight(int skirmishAIId, int unitDefId) {
	CAICallback* clb = skirmishAIId_callback[skirmishAIId];
	return clb->GetUnitDefHeight(unitDefId);
}

EXPORT(float) skirmishAiCallback_UnitDef_getRadius(int skirmishAIId, int unitDefId) {
	CAICallback* clb = skirmishAIId_callback[skirmishAIId];
	return clb->GetUnitDefRadius(unitDefId);
}

EXPORT(const char*) skirmishAiCallback_UnitDef_getName(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->name.c_str();
}

EXPORT(const char*) skirmishAiCallback_UnitDef_getHumanName(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->humanName.c_str();
}

EXPORT(const char*) skirmishAiCallback_UnitDef_getFileName(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->filename.c_str();
}

//EXPORT(int) skirmishAiCallback_UnitDef_getId(int skirmishAIId, int unitDefId) {
//	return getUnitDefById(skirmishAIId, unitDefId)->id;
//}

EXPORT(int) skirmishAiCallback_UnitDef_getAiHint(int skirmishAIId, int unitDefId) {
	return 0;
}

EXPORT(int) skirmishAiCallback_UnitDef_getCobId(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->cobID;
}

EXPORT(int) skirmishAiCallback_UnitDef_getTechLevel(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->techLevel;
}

EXPORT(const char*) skirmishAiCallback_UnitDef_getGaia(int skirmishAIId, int unitDefId) {
	return "";
}

EXPORT(float) skirmishAiCallback_UnitDef_getUpkeep(int skirmishAIId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->metalUpkeep;
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return ud->energyUpkeep;
	} else {
		return 0.0f;
	}
}

EXPORT(float) skirmishAiCallback_UnitDef_getResourceMake(int skirmishAIId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->metalMake;
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return ud->energyMake;
	} else {
		return 0.0f;
	}
}

EXPORT(float) skirmishAiCallback_UnitDef_getMakesResource(int skirmishAIId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->makesMetal;
	} else {
		return 0.0f;
	}
}

EXPORT(float) skirmishAiCallback_UnitDef_getCost(int skirmishAIId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->metalCost;
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return ud->energyCost;
	} else {
		return 0.0f;
	}
}

EXPORT(float) skirmishAiCallback_UnitDef_getExtractsResource(
		int skirmishAIId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->extractsMetal;
	} else {
		return 0.0f;
	}
}

EXPORT(float) skirmishAiCallback_UnitDef_getResourceExtractorRange(
		int skirmishAIId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->extractRange;
	} else {
		return 0.0f;
	}
}

EXPORT(float) skirmishAiCallback_UnitDef_getWindResourceGenerator(
		int skirmishAIId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);
	if (resourceId == resourceHandler->GetEnergyId()) {
		return ud->windGenerator;
	} else {
		return 0.0f;
	}
}

EXPORT(float) skirmishAiCallback_UnitDef_getTidalResourceGenerator(
		int skirmishAIId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);
	if (resourceId == resourceHandler->GetEnergyId()) {
		return ud->tidalGenerator;
	} else {
		return 0.0f;
	}
}

EXPORT(float) skirmishAiCallback_UnitDef_getStorage(int skirmishAIId,
		int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->metalStorage;
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return ud->energyStorage;
	} else {
		return 0.0f;
	}
}

EXPORT(bool) skirmishAiCallback_UnitDef_isSquareResourceExtractor(
		int skirmishAIId, int unitDefId, int resourceId) {

	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return ud->extractSquare;
	} else {
		return false;
	}
}

EXPORT(float) skirmishAiCallback_UnitDef_getBuildTime(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->buildTime;
}

EXPORT(float) skirmishAiCallback_UnitDef_getAutoHeal(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->autoHeal;
}

EXPORT(float) skirmishAiCallback_UnitDef_getIdleAutoHeal(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->idleAutoHeal;
}

EXPORT(int) skirmishAiCallback_UnitDef_getIdleTime(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->idleTime;
}

EXPORT(float) skirmishAiCallback_UnitDef_getPower(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->power;
}

EXPORT(float) skirmishAiCallback_UnitDef_getHealth(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->health;
}

EXPORT(int) skirmishAiCallback_UnitDef_getCategory(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->category;
}

EXPORT(float) skirmishAiCallback_UnitDef_getSpeed(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->speed;
}

EXPORT(float) skirmishAiCallback_UnitDef_getTurnRate(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->turnRate;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isTurnInPlace(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->turnInPlace;
}

EXPORT(float) skirmishAiCallback_UnitDef_getTurnInPlaceDistance(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->turnInPlaceDistance;
}

EXPORT(float) skirmishAiCallback_UnitDef_getTurnInPlaceSpeedLimit(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->turnInPlaceSpeedLimit;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isUpright(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->upright;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isCollide(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->collide;
}

EXPORT(float) skirmishAiCallback_UnitDef_getLosRadius(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->losRadius;
}

EXPORT(float) skirmishAiCallback_UnitDef_getAirLosRadius(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->airLosRadius;
}

EXPORT(float) skirmishAiCallback_UnitDef_getLosHeight(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->losHeight;
}

EXPORT(int) skirmishAiCallback_UnitDef_getRadarRadius(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->radarRadius;
}

EXPORT(int) skirmishAiCallback_UnitDef_getSonarRadius(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->sonarRadius;
}

EXPORT(int) skirmishAiCallback_UnitDef_getJammerRadius(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->jammerRadius;
}

EXPORT(int) skirmishAiCallback_UnitDef_getSonarJamRadius(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->sonarJamRadius;
}

EXPORT(int) skirmishAiCallback_UnitDef_getSeismicRadius(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->seismicRadius;
}

EXPORT(float) skirmishAiCallback_UnitDef_getSeismicSignature(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->seismicSignature;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isStealth(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->stealth;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isSonarStealth(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->sonarStealth;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isBuildRange3D(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->buildRange3D;
}

EXPORT(float) skirmishAiCallback_UnitDef_getBuildDistance(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->buildDistance;
}

EXPORT(float) skirmishAiCallback_UnitDef_getBuildSpeed(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->buildSpeed;
}

EXPORT(float) skirmishAiCallback_UnitDef_getReclaimSpeed(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->reclaimSpeed;
}

EXPORT(float) skirmishAiCallback_UnitDef_getRepairSpeed(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->repairSpeed;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMaxRepairSpeed(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->maxRepairSpeed;
}

EXPORT(float) skirmishAiCallback_UnitDef_getResurrectSpeed(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->resurrectSpeed;
}

EXPORT(float) skirmishAiCallback_UnitDef_getCaptureSpeed(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->captureSpeed;
}

EXPORT(float) skirmishAiCallback_UnitDef_getTerraformSpeed(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->terraformSpeed;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMass(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->mass;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isPushResistant(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->pushResistant;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isStrafeToAttack(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->strafeToAttack;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMinCollisionSpeed(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->minCollisionSpeed;
}

EXPORT(float) skirmishAiCallback_UnitDef_getSlideTolerance(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->slideTolerance;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMaxSlope(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->maxSlope;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMaxHeightDif(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->maxHeightDif;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMinWaterDepth(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->minWaterDepth;
}

EXPORT(float) skirmishAiCallback_UnitDef_getWaterline(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->waterline;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMaxWaterDepth(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->maxWaterDepth;
}

EXPORT(float) skirmishAiCallback_UnitDef_getArmoredMultiple(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->armoredMultiple;
}

EXPORT(int) skirmishAiCallback_UnitDef_getArmorType(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->armorType;
}
EXPORT(int) skirmishAiCallback_UnitDef_FlankingBonus_getMode(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->flankingBonusMode;
}

EXPORT(void) skirmishAiCallback_UnitDef_FlankingBonus_getDir(int skirmishAIId, int unitDefId, float* return_posF3_out) {
	getUnitDefById(skirmishAIId, unitDefId)->flankingBonusDir.copyInto(return_posF3_out);
}

EXPORT(float) skirmishAiCallback_UnitDef_FlankingBonus_getMax(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->flankingBonusMax;
}

EXPORT(float) skirmishAiCallback_UnitDef_FlankingBonus_getMin(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->flankingBonusMin;
}

EXPORT(float) skirmishAiCallback_UnitDef_FlankingBonus_getMobilityAdd(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->flankingBonusMobilityAdd;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMaxWeaponRange(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->maxWeaponRange;
}

EXPORT(const char*) skirmishAiCallback_UnitDef_getType(int skirmishAIId, int unitDefId) {
	return "$$deprecated$$";
}

EXPORT(const char*) skirmishAiCallback_UnitDef_getTooltip(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->tooltip.c_str();
}

EXPORT(const char*) skirmishAiCallback_UnitDef_getWreckName(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->wreckName.c_str();
}

EXPORT(const char*) skirmishAiCallback_UnitDef_getDeathExplosion(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->deathExplosion.c_str();
}

EXPORT(const char*) skirmishAiCallback_UnitDef_getSelfDExplosion(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->selfDExplosion.c_str();
}

EXPORT(const char*) skirmishAiCallback_UnitDef_getCategoryString(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->categoryString.c_str();
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToSelfD(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canSelfD;
}

EXPORT(int) skirmishAiCallback_UnitDef_getSelfDCountdown(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->selfDCountdown;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToSubmerge(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canSubmerge;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToFly(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canfly;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToMove(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canmove;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToHover(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canhover;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isFloater(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->floater;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isBuilder(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->builder;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isActivateWhenBuilt(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->activateWhenBuilt;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isOnOffable(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->onoffable;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isFullHealthFactory(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->fullHealthFactory;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isFactoryHeadingTakeoff(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->factoryHeadingTakeoff;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isReclaimable(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->reclaimable;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isCapturable(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->capturable;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToRestore(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canRestore;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToRepair(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canRepair;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToSelfRepair(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canSelfRepair;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToReclaim(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canReclaim;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToAttack(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canAttack;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToPatrol(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canPatrol;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToFight(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canFight;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToGuard(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canGuard;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToAssist(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canAssist;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAssistable(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canBeAssisted;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToRepeat(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canRepeat;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToFireControl(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canFireControl;
}

EXPORT(int) skirmishAiCallback_UnitDef_getFireState(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->fireState;
}

EXPORT(int) skirmishAiCallback_UnitDef_getMoveState(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->moveState;
}

EXPORT(float) skirmishAiCallback_UnitDef_getWingDrag(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->wingDrag;
}

EXPORT(float) skirmishAiCallback_UnitDef_getWingAngle(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->wingAngle;
}

EXPORT(float) skirmishAiCallback_UnitDef_getDrag(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->drag;
}

EXPORT(float) skirmishAiCallback_UnitDef_getFrontToSpeed(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->frontToSpeed;
}

EXPORT(float) skirmishAiCallback_UnitDef_getSpeedToFront(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->speedToFront;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMyGravity(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->myGravity;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMaxBank(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->maxBank;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMaxPitch(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->maxPitch;
}

EXPORT(float) skirmishAiCallback_UnitDef_getTurnRadius(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->turnRadius;
}

EXPORT(float) skirmishAiCallback_UnitDef_getWantedHeight(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->wantedHeight;
}

EXPORT(float) skirmishAiCallback_UnitDef_getVerticalSpeed(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->verticalSpeed;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToCrash(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canCrash;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isHoverAttack(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->hoverAttack;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAirStrafe(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->airStrafe;
}

EXPORT(float) skirmishAiCallback_UnitDef_getDlHoverFactor(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->dlHoverFactor;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMaxAcceleration(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->maxAcc;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMaxDeceleration(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->maxDec;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMaxAileron(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->maxAileron;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMaxElevator(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->maxElevator;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMaxRudder(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->maxRudder;
}

EXPORT(int) skirmishAiCallback_UnitDef_getYardMap(int skirmishAIId, int unitDefId, int facing, short* yardMap, int yardMap_sizeMax) {

	const UnitDef* unitDef = getUnitDefById(skirmishAIId, unitDefId);
	const std::vector<unsigned char>& yardMapInternal = unitDef->GetYardMap(facing);

	int yardMapSize = yardMapInternal.size();

	if ((yardMap != NULL) && !yardMapInternal.empty()) {
		yardMapSize = min(yardMapInternal.size(), yardMap_sizeMax);

		for (int i = 0; i < yardMapSize; ++i) {
			yardMap[i] = (short) yardMapInternal[i];
		}
	}

	return yardMapSize;
}

EXPORT(int) skirmishAiCallback_UnitDef_getXSize(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->xsize;
}

EXPORT(int) skirmishAiCallback_UnitDef_getZSize(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->zsize;
}

EXPORT(int) skirmishAiCallback_UnitDef_getBuildAngle(int skirmishAIId, int unitDefId) {
	return 0;
}

EXPORT(float) skirmishAiCallback_UnitDef_getLoadingRadius(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->loadingRadius;
}

EXPORT(float) skirmishAiCallback_UnitDef_getUnloadSpread(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->unloadSpread;
}

EXPORT(int) skirmishAiCallback_UnitDef_getTransportCapacity(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->transportCapacity;
}

EXPORT(int) skirmishAiCallback_UnitDef_getTransportSize(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->transportSize;
}

EXPORT(int) skirmishAiCallback_UnitDef_getMinTransportSize(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->minTransportSize;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAirBase(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->isAirBase;
}

EXPORT(float) skirmishAiCallback_UnitDef_getTransportMass(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->transportMass;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMinTransportMass(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->minTransportMass;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isHoldSteady(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->holdSteady;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isReleaseHeld(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->releaseHeld;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isNotTransportable(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->cantBeTransported;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isTransportByEnemy(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->transportByEnemy;
}

EXPORT(int) skirmishAiCallback_UnitDef_getTransportUnloadMethod(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->transportUnloadMethod;
}

EXPORT(float) skirmishAiCallback_UnitDef_getFallSpeed(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->fallSpeed;
}

EXPORT(float) skirmishAiCallback_UnitDef_getUnitFallSpeed(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->unitFallSpeed;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToCloak(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canCloak;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isStartCloaked(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->startCloaked;
}

EXPORT(float) skirmishAiCallback_UnitDef_getCloakCost(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->cloakCost;
}

EXPORT(float) skirmishAiCallback_UnitDef_getCloakCostMoving(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->cloakCostMoving;
}

EXPORT(float) skirmishAiCallback_UnitDef_getDecloakDistance(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->decloakDistance;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isDecloakSpherical(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->decloakSpherical;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isDecloakOnFire(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->decloakOnFire;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToKamikaze(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canKamikaze;
}

EXPORT(float) skirmishAiCallback_UnitDef_getKamikazeDist(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->kamikazeDist;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isTargetingFacility(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->targfac;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToDGun(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canDGun;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isNeedGeo(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->needGeo;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isFeature(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->isFeature;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isHideDamage(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->hideDamage;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isCommander(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->isCommander;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isShowPlayerName(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->showPlayerName;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToResurrect(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canResurrect;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToCapture(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canCapture;
}

EXPORT(int) skirmishAiCallback_UnitDef_getHighTrajectoryType(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->highTrajectoryType;
}

EXPORT(int) skirmishAiCallback_UnitDef_getNoChaseCategory(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->noChaseCategory;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isLeaveTracks(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->leaveTracks;
}

EXPORT(float) skirmishAiCallback_UnitDef_getTrackWidth(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->trackWidth;
}

EXPORT(float) skirmishAiCallback_UnitDef_getTrackOffset(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->trackOffset;
}

EXPORT(float) skirmishAiCallback_UnitDef_getTrackStrength(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->trackStrength;
}

EXPORT(float) skirmishAiCallback_UnitDef_getTrackStretch(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->trackStretch;
}

EXPORT(int) skirmishAiCallback_UnitDef_getTrackType(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->trackType;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToDropFlare(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canDropFlare;
}

EXPORT(float) skirmishAiCallback_UnitDef_getFlareReloadTime(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->flareReloadTime;
}

EXPORT(float) skirmishAiCallback_UnitDef_getFlareEfficiency(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->flareEfficiency;
}

EXPORT(float) skirmishAiCallback_UnitDef_getFlareDelay(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->flareDelay;
}

EXPORT(void) skirmishAiCallback_UnitDef_getFlareDropVector(int skirmishAIId, int unitDefId, float* return_posF3_out) {
	getUnitDefById(skirmishAIId, unitDefId)->flareDropVector.copyInto(return_posF3_out);
}

EXPORT(int) skirmishAiCallback_UnitDef_getFlareTime(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->flareTime;
}

EXPORT(int) skirmishAiCallback_UnitDef_getFlareSalvoSize(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->flareSalvoSize;
}

EXPORT(int) skirmishAiCallback_UnitDef_getFlareSalvoDelay(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->flareSalvoDelay;
}

//EXPORT(bool) skirmishAiCallback_UnitDef_isSmoothAnim(int skirmishAIId, int unitDefId) {
//	return getUnitDefById(skirmishAIId, unitDefId)->smoothAnim;
//}

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToLoopbackAttack(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canLoopbackAttack;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isLevelGround(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->levelGround;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isUseBuildingGroundDecal(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->useBuildingGroundDecal;
}

EXPORT(int) skirmishAiCallback_UnitDef_getBuildingDecalType(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->buildingDecalType;
}

EXPORT(int) skirmishAiCallback_UnitDef_getBuildingDecalSizeX(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->buildingDecalSizeX;
}

EXPORT(int) skirmishAiCallback_UnitDef_getBuildingDecalSizeY(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->buildingDecalSizeY;
}

EXPORT(float) skirmishAiCallback_UnitDef_getBuildingDecalDecaySpeed(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->buildingDecalDecaySpeed;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isFirePlatform(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->isFirePlatform;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMaxFuel(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->maxFuel;
}

EXPORT(float) skirmishAiCallback_UnitDef_getRefuelTime(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->refuelTime;
}

EXPORT(float) skirmishAiCallback_UnitDef_getMinAirBasePower(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->minAirBasePower;
}

EXPORT(int) skirmishAiCallback_UnitDef_getMaxThisUnit(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->maxThisUnit;
}

EXPORT(int) skirmishAiCallback_UnitDef_getDecoyDef(int skirmishAIId, int unitDefId) {

	const UnitDef* decoyDef = getUnitDefById(skirmishAIId, unitDefId)->decoyDef;
	return (decoyDef == NULL) ? -1 : decoyDef->id;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isDontLand(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->DontLand();
}

EXPORT(int) skirmishAiCallback_UnitDef_getShieldDef(int skirmishAIId, int unitDefId) {

	const WeaponDef* wd = getUnitDefById(skirmishAIId, unitDefId)->shieldWeaponDef;
	if (wd == NULL) {
		return -1;
	} else {
		return wd->id;
	}
}

EXPORT(int) skirmishAiCallback_UnitDef_getStockpileDef(int skirmishAIId,
		int unitDefId) {

	const WeaponDef* wd = getUnitDefById(skirmishAIId, unitDefId)->stockpileWeaponDef;
	if (wd == NULL) {
		return -1;
	} else {
		return wd->id;
	}
}

EXPORT(int) skirmishAiCallback_UnitDef_getBuildOptions(int skirmishAIId,
		int unitDefId, int* unitDefIds, int unitDefIds_sizeMax) {

	const std::map<int,std::string>& bo = getUnitDefById(skirmishAIId, unitDefId)->buildOptions;
	const size_t unitDefIds_sizeReal = bo.size();

	size_t unitDefIds_size = unitDefIds_sizeReal;

	if (unitDefIds != NULL) {
		unitDefIds_size = min(unitDefIds_sizeReal, unitDefIds_sizeMax);
		std::map<int,std::string>::const_iterator bb;
		size_t b;
		for (b=0, bb = bo.begin(); bb != bo.end() && b < unitDefIds_size; ++b, ++bb) {
			unitDefIds[b] = skirmishAiCallback_getUnitDefByName(skirmishAIId, bb->second.c_str());
		}
	}

	return unitDefIds_size;
}

EXPORT(int) skirmishAiCallback_UnitDef_getCustomParams(int skirmishAIId, int unitDefId,
		const char** keys, const char** values) {

	const std::map<std::string,std::string>& ps = getUnitDefById(skirmishAIId, unitDefId)->customParams;
	const size_t params_sizeReal = ps.size();

	if ((keys != NULL) && (values != NULL)) {
		fillCMap(&ps, keys, values);
	}

	return params_sizeReal;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isMoveDataAvailable(int skirmishAIId, int unitDefId) {
	// can not use getUnitDefMoveDataById() here, cause it would assert
	return (getUnitDefById(skirmishAIId, unitDefId)->movedata != NULL);
}

EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getMaxAcceleration(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->maxAcceleration;
}

EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getMaxBreaking(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->maxBreaking;
}

EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getMaxSpeed(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->maxSpeed;
}

EXPORT(short) skirmishAiCallback_UnitDef_MoveData_getMaxTurnRate(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->maxTurnRate;
}

EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getXSize(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->xsize;
}

EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getZSize(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->zsize;
}

EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getDepth(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->depth;
}

EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getMaxSlope(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->maxSlope;
}

EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getSlopeMod(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->slopeMod;
}

EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getDepthMod(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->depthMod;
}

EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getPathType(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->pathType;
}

EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getCrushStrength(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->crushStrength;
}

EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getMoveType(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->moveType;
}

EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getMoveFamily(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->moveFamily;
}

EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getTerrainClass(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->terrainClass;
}

EXPORT(bool) skirmishAiCallback_UnitDef_MoveData_getFollowGround(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->followGround;
}

EXPORT(bool) skirmishAiCallback_UnitDef_MoveData_isSubMarine(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->subMarine;
}

EXPORT(const char*) skirmishAiCallback_UnitDef_MoveData_getName(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDataById(skirmishAIId, unitDefId)->name.c_str();
}



EXPORT(int) skirmishAiCallback_UnitDef_getWeaponMounts(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->weapons.size();
}

EXPORT(const char*) skirmishAiCallback_UnitDef_WeaponMount_getName(int skirmishAIId, int unitDefId, int weaponMountId) {
	return getUnitDefById(skirmishAIId, unitDefId)->weapons.at(weaponMountId).name.c_str();
}

EXPORT(int) skirmishAiCallback_UnitDef_WeaponMount_getWeaponDef(int skirmishAIId, int unitDefId, int weaponMountId) {
	return getUnitDefById(skirmishAIId, unitDefId)->weapons.at(weaponMountId).def->id;
}

EXPORT(int) skirmishAiCallback_UnitDef_WeaponMount_getSlavedTo(int skirmishAIId, int unitDefId, int weaponMountId) {
	return getUnitDefById(skirmishAIId, unitDefId)->weapons.at(weaponMountId).slavedTo;
}

EXPORT(void) skirmishAiCallback_UnitDef_WeaponMount_getMainDir(int skirmishAIId, int unitDefId, int weaponMountId, float* return_posF3_out) {
	getUnitDefById(skirmishAIId, unitDefId)->weapons.at(weaponMountId).mainDir.copyInto(return_posF3_out);
}

EXPORT(float) skirmishAiCallback_UnitDef_WeaponMount_getMaxAngleDif(int skirmishAIId, int unitDefId, int weaponMountId) {
	return getUnitDefById(skirmishAIId, unitDefId)->weapons.at(weaponMountId).maxAngleDif;
}

EXPORT(float) skirmishAiCallback_UnitDef_WeaponMount_getFuelUsage(int skirmishAIId, int unitDefId, int weaponMountId) {
	return getUnitDefById(skirmishAIId, unitDefId)->weapons.at(weaponMountId).fuelUsage;
}

EXPORT(int) skirmishAiCallback_UnitDef_WeaponMount_getBadTargetCategory(int skirmishAIId, int unitDefId, int weaponMountId) {
	return getUnitDefById(skirmishAIId, unitDefId)->weapons.at(weaponMountId).badTargetCat;
}

EXPORT(int) skirmishAiCallback_UnitDef_WeaponMount_getOnlyTargetCategory(int skirmishAIId, int unitDefId, int weaponMountId) {
	return getUnitDefById(skirmishAIId, unitDefId)->weapons.at(weaponMountId).onlyTargetCat;
}

//########### END UnitDef





//########### BEGINN Unit
EXPORT(int) skirmishAiCallback_Unit_getLimit(int skirmishAIId) {
	return uh->MaxUnitsPerTeam();
}

EXPORT(int) skirmishAiCallback_Unit_getMax(int skirmishAIId) {
	return uh->MaxUnits();
}

EXPORT(int) skirmishAiCallback_Unit_getDef(int skirmishAIId, int unitId) {

	const UnitDef* unitDef;

	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		unitDef = skirmishAIId_cheatCallback[skirmishAIId]->GetUnitDef(unitId);
	} else {
		unitDef = skirmishAIId_callback[skirmishAIId]->GetUnitDef(unitId);
	}

	if (unitDef != NULL) {
		return unitDef->id;
	} else {
		return -1;
	}
}

EXPORT(int) skirmishAiCallback_Unit_getModParams(int skirmishAIId, int unitId) {

	const CUnit* unit = getUnit(unitId);
	if (unit && /*(skirmishAiCallback_Cheats_isEnabled(skirmishAIId) || */isAlliedUnit(skirmishAIId, unit)/*)*/) {
		return unit->modParams.size();
	} else {
		return 0;
	}
}

EXPORT(const char*) skirmishAiCallback_Unit_ModParam_getName(int skirmishAIId,
		int unitId, int modParamId)
{
	const char* name = "";

	const CUnit* unit = getUnit(unitId);
	if (unit && unitModParamIsVisible(skirmishAIId, *unit, modParamId)) {
		std::map<std::string, int>::const_iterator mi, mb, me;
		mb = unit->modParamsMap.begin();
		me = unit->modParamsMap.end();
		for (mi = mb; mi != me; ++mi) {
			if (mi->second == modParamId) {
				name = mi->first.c_str();
			}
		}
	}

	return name;
}

EXPORT(float) skirmishAiCallback_Unit_ModParam_getValue(int skirmishAIId,
		int unitId, int modParamId)
{
	float value = 0.0f;

	const CUnit* unit = getUnit(unitId);
	if (unit && unitModParamIsVisible(skirmishAIId, *unit, modParamId)) {
		value = unit->modParams[modParamId].value;
	}

	return value;
}

EXPORT(int) skirmishAiCallback_Unit_getTeam(int skirmishAIId, int unitId) {
//	return skirmishAIId_callback[skirmishAIId]->GetUnitTeam(unitId);
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->GetUnitTeam(unitId);
	} else {
		return skirmishAIId_callback[skirmishAIId]->GetUnitTeam(unitId);
	}
}

EXPORT(int) skirmishAiCallback_Unit_getAllyTeam(int skirmishAIId, int unitId) {
//	return skirmishAIId_callback[skirmishAIId]->GetUnitAllyTeam(unitId);
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->GetUnitAllyTeam(unitId);
	} else {
		return skirmishAIId_callback[skirmishAIId]->GetUnitAllyTeam(unitId);
	}
}

EXPORT(int) skirmishAiCallback_Unit_getAiHint(int skirmishAIId, int unitId) {
	return 0;
}

EXPORT(int) skirmishAiCallback_Unit_getSupportedCommands(int skirmishAIId, int unitId) {
	return skirmishAIId_callback[skirmishAIId]->GetUnitCommands(unitId)->size();
}

EXPORT(int) skirmishAiCallback_Unit_SupportedCommand_getId(int skirmishAIId, int unitId, int supportedCommandId) {
	return skirmishAIId_callback[skirmishAIId]->GetUnitCommands(unitId)->at(supportedCommandId).id;
}

EXPORT(const char*) skirmishAiCallback_Unit_SupportedCommand_getName(int skirmishAIId, int unitId, int supportedCommandId) {
	return skirmishAIId_callback[skirmishAIId]->GetUnitCommands(unitId)->at(supportedCommandId).name.c_str();
}

EXPORT(const char*) skirmishAiCallback_Unit_SupportedCommand_getToolTip(int skirmishAIId, int unitId, int supportedCommandId) {
	return skirmishAIId_callback[skirmishAIId]->GetUnitCommands(unitId)->at(supportedCommandId).tooltip.c_str();
}

EXPORT(bool) skirmishAiCallback_Unit_SupportedCommand_isShowUnique(int skirmishAIId, int unitId, int supportedCommandId) {
	return skirmishAIId_callback[skirmishAIId]->GetUnitCommands(unitId)->at(supportedCommandId).showUnique;
}

EXPORT(bool) skirmishAiCallback_Unit_SupportedCommand_isDisabled(int skirmishAIId, int unitId, int supportedCommandId) {
	return skirmishAIId_callback[skirmishAIId]->GetUnitCommands(unitId)->at(supportedCommandId).disabled;
}

EXPORT(int) skirmishAiCallback_Unit_SupportedCommand_getParams(int skirmishAIId,
		int unitId, int supportedCommandId, const char** params, int params_sizeMax) {

	const std::vector<std::string> ps = skirmishAIId_callback[skirmishAIId]->GetUnitCommands(unitId)->at(supportedCommandId).params;
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

EXPORT(int) skirmishAiCallback_Unit_getStockpile(int skirmishAIId, int unitId) {
	CAICallback* clb = skirmishAIId_callback[skirmishAIId];
	int stockpile;
	const bool fetchOk = clb->GetProperty(unitId, AIVAL_STOCKPILED, &stockpile);
	if (!fetchOk) {
		stockpile = -1;
	}
	return stockpile;
}

EXPORT(int) skirmishAiCallback_Unit_getStockpileQueued(int skirmishAIId, int unitId) {
	CAICallback* clb = skirmishAIId_callback[skirmishAIId];
	int stockpileQueue;
	const bool fetchOk = clb->GetProperty(unitId, AIVAL_STOCKPILE_QUED, &stockpileQueue);
	if (!fetchOk) {
		stockpileQueue = -1;
	}
	return stockpileQueue;
}

EXPORT(float) skirmishAiCallback_Unit_getCurrentFuel(int skirmishAIId, int unitId) {
	CAICallback* clb = skirmishAIId_callback[skirmishAIId];
	float currentFuel;
	const bool fetchOk = clb->GetProperty(unitId, AIVAL_CURRENT_FUEL, &currentFuel);
	if (!fetchOk) {
		currentFuel = -1.0f;
	}
	return currentFuel;
}

EXPORT(float) skirmishAiCallback_Unit_getMaxSpeed(int skirmishAIId, int unitId) {
	CAICallback* clb = skirmishAIId_callback[skirmishAIId];
	float maxSpeed;
	const bool fetchOk = clb->GetProperty(unitId, AIVAL_UNIT_MAXSPEED, &maxSpeed);
	if (!fetchOk) {
		maxSpeed = -1.0f;
	}
	return maxSpeed;
}

EXPORT(float) skirmishAiCallback_Unit_getMaxRange(int skirmishAIId, int unitId) {
	CAICallback* clb = skirmishAIId_callback[skirmishAIId];
	return clb->GetUnitMaxRange(unitId);
}

EXPORT(float) skirmishAiCallback_Unit_getMaxHealth(int skirmishAIId, int unitId) {
//	return skirmishAIId_callback[skirmishAIId]->GetUnitMaxHealth(unitId);
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->GetUnitMaxHealth(unitId);
	} else {
		return skirmishAIId_callback[skirmishAIId]->GetUnitMaxHealth(unitId);
	}
}


/**
 * Returns a units command queue.
 * The return value may be <code>NULL</code> in some cases,
 * eg. when cheats are disabled and we try to fetch from an enemy unit.
 * For internal use only.
 */
static inline const CCommandQueue* _intern_Unit_getCurrentCommandQueue(int skirmishAIId, int unitId) {
	const CCommandQueue* q = NULL;

	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		q = skirmishAIId_cheatCallback[skirmishAIId]->GetCurrentUnitCommands(unitId);
	} else {
		q = skirmishAIId_callback[skirmishAIId]->GetCurrentUnitCommands(unitId);
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

EXPORT(int) skirmishAiCallback_Unit_getCurrentCommands(int skirmishAIId, int unitId) {
	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(skirmishAIId, unitId);
	return (q? q->size(): 0);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getType(int skirmishAIId, int unitId) {
	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(skirmishAIId, unitId);
	return (q? q->GetType(): -1);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getId(int skirmishAIId, int unitId, int commandId) {

	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(skirmishAIId, unitId);
	return (CHECK_COMMAND_ID(q, commandId) ? q->at(commandId).id : 0);
}

EXPORT(short) skirmishAiCallback_Unit_CurrentCommand_getOptions(int skirmishAIId, int unitId, int commandId) {

	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(skirmishAIId, unitId);
	return (CHECK_COMMAND_ID(q, commandId) ? q->at(commandId).options : 0);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getTag(int skirmishAIId, int unitId, int commandId) {

	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(skirmishAIId, unitId);
	return (CHECK_COMMAND_ID(q, commandId) ? q->at(commandId).tag : 0);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getTimeOut(int skirmishAIId, int unitId, int commandId) {

	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(skirmishAIId, unitId);
	return (CHECK_COMMAND_ID(q, commandId) ? q->at(commandId).timeOut : 0);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getParams(int skirmishAIId,
		int unitId, int commandId, float* params, int params_sizeMax) {

	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(skirmishAIId, unitId);

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



EXPORT(float) skirmishAiCallback_Unit_getExperience(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->GetUnitExperience(unitId);
	} else {
		return skirmishAIId_callback[skirmishAIId]->GetUnitExperience(unitId);
	}
}

EXPORT(int) skirmishAiCallback_Unit_getGroup(int skirmishAIId, int unitId) {
	return skirmishAIId_callback[skirmishAIId]->GetUnitGroup(unitId);
}

EXPORT(float) skirmishAiCallback_Unit_getHealth(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->GetUnitHealth(unitId);
	} else {
		return skirmishAIId_callback[skirmishAIId]->GetUnitHealth(unitId);
	}
}

EXPORT(float) skirmishAiCallback_Unit_getSpeed(int skirmishAIId, int unitId) {
	return skirmishAIId_callback[skirmishAIId]->GetUnitSpeed(unitId);
}

EXPORT(float) skirmishAiCallback_Unit_getPower(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->GetUnitPower(unitId);
	} else {
		return skirmishAIId_callback[skirmishAIId]->GetUnitPower(unitId);
	}
}

EXPORT(void) skirmishAiCallback_Unit_getPos(int skirmishAIId, int unitId, float* return_posF3_out) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		skirmishAIId_cheatCallback[skirmishAIId]->GetUnitPos(unitId).copyInto(return_posF3_out);
	} else {
		skirmishAIId_callback[skirmishAIId]->GetUnitPos(unitId).copyInto(return_posF3_out);
	}
}

EXPORT(void) skirmishAiCallback_Unit_getVel(int skirmishAIId, int unitId, float* return_posF3_out) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		skirmishAIId_cheatCallback[skirmishAIId]->GetUnitVelocity(unitId).copyInto(return_posF3_out);
	} else {
		skirmishAIId_callback[skirmishAIId]->GetUnitVelocity(unitId).copyInto(return_posF3_out);
	}
}

//EXPORT(int) skirmishAiCallback_Unit_0MULTI1SIZE0ResourceInfo(int skirmishAIId, int unitId) {
//	return skirmishAiCallback_0MULTI1SIZE0Resource(skirmishAIId);
//}

EXPORT(float) skirmishAiCallback_Unit_getResourceUse(int skirmishAIId,
		int unitId, int resourceId) {

	int res = -1.0F;
	UnitResourceInfo resourceInfo;

	bool fetchOk;
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		fetchOk = skirmishAIId_cheatCallback[skirmishAIId]->GetUnitResourceInfo(unitId,
				&resourceInfo);
	} else {
		fetchOk = skirmishAIId_callback[skirmishAIId]->GetUnitResourceInfo(unitId,
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

EXPORT(float) skirmishAiCallback_Unit_getResourceMake(int skirmishAIId,
		int unitId, int resourceId) {

	int res = -1.0F;
	UnitResourceInfo resourceInfo;

	bool fetchOk;
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		fetchOk = skirmishAIId_cheatCallback[skirmishAIId]->GetUnitResourceInfo(unitId,
				&resourceInfo);
	} else {
		fetchOk = skirmishAIId_callback[skirmishAIId]->GetUnitResourceInfo(unitId,
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

EXPORT(bool) skirmishAiCallback_Unit_isActivated(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->IsUnitActivated(unitId);
	} else {
		return skirmishAIId_callback[skirmishAIId]->IsUnitActivated(unitId);
	}
}

EXPORT(bool) skirmishAiCallback_Unit_isBeingBuilt(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->UnitBeingBuilt(unitId);
	} else {
		return skirmishAIId_callback[skirmishAIId]->UnitBeingBuilt(unitId);
	}
}

EXPORT(bool) skirmishAiCallback_Unit_isCloaked(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->IsUnitCloaked(unitId);
	} else {
		return skirmishAIId_callback[skirmishAIId]->IsUnitCloaked(unitId);
	}
}

EXPORT(bool) skirmishAiCallback_Unit_isParalyzed(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->IsUnitParalyzed(unitId);
	} else {
		return skirmishAIId_callback[skirmishAIId]->IsUnitParalyzed(unitId);
	}
}

EXPORT(bool) skirmishAiCallback_Unit_isNeutral(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->IsUnitNeutral(unitId);
	} else {
		return skirmishAIId_callback[skirmishAIId]->IsUnitNeutral(unitId);
	}
}

EXPORT(int) skirmishAiCallback_Unit_getBuildingFacing(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->GetBuildingFacing(unitId);
	} else {
		return skirmishAIId_callback[skirmishAIId]->GetBuildingFacing(unitId);
	}
}

EXPORT(int) skirmishAiCallback_Unit_getLastUserOrderFrame(int skirmishAIId, int unitId) {

	if (!isControlledByLocalPlayer(skirmishAIId)) return -1;

	return uh->units[unitId]->commandAI->lastUserCommand;
}

//########### END Unit

EXPORT(int) skirmishAiCallback_getEnemyUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->GetEnemyUnits(unitIds, unitIds_sizeMax);
	} else {
		return skirmishAIId_callback[skirmishAIId]->GetEnemyUnits(unitIds, unitIds_sizeMax);
	}
}

EXPORT(int) skirmishAiCallback_getEnemyUnitsIn(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->GetEnemyUnits(unitIds, pos_posF3, radius, unitIds_sizeMax);
	} else {
		return skirmishAIId_callback[skirmishAIId]->GetEnemyUnits(unitIds, pos_posF3, radius, unitIds_sizeMax);
	}
}

EXPORT(int) skirmishAiCallback_getEnemyUnitsInRadarAndLos(int skirmishAIId, int* unitIds, int unitIds_sizeMax) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		// with cheats on, act like global-LOS -> getEnemyUnitsIn() == getEnemyUnitsInRadarAndLos()
		return skirmishAIId_cheatCallback[skirmishAIId]->GetEnemyUnits(unitIds, unitIds_sizeMax);
	} else {
		return skirmishAIId_callback[skirmishAIId]->GetEnemyUnitsInRadarAndLos(unitIds, unitIds_sizeMax);
	}
}

EXPORT(int) skirmishAiCallback_getFriendlyUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax) {
	return skirmishAIId_callback[skirmishAIId]->GetFriendlyUnits(unitIds, unitIds_sizeMax);
}

EXPORT(int) skirmishAiCallback_getFriendlyUnitsIn(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax) {
	return skirmishAIId_callback[skirmishAIId]->GetFriendlyUnits(unitIds, pos_posF3, radius, unitIds_sizeMax);
}

EXPORT(int) skirmishAiCallback_getNeutralUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->GetNeutralUnits(unitIds, unitIds_sizeMax);
	} else {
		return skirmishAIId_callback[skirmishAIId]->GetNeutralUnits(unitIds, unitIds_sizeMax);
	}
}

EXPORT(int) skirmishAiCallback_getNeutralUnitsIn(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIds_sizeMax) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		return skirmishAIId_cheatCallback[skirmishAIId]->GetNeutralUnits(unitIds, pos_posF3, radius, unitIds_sizeMax);
	} else {
		return skirmishAIId_callback[skirmishAIId]->GetNeutralUnits(unitIds, pos_posF3, radius, unitIds_sizeMax);
	}
}

EXPORT(int) skirmishAiCallback_getSelectedUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax) {
	return skirmishAIId_callback[skirmishAIId]->GetSelectedUnits(unitIds, unitIds_sizeMax);
}

EXPORT(int) skirmishAiCallback_getTeamUnits(int skirmishAIId, int* unitIds, int unitIds_sizeMax) {

	int a = 0;

	const int teamId = skirmishAIId_teamId[skirmishAIId];
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
EXPORT(int) skirmishAiCallback_getFeatureDefs(int skirmishAIId, int* featureDefIds, int featureDefIds_sizeMax) {

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

EXPORT(const char*) skirmishAiCallback_FeatureDef_getName(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->myName.c_str();
}

EXPORT(const char*) skirmishAiCallback_FeatureDef_getDescription(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->description.c_str();
}

EXPORT(const char*) skirmishAiCallback_FeatureDef_getFileName(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->filename.c_str();
}

//EXPORT(int) skirmishAiCallback_FeatureDef_getId(int skirmishAIId, int featureDefId) {
//	return getFeatureDefById(skirmishAIId, featureDefId)->id;
//}

EXPORT(float) skirmishAiCallback_FeatureDef_getContainedResource(int skirmishAIId, int featureDefId, int resourceId) {

	const FeatureDef* fd = getFeatureDefById(skirmishAIId, featureDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return fd->metal;
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return fd->energy;
	} else {
		return 0.0f;
	}
}

EXPORT(float) skirmishAiCallback_FeatureDef_getMaxHealth(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->maxHealth;
}

EXPORT(float) skirmishAiCallback_FeatureDef_getReclaimTime(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->reclaimTime;
}

EXPORT(float) skirmishAiCallback_FeatureDef_getMass(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->mass;
}

EXPORT(bool) skirmishAiCallback_FeatureDef_isUpright(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->upright;
}

EXPORT(int) skirmishAiCallback_FeatureDef_getDrawType(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->drawType;
}

EXPORT(const char*) skirmishAiCallback_FeatureDef_getModelName(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->modelname.c_str();
}

EXPORT(int) skirmishAiCallback_FeatureDef_getResurrectable(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->resurrectable;
}

EXPORT(int) skirmishAiCallback_FeatureDef_getSmokeTime(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->smokeTime;
}

EXPORT(bool) skirmishAiCallback_FeatureDef_isDestructable(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->destructable;
}

EXPORT(bool) skirmishAiCallback_FeatureDef_isReclaimable(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->reclaimable;
}

EXPORT(bool) skirmishAiCallback_FeatureDef_isBlocking(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->blocking;
}

EXPORT(bool) skirmishAiCallback_FeatureDef_isBurnable(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->burnable;
}

EXPORT(bool) skirmishAiCallback_FeatureDef_isFloating(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->floating;
}

EXPORT(bool) skirmishAiCallback_FeatureDef_isNoSelect(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->noSelect;
}

EXPORT(bool) skirmishAiCallback_FeatureDef_isGeoThermal(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->geoThermal;
}

EXPORT(const char*) skirmishAiCallback_FeatureDef_getDeathFeature(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->deathFeature.c_str();
}

EXPORT(int) skirmishAiCallback_FeatureDef_getXSize(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->xsize;
}

EXPORT(int) skirmishAiCallback_FeatureDef_getZSize(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->zsize;
}

EXPORT(int) skirmishAiCallback_FeatureDef_getCustomParams(int skirmishAIId, int featureDefId,
		const char** keys, const char** values) {

	const std::map<std::string,std::string>& ps = getFeatureDefById(skirmishAIId, featureDefId)->customParams;
	const size_t params_sizeReal = ps.size();

	if ((keys != NULL) && (values != NULL)) {
		fillCMap(&ps, keys, values);
	}

	return params_sizeReal;
}

//########### END FeatureDef


EXPORT(int) skirmishAiCallback_getFeatures(int skirmishAIId, int* featureIds, int featureIds_sizeMax) {

	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
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
			featureIds_size = skirmishAIId_callback[skirmishAIId]->GetFeatures(tmpIntArr[skirmishAIId], TMP_ARR_SIZE);
		} else {
			// return size and values
			featureIds_size = skirmishAIId_callback[skirmishAIId]->GetFeatures(featureIds, featureIds_sizeMax);
		}

		return featureIds_size;
	}
}

EXPORT(int) skirmishAiCallback_getFeaturesIn(int skirmishAIId, float* pos_posF3, float radius, int* featureIds, int featureIds_sizeMax) {

	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
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
			featureIds_size = skirmishAIId_callback[skirmishAIId]->GetFeatures(tmpIntArr[skirmishAIId], TMP_ARR_SIZE, pos_posF3, radius);
		} else {
			// return size and values
			featureIds_size = skirmishAIId_callback[skirmishAIId]->GetFeatures(featureIds, featureIds_sizeMax, pos_posF3, radius);
		}

		return featureIds_size;
	}
}

EXPORT(int) skirmishAiCallback_Feature_getDef(int skirmishAIId, int featureId) {

	const FeatureDef* def = skirmishAIId_callback[skirmishAIId]->GetFeatureDef(featureId);
	if (def == NULL) {
		 return -1;
	} else {
		return def->id;
	}
}

EXPORT(float) skirmishAiCallback_Feature_getHealth(int skirmishAIId, int featureId) {
	return skirmishAIId_callback[skirmishAIId]->GetFeatureHealth(featureId);
}

EXPORT(float) skirmishAiCallback_Feature_getReclaimLeft(int skirmishAIId, int featureId) {
	return skirmishAIId_callback[skirmishAIId]->GetFeatureReclaimLeft(featureId);
}

EXPORT(void) skirmishAiCallback_Feature_getPosition(int skirmishAIId, int featureId, float* return_posF3_out) {
	skirmishAIId_callback[skirmishAIId]->GetFeaturePos(featureId).copyInto(return_posF3_out);
}


//########### BEGINN WeaponDef
EXPORT(int) skirmishAiCallback_getWeaponDefs(int skirmishAIId) {
	return weaponDefHandler->numWeaponDefs;
}

EXPORT(int) skirmishAiCallback_getWeaponDefByName(int skirmishAIId, const char* weaponDefName) {

	int weaponDefId = -1;

	const WeaponDef* wd = skirmishAIId_callback[skirmishAIId]->GetWeapon(weaponDefName);
	if (wd != NULL) {
		weaponDefId = wd->id;
	}

	return weaponDefId;
}

//EXPORT(int) skirmishAiCallback_WeaponDef_getId(int skirmishAIId, int weaponDefId) {
//	return getWeaponDefById(skirmishAIId, weaponDefId)->id;
//}

EXPORT(const char*) skirmishAiCallback_WeaponDef_getName(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->name.c_str();
}

EXPORT(const char*) skirmishAiCallback_WeaponDef_getType(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->type.c_str();
}

EXPORT(const char*) skirmishAiCallback_WeaponDef_getDescription(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->description.c_str();
}

EXPORT(const char*) skirmishAiCallback_WeaponDef_getFileName(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->filename.c_str();
}

EXPORT(const char*) skirmishAiCallback_WeaponDef_getCegTag(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->cegTag.c_str();
}

EXPORT(float) skirmishAiCallback_WeaponDef_getRange(int skirmishAIId, int weaponDefId) {
return getWeaponDefById(skirmishAIId, weaponDefId)->range;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getHeightMod(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->heightmod;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getAccuracy(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->accuracy;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getSprayAngle(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->sprayAngle;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getMovingAccuracy(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->movingAccuracy;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getTargetMoveError(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->targetMoveError;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getLeadLimit(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->leadLimit;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getLeadBonus(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->leadBonus;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getPredictBoost(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->predictBoost;
}

EXPORT(int) skirmishAiCallback_WeaponDef_Damage_getParalyzeDamageTime(int skirmishAIId, int weaponDefId) {
	DamageArray da = getWeaponDefById(skirmishAIId, weaponDefId)->damages;
	return da.paralyzeDamageTime;
}

EXPORT(float) skirmishAiCallback_WeaponDef_Damage_getImpulseFactor(int skirmishAIId, int weaponDefId) {
	DamageArray da = getWeaponDefById(skirmishAIId, weaponDefId)->damages;
	return da.impulseFactor;
}

EXPORT(float) skirmishAiCallback_WeaponDef_Damage_getImpulseBoost(int skirmishAIId, int weaponDefId) {
	DamageArray da = getWeaponDefById(skirmishAIId, weaponDefId)->damages;
	return da.impulseBoost;
}

EXPORT(float) skirmishAiCallback_WeaponDef_Damage_getCraterMult(int skirmishAIId, int weaponDefId) {
	DamageArray da = getWeaponDefById(skirmishAIId, weaponDefId)->damages;
	return da.craterMult;
}

EXPORT(float) skirmishAiCallback_WeaponDef_Damage_getCraterBoost(int skirmishAIId, int weaponDefId) {
	DamageArray da = getWeaponDefById(skirmishAIId, weaponDefId)->damages;
	return da.craterBoost;
}

EXPORT(int) skirmishAiCallback_WeaponDef_Damage_getTypes(int skirmishAIId, int weaponDefId, float* types, int types_sizeMax) {

	const WeaponDef* weaponDef = getWeaponDefById(skirmishAIId, weaponDefId);
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

EXPORT(float) skirmishAiCallback_WeaponDef_getAreaOfEffect(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->areaOfEffect;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isNoSelfDamage(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->noSelfDamage;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getFireStarter(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->fireStarter;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getEdgeEffectiveness(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->edgeEffectiveness;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getSize(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->size;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getSizeGrowth(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->sizeGrowth;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getCollisionSize(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->collisionSize;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getSalvoSize(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->salvosize;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getSalvoDelay(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->salvodelay;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getReload(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->reload;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getBeamTime(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->beamtime;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isBeamBurst(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->beamburst;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isWaterBounce(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->waterBounce;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isGroundBounce(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->groundBounce;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getBounceRebound(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->bounceRebound;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getBounceSlip(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->bounceSlip;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getNumBounce(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->numBounce;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getMaxAngle(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->maxAngle;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getRestTime(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->restTime;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getUpTime(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->uptime;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getFlightTime(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->flighttime;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getCost(int skirmishAIId, int weaponDefId, int resourceId) {

	const WeaponDef* wd = getWeaponDefById(skirmishAIId, weaponDefId);
	if (resourceId == resourceHandler->GetMetalId()) {
		return wd->metalcost;
	} else if (resourceId == resourceHandler->GetEnergyId()) {
		return wd->energycost;
	} else {
		return 0.0f;
	}
}

EXPORT(float) skirmishAiCallback_WeaponDef_getSupplyCost(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->supplycost;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getProjectilesPerShot(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->projectilespershot;
}

//EXPORT(int) skirmishAiCallback_WeaponDef_getTdfId(int skirmishAIId, int weaponDefId) {
//	return getWeaponDefById(skirmishAIId, weaponDefId)->tdfId;
//}

EXPORT(bool) skirmishAiCallback_WeaponDef_isTurret(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->turret;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isOnlyForward(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->onlyForward;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isFixedLauncher(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->fixedLauncher;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isWaterWeapon(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->waterweapon;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isFireSubmersed(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->fireSubmersed;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isSubMissile(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->submissile;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isTracks(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->tracks;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isDropped(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->dropped;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isParalyzer(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->paralyzer;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isImpactOnly(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->impactOnly;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isNoAutoTarget(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->noAutoTarget;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isManualFire(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->manualfire;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getInterceptor(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->interceptor;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getTargetable(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->targetable;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isStockpileable(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->stockpile;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getCoverageRange(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->coverageRange;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getStockpileTime(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->stockpileTime;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getIntensity(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->intensity;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getThickness(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->thickness;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getLaserFlareSize(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->laserflaresize;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getCoreThickness(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->corethickness;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getDuration(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->duration;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getLodDistance(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->lodDistance;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getFalloffRate(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->falloffRate;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getGraphicsType(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->graphicsType;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isSoundTrigger(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->soundTrigger;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isSelfExplode(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->selfExplode;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isGravityAffected(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->gravityAffected;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getHighTrajectory(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->highTrajectory;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getMyGravity(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->myGravity;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isNoExplode(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->noExplode;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getStartVelocity(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->startvelocity;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getWeaponAcceleration(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->weaponacceleration;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getTurnRate(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->turnrate;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getMaxVelocity(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->maxvelocity;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getProjectileSpeed(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->projectilespeed;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getExplosionSpeed(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->explosionSpeed;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getOnlyTargetCategory(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->onlyTargetCategory;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getWobble(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->wobble;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getDance(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->dance;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getTrajectoryHeight(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->trajectoryHeight;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isLargeBeamLaser(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->largeBeamLaser;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isShield(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->isShield;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isShieldRepulser(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->shieldRepulser;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isSmartShield(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->smartShield;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isExteriorShield(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->exteriorShield;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isVisibleShield(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->visibleShield;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isVisibleShieldRepulse(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->visibleShieldRepulse;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getVisibleShieldHitFrames(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->visibleShieldHitFrames;
}

EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getResourceUse(int skirmishAIId, int weaponDefId, int resourceId) {

	const WeaponDef* wd = getWeaponDefById(skirmishAIId, weaponDefId);
	if (resourceId == resourceHandler->GetEnergyId()) {
		return wd->shieldEnergyUse;
	} else {
		return 0.0f;
	}
}

EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getRadius(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->shieldRadius;
}

EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getForce(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->shieldForce;
}

EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getMaxSpeed(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->shieldMaxSpeed;
}

EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getPower(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->shieldPower;
}

EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getPowerRegen(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->shieldPowerRegen;
}

EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getPowerRegenResource(int skirmishAIId, int weaponDefId, int resourceId) {

	const WeaponDef* wd = getWeaponDefById(skirmishAIId, weaponDefId);
	if (resourceId == resourceHandler->GetEnergyId()) {
		return wd->shieldPowerRegenEnergy;
	} else {
		return 0.0f;
	}
}

EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getStartingPower(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->shieldStartingPower;
}

EXPORT(int) skirmishAiCallback_WeaponDef_Shield_getRechargeDelay(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->shieldRechargeDelay;
}

EXPORT(void) skirmishAiCallback_WeaponDef_Shield_getGoodColor(int skirmishAIId, int weaponDefId, short* return_colorS3_out) {

	const float3& color = getWeaponDefById(skirmishAIId, weaponDefId)->shieldGoodColor;
	return_colorS3_out[0] = color.x * 256;
	return_colorS3_out[1] = color.y * 256;
	return_colorS3_out[2] = color.z * 256;
}

EXPORT(void) skirmishAiCallback_WeaponDef_Shield_getBadColor(int skirmishAIId, int weaponDefId, short* return_colorS3_out) {

	const float3& color = getWeaponDefById(skirmishAIId, weaponDefId)->shieldBadColor;
	return_colorS3_out[0] = color.x * 256;
	return_colorS3_out[1] = color.y * 256;
	return_colorS3_out[2] = color.z * 256;
}

EXPORT(short) skirmishAiCallback_WeaponDef_Shield_getAlpha(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->shieldAlpha * 256;
}

EXPORT(int) skirmishAiCallback_WeaponDef_Shield_getInterceptType(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->shieldInterceptType;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getInterceptedByShieldType(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->interceptedByShieldType;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isAvoidFriendly(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->avoidFriendly;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isAvoidFeature(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->avoidFeature;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isAvoidNeutral(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->avoidNeutral;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getTargetBorder(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->targetBorder;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getCylinderTargetting(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->cylinderTargetting;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getMinIntensity(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->minIntensity;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getHeightBoostFactor(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->heightBoostFactor;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getProximityPriority(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->proximityPriority;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getCollisionFlags(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->collisionFlags;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isSweepFire(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->sweepFire;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isAbleToAttackGround(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->canAttackGround;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getCameraShake(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->cameraShake;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getDynDamageExp(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->dynDamageExp;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getDynDamageMin(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->dynDamageMin;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getDynDamageRange(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->dynDamageRange;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isDynDamageInverted(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->dynDamageInverted;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getCustomParams(int skirmishAIId, int weaponDefId,
		const char** keys, const char** values) {

	const std::map<std::string,std::string>& ps = getWeaponDefById(skirmishAIId, weaponDefId)->customParams;
	const size_t params_sizeReal = ps.size();

	if ((keys != NULL) && (values != NULL)) {
		fillCMap(&ps, keys, values);
	}

	return params_sizeReal;
}

//########### END WeaponDef


EXPORT(bool) skirmishAiCallback_Debug_GraphDrawer_isEnabled(int skirmishAIId) {
	return skirmishAIId_callback[skirmishAIId]->IsDebugDrawerEnabled();
}

EXPORT(int) skirmishAiCallback_getGroups(int skirmishAIId, int* groupIds, int groupIds_sizeMax) {

	const std::vector<CGroup*>& gs = grouphandlers[skirmishAIId_teamId[skirmishAIId]]->groups;
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

EXPORT(int) skirmishAiCallback_Group_getSupportedCommands(int skirmishAIId, int groupId) {
	return skirmishAIId_callback[skirmishAIId]->GetGroupCommands(groupId)->size();
}

EXPORT(int) skirmishAiCallback_Group_SupportedCommand_getId(int skirmishAIId, int groupId, int supportedCommandId) {
	return skirmishAIId_callback[skirmishAIId]->GetGroupCommands(groupId)->at(supportedCommandId).id;
}

EXPORT(const char*) skirmishAiCallback_Group_SupportedCommand_getName(int skirmishAIId, int groupId, int supportedCommandId) {
	return skirmishAIId_callback[skirmishAIId]->GetGroupCommands(groupId)->at(supportedCommandId).name.c_str();
}

EXPORT(const char*) skirmishAiCallback_Group_SupportedCommand_getToolTip(int skirmishAIId, int groupId, int supportedCommandId) {
	return skirmishAIId_callback[skirmishAIId]->GetGroupCommands(groupId)->at(supportedCommandId).tooltip.c_str();
}

EXPORT(bool) skirmishAiCallback_Group_SupportedCommand_isShowUnique(int skirmishAIId, int groupId, int supportedCommandId) {
	return skirmishAIId_callback[skirmishAIId]->GetGroupCommands(groupId)->at(supportedCommandId).showUnique;
}

EXPORT(bool) skirmishAiCallback_Group_SupportedCommand_isDisabled(int skirmishAIId, int groupId, int supportedCommandId) {
	return skirmishAIId_callback[skirmishAIId]->GetGroupCommands(groupId)->at(supportedCommandId).disabled;
}

EXPORT(int) skirmishAiCallback_Group_SupportedCommand_getParams(int skirmishAIId,
		int groupId, int supportedCommandId, const char** params, int params_sizeMax) {

	const std::vector<std::string> ps = skirmishAIId_callback[skirmishAIId]->GetGroupCommands(groupId)->at(supportedCommandId).params;
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

EXPORT(int) skirmishAiCallback_Group_OrderPreview_getId(int skirmishAIId, int groupId) {

	if (!isControlledByLocalPlayer(skirmishAIId)) return -1;

	//TODO: need to add support for new gui
	Command tmpCmd = guihandler->GetOrderPreview();
	return tmpCmd.id;
}

EXPORT(short) skirmishAiCallback_Group_OrderPreview_getOptions(int skirmishAIId, int groupId) {

	if (!isControlledByLocalPlayer(skirmishAIId)) return '\0';

	//TODO: need to add support for new gui
	Command tmpCmd = guihandler->GetOrderPreview();
	return tmpCmd.options;
}

EXPORT(int) skirmishAiCallback_Group_OrderPreview_getTag(int skirmishAIId, int groupId) {

	if (!isControlledByLocalPlayer(skirmishAIId)) return 0;

	//TODO: need to add support for new gui
	Command tmpCmd = guihandler->GetOrderPreview();
	return tmpCmd.tag;
}

EXPORT(int) skirmishAiCallback_Group_OrderPreview_getTimeOut(int skirmishAIId, int groupId) {

	if (!isControlledByLocalPlayer(skirmishAIId)) return -1;

	//TODO: need to add support for new gui
	Command tmpCmd = guihandler->GetOrderPreview();
	return tmpCmd.timeOut;
}

EXPORT(int) skirmishAiCallback_Group_OrderPreview_getParams(int skirmishAIId,
		int groupId, float* params, int params_sizeMax) {

	if (!isControlledByLocalPlayer(skirmishAIId)) { return 0; }

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

EXPORT(bool) skirmishAiCallback_Group_isSelected(int skirmishAIId, int groupId) {

	if (!isControlledByLocalPlayer(skirmishAIId)) return false;

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
	callback->SkirmishAI_getTeamId = &skirmishAiCallback_SkirmishAI_getTeamId;
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
	callback->Game_getTeams = &skirmishAiCallback_Game_getTeams;
	callback->Game_getTeamSide = &skirmishAiCallback_Game_getTeamSide;
	callback->Game_getTeamColor = &skirmishAiCallback_Game_getTeamColor;
	callback->Game_getTeamIncomeMultiplier = &skirmishAiCallback_Game_getTeamIncomeMultiplier;
	callback->Game_getTeamAllyTeam = &skirmishAiCallback_Game_getTeamAllyTeam;
	callback->Game_getTeamResourceCurrent = &skirmishAiCallback_Game_getTeamResourceCurrent;
	callback->Game_getTeamResourceIncome = &skirmishAiCallback_Game_getTeamResourceIncome;
	callback->Game_getTeamResourceUsage = &skirmishAiCallback_Game_getTeamResourceUsage;
	callback->Game_getTeamResourceStorage = &skirmishAiCallback_Game_getTeamResourceStorage;
	callback->Game_isAllied = &skirmishAiCallback_Game_isAllied;
	callback->Game_isExceptionHandlingEnabled = &skirmishAiCallback_Game_isExceptionHandlingEnabled;
	callback->Game_isDebugModeEnabled = &skirmishAiCallback_Game_isDebugModeEnabled;
	callback->Game_isPaused = &skirmishAiCallback_Game_isPaused;
	callback->Game_getSpeedFactor = &skirmishAiCallback_Game_getSpeedFactor;
	callback->Game_getSetupScript = &skirmishAiCallback_Game_getSetupScript;
	callback->Game_getCategoryFlag = &skirmishAiCallback_Game_getCategoryFlag;
	callback->Game_getCategoriesFlag = &skirmishAiCallback_Game_getCategoriesFlag;
	callback->Game_getCategoryName = &skirmishAiCallback_Game_getCategoryName;
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
	callback->UnitDef_getMaxWeaponRange = &skirmishAiCallback_UnitDef_getMaxWeaponRange;
	callback->UnitDef_getType = &skirmishAiCallback_UnitDef_getType;
	callback->UnitDef_getTooltip = &skirmishAiCallback_UnitDef_getTooltip;
	callback->UnitDef_getWreckName = &skirmishAiCallback_UnitDef_getWreckName;
	callback->UnitDef_getDeathExplosion = &skirmishAiCallback_UnitDef_getDeathExplosion;
	callback->UnitDef_getSelfDExplosion = &skirmishAiCallback_UnitDef_getSelfDExplosion;
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
	callback->UnitDef_MoveData_getXSize = &skirmishAiCallback_UnitDef_MoveData_getXSize;
	callback->UnitDef_MoveData_getZSize = &skirmishAiCallback_UnitDef_MoveData_getZSize;
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
	callback->Unit_getMax = &skirmishAiCallback_Unit_getMax;
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
	callback->Unit_getVel = &skirmishAiCallback_Unit_getVel;
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
	callback->Mod_getHash = &skirmishAiCallback_Mod_getHash;
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
	callback->Map_getHash = &skirmishAiCallback_Map_getHash;
	callback->Map_getName = &skirmishAiCallback_Map_getName;
	callback->Map_getHumanName = &skirmishAiCallback_Map_getHumanName;
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
	callback->Debug_GraphDrawer_isEnabled = &skirmishAiCallback_Debug_GraphDrawer_isEnabled;
}

SSkirmishAICallback* skirmishAiCallback_getInstanceFor(int skirmishAIId, int teamId, CAICallback* aiCallback, CAICheats* aiCheats) {

	SSkirmishAICallback* callback = new SSkirmishAICallback();
	skirmishAiCallback_init(callback);

	skirmishAIId_callback[skirmishAIId]      = aiCallback;
	skirmishAIId_cheatCallback[skirmishAIId] = aiCheats;
	skirmishAIId_cCallback[skirmishAIId]     = callback;
	skirmishAIId_usesCheats[skirmishAIId]    = false;
	skirmishAIId_teamId[skirmishAIId]        = teamId;

	return callback;
}

void skirmishAiCallback_release(int skirmishAIId) {

	skirmishAIId_cheatCallback.erase(skirmishAIId);
	skirmishAIId_callback.erase(skirmishAIId);

	SSkirmishAICallback* callback = skirmishAIId_cCallback[skirmishAIId];
	skirmishAIId_cCallback.erase(skirmishAIId);
	delete callback;

	skirmishAIId_teamId.erase(skirmishAIId);
}

