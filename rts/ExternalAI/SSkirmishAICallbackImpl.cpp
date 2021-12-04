/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ExternalAI/AICallback.h"
#include "ExternalAI/AICheats.h"
#include "ExternalAI/AILibraryManager.h"
#include "ExternalAI/SSkirmishAICallbackImpl.h"
#include "ExternalAI/SkirmishAILibraryInfo.h"
#include "ExternalAI/SkirmishAIWrapper.h"
#include "ExternalAI/SAIInterfaceCallbackImpl.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "ExternalAI/Interface/AISCommands.h"
#include "ExternalAI/Interface/SSkirmishAICallback.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"

#include "Game/GlobalUnsynced.h" // for myTeam
#include "Game/GameVersion.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/UI/GuiHandler.h"
#include "Map/ReadMap.h"
#include "Map/MetalMap.h"
#include "Map/MapInfo.h"
#include "Lua/LuaRulesParams.h"
#include "Lua/LuaHandleSynced.h"
#include "Sim/Features/Feature.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Game/UI/Groups/Group.h"
#include "Game/UI/Groups/GroupHandler.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/Resource.h"
#include "Sim/Misc/ResourceHandler.h"
#include "Sim/Misc/ResourceMapAnalyzer.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/QuadField.h" // for quadField.GetFeaturesExact(pos, radius)
#include "System/SafeCStrings.h"
#include "System/SpringMath.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/Log/ILog.h"


static std::array<std::pair<CAICallback, CAICheats>, MAX_AIS> AI_LEGACY_CALLBACKS;
static std::array<SSkirmishAICallback, MAX_AIS> AI_CALLBACK_WRAPPERS;

static std::array<std::pair<bool, bool>, MAX_AIS> AI_CHEAT_FLAGS = {{{false, false}}};
static std::array<int, MAX_AIS> AI_TEAM_IDS = {{-1}};


static std::vector<PointMarker> AI_TMP_POINT_MARKERS[MAX_AIS];
static std::vector<LineMarker> AI_TMP_LINE_MARKERS[MAX_AIS];

static constexpr size_t MAX_NUM_MARKERS = 16384;


static inline CAICallback* GetCallBack(int skirmishAIId) { return &AI_LEGACY_CALLBACKS[skirmishAIId].first; }
static inline CAICheats* GetCheatCallBack(int skirmishAIId) { return &AI_LEGACY_CALLBACKS[skirmishAIId].second; }


static void CheckSkirmishAIId(int skirmishAIId, const char* caller) {
	if (skirmishAIId >= 0 && skirmishAIId < MAX_AIS)
		return;

	const char* fmt = "[%s][caller=%s] illegal skirmish-AI ID %i";
	char msg[512];

	SNPRINTF(msg, sizeof(msg), fmt, skirmishAIId);

	// log exception to the engine and die
	skirmishAiCallback_Log_exception(skirmishAIId, msg, 1, true);
	assert(false);
}

static size_t ShallowCopyKeyValPairs(
	const spring::unordered_map<std::string, std::string>& map,
	const char* cMapKeys[],
	const char* cMapValues[]
) {
	auto it = map.cbegin();

	for (int i = 0; it != map.cend(); ++i, it++) {
		cMapKeys[i] = it->first.c_str();
		cMapValues[i] = it->second.c_str();
	}

	return (map.size());
}



static void toFloatArr(const short color[3], const short alpha, float arrColor[4]) {
	arrColor[0] = color[0] / 255.0f;
	arrColor[1] = color[1] / 255.0f;
	arrColor[2] = color[2] / 255.0f;
	arrColor[3] =    alpha / 255.0f;
}

static bool isControlledByLocalPlayer(int skirmishAIId) {
	return (gu->myTeam == AI_TEAM_IDS[skirmishAIId]);
}


static const CUnit* getUnit(int unitId) {
	return (unitHandler.GetUnit(unitId));
}

static bool isUnitInLos(int skirmishAIId, const CUnit* unit) {
	const int teamId = AI_TEAM_IDS[skirmishAIId];
	if (teamHandler.AlliedTeams(unit->team, teamId))
		return true;

	const int allyID = teamHandler.AllyTeam(teamId);
	return (unit->losStatus[allyID] & LOS_INLOS);
}

static int unitModParamLosMask(int skirmishAIId, const CUnit* unit) {

	const int teamId = AI_TEAM_IDS[skirmishAIId];
	const int allyID = teamHandler.AllyTeam(teamId);
	const int losStatus = unit->losStatus[allyID];

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

	if (unit->allyteam == allyID || skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return (losMask | LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK);

	// ingame alliances
	if (teamHandler.AlliedTeams(unit->team, teamId))
		return (losMask | LuaRulesParams::RULESPARAMLOS_ALLIED_MASK);

	if (losStatus & LOS_INLOS)
		return (losMask | LuaRulesParams::RULESPARAMLOS_INLOS_MASK);

	if (losStatus & (LOS_PREVLOS | LOS_CONTRADAR))
		return (losMask | LuaRulesParams::RULESPARAMLOS_TYPED_MASK);

	if (losStatus & LOS_INRADAR)
		return (losMask | LuaRulesParams::RULESPARAMLOS_INRADAR_MASK);

	return losMask;
}

static int featureModParamLosMask(int skirmishAIId, const CFeature* feature) {

	const int teamId = AI_TEAM_IDS[skirmishAIId];
	const int allyID = teamHandler.AllyTeam(teamId);

	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

	if (feature->allyteam == allyID || skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return (losMask | LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK);

	// ingame alliances
	if (teamHandler.AlliedTeams(feature->team, teamId))
		return (losMask | LuaRulesParams::RULESPARAMLOS_ALLIED_MASK);

	if (feature->IsInLosForAllyTeam(allyID))
		return (losMask | LuaRulesParams::RULESPARAMLOS_INLOS_MASK);

	return losMask;
}

static inline bool modParamIsVisible(const LuaRulesParams::Param& param, const int losMask) {
	return (param.los & losMask) > 0;
}

static const CTeam* getTeam(int teamId) {
	return (teamId < teamHandler.ActiveTeams()) ? teamHandler.Team(teamId) : nullptr;
}

static bool isAlliedTeam(int skirmishAIId, const CTeam* team) {
	return teamHandler.AlliedTeams(team->teamNum, AI_TEAM_IDS[skirmishAIId]);
}

static int teamModParamLosMask(int skirmishAIId, const CTeam* team) {
	const int teamId = AI_TEAM_IDS[skirmishAIId];
	int losMask = LuaRulesParams::RULESPARAMLOS_PUBLIC_MASK;

	if (isAlliedTeam(skirmishAIId, team) || skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return (losMask | LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK);

	// ingame alliances
	if (teamHandler.AlliedTeams(team->teamNum, teamId))
		return (losMask | LuaRulesParams::RULESPARAMLOS_ALLIED_MASK);

	return losMask;
}

static inline int gameModParamLosMask() {
	return LuaRulesParams::RULESPARAMLOS_PRIVATE_MASK;
}


static float getRulesParamFloatValueByName(
	const LuaRulesParams::Params& params,
	const int losMask,
	const char* rulesParamName,
	float defaultValue
) {
	float value = defaultValue;
	const std::string key(rulesParamName);
	const auto it = params.find(key);

	if (it == params.end())
		return value;

	if (modParamIsVisible(it->second, losMask))
		value = it->second.valueInt;

	return value;
}

static const char* getRulesParamStringValueByName(
	const LuaRulesParams::Params& params,
	const int losMask,
	const char* rulesParamName,
	const char* defaultValue
) {
	const char* value = defaultValue;
	const std::string key(rulesParamName);
	const auto it = params.find(key);

	if (it == params.end())
		return value;

	if (modParamIsVisible(it->second, losMask))
		value = it->second.valueString.c_str();

	return value;
}



static inline const UnitDef* getUnitDefById(int skirmishAIId, int unitDefId) {
	return unitDefHandler->GetUnitDefByID(unitDefId);
}

static inline const MoveDef* getUnitDefMoveDefById(int skirmishAIId, int unitDefId) {
	const unsigned int mdType = getUnitDefById(skirmishAIId, unitDefId)->pathType;
	const MoveDef* moveDef = (mdType != -1U)? moveDefHandler.GetMoveDefByPathType(mdType): nullptr;

	// NOTE There is a callback method to check whether MoveData is available, use it.
	return moveDef;
}

static inline const WeaponDef* getWeaponDefById(int skirmishAIId, int weaponDefId) {
	return (weaponDefHandler->GetWeaponDefByID(weaponDefId));
}

static inline const FeatureDef* getFeatureDefById(int skirmishAIId, int featureDefId) {
	return (featureDefHandler->GetFeatureDefByID(featureDefId));
}


//FIXME: get rid of this function (=call functions directly)
static int wrapper_HandleCommand(CAICallback* clb, CAICheats* clbCheat, int cmdId, void* cmdData) {
	if (clbCheat != nullptr) {
		const int ret = clbCheat->HandleCommand(cmdId, cmdData);
		if (ret != 0) {
			// cheat interface handled the command
			return ret;
		}
	}
	return clb->HandleCommand(cmdId, cmdData);
}



template<typename STypeCommand> static void SetCommonCmdParams(STypeCommand* dst, const RawCommand* src, int unitId, int groupId) {
	dst->unitId = unitId;
	dst->groupId = groupId;
	dst->options = src->options;
	dst->timeOut = src->timeOut;
}

EXPORT(int) skirmishAiCallback_Engine_executeCommand(
	int skirmishAIId,
	int unitId,
	int groupId,
	void* commandData
) {
	// NOTE:
	//   executeCommand expects a RawCommand
	//   handleCommand expects an S*Command
	RawCommand* rc = static_cast<RawCommand*>(commandData);
	Command c;

	c.FromRawCommand(*rc);

	const int maxUnits = skirmishAiCallback_Unit_getMax(skirmishAIId);
	const int aiCmdId = extractAICommandTopic(&c, maxUnits);

	int ret = -1;

	switch (aiCmdId) {
		case COMMAND_UNIT_STOP: {
			SStopUnitCommand cmd;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_WAIT: {
			SWaitUnitCommand cmd;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_WAIT_TIME: {
			STimeWaitUnitCommand cmd;
			cmd.time = rc->params[0];

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_WAIT_DEATH: {
			SDeathWaitUnitCommand cmd;
			cmd.toDieUnitId = (int) rc->params[0];

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_WAIT_SQUAD: {
			SSquadWaitUnitCommand cmd;
			cmd.numUnits = (int) rc->params[0];

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_WAIT_GATHER: {
			SGatherWaitUnitCommand cmd;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_MOVE: {
			SMoveUnitCommand cmd;
			cmd.toPos_posF3 = rc->params;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_PATROL: {
			SPatrolUnitCommand cmd;
			cmd.toPos_posF3 = rc->params;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_FIGHT: {
			SFightUnitCommand cmd;
			cmd.toPos_posF3 = rc->params;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_ATTACK: {
			SAttackUnitCommand cmd;
			cmd.toAttackUnitId = (int) rc->params[0];

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_ATTACK_AREA: {
			SAttackAreaUnitCommand cmd;
			cmd.toAttackPos_posF3 = rc->params;
			cmd.radius = (rc->numParams >= 4)? rc->params[3]: 0.0f;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_GUARD: {
			SGuardUnitCommand cmd;
			cmd.toGuardUnitId = (int) rc->params[0];

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_AI_SELECT: {
			SAiSelectUnitCommand cmd;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_GROUP_ADD: {
			SGroupAddUnitCommand cmd;
			cmd.toGroupId = (int) rc->params[0];

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_GROUP_CLEAR: {
			SGroupClearUnitCommand cmd;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_REPAIR: {
			SRepairUnitCommand cmd;
			cmd.toRepairUnitId = (int) rc->params[0];

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_SET_FIRE_STATE: {
			SSetFireStateUnitCommand cmd;
			cmd.fireState = (int) rc->params[0];

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_SET_MOVE_STATE: {
			SSetMoveStateUnitCommand cmd;
			cmd.moveState = (int) rc->params[0];

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_SET_BASE: {
			SSetBaseUnitCommand cmd;
			cmd.basePos_posF3 = rc->params;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_SELF_DESTROY: {
			SSelfDestroyUnitCommand cmd;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_LOAD_UNITS: {
			if (rc->numParams > 0) {
				std::vector<int> tmpUnitIds(rc->numParams);

				SLoadUnitsUnitCommand cmd;
				cmd.toLoadUnitIds_size = rc->numParams;
				cmd.toLoadUnitIds = &tmpUnitIds[0];

				for (int u = 0; u < rc->numParams; ++u) {
					cmd.toLoadUnitIds[u] = static_cast<int>(rc->params[u]);
				}

				SetCommonCmdParams(&cmd, rc, unitId, groupId);
				ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
			}
		} break;
		case COMMAND_UNIT_LOAD_UNITS_AREA: {
			SLoadUnitsAreaUnitCommand cmd;
			cmd.pos_posF3 = rc->params;
			cmd.radius = (rc->numParams >= 4)? rc->params[3]: 0.0f;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_LOAD_ONTO: {
			SLoadOntoUnitCommand cmd;
			cmd.transporterUnitId = (int) rc->params[0];

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_UNLOAD_UNIT: {
			SUnloadUnitCommand cmd;
			cmd.toPos_posF3 = rc->params;
			cmd.toUnloadUnitId = (int) rc->params[3];

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_UNLOAD_UNITS_AREA: {
			SUnloadUnitsAreaUnitCommand cmd;
			cmd.toPos_posF3 = rc->params;
			cmd.radius = (rc->numParams >= 4)? rc->params[3]: 0.0f;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_SET_ON_OFF: {
			SSetOnOffUnitCommand cmd;
			cmd.on      = (bool) rc->params[0];

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_RECLAIM_UNIT: {
			SReclaimUnitUnitCommand cmd;
			cmd.toReclaimUnitId = (int) rc->params[0];

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_RECLAIM_FEATURE: {
			SReclaimFeatureUnitCommand cmd;
			cmd.toReclaimFeatureId = ((int) rc->params[0]) - maxUnits;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_RECLAIM_AREA: {
			SReclaimAreaUnitCommand cmd;
			cmd.pos_posF3 = rc->params;
			cmd.radius    = (rc->numParams >= 4)? rc->params[3]: 0.0f;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_CLOAK: {
			SCloakUnitCommand cmd;
			cmd.cloak = (bool) rc->params[0];

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_STOCKPILE: {
			SStockpileUnitCommand cmd;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_D_GUN: {
			SDGunUnitCommand cmd;
			cmd.toAttackUnitId = static_cast<int>(rc->params[0]);

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_D_GUN_POS: {
			SDGunPosUnitCommand cmd;
			cmd.pos_posF3 = rc->params;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_RESTORE_AREA: {
			SRestoreAreaUnitCommand cmd;
			cmd.pos_posF3 = rc->params;
			cmd.radius = (rc->numParams >= 4)? rc->params[3]: 0.0f;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_SET_REPEAT: {
			SSetRepeatUnitCommand cmd;
			cmd.repeat = (bool) rc->params[0];

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_SET_TRAJECTORY: {
			SSetTrajectoryUnitCommand cmd;
			cmd.trajectory = static_cast<int>(rc->params[0]);

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_RESURRECT: {
			SResurrectUnitCommand cmd;
			cmd.toResurrectFeatureId = static_cast<int>(rc->params[0]);

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_RESURRECT_AREA: {
			SResurrectAreaUnitCommand cmd;
			cmd.pos_posF3 = rc->params;
			cmd.radius = (rc->numParams >= 4)? rc->params[3]: 0.0f;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_CAPTURE: {
			SCaptureUnitCommand cmd;
			cmd.toCaptureUnitId = static_cast<int>(rc->params[0]);

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_CAPTURE_AREA: {
			SCaptureAreaUnitCommand cmd;
			cmd.pos_posF3 = rc->params;
			cmd.radius = (rc->numParams >= 4)? rc->params[3]: 0.0f;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_SET_AUTO_REPAIR_LEVEL: {
			SSetAutoRepairLevelUnitCommand cmd;
			cmd.autoRepairLevel = static_cast<int>(rc->params[0]);

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_SET_IDLE_MODE: {
			SSetIdleModeUnitCommand cmd;
			cmd.idleMode = static_cast<int>(rc->params[0]);

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
		case COMMAND_UNIT_BUILD: {
			SBuildUnitCommand cmd;

			cmd.toBuildUnitDefId = -rc->id[0];

			if (rc->numParams >= 3) {
				cmd.buildPos_posF3 = rc->params;
			} else {
				cmd.buildPos_posF3 = nullptr;
			}

			cmd.facing = (rc->numParams >= 4)? rc->params[3]: UNIT_COMMAND_BUILD_NO_FACING;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;

		default:
		case COMMAND_UNIT_CUSTOM: {
			SCustomUnitCommand cmd;
			cmd.cmdId       = rc->id[0];
			cmd.params_size = rc->numParams;
			cmd.params      = rc->params;

			SetCommonCmdParams(&cmd, rc, unitId, groupId);
			ret = skirmishAiCallback_Engine_handleCommand(skirmishAIId, COMMAND_TO_ID_ENGINE, -1, aiCmdId, &cmd);
		} break;
	}

	return ret;
}

EXPORT(int) skirmishAiCallback_Engine_handleCommand(
	int skirmishAIId,
	int /*toId*/,
	int commandId,
	int commandTopic,
	void* commandData
) {
	int ret = 0;

	CAICallback* clb = GetCallBack(skirmishAIId);
	// if this is not NULL, cheating is enabled
	CAICheats* clbCheat = nullptr;

	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		clbCheat = GetCheatCallBack(skirmishAIId);

	switch (commandTopic) {
		case COMMAND_CHEATS_SET_MY_INCOME_MULTIPLIER: {
			const SSetMyIncomeMultiplierCheatCommand* cmd = static_cast<SSetMyIncomeMultiplierCheatCommand*>(commandData);

			if (clbCheat != nullptr) {
				clbCheat->SetMyIncomeMultiplier(cmd->factor);
				ret = 0;
			} else {
				ret = -1;
			}
		} break;

		case COMMAND_CHEATS_GIVE_ME_RESOURCE: {
			const SGiveMeResourceCheatCommand* cmd = static_cast<SGiveMeResourceCheatCommand*>(commandData);

			if (clbCheat != nullptr) {
				ret = -2;

				if (cmd->resourceId == resourceHandler->GetMetalId ()) { clbCheat->GiveMeMetal (cmd->amount); ret = 0; } break;
				if (cmd->resourceId == resourceHandler->GetEnergyId()) { clbCheat->GiveMeEnergy(cmd->amount); ret = 0; } break;
			} else {
				ret = -1;
			}
		} break;

		case COMMAND_CHEATS_GIVE_ME_NEW_UNIT: {
			SGiveMeNewUnitCheatCommand* cmd = static_cast<SGiveMeNewUnitCheatCommand*>(commandData);

			if (clbCheat != nullptr) {
				cmd->ret_newUnitId = clbCheat->CreateUnit(getUnitDefById(skirmishAIId, cmd->unitDefId)->name.c_str(), cmd->pos_posF3);

				if (cmd->ret_newUnitId > 0) {
					ret = 0;
				} else {
					ret = -2;
				}
			} else {
				ret = -1;
				cmd->ret_newUnitId = -1;
			}
		} break;


		case COMMAND_SEND_START_POS: {
			const SSendStartPosCommand* cmd = static_cast<SSendStartPosCommand*>(commandData);
			clb->SendStartPos(cmd->ready, cmd->pos_posF3);
		} break;

		case COMMAND_DRAWER_POINT_ADD: {
			const SAddPointDrawCommand* cmd = static_cast<SAddPointDrawCommand*>(commandData);
			AIHCAddMapPoint data = {cmd->pos_posF3, cmd->label};
			wrapper_HandleCommand(clb, clbCheat, AIHCAddMapPointId, &data);
		} break;

		case COMMAND_DRAWER_POINT_REMOVE: {
			const SRemovePointDrawCommand* cmd = static_cast<SRemovePointDrawCommand*>(commandData);
			AIHCRemoveMapPoint data = {cmd->pos_posF3};
			wrapper_HandleCommand(clb, clbCheat, AIHCRemoveMapPointId, &data);
		} break;

		case COMMAND_DRAWER_LINE_ADD: {
			const SAddLineDrawCommand* cmd = static_cast<SAddLineDrawCommand*>(commandData);
			AIHCAddMapLine data = {cmd->posFrom_posF3, cmd->posTo_posF3};
			wrapper_HandleCommand(clb, clbCheat, AIHCAddMapLineId, &data);
		} break;


		case COMMAND_SEND_TEXT_MESSAGE: {
			const SSendTextMessageCommand* cmd = static_cast<SSendTextMessageCommand*>(commandData);
			clb->SendTextMsg(cmd->text, cmd->zone);
		} break;

		case COMMAND_SET_LAST_POS_MESSAGE: {
			const SSetLastPosMessageCommand* cmd = static_cast<SSetLastPosMessageCommand*>(commandData);
			clb->SetLastMsgPos(cmd->pos_posF3);
		} break;

		case COMMAND_SEND_RESOURCES: {
			SSendResourcesCommand* cmd = static_cast<SSendResourcesCommand*>(commandData);
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
		} break;

		case COMMAND_SEND_UNITS: {
			SSendUnitsCommand* cmd = static_cast<SSendUnitsCommand*>(commandData);

			std::vector<int> tmpUnitIds(cmd->unitIds_size);

			for (unsigned int i = 0; i < tmpUnitIds.size(); ++i) {
				tmpUnitIds[i] = cmd->unitIds[i];
			}

			cmd->ret_sentUnits = clb->SendUnits(tmpUnitIds, cmd->receivingTeamId);
		} break;

		case COMMAND_GROUP_CREATE: {
			SCreateGroupCommand* cmd = static_cast<SCreateGroupCommand*>(commandData);
			cmd->ret_groupId = clb->CreateGroup();
		} break;
		case COMMAND_GROUP_ERASE: {
			const SEraseGroupCommand* cmd = static_cast<SEraseGroupCommand*>(commandData);
			clb->EraseGroup(cmd->groupId);
		} break;
/*
		case COMMAND_GROUP_ADD_UNIT: {
			SAddUnitToGroupCommand* cmd = (SAddUnitToGroupCommand*) commandData;
			cmd->ret_isExecuted = clb->AddUnitToGroup(cmd->unitId, cmd->groupId);
		} break;
		case COMMAND_GROUP_REMOVE_UNIT: {
			SRemoveUnitFromGroupCommand* cmd =
					(SRemoveUnitFromGroupCommand*) commandData;
			cmd->ret_isExecuted = clb->RemoveUnitFromGroup(cmd->unitId);
		} break;
*/
		case COMMAND_PATH_INIT: {
			SInitPathCommand* cmd = static_cast<SInitPathCommand*>(commandData);
			cmd->ret_pathId = clb->InitPath(cmd->start_posF3, cmd->end_posF3, cmd->pathType, cmd->goalRadius);
		} break;

		case COMMAND_PATH_GET_APPROXIMATE_LENGTH: {
			SGetApproximateLengthPathCommand* cmd = static_cast<SGetApproximateLengthPathCommand*>(commandData);
			cmd->ret_approximatePathLength = clb->GetPathLength(cmd->start_posF3, cmd->end_posF3, cmd->pathType, cmd->goalRadius);
		} break;

		case COMMAND_PATH_GET_NEXT_WAYPOINT: {
			SGetNextWaypointPathCommand* cmd = static_cast<SGetNextWaypointPathCommand*>(commandData);
			clb->GetNextWaypoint(cmd->pathId).copyInto(cmd->ret_nextWaypoint_posF3_out);
		} break;

		case COMMAND_PATH_FREE: {
			clb->FreePath(static_cast<SFreePathCommand*>(commandData)->pathId);
		} break;

		// @see AI/Wrappers/Cpp/bin/wrappCallback.awk, printMember
		#define SSAICALLBACK_CALL_LUA(HandleName, HANDLENAME)  \
			case COMMAND_CALL_LUA_ ## HANDLENAME: {  \
				SCallLua ## HandleName ## Command* cmd = static_cast<SCallLua ## HandleName ## Command*>(commandData);  \
				size_t len = 0;                                                                                         \
				const char* outData = clb->CallLua ## HandleName(cmd->inData, cmd->inSize, &len);                       \
				if (cmd->ret_outData != NULL) {                                                                         \
					if (outData != NULL) {                                                                              \
						len = std::min(len, size_t(MAX_RESPONSE_SIZE - 1));                                             \
						strncpy(cmd->ret_outData, outData, len);                                                        \
						cmd->ret_outData[len] = '\0';                                                                   \
					} else {                                                                                            \
						cmd->ret_outData[0] = '\0';                                                                     \
					}  \
				}  \
			} break;

		SSAICALLBACK_CALL_LUA(Rules, RULES)
		SSAICALLBACK_CALL_LUA(UI, UI)

		#undef SSAICALLBACK_CALL_LUA


		case COMMAND_DRAWER_ADD_NOTIFICATION: {
			const SAddNotificationDrawerCommand* cmd = static_cast<SAddNotificationDrawerCommand*>(commandData);
			clb->AddNotification(cmd->pos_posF3,
					float3(
						cmd->color_colorS3[0] / 256.0f,
						cmd->color_colorS3[1] / 256.0f,
						cmd->color_colorS3[2] / 256.0f),
					cmd->alpha / 256.0f);
		} break;

		case COMMAND_DRAWER_PATH_START: {
			const SStartPathDrawerCommand* cmd = static_cast<SStartPathDrawerCommand*>(commandData);
			float arrColor[4];
			toFloatArr(cmd->color_colorS3, cmd->alpha, arrColor);
			clb->LineDrawerStartPath(cmd->pos_posF3, arrColor);
		} break;

		case COMMAND_DRAWER_PATH_FINISH: {
			// noop
		} break;

		case COMMAND_DRAWER_PATH_DRAW_LINE: {
			const SDrawLinePathDrawerCommand* cmd = static_cast<SDrawLinePathDrawerCommand*>(commandData);
			float arrColor[4];
			toFloatArr(cmd->color_colorS3, cmd->alpha, arrColor);
			clb->LineDrawerDrawLine(cmd->endPos_posF3, arrColor);
		} break;

		case COMMAND_DRAWER_PATH_DRAW_LINE_AND_ICON: {
			const SDrawLineAndIconPathDrawerCommand* cmd = static_cast<SDrawLineAndIconPathDrawerCommand*>(commandData);
			float arrColor[4];
			toFloatArr(cmd->color_colorS3, cmd->alpha, arrColor);
			clb->LineDrawerDrawLineAndIcon(cmd->cmdId, cmd->endPos_posF3, arrColor);
		} break;

		case COMMAND_DRAWER_PATH_DRAW_ICON_AT_LAST_POS: {
			const SDrawIconAtLastPosPathDrawerCommand* cmd = static_cast<SDrawIconAtLastPosPathDrawerCommand*>(commandData);
			clb->LineDrawerDrawIconAtLastPos(cmd->cmdId);
		} break;

		case COMMAND_DRAWER_PATH_BREAK: {
			const SBreakPathDrawerCommand* cmd = static_cast<SBreakPathDrawerCommand*>(commandData);
			float arrColor[4];
			toFloatArr(cmd->color_colorS3, cmd->alpha, arrColor);
			clb->LineDrawerBreak(cmd->endPos_posF3, arrColor);
		} break;

		case COMMAND_DRAWER_PATH_RESTART: {
			const SRestartPathDrawerCommand* cmd = static_cast<SRestartPathDrawerCommand*>(commandData);
			if (cmd->sameColor) {
				clb->LineDrawerRestartSameColor();
			} else {
				clb->LineDrawerRestart();
			}
		} break;

		case COMMAND_DRAWER_FIGURE_CREATE_SPLINE: {
			SCreateSplineFigureDrawerCommand* cmd = static_cast<SCreateSplineFigureDrawerCommand*>(commandData);
			cmd->ret_newFigureGroupId = clb->CreateSplineFigure(
				cmd->pos1_posF3, cmd->pos2_posF3,
				cmd->pos3_posF3, cmd->pos4_posF3,
				cmd->width,
				cmd->arrow,
				cmd->lifeTime,
				cmd->figureGroupId
			);
		} break;

		case COMMAND_DRAWER_FIGURE_CREATE_LINE: {
			SCreateLineFigureDrawerCommand* cmd = static_cast<SCreateLineFigureDrawerCommand*>(commandData);
			cmd->ret_newFigureGroupId = clb->CreateLineFigure(
				cmd->pos1_posF3, cmd->pos2_posF3,
				cmd->width,
				cmd->arrow,
				cmd->lifeTime,
				cmd->figureGroupId
			);
		} break;

		case COMMAND_DRAWER_FIGURE_SET_COLOR: {
			const SSetColorFigureDrawerCommand* cmd = static_cast<SSetColorFigureDrawerCommand*>(commandData);
			clb->SetFigureColor(cmd->figureGroupId,
					cmd->color_colorS3[0] / 256.0f,
					cmd->color_colorS3[1] / 256.0f,
					cmd->color_colorS3[2] / 256.0f,
					cmd->alpha / 256.0f);
		} break;

		case COMMAND_DRAWER_FIGURE_DELETE: {
			const SDeleteFigureDrawerCommand* cmd = static_cast<SDeleteFigureDrawerCommand*>(commandData);
			clb->DeleteFigureGroup(cmd->figureGroupId);
		} break;

		case COMMAND_DRAWER_DRAW_UNIT: {
			const SDrawUnitDrawerCommand* cmd = static_cast<SDrawUnitDrawerCommand*>(commandData);
			clb->DrawUnit(getUnitDefById(skirmishAIId,
					cmd->toDrawUnitDefId)->name.c_str(), cmd->pos_posF3,
					cmd->rotation, cmd->lifeTime, cmd->teamId, cmd->transparent,
					cmd->drawBorder, cmd->facing);
		} break;

		case COMMAND_TRACE_RAY: {
			STraceRayCommand* cCmdData = static_cast<STraceRayCommand*>(commandData);
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
			SFeatureTraceRayCommand* cCmdData = static_cast<SFeatureTraceRayCommand*>(commandData);
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
			const SPauseCommand* cmd = static_cast<SPauseCommand*>(commandData);
			AIHCPause cppCmdData = {
				cmd->enable,
				cmd->reason
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCPauseId, &cppCmdData);
		} break;

		case COMMAND_DEBUG_DRAWER_GRAPH_LINE_ADD_POINT: {
			SAddPointLineGraphDrawerDebugCommand* cCmdData = static_cast<SAddPointLineGraphDrawerDebugCommand*>(commandData);
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_ADD_GRAPH_POINT,
				cCmdData->x, cCmdData->y,
				0.0f, 0.0f,
				cCmdData->lineId, 0,
				ZeroVector,
				"",
				0, nullptr
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;
		case COMMAND_DEBUG_DRAWER_GRAPH_LINE_DELETE_POINTS: {
			SDeletePointsLineGraphDrawerDebugCommand* cCmdData = static_cast<SDeletePointsLineGraphDrawerDebugCommand*>(commandData);
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_DEL_GRAPH_POINTS,
				0.0f, 0.0f,
				0.0f, 0.0f,
				cCmdData->lineId, cCmdData->numPoints,
				ZeroVector,
				"",
				0, nullptr
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;
		case COMMAND_DEBUG_DRAWER_GRAPH_SET_POS: {
			SSetPositionGraphDrawerDebugCommand* cCmdData = static_cast<SSetPositionGraphDrawerDebugCommand*>(commandData);
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_GRAPH_POS,
				cCmdData->x, cCmdData->y,
				0.0f, 0.0f,
				0, 0,
				ZeroVector,
				"",
				0, nullptr
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;
		case COMMAND_DEBUG_DRAWER_GRAPH_SET_SIZE: {
			SSetSizeGraphDrawerDebugCommand* cCmdData = static_cast<SSetSizeGraphDrawerDebugCommand*>(commandData);
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_GRAPH_SIZE,
				0.0f, 0.0f,
				cCmdData->w, cCmdData->h,
				0, 0,
				ZeroVector,
				"",
				0, nullptr
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;
		case COMMAND_DEBUG_DRAWER_GRAPH_LINE_SET_COLOR: {
			SSetColorLineGraphDrawerDebugCommand* cCmdData = static_cast<SSetColorLineGraphDrawerDebugCommand*>(commandData);
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_GRAPH_LINE_COLOR,
				0.0f, 0.0f,
				0.0f, 0.0f,
				cCmdData->lineId, 0,
				float3(cCmdData->color_colorS3[0] / 256.0f, cCmdData->color_colorS3[1] / 256.0f, cCmdData->color_colorS3[2] / 256.0f),
				"",
				0, nullptr
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;
		case COMMAND_DEBUG_DRAWER_GRAPH_LINE_SET_LABEL: {
			SSetLabelLineGraphDrawerDebugCommand* cCmdData = static_cast<SSetLabelLineGraphDrawerDebugCommand*>(commandData);
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_GRAPH_LINE_LABEL,
				0.0f, 0.0f,
				0.0f, 0.0f,
				cCmdData->lineId, 0,
				ZeroVector,
				std::string(cCmdData->label),
				0, nullptr
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;


		case COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_ADD: {
			SAddOverlayTextureDrawerDebugCommand* cCmdData = static_cast<SAddOverlayTextureDrawerDebugCommand*>(commandData);
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_ADD_OVERLAY_TEXTURE,
				0.0f, 0.0f,
				float(cCmdData->w), float(cCmdData->h),
				0, 0,
				ZeroVector,
				"",
				0, cCmdData->texData
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);

			cCmdData->ret_overlayTextureId = cppCmdData.texHandle;
		} break;
		case COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_UPDATE: {
			SUpdateOverlayTextureDrawerDebugCommand* cCmdData = static_cast<SUpdateOverlayTextureDrawerDebugCommand*>(commandData);
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_UPDATE_OVERLAY_TEXTURE,
				float(cCmdData->x), float(cCmdData->y),
				float(cCmdData->w), float(cCmdData->h),
				0, 0,
				ZeroVector,
				"",
				cCmdData->overlayTextureId, cCmdData->texData
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;

		case COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_DELETE: {
			SDeleteOverlayTextureDrawerDebugCommand* cCmdData = static_cast<SDeleteOverlayTextureDrawerDebugCommand*>(commandData);
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_DEL_OVERLAY_TEXTURE,
				0.0f, 0.0f,
				0.0f, 0.0f,
				0, 0,
				ZeroVector,
				"",
				cCmdData->overlayTextureId, nullptr
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;
		case COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_POS: {
			SSetPositionOverlayTextureDrawerDebugCommand* cCmdData = static_cast<SSetPositionOverlayTextureDrawerDebugCommand*>(commandData);
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_OVERLAY_TEXTURE_POS,
				cCmdData->x, cCmdData->y,
				0.0f, 0.0f,
				0, 0,
				ZeroVector,
				"",
				cCmdData->overlayTextureId, nullptr
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;
		case COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_SIZE: {
			SSetSizeOverlayTextureDrawerDebugCommand* cCmdData = static_cast<SSetSizeOverlayTextureDrawerDebugCommand*>(commandData);
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_OVERLAY_TEXTURE_SIZE,
				0.0f, 0.0f,
				cCmdData->w, cCmdData->h,
				0, 0,
				ZeroVector,
				"",
				cCmdData->overlayTextureId, nullptr
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;
		case COMMAND_DEBUG_DRAWER_OVERLAYTEXTURE_SET_LABEL: {
			SSetLabelOverlayTextureDrawerDebugCommand* cCmdData = static_cast<SSetLabelOverlayTextureDrawerDebugCommand*>(commandData);
			AIHCDebugDraw cppCmdData = {
				AIHCDebugDraw::AIHC_DEBUGDRAWER_MODE_SET_OVERLAY_TEXTURE_LABEL,
				0.0f, 0.0f,
				0.0f, 0.0f,
				0, 0,
				ZeroVector,
				std::string(cCmdData->label),
				cCmdData->overlayTextureId, nullptr
			};

			wrapper_HandleCommand(clb, clbCheat, AIHCDebugDrawId, &cppCmdData);
		} break;


		default: {
			Command c;

			// check if data is a known command
			if (newCommand(commandData, commandTopic, unitHandler.MaxUnits(), &c)) {
				c.SetAICmdID(commandId);

				const SStopUnitCommand* cmd = static_cast<SStopUnitCommand*>(commandData);

				if (cmd->unitId >= 0) {
					ret = clb->GiveOrder(cmd->unitId, &c);
				} else {
					ret = clb->GiveGroupOrder(cmd->groupId, &c);
				}
			} else {
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

EXPORT(const char*) skirmishAiCallback_Engine_Version_getCommits(int skirmishAIId) {
	return aiInterfaceCallback_Engine_Version_getCommits(-1);
}

EXPORT(const char*) skirmishAiCallback_Engine_Version_getHash(int skirmishAIId) {
	return aiInterfaceCallback_Engine_Version_getHash(-1);
}

EXPORT(const char*) skirmishAiCallback_Engine_Version_getBranch(int skirmishAIId) {
	return aiInterfaceCallback_Engine_Version_getBranch(-1);
}

EXPORT(const char*) skirmishAiCallback_Engine_Version_getAdditional(int skirmishAIId) {
	return aiInterfaceCallback_Engine_Version_getAdditional(-1);
}

EXPORT(const char*) skirmishAiCallback_Engine_Version_getBuildTime(int skirmishAIId) {
	return aiInterfaceCallback_Engine_Version_getBuildTime(-1);
}

EXPORT(bool) skirmishAiCallback_Engine_Version_isRelease(int skirmishAIId) {
	return aiInterfaceCallback_Engine_Version_isRelease(-1);
}

EXPORT(const char*) skirmishAiCallback_Engine_Version_getNormal(int skirmishAIId) {
	return aiInterfaceCallback_Engine_Version_getNormal(-1);
}

EXPORT(const char*) skirmishAiCallback_Engine_Version_getSync(int skirmishAIId) {
	return aiInterfaceCallback_Engine_Version_getSync(-1);
}

EXPORT(const char*) skirmishAiCallback_Engine_Version_getFull(int skirmishAIId) {
	return aiInterfaceCallback_Engine_Version_getFull(-1);
}

EXPORT(int) skirmishAiCallback_Teams_getSize(int skirmishAIId) {
	return teamHandler.ActiveTeams();
}

EXPORT(int) skirmishAiCallback_SkirmishAIs_getSize(int skirmishAIId) {
	return aiInterfaceCallback_SkirmishAIs_getSize(-1);
}

EXPORT(int) skirmishAiCallback_SkirmishAIs_getMax(int skirmishAIId) {
	return aiInterfaceCallback_SkirmishAIs_getMax(-1);
}

EXPORT(int) skirmishAiCallback_SkirmishAI_getTeamId(int skirmishAIId) {
	return AI_TEAM_IDS[skirmishAIId];
}

static inline const CSkirmishAILibraryInfo* getSkirmishAILibraryInfo(int skirmishAIId) {
	const CSkirmishAILibraryInfo* info = nullptr;

	const AILibraryManager* libMan = AILibraryManager::GetInstance();
	const SkirmishAIKey* key = skirmishAIHandler.GetLocalSkirmishAILibraryKey(skirmishAIId);

	assert(key != nullptr);

	const auto& infs = libMan->GetSkirmishAIInfos();
	const auto inf = infs.find(*key);

	if (inf != infs.end())
		info = &(inf->second);

	assert(info != nullptr);
	return info;
}

EXPORT(int) skirmishAiCallback_SkirmishAI_Info_getSize(int skirmishAIId) {
	return (int) getSkirmishAILibraryInfo(skirmishAIId)->size();
}

// note: fine to return c_str(), points to data held by SkirmishAIHandler
EXPORT(const char*) skirmishAiCallback_SkirmishAI_Info_getKey(int skirmishAIId, int infoIndex) {
	return ((getSkirmishAILibraryInfo(skirmishAIId)->GetKeyAt(infoIndex)).c_str());
}

EXPORT(const char*) skirmishAiCallback_SkirmishAI_Info_getValue(int skirmishAIId, int infoIndex) {
	return ((getSkirmishAILibraryInfo(skirmishAIId)->GetValueAt(infoIndex)).c_str());
}

EXPORT(const char*) skirmishAiCallback_SkirmishAI_Info_getDescription(int skirmishAIId, int infoIndex) {
	return ((getSkirmishAILibraryInfo(skirmishAIId)->GetDescriptionAt(infoIndex)).c_str());
}

EXPORT(const char*) skirmishAiCallback_SkirmishAI_Info_getValueByKey(int skirmishAIId, const char* const key) {
	return ((getSkirmishAILibraryInfo(skirmishAIId)->GetInfo(key)).c_str());
}

static inline bool checkOptionIndex(int optionIndex, const std::vector<std::string>& optionKeys) {
	return ((optionIndex < 0) || ((size_t)optionIndex >= optionKeys.size()));
}

EXPORT(int) skirmishAiCallback_SkirmishAI_OptionValues_getSize(int skirmishAIId) {
	return (int)skirmishAIHandler.GetSkirmishAI(skirmishAIId)->options.size();
}


EXPORT(const char*) skirmishAiCallback_SkirmishAI_OptionValues_getKey(int skirmishAIId, int optionIndex) {
	const SkirmishAIData* data = skirmishAIHandler.GetSkirmishAI(skirmishAIId);
	const auto& optionKeys = data->optionKeys;

	if (checkOptionIndex(optionIndex, optionKeys))
		return nullptr;

	return (optionKeys[optionIndex].c_str());
}

EXPORT(const char*) skirmishAiCallback_SkirmishAI_OptionValues_getValue(int skirmishAIId, int optionIndex) {
	const SkirmishAIData* data = skirmishAIHandler.GetSkirmishAI(skirmishAIId);
	const auto& optionKeys = data->optionKeys;

	if (checkOptionIndex(optionIndex, optionKeys))
		return nullptr;

	const auto& options = data->options;
	const auto option = options.find(optionKeys[optionIndex]);

	if (option == options.end())
		return nullptr;

	return (option->second.c_str());
}


EXPORT(const char*) skirmishAiCallback_SkirmishAI_OptionValues_getValueByKey(int skirmishAIId, const char* const key) {
	const SkirmishAIData* data = skirmishAIHandler.GetSkirmishAI(skirmishAIId);

	const auto& options = data->options;
	const auto option = options.find(key);

	if (option == options.end())
		return nullptr;

	return (option->second.c_str());
}


EXPORT(void) skirmishAiCallback_Log_log(int skirmishAIId, const char* const msg) {
	CheckSkirmishAIId(skirmishAIId, __func__);

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(skirmishAIId);
	const std::string& aiName = info->GetName();
	const std::string& aiVersion = info->GetVersion();
	LOG("Skirmish AI <%s-%s>: %s", aiName.c_str(), aiVersion.c_str(), msg);
}

EXPORT(void) skirmishAiCallback_Log_exception(int skirmishAIId, const char* const msg, int severity, bool die) {
	CheckSkirmishAIId(skirmishAIId, __func__);

	const CSkirmishAILibraryInfo* info = getSkirmishAILibraryInfo(skirmishAIId);
	const char* aiName = (info->GetName()).c_str();
	const char* aiVersion = (info->GetVersion()).c_str();
	const char* status = die? "AI shutting down" : "AI still running";

	LOG_L(L_ERROR, "Skirmish AI <%s-%s>: severity %i: [%s] %s", aiName, aiVersion, severity, status, msg);

	if (!die)
		return;

	skirmishAIHandler.SetLocalKillFlag(skirmishAIId, 4 /* = AI crashed */);
}

EXPORT(char) skirmishAiCallback_DataDirs_getPathSeparator(int UNUSED_skirmishAIId) {
	return aiInterfaceCallback_DataDirs_getPathSeparator(-1);
}

EXPORT(int) skirmishAiCallback_DataDirs_Roots_getSize(int UNUSED_skirmishAIId) {
	return aiInterfaceCallback_DataDirs_Roots_getSize(-1);
}

EXPORT(bool) skirmishAiCallback_DataDirs_Roots_getDir(int UNUSED_skirmishAIId, char* path, int pathMaxSize, int dirIndex) {
	return aiInterfaceCallback_DataDirs_Roots_getDir(-1, path, pathMaxSize, dirIndex);
}

EXPORT(bool) skirmishAiCallback_DataDirs_Roots_locatePath(int UNUSED_skirmishAIId, char* path, int pathMaxSize, const char* const relPath, bool writeable, bool create, bool dir) {
	return aiInterfaceCallback_DataDirs_Roots_locatePath(-1, path, pathMaxSize, relPath, writeable, create, dir);
}

EXPORT(char*) skirmishAiCallback_DataDirs_Roots_allocatePath(int UNUSED_skirmishAIId, const char* const relPath, bool writeable, bool create, bool dir) {
	return aiInterfaceCallback_DataDirs_Roots_allocatePath(-1, relPath, writeable, create, dir);
}

EXPORT(const char*) skirmishAiCallback_DataDirs_getConfigDir(int skirmishAIId) {
	CheckSkirmishAIId(skirmishAIId, __func__);

	return ((getSkirmishAILibraryInfo(skirmishAIId)->GetDataDir()).c_str());
}

EXPORT(bool) skirmishAiCallback_DataDirs_locatePath(
	int skirmishAIId,
	char* path,
	int pathMaxSize,
	const char* const relPath,
	bool writeable,
	bool create,
	bool dir,
	bool common
) {
	assert(relPath != nullptr);

	const char ps = skirmishAiCallback_DataDirs_getPathSeparator(skirmishAIId);

	const std::string& aiShortName = skirmishAiCallback_SkirmishAI_Info_getValueByKey(skirmishAIId, SKIRMISH_AI_PROPERTY_SHORT_NAME);
	const std::string& aiVersion = skirmishAiCallback_SkirmishAI_Info_getValueByKey(skirmishAIId, SKIRMISH_AI_PROPERTY_VERSION);

	std::string aiRelPath(SKIRMISH_AI_DATA_DIR);

	// relPath="log/RAI.log", aiRelPath="AI/Skirmish/RAI/0.1" (e.g.)
	aiRelPath += (ps + aiShortName);
	aiRelPath += (ps + ((common)? "common": aiVersion));
	aiRelPath += (ps + std::string(relPath));

	return skirmishAiCallback_DataDirs_Roots_locatePath(skirmishAIId, path, pathMaxSize, aiRelPath.c_str(), writeable, create, dir);
}

EXPORT(char*) skirmishAiCallback_DataDirs_allocatePath(
	int skirmishAIId,
	const char* const relPath,
	bool writeable,
	bool create,
	bool dir,
	bool common
) {
	static char path[2048];

	if (!skirmishAiCallback_DataDirs_locatePath(skirmishAIId, &path[0], sizeof(path), relPath, writeable, create, dir, common))
		path[0] = 0;

	return &path[0];
}


EXPORT(const char*) skirmishAiCallback_DataDirs_getWriteableDir(int skirmishAIId) {
	CheckSkirmishAIId(skirmishAIId, __func__);

	static std::vector<std::string> writeableDataDirs;

	// fill up writeableDataDirs until teamId index is in there
	// if it is not yet
	for (size_t wdd = writeableDataDirs.size(); wdd <= (size_t)skirmishAIId; ++wdd) {
		writeableDataDirs.emplace_back("");
	}

	if (writeableDataDirs[skirmishAIId].empty()) {
		char tmpRes[1024];

		if (!skirmishAiCallback_DataDirs_locatePath(skirmishAIId, tmpRes, sizeof(tmpRes), "", true, true, true, false)) {
			char errorMsg[1093];
			SNPRINTF(errorMsg, sizeof(errorMsg),
					"Unable to create writable data-dir for Skirmish AI (ID:%i): %s",
					skirmishAIId, tmpRes);
			skirmishAiCallback_Log_exception(skirmishAIId, errorMsg, 1, true);
			return nullptr;
		} else {
			writeableDataDirs[skirmishAIId] = tmpRes;
		}
	}

	return writeableDataDirs[skirmishAIId].c_str();
}


//##############################################################################
EXPORT(bool) skirmishAiCallback_Cheats_isEnabled(int skirmishAIId) {
	return AI_CHEAT_FLAGS[skirmishAIId].first;
}

EXPORT(bool) skirmishAiCallback_Cheats_setEnabled(int skirmishAIId, bool enabled)
{
	if ((AI_CHEAT_FLAGS[skirmishAIId].first = enabled) && !AI_CHEAT_FLAGS[skirmishAIId].second) {
		LOG("[%s] SkirmishAI (id %i, team %i) is using cheats!", __func__, skirmishAIId, AI_TEAM_IDS[skirmishAIId]);
		AI_CHEAT_FLAGS[skirmishAIId].second = true;
	}

	return (enabled == skirmishAiCallback_Cheats_isEnabled(skirmishAIId));
}

EXPORT(bool) skirmishAiCallback_Cheats_setEventsEnabled(int skirmishAIId, bool enabled)
{
	if (!skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return false;

	GetCheatCallBack(skirmishAIId)->EnableCheatEvents(enabled);
	return true;
}

EXPORT(bool) skirmishAiCallback_Cheats_isOnlyPassive(int skirmishAIId) {
	return CAICheats::OnlyPassiveCheats();
}

EXPORT(int) skirmishAiCallback_Game_getAiInterfaceVersion(int skirmishAIId) {
	return wrapper_HandleCommand(GetCallBack(skirmishAIId), nullptr, AIHCQuerySubVersionId, nullptr);
}

EXPORT(int) skirmishAiCallback_Game_getCurrentFrame(int skirmishAIId) {
	return GetCallBack(skirmishAIId)->GetCurrentFrame();
}

EXPORT(int) skirmishAiCallback_Game_getMyTeam(int skirmishAIId) {
	return GetCallBack(skirmishAIId)->GetMyTeam();
}

EXPORT(int) skirmishAiCallback_Game_getMyAllyTeam(int skirmishAIId) {
	return GetCallBack(skirmishAIId)->GetMyAllyTeam();
}

EXPORT(int) skirmishAiCallback_Game_getPlayerTeam(int skirmishAIId, int player) {
	return GetCallBack(skirmishAIId)->GetPlayerTeam(player);
}

EXPORT(int) skirmishAiCallback_Game_getTeams(int skirmishAIId) {
	return teamHandler.ActiveTeams();
}

EXPORT(const char*) skirmishAiCallback_Game_getTeamSide(int skirmishAIId, int otherTeamId) {
	return GetCallBack(skirmishAIId)->GetTeamSide(otherTeamId);
}

EXPORT(void) skirmishAiCallback_Game_getTeamColor(int skirmishAIId, int otherTeamId, short* return_colorS3_out) {

	const unsigned char* color = teamHandler.Team(otherTeamId)->color;
	return_colorS3_out[0] = color[0];
	return_colorS3_out[1] = color[1];
	return_colorS3_out[2] = color[2];
}

EXPORT(float) skirmishAiCallback_Game_getTeamIncomeMultiplier(int skirmishAIId, int otherTeamId) {
	return teamHandler.Team(otherTeamId)->GetIncomeMultiplier();
}

EXPORT(int) skirmishAiCallback_Game_getTeamAllyTeam(int skirmishAIId, int otherTeamId) {
	return teamHandler.AllyTeam(otherTeamId);
}



EXPORT(float) skirmishAiCallback_Game_getTeamResourceCurrent(int skirmishAIId, int otherTeamId, int resourceId)
{
	float res = -1.0f;

	if (teamHandler.AlliedTeams(AI_TEAM_IDS[skirmishAIId], otherTeamId) || skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		if (resourceId == resourceHandler->GetMetalId()) {
			res = teamHandler.Team(otherTeamId)->res.metal;
		} else if (resourceId == resourceHandler->GetEnergyId()) {
			res = teamHandler.Team(otherTeamId)->res.energy;
		}
	}

	return res;
}

EXPORT(float) skirmishAiCallback_Game_getTeamResourceIncome(int skirmishAIId, int otherTeamId, int resourceId)
{
	float res = -1.0f;

	if (teamHandler.AlliedTeams(AI_TEAM_IDS[skirmishAIId], otherTeamId) || skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		if (resourceId == resourceHandler->GetMetalId()) {
			res = teamHandler.Team(otherTeamId)->resPrevIncome.metal;
		} else if (resourceId == resourceHandler->GetEnergyId()) {
			res = teamHandler.Team(otherTeamId)->resPrevIncome.energy;
		}
	}

	return res;
}

EXPORT(float) skirmishAiCallback_Game_getTeamResourceUsage(int skirmishAIId, int otherTeamId, int resourceId)
{
	float res = -1.0f;

	if (teamHandler.AlliedTeams(AI_TEAM_IDS[skirmishAIId], otherTeamId) || skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		if (resourceId == resourceHandler->GetMetalId()) {
			res = teamHandler.Team(otherTeamId)->resPrevExpense.metal;
		} else if (resourceId == resourceHandler->GetEnergyId()) {
			res = teamHandler.Team(otherTeamId)->resPrevExpense.energy;
		}
	}

	return res;
}

EXPORT(float) skirmishAiCallback_Game_getTeamResourceStorage(int skirmishAIId, int otherTeamId, int resourceId)
{
	float res = -1.0f;

	if (teamHandler.AlliedTeams(AI_TEAM_IDS[skirmishAIId], otherTeamId) || skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		if (resourceId == resourceHandler->GetMetalId()) {
			res = teamHandler.Team(otherTeamId)->resStorage.metal;
		} else if (resourceId == resourceHandler->GetEnergyId()) {
			res = teamHandler.Team(otherTeamId)->resStorage.energy;
		}
	}

	return res;
}

EXPORT(float) skirmishAiCallback_Game_getTeamResourcePull(int skirmishAIId, int otherTeamId, int resourceId)
{
	float res = -1.0f;

	if (teamHandler.AlliedTeams(AI_TEAM_IDS[skirmishAIId], otherTeamId) || skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		if (resourceId == resourceHandler->GetMetalId()) {
			res = teamHandler.Team(otherTeamId)->resStorage.metal;
		} else if (resourceId == resourceHandler->GetEnergyId()) {
			res = teamHandler.Team(otherTeamId)->resStorage.energy;
		}
	}

	return res;
}

EXPORT(float) skirmishAiCallback_Game_getTeamResourceShare(int skirmishAIId, int otherTeamId, int resourceId)
{
	float res = -1.0f;

	if (teamHandler.AlliedTeams(AI_TEAM_IDS[skirmishAIId], otherTeamId) || skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		if (resourceId == resourceHandler->GetMetalId()) {
			res = teamHandler.Team(otherTeamId)->resShare.metal;
		} else if (resourceId == resourceHandler->GetEnergyId()) {
			res = teamHandler.Team(otherTeamId)->resShare.energy;
		}
	}

	return res;
}

EXPORT(float) skirmishAiCallback_Game_getTeamResourceSent(int skirmishAIId, int otherTeamId, int resourceId)
{
	float res = -1.0f;

	if (teamHandler.AlliedTeams(AI_TEAM_IDS[skirmishAIId], otherTeamId) || skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		if (resourceId == resourceHandler->GetMetalId()) {
			res = teamHandler.Team(otherTeamId)->resPrevSent.metal;
		} else if (resourceId == resourceHandler->GetEnergyId()) {
			res = teamHandler.Team(otherTeamId)->resPrevSent.energy;
		}
	}

	return res;
}

EXPORT(float) skirmishAiCallback_Game_getTeamResourceReceived(int skirmishAIId, int otherTeamId, int resourceId)
{
	float res = -1.0f;

	if (teamHandler.AlliedTeams(AI_TEAM_IDS[skirmishAIId], otherTeamId) || skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		if (resourceId == resourceHandler->GetMetalId()) {
			res = teamHandler.Team(otherTeamId)->resPrevReceived.metal;
		} else if (resourceId == resourceHandler->GetEnergyId()) {
			res = teamHandler.Team(otherTeamId)->resPrevReceived.energy;
		}
	}

	return res;
}

EXPORT(float) skirmishAiCallback_Game_getTeamResourceExcess(int skirmishAIId, int otherTeamId, int resourceId)
{
	float res = -1.0f;

	if (teamHandler.AlliedTeams(AI_TEAM_IDS[skirmishAIId], otherTeamId) || skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		if (resourceId == resourceHandler->GetMetalId()) {
			res = teamHandler.Team(otherTeamId)->resPrevExcess.metal;
		} else if (resourceId == resourceHandler->GetEnergyId()) {
			res = teamHandler.Team(otherTeamId)->resPrevExcess.energy;
		}
	}

	return res;
}

EXPORT(bool) skirmishAiCallback_Game_isAllied(int skirmishAIId, int firstAllyTeamId, int secondAllyTeamId) {
	return teamHandler.Ally(firstAllyTeamId, secondAllyTeamId);
}





EXPORT(int) skirmishAiCallback_WeaponDef_getNumDamageTypes(int skirmishAIId) {
	int numDamageTypes = -1;

	if (!GetCallBack(skirmishAIId)->GetValue(AIVAL_NUMDAMAGETYPES, &numDamageTypes))
		numDamageTypes = -1;

	return numDamageTypes;
}

EXPORT(bool) skirmishAiCallback_Game_isDebugModeEnabled(int skirmishAIId) {
	bool debugModeEnabled = false;

	if (!GetCallBack(skirmishAIId)->GetValue(AIVAL_DEBUG_MODE, &debugModeEnabled))
		debugModeEnabled = false;

	return debugModeEnabled;
}

EXPORT(bool) skirmishAiCallback_Game_isPaused(int skirmishAIId) {
	bool paused = false;

	if (!GetCallBack(skirmishAIId)->GetValue(AIVAL_GAME_PAUSED, &paused))
		paused = false;

	return paused;
}

EXPORT(float) skirmishAiCallback_Game_getSpeedFactor(int skirmishAIId) {
	float speedFactor = 0.0f;

	if (!GetCallBack(skirmishAIId)->GetValue(AIVAL_GAME_SPEED_FACTOR, &speedFactor))
		speedFactor = 0.0f;

	return speedFactor;
}

EXPORT(int) skirmishAiCallback_Game_getCategoryFlag(int skirmishAIId, const char* categoryName) {
	return CCategoryHandler::Instance()->GetCategory(categoryName);
}

EXPORT(int) skirmishAiCallback_Game_getCategoriesFlag(int skirmishAIId, const char* categoryNames) {
	return CCategoryHandler::Instance()->GetCategories(categoryNames);
}

EXPORT(void) skirmishAiCallback_Game_getCategoryName(int skirmishAIId, int categoryFlag, char* name, int nameMaxSize) {

	const std::vector<std::string>& names = CCategoryHandler::Instance()->GetCategoryNames(categoryFlag);

	const char* theName = "";
	if (!names.empty()) {
		theName = names.begin()->c_str();
	}
	STRCPY_T(name, nameMaxSize, theName);
}


EXPORT(float) skirmishAiCallback_Game_getRulesParamFloat(int skirmishAIId, const char* rulesParamName, float defaultValue) {
	return getRulesParamFloatValueByName(CSplitLuaHandle::GetGameParams(), gameModParamLosMask(), rulesParamName, defaultValue);
}

EXPORT(const char*) skirmishAiCallback_Game_getRulesParamString(int skirmishAIId, const char* rulesParamName, const char* defaultValue) {
	return getRulesParamStringValueByName(CSplitLuaHandle::GetGameParams(), gameModParamLosMask(), rulesParamName, defaultValue);
}


EXPORT(float) skirmishAiCallback_Gui_getViewRange(int skirmishAIId) { return 0.0f; }
EXPORT(float) skirmishAiCallback_Gui_getScreenX(int skirmishAIId) { return 0.0f; }
EXPORT(float) skirmishAiCallback_Gui_getScreenY(int skirmishAIId) { return 0.0f; }

EXPORT(void) skirmishAiCallback_Gui_Camera_getDirection(int skirmishAIId, float* dir) {} // DEPRECATED
EXPORT(void) skirmishAiCallback_Gui_Camera_getPosition(int skirmishAIId, float* pos) {} // DEPRECATED







//########### BEGINN Mod

EXPORT(const char*) skirmishAiCallback_Mod_getFileName(int skirmishAIId) {
	return modInfo.filename.c_str();
}

EXPORT(int) skirmishAiCallback_Mod_getHash(int skirmishAIId) {
	return archiveScanner->GetArchiveCompleteChecksum(modInfo.humanNameVersioned);
}
EXPORT(const char*) skirmishAiCallback_Mod_getHumanName(int skirmishAIId) {
	return modInfo.humanNameVersioned.c_str();
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

EXPORT(int) skirmishAiCallback_Mod_getRadarMipLevel(int skirmishAIId) {
	return modInfo.radarMipLevel;
}

EXPORT(bool) skirmishAiCallback_Mod_getRequireSonarUnderWater(int skirmishAIId) {
	return modInfo.requireSonarUnderWater;
}

//########### END Mod



//########### BEGINN Map
EXPORT(bool) skirmishAiCallback_Map_isPosInCamera(int skirmishAIId, float* pos_posF3, float radius) {
	return GetCallBack(skirmishAIId)->PosInCamera(pos_posF3, radius);
}

EXPORT(int) skirmishAiCallback_Map_getChecksum(int skirmishAIId) {
	unsigned int checksum;

	if (!GetCallBack(skirmishAIId)->GetValue(AIVAL_MAP_CHECKSUM, &checksum))
		checksum = -1;

	return checksum;
}

EXPORT(int) skirmishAiCallback_Map_getWidth(int skirmishAIId) {
	return GetCallBack(skirmishAIId)->GetMapWidth();
}

EXPORT(int) skirmishAiCallback_Map_getHeight(int skirmishAIId) {
	return GetCallBack(skirmishAIId)->GetMapHeight();
}

EXPORT(int) skirmishAiCallback_Map_getHeightMap(int skirmishAIId, float* heights,
		int heightsMaxSize) {

	const int heightsRealSize = mapDims.mapx * mapDims.mapy;

	int heightsSize = heightsRealSize;

	if (heights != nullptr) {
		const float* tmpMap = GetCallBack(skirmishAIId)->GetHeightMap();
		heightsSize = std::min(heightsRealSize, heightsMaxSize);

		for (int i = 0; i < heightsSize; ++i) {
			heights[i] = tmpMap[i];
		}
	}

	return heightsSize;
}

EXPORT(int) skirmishAiCallback_Map_getCornersHeightMap(int skirmishAIId,
		float* cornerHeights, int cornerHeightsMaxSize) {

	const int cornerHeightsRealSize = mapDims.mapxp1 * mapDims.mapyp1;

	int cornerHeightsSize = cornerHeightsRealSize;

	if (cornerHeights != nullptr) {
		const float* tmpMap =  GetCallBack(skirmishAIId)->GetCornersHeightMap();
		cornerHeightsSize = std::min(cornerHeightsRealSize, cornerHeightsMaxSize);

		for (int i = 0; i < cornerHeightsSize; ++i) {
			cornerHeights[i] = tmpMap[i];
		}
	}

	return cornerHeightsSize;
}

EXPORT(float) skirmishAiCallback_Map_getMinHeight(int skirmishAIId) {
	return GetCallBack(skirmishAIId)->GetMinHeight();
}

EXPORT(float) skirmishAiCallback_Map_getMaxHeight(int skirmishAIId) {
	return GetCallBack(skirmishAIId)->GetMaxHeight();
}

EXPORT(int) skirmishAiCallback_Map_getSlopeMap(int skirmishAIId,
		float* slopes, int slopesMaxSize) {

	const int slopesRealSize = mapDims.hmapx * mapDims.hmapy;

	int slopesSize = slopesRealSize;

	if (slopes != nullptr) {
		const float* tmpMap =  GetCallBack(skirmishAIId)->GetSlopeMap();
		slopesSize = std::min(slopesRealSize, slopesMaxSize);

		for (int i = 0; i < slopesSize; ++i) {
			slopes[i] = tmpMap[i];
		}
	}

	return slopesSize;
}

#define GET_SENSOR_MAP(name, sensor)	\
EXPORT(int) skirmishAiCallback_Map_get##name##Map(int skirmishAIId,	\
		int* sensor##Values, int sensor##ValuesMaxSize) {	\
\
	const int sensor##ValuesRealSize = losHandler->sensor.size.x * losHandler->sensor.size.y;	\
\
	int sensor##ValuesSize = sensor##ValuesRealSize;	\
\
	if (sensor##Values != NULL) {	\
		int teamId = AI_TEAM_IDS[skirmishAIId];	\
		const unsigned short* tmpMap = &losHandler->sensor.losMaps[teamHandler.AllyTeam(teamId)].front();	\
		sensor##ValuesSize = std::min(sensor##ValuesRealSize, sensor##ValuesMaxSize);	\
		int i;	\
		for (i=0; i < sensor##ValuesSize; ++i) {	\
			sensor##Values[i] = tmpMap[i];	\
		}	\
	}	\
\
	return sensor##ValuesSize;	\
}

// skirmishAiCallback_Map_getLosMap
GET_SENSOR_MAP(Los, los)

// skirmishAiCallback_Map_getAirLosMap
GET_SENSOR_MAP(AirLos, airLos)

// skirmishAiCallback_Map_getRadarMap
GET_SENSOR_MAP(Radar, radar)

// skirmishAiCallback_Map_getSonarMap
GET_SENSOR_MAP(Sonar, sonar)

// skirmishAiCallback_Map_getSeismicMap
GET_SENSOR_MAP(Seismic, seismic)

// skirmishAiCallback_Map_getJammerMap
GET_SENSOR_MAP(Jammer, jammer)

// skirmishAiCallback_Map_getSonarJammerMap
GET_SENSOR_MAP(SonarJammer, sonarJammer)

EXPORT(int) skirmishAiCallback_Map_getResourceMapRaw(
	int skirmishAIId,
	int resourceId,
	short* resources,
	int resourcesMaxSize
) {
	int resourcesRealSize = 0;

	if (resourceId == resourceHandler->GetMetalId())
		resourcesRealSize = metalMap.GetSizeX() * metalMap.GetSizeZ();

	int resourcesSize = resourcesRealSize;

	if ((resources != nullptr) && (resourcesRealSize > 0)) {
		resourcesSize = std::min(resourcesRealSize, resourcesMaxSize);

		const unsigned char* tmpMap = nullptr;

		if (resourceId == resourceHandler->GetMetalId()) {
			tmpMap = GetCallBack(skirmishAIId)->GetMetalMap();
		} else {
			resourcesSize = 0;
		}

		if (tmpMap == nullptr)
			return 0;

		for (int i = 0; i < resourcesSize; ++i) {
			resources[i] = tmpMap[i];
		}
	}

	return resourcesSize;
}

static inline const CResourceMapAnalyzer* getResourceMapAnalyzer(int resourceId) {
	return resourceHandler->GetResourceMapAnalyzer(resourceId);
}

EXPORT(int) skirmishAiCallback_Map_getResourceMapSpotsPositions(
	int skirmishAIId,
	int resourceId,
	float* spots,
	int spotsMaxSize
) {
	const std::vector<float3>& intSpots = getResourceMapAnalyzer(resourceId)->GetSpots();
	const int spotsRealSize = intSpots.size() * 3;

	size_t spotsSize = spotsRealSize;

	if (spots != nullptr) {
		spotsSize = std::min(spotsRealSize, std::max(0, spotsMaxSize));

		size_t si = 0;
		for (auto s = intSpots.cbegin(); s != intSpots.cend() && si < spotsSize; ++s) {
			spots[si++] = s->x;
			spots[si++] = s->y;
			spots[si++] = s->z;
		}
	}

	return static_cast<int>(spotsSize);
}

EXPORT(float) skirmishAiCallback_Map_getResourceMapSpotsAverageIncome(int skirmishAIId, int resourceId) {
	return getResourceMapAnalyzer(resourceId)->GetAverageIncome();
}

EXPORT(void) skirmishAiCallback_Map_getResourceMapSpotsNearest(
	int skirmishAIId,
	int resourceId,
	float* pos_posF3,
	float* return_posF3_out
) {
	getResourceMapAnalyzer(resourceId)->GetNearestSpot(pos_posF3, AI_TEAM_IDS[skirmishAIId]).copyInto(return_posF3_out);
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
	return GetCallBack(skirmishAIId)->GetElevation(x, z);
}


EXPORT(float) skirmishAiCallback_Map_getMaxResource(int skirmishAIId, int resourceId) {

	if (resourceId == resourceHandler->GetMetalId())
		return GetCallBack(skirmishAIId)->GetMaxMetal();

	return -1.0f;
}

EXPORT(float) skirmishAiCallback_Map_getExtractorRadius(int skirmishAIId, int resourceId) {

	if (resourceId == resourceHandler->GetMetalId())
		return GetCallBack(skirmishAIId)->GetExtractorRadius();

	return -1.0f;
}

EXPORT(float) skirmishAiCallback_Map_getMinWind(int skirmishAIId) {
	return GetCallBack(skirmishAIId)->GetMinWind();
}

EXPORT(float) skirmishAiCallback_Map_getMaxWind(int skirmishAIId) {
	return GetCallBack(skirmishAIId)->GetMaxWind();
}

EXPORT(float) skirmishAiCallback_Map_getCurWind(int skirmishAIId) {
	return GetCallBack(skirmishAIId)->GetCurWind();
}

EXPORT(float) skirmishAiCallback_Map_getTidalStrength(int skirmishAIId) {
	return GetCallBack(skirmishAIId)->GetTidalStrength();
}

EXPORT(float) skirmishAiCallback_Map_getGravity(int skirmishAIId) {
	return GetCallBack(skirmishAIId)->GetGravity();
}

EXPORT(float) skirmishAiCallback_Map_getWaterDamage(int skirmishAIId) {
	return mapInfo->water.damage;
}


EXPORT(bool) skirmishAiCallback_Map_isDeformable(int skirmishAIId) {
	return !mapInfo->map.notDeformable;
}

EXPORT(float) skirmishAiCallback_Map_getHardness(int skirmishAIId) {
	return mapInfo->map.hardness;
}

EXPORT(int) skirmishAiCallback_Map_getHardnessModMap(int skirmishAIId, float* hardMods, int hardModsMaxSize) {
	const int hardModsRealSize = mapDims.hmapx * mapDims.hmapy;

	int hardModsSize = hardModsRealSize;

	if (hardMods != nullptr) {
		const unsigned char* typeMap = readMap->GetTypeMapSynced();
		hardModsSize = std::min(hardModsRealSize, hardModsMaxSize);

		for (int i = 0; i < hardModsSize; ++i) {
			hardMods[i] = mapInfo->terrainTypes[typeMap[i]].hardness;
		}
	}

	return hardModsSize;
}

EXPORT(int) skirmishAiCallback_Map_getSpeedModMap(
	int skirmishAIId,
	int speedModClass,
	float* speedMods,
	int speedModsMaxSize
) {
	const int speedModsRealSize = mapDims.hmapx * mapDims.hmapy;

	int speedModsSize = speedModsRealSize;

	if (speedMods != nullptr) {
		const unsigned char* typeMap = readMap->GetTypeMapSynced();
		speedModsSize = std::min(speedModsRealSize, speedModsMaxSize);

		switch (speedModClass) {
			case MoveDef::Tank:
				for (int i = 0; i < speedModsSize; ++i) {
					speedMods[i] = mapInfo->terrainTypes[typeMap[i]].tankSpeed;
				}
				break;
			case MoveDef::KBot:
				for (int i = 0; i < speedModsSize; ++i) {
					speedMods[i] = mapInfo->terrainTypes[typeMap[i]].kbotSpeed;
				}
				break;
			case MoveDef::Hover:
				for (int i = 0; i < speedModsSize; ++i) {
					speedMods[i] = mapInfo->terrainTypes[typeMap[i]].hoverSpeed;
				}
				break;
			case MoveDef::Ship:
				for (int i = 0; i < speedModsSize; ++i) {
					speedMods[i] = mapInfo->terrainTypes[typeMap[i]].shipSpeed;
				}
				break;
			default:
				assert(false);
		}
	}

	return speedModsSize;
}


EXPORT(bool) skirmishAiCallback_Map_isPossibleToBuildAt(int skirmishAIId, int unitDefId, float* pos_posF3, int facing) {
	return GetCallBack(skirmishAIId)->CanBuildAt(getUnitDefById(skirmishAIId, unitDefId), pos_posF3, facing);
}

EXPORT(void) skirmishAiCallback_Map_findClosestBuildSite(
	int skirmishAIId,
	int unitDefId,
	float* pos_posF3,
	float searchRadius,
	int minDist,
	int facing,
	float* return_posF3_out
) {
	const UnitDef* unitDef = getUnitDefById(skirmishAIId, unitDefId);
	const float3 buildPos = GetCallBack(skirmishAIId)->ClosestBuildSite(unitDef, pos_posF3, searchRadius, minDist, facing);

	buildPos.copyInto(return_posF3_out);
}

EXPORT(int) skirmishAiCallback_Map_getPoints(int skirmishAIId, bool includeAllies) {
	GetCallBack(skirmishAIId)->GetMapPoints(AI_TMP_POINT_MARKERS[skirmishAIId], MAX_NUM_MARKERS, includeAllies);
	return (int)AI_TMP_POINT_MARKERS[skirmishAIId].size();
}

EXPORT(void) skirmishAiCallback_Map_Point_getPosition(int skirmishAIId, int pointId, float* return_posF3_out) {
	AI_TMP_POINT_MARKERS[skirmishAIId][pointId].pos.copyInto(return_posF3_out);
}

EXPORT(void) skirmishAiCallback_Map_Point_getColor(int skirmishAIId, int pointId, short* return_colorS3_out) {
	const unsigned char* color = AI_TMP_POINT_MARKERS[skirmishAIId][pointId].color;

	return_colorS3_out[0] = color[0];
	return_colorS3_out[1] = color[1];
	return_colorS3_out[2] = color[2];
}

EXPORT(const char*) skirmishAiCallback_Map_Point_getLabel(int skirmishAIId, int pointId) {
	return AI_TMP_POINT_MARKERS[skirmishAIId][pointId].label;
}

EXPORT(int) skirmishAiCallback_Map_getLines(int skirmishAIId, bool includeAllies) {
	GetCallBack(skirmishAIId)->GetMapLines(AI_TMP_LINE_MARKERS[skirmishAIId], MAX_NUM_MARKERS, includeAllies);
	return (int)AI_TMP_LINE_MARKERS[skirmishAIId].size();
}

EXPORT(void) skirmishAiCallback_Map_Line_getFirstPosition(int skirmishAIId, int lineId, float* return_posF3_out) {
	AI_TMP_LINE_MARKERS[skirmishAIId][lineId].pos.copyInto(return_posF3_out);
}

EXPORT(void) skirmishAiCallback_Map_Line_getSecondPosition(int skirmishAIId, int lineId, float* return_posF3_out) {
	AI_TMP_LINE_MARKERS[skirmishAIId][lineId].pos2.copyInto(return_posF3_out);
}

EXPORT(void) skirmishAiCallback_Map_Line_getColor(int skirmishAIId, int lineId, short* return_colorS3_out) {
	const unsigned char* color = AI_TMP_LINE_MARKERS[skirmishAIId][lineId].color;

	return_colorS3_out[0] = color[0];
	return_colorS3_out[1] = color[1];
	return_colorS3_out[2] = color[2];
}

EXPORT(void) skirmishAiCallback_Map_getStartPos(int skirmishAIId, float* return_posF3_out) {
	GetCallBack(skirmishAIId)->GetStartPos()->copyInto(return_posF3_out);
}

EXPORT(void) skirmishAiCallback_Map_getMousePos(int skirmishAIId, float* return_posF3_out) {
	GetCallBack(skirmishAIId)->GetMousePos().copyInto(return_posF3_out);
}

//########### END Map


EXPORT(int) skirmishAiCallback_File_getSize(int skirmishAIId, const char* fileName) {
	return GetCallBack(skirmishAIId)->GetFileSize(fileName);
}

EXPORT(bool) skirmishAiCallback_File_getContent(int skirmishAIId, const char* fileName, void* buffer, int bufferLen) {
	return GetCallBack(skirmishAIId)->ReadFile(fileName, buffer, bufferLen);
}





// BEGINN OBJECT Resource
EXPORT(int) skirmishAiCallback_getResources(int skirmishAIId) {
	return resourceHandler->GetNumResources();
}

EXPORT(int) skirmishAiCallback_getResourceByName(int skirmishAIId, const char* resourceName) {
	return resourceHandler->GetResourceId(resourceName);
}

EXPORT(const char*) skirmishAiCallback_Resource_getName(int skirmishAIId, int resourceId) {
	return resourceHandler->GetResource(resourceId)->name.c_str();
}

EXPORT(float) skirmishAiCallback_Resource_getOptimum(int skirmishAIId, int resourceId) {
	return resourceHandler->GetResource(resourceId)->optimum;
}

EXPORT(float) skirmishAiCallback_Economy_getCurrent(int skirmishAIId, int resourceId) {
	CAICallback* clb = GetCallBack(skirmishAIId);

	if (resourceId == resourceHandler->GetMetalId())
		return clb->GetMetal();

	if (resourceId == resourceHandler->GetEnergyId())
		return clb->GetEnergy();

	return -1.0f;
}

EXPORT(float) skirmishAiCallback_Economy_getIncome(int skirmishAIId, int resourceId) {
	CAICallback* clb = GetCallBack(skirmishAIId);

	if (resourceId == resourceHandler->GetMetalId())
		return clb->GetMetalIncome();

	if (resourceId == resourceHandler->GetEnergyId())
		return clb->GetEnergyIncome();

	return -1.0f;
}

EXPORT(float) skirmishAiCallback_Economy_getUsage(int skirmishAIId, int resourceId) {
	CAICallback* clb = GetCallBack(skirmishAIId);

	if (resourceId == resourceHandler->GetMetalId())
		return clb->GetMetalUsage();

	if (resourceId == resourceHandler->GetEnergyId())
		return clb->GetEnergyUsage();

	return -1.0f;
}

EXPORT(float) skirmishAiCallback_Economy_getStorage(int skirmishAIId, int resourceId) {
	CAICallback* clb = GetCallBack(skirmishAIId);

	if (resourceId == resourceHandler->GetMetalId())
		return clb->GetMetalStorage();

	if (resourceId == resourceHandler->GetEnergyId())
		return clb->GetEnergyStorage();

	return -1.0f;
}

EXPORT(float) skirmishAiCallback_Economy_getPull(int skirmishAIId, int resourceId) {
	const int teamId = AI_TEAM_IDS[skirmishAIId];

	if (resourceId == resourceHandler->GetMetalId())
		return teamHandler.Team(teamId)->resPrevPull.metal;

	if (resourceId == resourceHandler->GetEnergyId())
		return teamHandler.Team(teamId)->resPrevPull.energy;

	return -1.0f;
}

EXPORT(float) skirmishAiCallback_Economy_getShare(int skirmishAIId, int resourceId) {
	int teamId = AI_TEAM_IDS[skirmishAIId];

	if (resourceId == resourceHandler->GetMetalId())
		return teamHandler.Team(teamId)->resShare.metal;

	if (resourceId == resourceHandler->GetEnergyId())
		return teamHandler.Team(teamId)->resShare.energy;

	return -1.0f;
}

EXPORT(float) skirmishAiCallback_Economy_getSent(int skirmishAIId, int resourceId) {
	int teamId = AI_TEAM_IDS[skirmishAIId];

	if (resourceId == resourceHandler->GetMetalId())
		return teamHandler.Team(teamId)->resPrevSent.metal;

	if (resourceId == resourceHandler->GetEnergyId())
		return teamHandler.Team(teamId)->resPrevSent.energy;

	return -1.0f;
}

EXPORT(float) skirmishAiCallback_Economy_getReceived(int skirmishAIId, int resourceId) {
	int teamId = AI_TEAM_IDS[skirmishAIId];

	if (resourceId == resourceHandler->GetMetalId())
		return teamHandler.Team(teamId)->resPrevReceived.metal;

	if (resourceId == resourceHandler->GetEnergyId())
		return teamHandler.Team(teamId)->resPrevReceived.energy;

	return -1.0f;
}

EXPORT(float) skirmishAiCallback_Economy_getExcess(int skirmishAIId, int resourceId) {
	int teamId = AI_TEAM_IDS[skirmishAIId];

	if (resourceId == resourceHandler->GetMetalId())
		return teamHandler.Team(teamId)->resPrevExcess.metal;

	if (resourceId == resourceHandler->GetEnergyId())
		return teamHandler.Team(teamId)->resPrevExcess.energy;

	return -1.0f;
}

EXPORT(const char*) skirmishAiCallback_Game_getSetupScript(int skirmishAIId) {
	// points to GameSetup text
	const char* setupScript = "";

	if (!GetCallBack(skirmishAIId)->GetValue(AIVAL_SCRIPT, &setupScript))
		return "";

	return setupScript;
}







//########### BEGINN UnitDef
EXPORT(int) skirmishAiCallback_getUnitDefs(
	int skirmishAIId,
	int* unitDefIds,
	int unitDefIdsMaxSize
) {
	const int unitDefIdsRealSize = GetCallBack(skirmishAIId)->GetNumUnitDefs();

	int unitDefIdsSize = unitDefIdsRealSize;

	if (unitDefIds != nullptr) {
		std::vector<const UnitDef*> defList(unitDefIdsRealSize, nullptr);
		GetCallBack(skirmishAIId)->GetUnitDefList(&defList[0]);
		unitDefIdsSize = std::min(unitDefIdsRealSize, unitDefIdsMaxSize);

		for (int ud = 0; ud < unitDefIdsSize; ++ud) {
			// AI's should double-check for this
			unitDefIds[ud] = (defList[ud] != nullptr)? defList[ud]->id: -1;
		}
	}

	return unitDefIdsSize;
}

EXPORT(int) skirmishAiCallback_getUnitDefByName(int skirmishAIId, const char* unitName) {
	const UnitDef* ud = GetCallBack(skirmishAIId)->GetUnitDef(unitName);

	if (ud != nullptr)
		return ud->id;

	return -1;
}

EXPORT(float) skirmishAiCallback_UnitDef_getHeight(int skirmishAIId, int unitDefId) {
	return GetCallBack(skirmishAIId)->GetUnitDefHeight(unitDefId);
}

EXPORT(float) skirmishAiCallback_UnitDef_getRadius(int skirmishAIId, int unitDefId) {
	return GetCallBack(skirmishAIId)->GetUnitDefRadius(unitDefId);
}

EXPORT(const char*) skirmishAiCallback_UnitDef_getName(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->name.c_str();
}

EXPORT(const char*) skirmishAiCallback_UnitDef_getHumanName(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->humanName.c_str();
}


EXPORT(float) skirmishAiCallback_UnitDef_getUpkeep(int skirmishAIId, int unitDefId, int resourceId) {
	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);

	if (resourceId == resourceHandler->GetMetalId())
		return ud->metalUpkeep;

	if (resourceId == resourceHandler->GetEnergyId())
		return ud->energyUpkeep;

	return 0.0f;
}

EXPORT(float) skirmishAiCallback_UnitDef_getResourceMake(int skirmishAIId, int unitDefId, int resourceId) {
	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);

	if (resourceId == resourceHandler->GetMetalId())
		return ud->metalMake;

	if (resourceId == resourceHandler->GetEnergyId())
		return ud->energyMake;

	return 0.0f;
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

EXPORT(float) skirmishAiCallback_UnitDef_getCost(int skirmishAIId, int unitDefId, int resourceId) {
	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);

	if (resourceId == resourceHandler->GetMetalId())
		return ud->metal;

	if (resourceId == resourceHandler->GetEnergyId())
		return ud->energy;

	return 0.0f;
}

EXPORT(float) skirmishAiCallback_UnitDef_getExtractsResource(int skirmishAIId, int unitDefId, int resourceId) {
	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);

	if (resourceId == resourceHandler->GetMetalId())
		return ud->extractsMetal;

	return 0.0f;
}

EXPORT(float) skirmishAiCallback_UnitDef_getResourceExtractorRange(int skirmishAIId, int unitDefId, int resourceId) {
	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);

	if (resourceId == resourceHandler->GetMetalId())
		return ud->extractRange;

	return 0.0f;
}

EXPORT(float) skirmishAiCallback_UnitDef_getWindResourceGenerator(int skirmishAIId, int unitDefId, int resourceId) {
	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);

	if (resourceId == resourceHandler->GetEnergyId())
		return ud->windGenerator;

	return 0.0f;
}

EXPORT(float) skirmishAiCallback_UnitDef_getTidalResourceGenerator(int skirmishAIId, int unitDefId, int resourceId) {
	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);

	if (resourceId == resourceHandler->GetEnergyId())
		return ud->tidalGenerator;

	return 0.0f;
}

EXPORT(float) skirmishAiCallback_UnitDef_getStorage(int skirmishAIId, int unitDefId, int resourceId) {
	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);

	if (resourceId == resourceHandler->GetMetalId())
		return ud->metalStorage;

	if (resourceId == resourceHandler->GetEnergyId())
		return ud->energyStorage;

	return 0.0f;
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
	return 0.0f;
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


EXPORT(const char*) skirmishAiCallback_UnitDef_getTooltip(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->tooltip.c_str();
}

EXPORT(const char*) skirmishAiCallback_UnitDef_getWreckName(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->wreckName.c_str();
}

EXPORT(int) skirmishAiCallback_UnitDef_getDeathExplosion(int skirmishAIId, int unitDefId) {
	const WeaponDef* wd = getUnitDefById(skirmishAIId, unitDefId)->deathExpWeaponDef;
	return (wd == nullptr) ? -1 : wd->id;
}

EXPORT(int) skirmishAiCallback_UnitDef_getSelfDExplosion(int skirmishAIId, int unitDefId) {
	const WeaponDef* wd = getUnitDefById(skirmishAIId, unitDefId)->selfdExpWeaponDef;
	return (wd == nullptr) ? -1 : wd->id;
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
	const MoveDef* md = getUnitDefMoveDefById(skirmishAIId, unitDefId);

	return ((md != nullptr) ? (md->speedModClass == MoveDef::Hover) : false);
}

EXPORT(bool) skirmishAiCallback_UnitDef_isFloater(int skirmishAIId, int unitDefId) {
	const UnitDef* ud = getUnitDefById(skirmishAIId, unitDefId);
	const MoveDef* md = (ud->pathType != -1U)? moveDefHandler.GetMoveDefByPathType(ud->pathType): nullptr;

	return ((md != nullptr) ? md->FloatOnWater() : ud->floatOnWater);
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

EXPORT(int) skirmishAiCallback_UnitDef_getYardMap(
	int skirmishAIId,
	int unitDefId,
	int facing,
	short* yardMap,
	int yardMapMaxSize
) {
	const UnitDef* unitDef = getUnitDefById(skirmishAIId, unitDefId);
	const YardMapStatus* udYardMap = unitDef->GetYardMapPtr();

	int yardMapSize = unitDef->yardmap.size();

	if (yardMap != nullptr && udYardMap != nullptr) {
		yardMapSize = std::min(yardMapSize, yardMapMaxSize);

		const int xsize = unitDef->xsize;
		const int zsize = unitDef->zsize;

		int2 xdir(1, 0);
		int2 zdir(0, 1);

		int rowWidth = xsize;
		int startIdx = 0; // position of udYardMap[0] in the new destination array

		switch (facing) {
			case FACING_SOUTH:
				xdir = int2( 1, 0);
				zdir = int2( 0, 1);
				rowWidth = xsize;
				startIdx  = 0; // topleft
				break;
			case FACING_NORTH:
				xdir = int2(-1, 0);
				zdir = int2( 0,-1);
				rowWidth = xsize;
				startIdx  = (xsize * zsize) - 1; // bottomright
				break;
			case FACING_EAST:
				xdir = int2( 0, 1);
				zdir = int2(-1, 0);
				rowWidth = zsize;
				startIdx  = (xsize - 1) * zsize; // bottomleft
				break;
			case FACING_WEST:
				xdir = int2( 0,-1);
				zdir = int2( 1, 0);
				rowWidth = zsize;
				startIdx  = zsize - 1;  // topright
				break;
			default:
				assert(false);
		}

		// rotate yardmap for backward compatibility
		for (int z = 0; z < zsize; ++z) {
			for (int x = 0; x < xsize; ++x) {
				const int xt = xdir.x * x + xdir.y * z;
				const int zt = zdir.x * x + zdir.y * z;
				const int idx = startIdx + xt + zt * rowWidth;
				assert(idx >= 0 && idx < yardMapSize);
				yardMap[idx] = (short) udYardMap[x + z * xsize];
			}
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
	return false; // DEPRECATED
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

EXPORT(bool) skirmishAiCallback_UnitDef_canManualFire(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canManualFire;
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

EXPORT(bool) skirmishAiCallback_UnitDef_isAbleToLoopbackAttack(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->canLoopbackAttack;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isLevelGround(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->levelGround;
}


EXPORT(bool) skirmishAiCallback_UnitDef_isFirePlatform(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->isFirePlatform;
}

EXPORT(int) skirmishAiCallback_UnitDef_getMaxThisUnit(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->maxThisUnit;
}

EXPORT(int) skirmishAiCallback_UnitDef_getDecoyDef(int skirmishAIId, int unitDefId) {
	const UnitDef* decoyDef = getUnitDefById(skirmishAIId, unitDefId)->decoyDef;
	return (decoyDef == nullptr) ? -1 : decoyDef->id;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isDontLand(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->DontLand();
}

EXPORT(int) skirmishAiCallback_UnitDef_getShieldDef(int skirmishAIId, int unitDefId) {
	const WeaponDef* wd = getUnitDefById(skirmishAIId, unitDefId)->shieldWeaponDef;

	if (wd == nullptr)
		return -1;

	return wd->id;
}

EXPORT(int) skirmishAiCallback_UnitDef_getStockpileDef(int skirmishAIId, int unitDefId) {
	const WeaponDef* wd = getUnitDefById(skirmishAIId, unitDefId)->stockpileWeaponDef;

	if (wd == nullptr)
		return -1;

	return wd->id;
}

EXPORT(int) skirmishAiCallback_UnitDef_getBuildOptions(
	int skirmishAIId,
	int unitDefId,
	int* unitDefIds,
	int unitDefIdsMaxSize
) {
	const auto& bo = getUnitDefById(skirmishAIId, unitDefId)->buildOptions;
	const int unitDefIdsRealSize = bo.size();

	size_t unitDefIdsSize = unitDefIdsRealSize;

	if (unitDefIds != nullptr) {
		unitDefIdsSize = std::min(unitDefIdsRealSize, unitDefIdsMaxSize);

		auto bb = bo.cbegin();

		for (size_t b = 0; bb != bo.cend() && b < unitDefIdsSize; ++b, ++bb) {
			unitDefIds[b] = skirmishAiCallback_getUnitDefByName(skirmishAIId, bb->second.c_str());
		}
	}

	return unitDefIdsSize;
}

EXPORT(int) skirmishAiCallback_UnitDef_getCustomParams(
	int skirmishAIId,
	int unitDefId,
	const char** keys,
	const char** values
) {
	const auto& ps = getUnitDefById(skirmishAIId, unitDefId)->customParams;
	const size_t paramsRealSize = ps.size();

	if ((keys != nullptr) && (values != nullptr))
		ShallowCopyKeyValPairs(ps, keys, values);

	return paramsRealSize;
}

EXPORT(bool) skirmishAiCallback_UnitDef_isMoveDataAvailable(int skirmishAIId, int unitDefId) {
	// NOTE We can not use getUnitDefMoveDefById() here, cause it would assert
	return (getUnitDefById(skirmishAIId, unitDefId)->pathType != -1U);
}


EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getXSize(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDefById(skirmishAIId, unitDefId)->xsize;
}

EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getZSize(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDefById(skirmishAIId, unitDefId)->zsize;
}

EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getDepth(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDefById(skirmishAIId, unitDefId)->depth;
}

EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getMaxSlope(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDefById(skirmishAIId, unitDefId)->maxSlope;
}

EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getSlopeMod(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDefById(skirmishAIId, unitDefId)->slopeMod;
}

EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getDepthMod(int skirmishAIId, int unitDefId, float height) {
	return getUnitDefMoveDefById(skirmishAIId, unitDefId)->GetDepthMod(height);
}

EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getPathType(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDefById(skirmishAIId, unitDefId)->pathType;
}

EXPORT(float) skirmishAiCallback_UnitDef_MoveData_getCrushStrength(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDefById(skirmishAIId, unitDefId)->crushStrength;
}


EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getSpeedModClass(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDefById(skirmishAIId, unitDefId)->speedModClass;
}

EXPORT(int) skirmishAiCallback_UnitDef_MoveData_getTerrainClass(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDefById(skirmishAIId, unitDefId)->terrainClass;
}

EXPORT(bool) skirmishAiCallback_UnitDef_MoveData_getFollowGround(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDefById(skirmishAIId, unitDefId)->followGround;
}

EXPORT(bool) skirmishAiCallback_UnitDef_MoveData_isSubMarine(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDefById(skirmishAIId, unitDefId)->isSubmarine;
}

EXPORT(const char*) skirmishAiCallback_UnitDef_MoveData_getName(int skirmishAIId, int unitDefId) {
	return getUnitDefMoveDefById(skirmishAIId, unitDefId)->name.c_str();
}



EXPORT(int) skirmishAiCallback_UnitDef_getWeaponMounts(int skirmishAIId, int unitDefId) {
	return getUnitDefById(skirmishAIId, unitDefId)->NumWeapons();
}

EXPORT(const char*) skirmishAiCallback_UnitDef_WeaponMount_getName(int skirmishAIId, int unitDefId, int weaponMountId) {
	return ""; // getUnitDefById(skirmishAIId, unitDefId)->weapons.at(weaponMountId).name.c_str();
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
	return getUnitDefById(skirmishAIId, unitDefId)->weapons.at(weaponMountId).maxMainDirAngleDif;
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
	const int team = AI_TEAM_IDS[skirmishAIId];
	const int limit = teamHandler.Team(team)->GetMaxUnits();
	return limit;
}

EXPORT(int) skirmishAiCallback_Unit_getMax(int skirmishAIId) {
	return unitHandler.MaxUnits();
}

EXPORT(int) skirmishAiCallback_Unit_getDef(int skirmishAIId, int unitId) {
	const UnitDef* unitDef = nullptr;

	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		unitDef = GetCheatCallBack(skirmishAIId)->GetUnitDef(unitId);
	} else {
		unitDef = GetCallBack(skirmishAIId)->GetUnitDef(unitId);
	}

	if (unitDef != nullptr)
		return unitDef->id;

	return -1;
}

EXPORT(float) skirmishAiCallback_Unit_getRulesParamFloat(int skirmishAIId, int unitId, const char* rulesParamName, float defaultValue) {
	const CUnit* unit = getUnit(unitId);

	if (unit == nullptr)
		return defaultValue;

	return getRulesParamFloatValueByName(unit->modParams, unitModParamLosMask(skirmishAIId, unit), rulesParamName, defaultValue);
}

EXPORT(const char*) skirmishAiCallback_Unit_getRulesParamString(int skirmishAIId, int unitId, const char* rulesParamName, const char* defaultValue) {
	const CUnit* unit = getUnit(unitId);

	if (unit == nullptr)
		return defaultValue;

	return getRulesParamStringValueByName(unit->modParams, unitModParamLosMask(skirmishAIId, unit), rulesParamName, defaultValue);
}

EXPORT(int) skirmishAiCallback_Unit_getTeam(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->GetUnitTeam(unitId);

	return GetCallBack(skirmishAIId)->GetUnitTeam(unitId);
}

EXPORT(int) skirmishAiCallback_Unit_getAllyTeam(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->GetUnitAllyTeam(unitId);

	return GetCallBack(skirmishAIId)->GetUnitAllyTeam(unitId);
}


EXPORT(int) skirmishAiCallback_Unit_getSupportedCommands(int skirmishAIId, int unitId) {
	return GetCallBack(skirmishAIId)->GetUnitCommands(unitId)->size();
}

EXPORT(int) skirmishAiCallback_Unit_SupportedCommand_getId(int skirmishAIId, int unitId, int supportedCommandId) {
	return GetCallBack(skirmishAIId)->GetUnitCommands(unitId)->at(supportedCommandId)->id;
}

EXPORT(const char*) skirmishAiCallback_Unit_SupportedCommand_getName(int skirmishAIId, int unitId, int supportedCommandId) {
	return GetCallBack(skirmishAIId)->GetUnitCommands(unitId)->at(supportedCommandId)->name.c_str();
}

EXPORT(const char*) skirmishAiCallback_Unit_SupportedCommand_getToolTip(int skirmishAIId, int unitId, int supportedCommandId) {
	return GetCallBack(skirmishAIId)->GetUnitCommands(unitId)->at(supportedCommandId)->tooltip.c_str();
}

EXPORT(bool) skirmishAiCallback_Unit_SupportedCommand_isShowUnique(int skirmishAIId, int unitId, int supportedCommandId) {
	return GetCallBack(skirmishAIId)->GetUnitCommands(unitId)->at(supportedCommandId)->showUnique;
}

EXPORT(bool) skirmishAiCallback_Unit_SupportedCommand_isDisabled(int skirmishAIId, int unitId, int supportedCommandId) {
	return GetCallBack(skirmishAIId)->GetUnitCommands(unitId)->at(supportedCommandId)->disabled;
}

EXPORT(int) skirmishAiCallback_Unit_SupportedCommand_getParams(
	int skirmishAIId,
	int unitId,
	int supportedCommandId,
	const char** params,
	int maxNumParams
) {
	/*const*/ CAICallback* cb = GetCallBack(skirmishAIId);
	const std::vector<const SCommandDescription*>* cq = cb->GetUnitCommands(unitId);
	const std::vector<std::string>& ps = cq->at(supportedCommandId)->params;

	const int cqNumParams = ps.size();
	// NOTE: LegacyCpp AI interface wrapper expects real array size as return-value after 1st call with outArray=nullptr
	int retNumParams = cqNumParams;

	if (params != nullptr) {
		for (int i = 0, n = (retNumParams = std::min(cqNumParams, maxNumParams)); i < n; i++) {
			params[i] = ps.at(i).c_str();
		}
	}

	return retNumParams;
}

EXPORT(int) skirmishAiCallback_Unit_getStockpile(int skirmishAIId, int unitId) {
	int stockpile;

	if (!GetCallBack(skirmishAIId)->GetProperty(unitId, AIVAL_STOCKPILED, &stockpile))
		stockpile = -1;

	return stockpile;
}

EXPORT(int) skirmishAiCallback_Unit_getStockpileQueued(int skirmishAIId, int unitId) {
	int stockpileQueue;

	if (!GetCallBack(skirmishAIId)->GetProperty(unitId, AIVAL_STOCKPILE_QUED, &stockpileQueue))
		stockpileQueue = -1;

	return stockpileQueue;
}

EXPORT(float) skirmishAiCallback_Unit_getMaxSpeed(int skirmishAIId, int unitId) {
	float maxSpeed;

	if (!GetCallBack(skirmishAIId)->GetProperty(unitId, AIVAL_UNIT_MAXSPEED, &maxSpeed))
		maxSpeed = -1.0f;

	return maxSpeed;
}


EXPORT(float) skirmishAiCallback_Unit_getMaxRange(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		const CUnit* unit = getUnit(unitId);
		return (unit != nullptr)? unit->maxRange : -1.0f;
	}

	return GetCallBack(skirmishAIId)->GetUnitMaxRange(unitId);
}

EXPORT(float) skirmishAiCallback_Unit_getMaxHealth(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->GetUnitMaxHealth(unitId);

	return GetCallBack(skirmishAIId)->GetUnitMaxHealth(unitId);
}


/**
 * Returns a units command queue.
 * The return value may be <code>NULL</code> in some cases,
 * eg. when cheats are disabled and we try to fetch from an enemy unit.
 * For internal use only.
 */
static inline const CCommandQueue* _intern_Unit_getCurrentCommandQueue(int skirmishAIId, int unitId) {
	const CCommandQueue* q = nullptr;

	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		q = GetCheatCallBack(skirmishAIId)->GetCurrentUnitCommands(unitId);
	} else {
		q = GetCallBack(skirmishAIId)->GetCurrentUnitCommands(unitId);
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
	return ((q != nullptr)? q->size(): 0);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getType(int skirmishAIId, int unitId) {
	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(skirmishAIId, unitId);
	return ((q != nullptr)? q->GetType(): -1);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getId(int skirmishAIId, int unitId, int commandId) {
	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(skirmishAIId, unitId);
	return (CHECK_COMMAND_ID(q, commandId) ? q->at(commandId).GetID() : 0);
}

EXPORT(short) skirmishAiCallback_Unit_CurrentCommand_getOptions(int skirmishAIId, int unitId, int commandId) {
	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(skirmishAIId, unitId);
	return (CHECK_COMMAND_ID(q, commandId) ? q->at(commandId).GetOpts() : 0);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getTag(int skirmishAIId, int unitId, int commandId) {
	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(skirmishAIId, unitId);
	return (CHECK_COMMAND_ID(q, commandId) ? q->at(commandId).GetTag() : 0);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getTimeOut(int skirmishAIId, int unitId, int commandId) {
	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(skirmishAIId, unitId);
	return (CHECK_COMMAND_ID(q, commandId) ? q->at(commandId).GetTimeOut() : 0);
}

EXPORT(int) skirmishAiCallback_Unit_CurrentCommand_getParams(
	int skirmishAIId,
	int unitId,
	int commandId,
	float* params,
	int maxNumParams
) {
	const CCommandQueue* q = _intern_Unit_getCurrentCommandQueue(skirmishAIId, unitId);

	if (!CHECK_COMMAND_ID(q, commandId))
		return -1;

	const int cqNumParams = q->at(commandId).GetNumParams();
	// NOTE: LegacyCpp AI interface wrapper expects real array size as return-value after 1st call with outArray=nullptr
	int retNumParams = cqNumParams;

	if (params != nullptr) {
		const float* cmdParams = q->at(commandId).GetParams();

		for (int i = 0, n = (retNumParams = std::min(cqNumParams, maxNumParams)); i < n; i++) {
			params[i] = cmdParams[i];
		}
	}

	return retNumParams;
}

#undef CHECK_COMMAND_ID



EXPORT(float) skirmishAiCallback_Unit_getExperience(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->GetUnitExperience(unitId);

	return GetCallBack(skirmishAIId)->GetUnitExperience(unitId);
}

EXPORT(int) skirmishAiCallback_Unit_getGroup(int skirmishAIId, int unitId) {
	return GetCallBack(skirmishAIId)->GetUnitGroup(unitId);
}

EXPORT(float) skirmishAiCallback_Unit_getHealth(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->GetUnitHealth(unitId);

	return GetCallBack(skirmishAIId)->GetUnitHealth(unitId);
}

EXPORT(float) skirmishAiCallback_Unit_getParalyzeDamage(int skirmishAIId, int unitId) {
	float result = -1.0f;

	const CUnit* unit = getUnit(unitId);
	if (unit == nullptr)
		return result;

	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return unit->paralyzeDamage;

	const int teamId = AI_TEAM_IDS[skirmishAIId];
	const int allyId = teamHandler.AllyTeam(teamId);

	if (teamHandler.Ally(unit->allyteam, allyId)) {
		result = unit->paralyzeDamage;
	} else if (unit->losStatus[allyId] & LOS_INLOS) {
		const UnitDef* unitDef = unit->unitDef;
		const UnitDef* decoyDef = unitDef->decoyDef;

		if (decoyDef == nullptr) {
			result = unit->paralyzeDamage;
		} else {
			result = unit->paralyzeDamage * (decoyDef->health / unitDef->health);
		}
	}

	return result;
}

EXPORT(float) skirmishAiCallback_Unit_getCaptureProgress(int skirmishAIId, int unitId) {
	const float fail = -1.0f;

	const CUnit* unit = getUnit(unitId);
	if (unit == nullptr)
		return fail;

	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return unit->captureProgress;

	const int teamId = AI_TEAM_IDS[skirmishAIId];
	const int allyId = teamHandler.AllyTeam(teamId);

	if (teamHandler.Ally(unit->allyteam, allyId) || unit->losStatus[allyId] & LOS_INLOS)
		return unit->captureProgress;

	return fail;
}

EXPORT(float) skirmishAiCallback_Unit_getBuildProgress(int skirmishAIId, int unitId) {
	const float fail = -1.0f;

	const CUnit* unit = getUnit(unitId);
	if (unit == nullptr)
		return fail;

	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return unit->buildProgress;

	const int teamId = AI_TEAM_IDS[skirmishAIId];
	const int allyId = teamHandler.AllyTeam(teamId);

	if (teamHandler.Ally(unit->allyteam, allyId) || unit->losStatus[allyId] & LOS_INLOS)
		return unit->buildProgress;

	return fail;
}

EXPORT(float) skirmishAiCallback_Unit_getSpeed(int skirmishAIId, int unitId) {
	return GetCallBack(skirmishAIId)->GetUnitSpeed(unitId);
}

EXPORT(float) skirmishAiCallback_Unit_getPower(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->GetUnitPower(unitId);

	return GetCallBack(skirmishAIId)->GetUnitPower(unitId);
}


EXPORT(void) skirmishAiCallback_Unit_getPos(int skirmishAIId, int unitId, float* return_posF3_out) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		GetCheatCallBack(skirmishAIId)->GetUnitPos(unitId).copyInto(return_posF3_out);
		return;
	}

	GetCallBack(skirmishAIId)->GetUnitPos(unitId).copyInto(return_posF3_out);
}

EXPORT(void) skirmishAiCallback_Unit_getVel(int skirmishAIId, int unitId, float* return_posF3_out) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		GetCheatCallBack(skirmishAIId)->GetUnitVelocity(unitId).copyInto(return_posF3_out);
		return;
	}

	GetCallBack(skirmishAIId)->GetUnitVelocity(unitId).copyInto(return_posF3_out);
}


//EXPORT(int) skirmishAiCallback_Unit_0MULTI1SIZE0ResourceInfo(int skirmishAIId, int unitId) {
//	return skirmishAiCallback_0MULTI1SIZE0Resource(skirmishAIId);
//}

EXPORT(float) skirmishAiCallback_Unit_getResourceUse(
	int skirmishAIId,
	int unitId,
	int resourceId
) {
	float res = -1.0f;
	bool fetchOk = false;

	UnitResourceInfo resourceInfo;

	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		fetchOk = GetCheatCallBack(skirmishAIId)->GetUnitResourceInfo(unitId, &resourceInfo);
	} else {
		fetchOk = GetCallBack(skirmishAIId)->GetUnitResourceInfo(unitId, &resourceInfo);
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

EXPORT(float) skirmishAiCallback_Unit_getResourceMake(
	int skirmishAIId,
	int unitId,
	int resourceId
) {
	float res = -1.0f;
	bool fetchOk = false;

	UnitResourceInfo resourceInfo;

	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		fetchOk = GetCheatCallBack(skirmishAIId)->GetUnitResourceInfo(unitId, &resourceInfo);
	} else {
		fetchOk = GetCallBack(skirmishAIId)->GetUnitResourceInfo(unitId, &resourceInfo);
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
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->IsUnitActivated(unitId);

	return GetCallBack(skirmishAIId)->IsUnitActivated(unitId);
}

EXPORT(bool) skirmishAiCallback_Unit_isBeingBuilt(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->UnitBeingBuilt(unitId);

	return GetCallBack(skirmishAIId)->UnitBeingBuilt(unitId);
}

EXPORT(bool) skirmishAiCallback_Unit_isCloaked(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->IsUnitCloaked(unitId);

	return GetCallBack(skirmishAIId)->IsUnitCloaked(unitId);
}

EXPORT(bool) skirmishAiCallback_Unit_isParalyzed(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->IsUnitParalyzed(unitId);

	return GetCallBack(skirmishAIId)->IsUnitParalyzed(unitId);
}

EXPORT(bool) skirmishAiCallback_Unit_isNeutral(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->IsUnitNeutral(unitId);

	return GetCallBack(skirmishAIId)->IsUnitNeutral(unitId);
}

EXPORT(int) skirmishAiCallback_Unit_getBuildingFacing(int skirmishAIId, int unitId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->GetBuildingFacing(unitId);

	return GetCallBack(skirmishAIId)->GetBuildingFacing(unitId);
}

EXPORT(int) skirmishAiCallback_Unit_getLastUserOrderFrame(int skirmishAIId, int unitId) {
	if (!isControlledByLocalPlayer(skirmishAIId))
		return -1;

	return unitHandler.GetUnit(unitId)->commandAI->lastUserCommand;
}

EXPORT(int) skirmishAiCallback_Unit_getWeapons(int skirmishAIId, int unitId) {
	const CUnit* unit = getUnit(unitId);

	if (unit != nullptr && (skirmishAiCallback_Cheats_isEnabled(skirmishAIId) || isUnitInLos(skirmishAIId, unit)))
		return unit->weapons.size();

	return 0;
}

EXPORT(int) skirmishAiCallback_Unit_getWeapon(int skirmishAIId, int unitId, int weaponMountId) {
	const CUnit* unit = getUnit(unitId);

	if (unit == nullptr || ((size_t)weaponMountId >= unit->weapons.size()))
		return -1;

	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId) || isUnitInLos(skirmishAIId, unit))
		return weaponMountId;

	return -1;
}

//########### END Unit

EXPORT(int) skirmishAiCallback_getEnemyUnits(int skirmishAIId, int* unitIds, int unitIdsMaxSize) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->GetEnemyUnits(unitIds, unitIdsMaxSize);

	return GetCallBack(skirmishAIId)->GetEnemyUnits(unitIds, unitIdsMaxSize);
}

EXPORT(int) skirmishAiCallback_getEnemyUnitsIn(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIdsMaxSize) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->GetEnemyUnits(unitIds, pos_posF3, radius, unitIdsMaxSize);

	return GetCallBack(skirmishAIId)->GetEnemyUnits(unitIds, pos_posF3, radius, unitIdsMaxSize);
}

EXPORT(int) skirmishAiCallback_getEnemyUnitsInRadarAndLos(int skirmishAIId, int* unitIds, int unitIdsMaxSize) {
	// with cheats on, act like global-LOS -> getEnemyUnitsIn() == getEnemyUnitsInRadarAndLos()
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->GetEnemyUnits(unitIds, unitIdsMaxSize);

	return GetCallBack(skirmishAIId)->GetEnemyUnitsInRadarAndLos(unitIds, unitIdsMaxSize);
}

EXPORT(int) skirmishAiCallback_getFriendlyUnits(int skirmishAIId, int* unitIds, int unitIdsMaxSize) {
	return GetCallBack(skirmishAIId)->GetFriendlyUnits(unitIds, unitIdsMaxSize);
}

EXPORT(int) skirmishAiCallback_getFriendlyUnitsIn(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIdsMaxSize) {
	return GetCallBack(skirmishAIId)->GetFriendlyUnits(unitIds, pos_posF3, radius, unitIdsMaxSize);
}

EXPORT(int) skirmishAiCallback_getNeutralUnits(int skirmishAIId, int* unitIds, int unitIdsMaxSize) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->GetNeutralUnits(unitIds, unitIdsMaxSize);

	return GetCallBack(skirmishAIId)->GetNeutralUnits(unitIds, unitIdsMaxSize);
}

EXPORT(int) skirmishAiCallback_getNeutralUnitsIn(int skirmishAIId, float* pos_posF3, float radius, int* unitIds, int unitIdsMaxSize) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId))
		return GetCheatCallBack(skirmishAIId)->GetNeutralUnits(unitIds, pos_posF3, radius, unitIdsMaxSize);

	return GetCallBack(skirmishAIId)->GetNeutralUnits(unitIds, pos_posF3, radius, unitIdsMaxSize);
}

EXPORT(int) skirmishAiCallback_getSelectedUnits(int skirmishAIId, int* unitIds, int unitIdsMaxSize) {
	return GetCallBack(skirmishAIId)->GetSelectedUnits(unitIds, unitIdsMaxSize);
}

EXPORT(int) skirmishAiCallback_getTeamUnits(int skirmishAIId, int* unitIds, int unitIdsMaxSize) {
	int a = 0;

	const int teamId = AI_TEAM_IDS[skirmishAIId];

	for (const CUnit* u: unitHandler.GetActiveUnits()) {
		if (u->team != teamId)
			continue;

		if (a >= unitIdsMaxSize)
			break;

		if (unitIds != nullptr)
			unitIds[a] = u->id;

		a++;
	}

	return a;
}


//########### BEGINN Team
EXPORT(bool) skirmishAiCallback_Team_hasAIController(int skirmishAIId, int teamId) {
	// return (AI_TEAM_IDS[skirmishAIId] == teamId);
	const auto pred = [&](const int aiTeamID) { return (teamId == aiTeamID); };
	const auto iter = std::find_if(AI_TEAM_IDS.begin(), AI_TEAM_IDS.end(), pred);
	return (iter != AI_TEAM_IDS.end());
}

EXPORT(int) skirmishAiCallback_getEnemyTeams(int skirmishAIId, int* teamIds, int teamIdsMaxSize) {
	int a = 0;

	const int teamId = AI_TEAM_IDS[skirmishAIId];

	if (teamIds != nullptr) {
		for (int i = 0; i < teamHandler.ActiveTeams() && a < teamIdsMaxSize; i++) {
			if (!teamHandler.AlliedTeams(i, teamId))
				teamIds[a++] = i;

		}
	} else {
		for (int i = 0; i < teamHandler.ActiveTeams(); i++) {
			if (!teamHandler.AlliedTeams(i, teamId))
				a++;

		}
	}

	return a;
}

EXPORT(int) skirmishAiCallback_getAllyTeams(int skirmishAIId, int* teamIds, int teamIdsMaxSize) {
	int a = 0;

	const int teamId = AI_TEAM_IDS[skirmishAIId];

	if (teamIds != nullptr) {
		for (int i = 0; i < teamHandler.ActiveTeams() && a < teamIdsMaxSize; i++) {
			if (teamHandler.AlliedTeams(i, teamId))
				teamIds[a++] = i;

		}
	} else {
		for (int i = 0; i < teamHandler.ActiveTeams(); i++) {
			if (teamHandler.AlliedTeams(i, teamId))
				a++;

		}
	}

	return a;
}


EXPORT(float) skirmishAiCallback_Team_getRulesParamFloat(int skirmishAIId, int teamId, const char* rulesParamName, float defaultValue) {
	const CTeam* team = getTeam(teamId);

	if (team == nullptr)
		return defaultValue;

	return getRulesParamFloatValueByName(team->modParams, teamModParamLosMask(skirmishAIId, team), rulesParamName, defaultValue);
}

EXPORT(const char*) skirmishAiCallback_Team_getRulesParamString(int skirmishAIId, int teamId, const char* rulesParamName, const char* defaultValue) {
	const CTeam* team = getTeam(teamId);

	if (team == nullptr)
		return defaultValue;

	return getRulesParamStringValueByName(team->modParams, teamModParamLosMask(skirmishAIId, team), rulesParamName, defaultValue);
}

//########### END Team


//########### BEGINN FeatureDef
EXPORT(int) skirmishAiCallback_getFeatureDefs(int skirmishAIId, int* featureDefIds, int featureDefIdsMaxSize) {
	const auto& featureDefs = featureDefHandler->GetFeatureDefsVec();
	const int featureDefIdsRealSize = featureDefs.size();

	int featureDefIdsSize = featureDefIdsRealSize;

	if (featureDefIds != nullptr) {
		featureDefIdsSize = std::min(featureDefIdsRealSize, featureDefIdsMaxSize);

		// skip dummy
		auto fdi = ++(featureDefs.cbegin());

		for (int f = 0; f < featureDefIdsSize; ++f, ++fdi) {
			featureDefIds[f] = fdi->id;
		}
	}

	return featureDefIdsSize;
}

EXPORT(const char*) skirmishAiCallback_FeatureDef_getName(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->name.c_str();
}

EXPORT(const char*) skirmishAiCallback_FeatureDef_getDescription(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->description.c_str();
}


EXPORT(float) skirmishAiCallback_FeatureDef_getContainedResource(int skirmishAIId, int featureDefId, int resourceId) {

	const FeatureDef* fd = getFeatureDefById(skirmishAIId, featureDefId);

	if (resourceId == resourceHandler->GetMetalId())
		return fd->metal;

	if (resourceId == resourceHandler->GetEnergyId())
		return fd->energy;

	return 0.0f;
}

EXPORT(float) skirmishAiCallback_FeatureDef_getMaxHealth(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->health;
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
	return getFeatureDefById(skirmishAIId, featureDefId)->modelName.c_str();
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
	return getFeatureDefById(skirmishAIId, featureDefId)->collidable;
}

EXPORT(bool) skirmishAiCallback_FeatureDef_isBurnable(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->burnable;
}

EXPORT(bool) skirmishAiCallback_FeatureDef_isFloating(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->floating;
}

EXPORT(bool) skirmishAiCallback_FeatureDef_isNoSelect(int skirmishAIId, int featureDefId) {
	return !getFeatureDefById(skirmishAIId, featureDefId)->selectable;
}

EXPORT(bool) skirmishAiCallback_FeatureDef_isGeoThermal(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->geoThermal;
}


EXPORT(int) skirmishAiCallback_FeatureDef_getXSize(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->xsize;
}

EXPORT(int) skirmishAiCallback_FeatureDef_getZSize(int skirmishAIId, int featureDefId) {
	return getFeatureDefById(skirmishAIId, featureDefId)->zsize;
}

EXPORT(int) skirmishAiCallback_FeatureDef_getCustomParams(
	int skirmishAIId,
	int featureDefId,
	const char** keys,
	const char** values
) {
	const auto& ps = getFeatureDefById(skirmishAIId, featureDefId)->customParams;
	const size_t paramsRealSize = ps.size();

	if ((keys != nullptr) && (values != nullptr))
		ShallowCopyKeyValPairs(ps, keys, values);

	return paramsRealSize;
}

//########### END FeatureDef


EXPORT(int) skirmishAiCallback_getFeatures(int skirmishAIId, int* featureIds, int featureIdsMaxSize) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		// cheating
		const auto& activeFeatureIDs = featureHandler.GetActiveFeatureIDs();
		const int featureIdsRealSize = activeFeatureIDs.size();

		int featureIdsSize = featureIdsRealSize;

		if (featureIds != nullptr) {
			featureIdsSize = std::min(featureIdsRealSize, featureIdsMaxSize);

			size_t f = 0;

			for (auto it = activeFeatureIDs.cbegin(); it != activeFeatureIDs.cend() && f < featureIdsSize; ++it) {
				const CFeature* feature = featureHandler.GetFeature(*it);

				assert(feature != nullptr);
				featureIds[f++] = feature->id;
			}
		}

		return featureIdsSize;
	}

	// if (featureIds == NULL), this will only return the number of features
	return GetCallBack(skirmishAIId)->GetFeatures(featureIds, featureIdsMaxSize);
}

EXPORT(int) skirmishAiCallback_getFeaturesIn(int skirmishAIId, float* pos_posF3, float radius, int* featureIds, int featureIdsMaxSize) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		// cheating
		QuadFieldQuery qfQuery;
		quadField.GetFeaturesExact(qfQuery, pos_posF3, radius);
		const int featureIdsRealSize = qfQuery.features->size();

		int featureIdsSize = featureIdsRealSize;

		if (featureIds != nullptr) {
			featureIdsSize = std::min(featureIdsRealSize, featureIdsMaxSize);

			size_t f = 0;

			for (const CFeature* feature: *qfQuery.features) {

				assert(feature != nullptr);
				featureIds[f++] = feature->id;
			}
		}

		return featureIdsSize;
	}

	// if (featureIds == NULL), this will only return the number of features
	return GetCallBack(skirmishAIId)->GetFeatures(featureIds, featureIdsMaxSize, pos_posF3, radius);
}


EXPORT(int) skirmishAiCallback_Feature_getDef(int skirmishAIId, int featureId) {
	const FeatureDef* def = nullptr;

	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		const CFeature* f = featureHandler.GetFeature(featureId);

		if (f != nullptr) {
			def = f->def;
		}
	} else {
		def = GetCallBack(skirmishAIId)->GetFeatureDef(featureId);
	}

	return (def != nullptr)? def->id: -1;
}

EXPORT(float) skirmishAiCallback_Feature_getHealth(int skirmishAIId, int featureId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		const CFeature* f = featureHandler.GetFeature(featureId);
		return (f != nullptr)? f->health: 0.0f;
	}

	return GetCallBack(skirmishAIId)->GetFeatureHealth(featureId);
}

EXPORT(float) skirmishAiCallback_Feature_getReclaimLeft(int skirmishAIId, int featureId) {
	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		const CFeature* f = featureHandler.GetFeature(featureId);
		return (f != nullptr)? f->reclaimLeft: 0.0f;
	}

	return GetCallBack(skirmishAIId)->GetFeatureReclaimLeft(featureId);
}

EXPORT(void) skirmishAiCallback_Feature_getPosition(int skirmishAIId, int featureId, float* return_posF3_out) {
	float3 pos;

	if (skirmishAiCallback_Cheats_isEnabled(skirmishAIId)) {
		const CFeature* f = featureHandler.GetFeature(featureId);

		if (f != nullptr) {
			pos = f->pos;
		}
	} else {
		pos = GetCallBack(skirmishAIId)->GetFeaturePos(featureId);
	}

	pos.copyInto(return_posF3_out);
}

EXPORT(float) skirmishAiCallback_Feature_getRulesParamFloat(int skirmishAIId, int featureId, const char* rulesParamName, float defaultValue) {
	const CFeature* feature = featureHandler.GetFeature(featureId);

	if (feature == nullptr)
		return defaultValue;

	return getRulesParamFloatValueByName(feature->modParams, featureModParamLosMask(skirmishAIId, feature), rulesParamName, defaultValue);
}

EXPORT(const char*) skirmishAiCallback_Feature_getRulesParamString(int skirmishAIId, int featureId, const char* rulesParamName, const char* defaultValue) {
	const CFeature* feature = featureHandler.GetFeature(featureId);

	if (feature == nullptr)
		return defaultValue;

	return getRulesParamStringValueByName(feature->modParams, featureModParamLosMask(skirmishAIId, feature), rulesParamName, defaultValue);
}


//########### BEGINN WeaponDef
EXPORT(int) skirmishAiCallback_getWeaponDefs(int skirmishAIId) {
	return (weaponDefHandler->NumWeaponDefs());
}

EXPORT(int) skirmishAiCallback_getWeaponDefByName(int skirmishAIId, const char* weaponDefName) {
	const WeaponDef* wd = GetCallBack(skirmishAIId)->GetWeapon(weaponDefName);

	if (wd != nullptr)
		return wd->id;

	return -1;
}


EXPORT(const char*) skirmishAiCallback_WeaponDef_getName(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->name.c_str();
}

EXPORT(const char*) skirmishAiCallback_WeaponDef_getType(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->type.c_str();
}

EXPORT(const char*) skirmishAiCallback_WeaponDef_getDescription(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->description.c_str();
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
	return getWeaponDefById(skirmishAIId, weaponDefId)->damages.paralyzeDamageTime;
}

EXPORT(float) skirmishAiCallback_WeaponDef_Damage_getImpulseFactor(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->damages.impulseFactor;
}

EXPORT(float) skirmishAiCallback_WeaponDef_Damage_getImpulseBoost(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->damages.impulseBoost;
}

EXPORT(float) skirmishAiCallback_WeaponDef_Damage_getCraterMult(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->damages.craterMult;
}

EXPORT(float) skirmishAiCallback_WeaponDef_Damage_getCraterBoost(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->damages.craterBoost;
}

EXPORT(int) skirmishAiCallback_WeaponDef_Damage_getTypes(int skirmishAIId, int weaponDefId, float* types, int typesMaxSize) {
	const WeaponDef* weaponDef = getWeaponDefById(skirmishAIId, weaponDefId);
	const int typesRealSize = weaponDef->damages.GetNumTypes();

	size_t typesSize = typesRealSize;

	if (types != nullptr) {
		typesSize = std::min(typesRealSize, typesMaxSize);

		for (size_t i = 0; i < typesSize; ++i) {
			types[i] = weaponDef->damages.Get(i);
		}
	}

	return typesSize;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getAreaOfEffect(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->damages.damageAreaOfEffect;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isNoSelfDamage(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->noSelfDamage;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getFireStarter(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->fireStarter;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getEdgeEffectiveness(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->damages.edgeEffectiveness;
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

EXPORT(float) skirmishAiCallback_WeaponDef_getUpTime(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->uptime;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getFlightTime(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->flighttime;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getCost(int skirmishAIId, int weaponDefId, int resourceId) {
	const WeaponDef* wd = getWeaponDefById(skirmishAIId, weaponDefId);

	if (resourceId == resourceHandler->GetMetalId())
		return wd->metalcost;

	if (resourceId == resourceHandler->GetEnergyId())
		return wd->energycost;

	return 0.0f;
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
	return false;
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


EXPORT(float) skirmishAiCallback_WeaponDef_getDuration(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->duration;
}


EXPORT(float) skirmishAiCallback_WeaponDef_getFalloffRate(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->falloffRate;
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
	return getWeaponDefById(skirmishAIId, weaponDefId)->projectilespeed * GAME_SPEED; // DEPRECATED
}

EXPORT(float) skirmishAiCallback_WeaponDef_getProjectileSpeed(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->projectilespeed;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getExplosionSpeed(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->damages.explosionSpeed;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getOnlyTargetCategory(int skirmishAIId, int weaponDefId) {
	return 0xFFFFFFFF;
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

	if (resourceId == resourceHandler->GetEnergyId())
		return wd->shieldEnergyUse;

	return 0.0f;
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

	if (resourceId == resourceHandler->GetEnergyId())
		return wd->shieldPowerRegenEnergy;

	return 0.0f;
}

EXPORT(float) skirmishAiCallback_WeaponDef_Shield_getStartingPower(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->shieldStartingPower;
}

EXPORT(int) skirmishAiCallback_WeaponDef_Shield_getRechargeDelay(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->shieldRechargeDelay;
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
	return getWeaponDefById(skirmishAIId, weaponDefId)->cylinderTargeting;
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
	return 0.0f; // DEPRECATED
}

EXPORT(float) skirmishAiCallback_WeaponDef_getDynDamageExp(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->damages.dynDamageExp;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getDynDamageMin(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->damages.dynDamageMin;
}

EXPORT(float) skirmishAiCallback_WeaponDef_getDynDamageRange(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->damages.dynDamageRange;
}

EXPORT(bool) skirmishAiCallback_WeaponDef_isDynDamageInverted(int skirmishAIId, int weaponDefId) {
	return getWeaponDefById(skirmishAIId, weaponDefId)->damages.dynDamageInverted;
}

EXPORT(int) skirmishAiCallback_WeaponDef_getCustomParams(
	int skirmishAIId,
	int weaponDefId,
	const char** keys,
	const char** values
) {
	const auto& ps = getWeaponDefById(skirmishAIId, weaponDefId)->customParams;
	const size_t paramsRealSize = ps.size();

	if ((keys != nullptr) && (values != nullptr))
		ShallowCopyKeyValPairs(ps, keys, values);

	return paramsRealSize;
}

//########### END WeaponDef


//########### BEGINN Weapon
EXPORT(int) skirmishAiCallback_Unit_Weapon_getDef(int skirmishAIId, int unitId, int weaponId) {
	const CUnit* unit = getUnit(unitId);

	if (unit == nullptr || ((size_t)weaponId >= unit->weapons.size()))
		return -1;

	return unit->weapons[weaponId]->weaponDef->id;
}

EXPORT(int) skirmishAiCallback_Unit_Weapon_getReloadFrame(int skirmishAIId, int unitId, int weaponId) {
	const CUnit* unit = getUnit(unitId);

	if (unit == nullptr || ((size_t)weaponId >= unit->weapons.size()))
		return -1;

	return unit->weapons[weaponId]->reloadStatus;
}

EXPORT(int) skirmishAiCallback_Unit_Weapon_getReloadTime(int skirmishAIId, int unitId, int weaponId) {
	const CUnit* unit = getUnit(unitId);

	if (unit == nullptr || ((size_t)weaponId >= unit->weapons.size()))
		return -1;

	const CWeapon* weapon = unit->weapons[weaponId];
	return weapon->reloadTime / unit->reloadSpeed;
}

EXPORT(float) skirmishAiCallback_Unit_Weapon_getRange(int skirmishAIId, int unitId, int weaponId) {
	const CUnit* unit = getUnit(unitId);

	if (unit == nullptr || ((size_t)weaponId >= unit->weapons.size()))
		return -1.0f;

	return unit->weapons[weaponId]->range;
}

EXPORT(bool) skirmishAiCallback_Unit_Weapon_isShieldEnabled(int skirmishAIId, int unitId, int weaponId) {
	const CUnit* unit = getUnit(unitId);

	if (unit == nullptr)
		return false;

	const CPlasmaRepulser* shield = nullptr;

	if ((size_t)weaponId >= unit->weapons.size()) {
		shield = static_cast<const CPlasmaRepulser*>(unit->shieldWeapon);
	} else {
		shield = dynamic_cast<const CPlasmaRepulser*>(unit->weapons[weaponId]);
	}

	if (shield == nullptr)
		return false;

	return shield->IsEnabled();
}

EXPORT(float) skirmishAiCallback_Unit_Weapon_getShieldPower(int skirmishAIId, int unitId, int weaponId) {
	const CUnit* unit = getUnit(unitId);

	if (unit == nullptr)
		return -1.0f;

	const CPlasmaRepulser* shield = nullptr;

	if ((size_t)weaponId >= unit->weapons.size()) {
		shield = static_cast<const CPlasmaRepulser*>(unit->shieldWeapon);
	} else {
		shield = dynamic_cast<const CPlasmaRepulser*>(unit->weapons[weaponId]);
	}

	if (shield == nullptr)
		return -1.0f;

	return shield->GetCurPower();
}

//########### END Weapon


EXPORT(bool) skirmishAiCallback_Debug_GraphDrawer_isEnabled(int skirmishAIId) {
	return GetCallBack(skirmishAIId)->IsDebugDrawerEnabled();
}

EXPORT(int) skirmishAiCallback_getGroups(int skirmishAIId, int* groupIds, int maxGroups) {
	const CGroupHandler& gh = uiGroupHandlers[ AI_TEAM_IDS[skirmishAIId] ];
	const std::vector<CGroup>& gs = gh.GetGroups();

	const int numGroups = gs.size();
	int numGroupsRet = numGroups;

	if (groupIds != nullptr) {
		numGroupsRet = std::min(numGroups, maxGroups);

		for (int i = 0; i < numGroupsRet; ++i) {
			groupIds[i] = gs[i].id;
		}
	}

	return numGroupsRet;
}

EXPORT(int) skirmishAiCallback_Group_getSupportedCommands(int skirmishAIId, int groupId) {
	return GetCallBack(skirmishAIId)->GetGroupCommands(groupId)->size();
}

EXPORT(int) skirmishAiCallback_Group_SupportedCommand_getId(int skirmishAIId, int groupId, int supportedCommandId) {
	return GetCallBack(skirmishAIId)->GetGroupCommands(groupId)->at(supportedCommandId)->id;
}

EXPORT(const char*) skirmishAiCallback_Group_SupportedCommand_getName(int skirmishAIId, int groupId, int supportedCommandId) {
	return GetCallBack(skirmishAIId)->GetGroupCommands(groupId)->at(supportedCommandId)->name.c_str();
}

EXPORT(const char*) skirmishAiCallback_Group_SupportedCommand_getToolTip(int skirmishAIId, int groupId, int supportedCommandId) {
	return GetCallBack(skirmishAIId)->GetGroupCommands(groupId)->at(supportedCommandId)->tooltip.c_str();
}

EXPORT(bool) skirmishAiCallback_Group_SupportedCommand_isShowUnique(int skirmishAIId, int groupId, int supportedCommandId) {
	return GetCallBack(skirmishAIId)->GetGroupCommands(groupId)->at(supportedCommandId)->showUnique;
}

EXPORT(bool) skirmishAiCallback_Group_SupportedCommand_isDisabled(int skirmishAIId, int groupId, int supportedCommandId) {
	return GetCallBack(skirmishAIId)->GetGroupCommands(groupId)->at(supportedCommandId)->disabled;
}

EXPORT(int) skirmishAiCallback_Group_SupportedCommand_getParams(
	int skirmishAIId,
	int groupId,
	int supportedCommandId,
	const char** params,
	int maxNumParams
) {
	const std::vector<std::string>& ps = GetCallBack(skirmishAIId)->GetGroupCommands(groupId)->at(supportedCommandId)->params;
	const int cqNumParams = ps.size();
	// NOTE: LegacyCpp AI interface wrapper expects real array size as return-value after 1st call with outArray=nullptr
	int retNumParams = cqNumParams;

	if (params != nullptr) {
		for (int i = 0, n = (retNumParams = std::min(cqNumParams, maxNumParams)); i < n; i++) {
			params[i] = ps.at(i).c_str();
		}
	}

	return retNumParams;
}

EXPORT(int) skirmishAiCallback_Group_OrderPreview_getId(int skirmishAIId, int groupId) {
	if (!isControlledByLocalPlayer(skirmishAIId))
		return -1;

	return (guihandler->GetOrderPreview()).GetID();
}

EXPORT(short) skirmishAiCallback_Group_OrderPreview_getOptions(int skirmishAIId, int groupId) {
	if (!isControlledByLocalPlayer(skirmishAIId))
		return 0;

	return (guihandler->GetOrderPreview()).GetOpts();
}

EXPORT(int) skirmishAiCallback_Group_OrderPreview_getTag(int skirmishAIId, int groupId) {
	if (!isControlledByLocalPlayer(skirmishAIId))
		return 0;

	return (guihandler->GetOrderPreview()).GetTag();
}

EXPORT(int) skirmishAiCallback_Group_OrderPreview_getTimeOut(int skirmishAIId, int groupId) {
	if (!isControlledByLocalPlayer(skirmishAIId))
		return -1;

	return (guihandler->GetOrderPreview()).GetTimeOut();
}

EXPORT(int) skirmishAiCallback_Group_OrderPreview_getParams(
	int skirmishAIId,
	int groupId,
	float* params,
	int maxNumParams
) {
	if (!isControlledByLocalPlayer(skirmishAIId))
		return 0;

	const Command& guiCommand = guihandler->GetOrderPreview();
	const int cqNumParams = guiCommand.GetNumParams();
	// NOTE: LegacyCpp AI interface wrapper expects real array size as return-value after 1st call with outArray=nullptr
	int retNumParams = cqNumParams;

	if (params != nullptr) {
		const float* cmdParams = guiCommand.GetParams();

		for (int i = 0, n = (retNumParams = std::min(cqNumParams, maxNumParams)); i < n; i++) {
			params[i] = cmdParams[i];
		}
	}

	return retNumParams;
}

EXPORT(bool) skirmishAiCallback_Group_isSelected(int skirmishAIId, int groupId) {
	if (!isControlledByLocalPlayer(skirmishAIId))
		return false;

	return (selectedUnitsHandler.IsGroupSelected(groupId));
}

//##############################################################################




static void skirmishAiCallback_init(SSkirmishAICallback* callback) {
	memset(callback, 0, sizeof(SSkirmishAICallback));

	// register function pointers to accessors (which wrap around the legacy callbacks)
	callback->Engine_handleCommand = &skirmishAiCallback_Engine_handleCommand;
	callback->Engine_executeCommand = &skirmishAiCallback_Engine_executeCommand;

	callback->Engine_Version_getMajor = &skirmishAiCallback_Engine_Version_getMajor;
	callback->Engine_Version_getMinor = &skirmishAiCallback_Engine_Version_getMinor;
	callback->Engine_Version_getPatchset = &skirmishAiCallback_Engine_Version_getPatchset;
	callback->Engine_Version_getCommits = &skirmishAiCallback_Engine_Version_getCommits;
	callback->Engine_Version_getHash = &skirmishAiCallback_Engine_Version_getHash;
	callback->Engine_Version_getBranch = &skirmishAiCallback_Engine_Version_getBranch;
	callback->Engine_Version_getAdditional = &skirmishAiCallback_Engine_Version_getAdditional;
	callback->Engine_Version_getBuildTime = &skirmishAiCallback_Engine_Version_getBuildTime;
	callback->Engine_Version_isRelease = &skirmishAiCallback_Engine_Version_isRelease;
	callback->Engine_Version_getNormal = &skirmishAiCallback_Engine_Version_getNormal;
	callback->Engine_Version_getSync = &skirmishAiCallback_Engine_Version_getSync;
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
	callback->Game_getTeamResourcePull = &skirmishAiCallback_Game_getTeamResourcePull;
	callback->Game_getTeamResourceShare = &skirmishAiCallback_Game_getTeamResourceShare;
	callback->Game_getTeamResourceSent = &skirmishAiCallback_Game_getTeamResourceSent;
	callback->Game_getTeamResourceReceived = &skirmishAiCallback_Game_getTeamResourceReceived;
	callback->Game_getTeamResourceExcess = &skirmishAiCallback_Game_getTeamResourceExcess;
	callback->Game_isAllied = &skirmishAiCallback_Game_isAllied;
	callback->Game_isDebugModeEnabled = &skirmishAiCallback_Game_isDebugModeEnabled;
	callback->Game_isPaused = &skirmishAiCallback_Game_isPaused;
	callback->Game_getSpeedFactor = &skirmishAiCallback_Game_getSpeedFactor;
	callback->Game_getSetupScript = &skirmishAiCallback_Game_getSetupScript;
	callback->Game_getCategoryFlag = &skirmishAiCallback_Game_getCategoryFlag;
	callback->Game_getCategoriesFlag = &skirmishAiCallback_Game_getCategoriesFlag;
	callback->Game_getCategoryName = &skirmishAiCallback_Game_getCategoryName;
	callback->Game_getRulesParamFloat = &skirmishAiCallback_Game_getRulesParamFloat;
	callback->Game_getRulesParamString = &skirmishAiCallback_Game_getRulesParamString;
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
	callback->Economy_getPull = &skirmishAiCallback_Economy_getPull;
	callback->Economy_getShare = &skirmishAiCallback_Economy_getShare;
	callback->Economy_getSent = &skirmishAiCallback_Economy_getSent;
	callback->Economy_getReceived = &skirmishAiCallback_Economy_getReceived;
	callback->Economy_getExcess = &skirmishAiCallback_Economy_getExcess;
	callback->File_getSize = &skirmishAiCallback_File_getSize;
	callback->File_getContent = &skirmishAiCallback_File_getContent;
	callback->getUnitDefs = &skirmishAiCallback_getUnitDefs;
	callback->getUnitDefByName = &skirmishAiCallback_getUnitDefByName;
	callback->UnitDef_getHeight = &skirmishAiCallback_UnitDef_getHeight;
	callback->UnitDef_getRadius = &skirmishAiCallback_UnitDef_getRadius;
	callback->UnitDef_getName = &skirmishAiCallback_UnitDef_getName;
	callback->UnitDef_getHumanName = &skirmishAiCallback_UnitDef_getHumanName;
	callback->UnitDef_getUpkeep = &skirmishAiCallback_UnitDef_getUpkeep;
	callback->UnitDef_getResourceMake = &skirmishAiCallback_UnitDef_getResourceMake;
	callback->UnitDef_getMakesResource = &skirmishAiCallback_UnitDef_getMakesResource;
	callback->UnitDef_getCost = &skirmishAiCallback_UnitDef_getCost;
	callback->UnitDef_getExtractsResource = &skirmishAiCallback_UnitDef_getExtractsResource;
	callback->UnitDef_getResourceExtractorRange = &skirmishAiCallback_UnitDef_getResourceExtractorRange;
	callback->UnitDef_getWindResourceGenerator = &skirmishAiCallback_UnitDef_getWindResourceGenerator;
	callback->UnitDef_getTidalResourceGenerator = &skirmishAiCallback_UnitDef_getTidalResourceGenerator;
	callback->UnitDef_getStorage = &skirmishAiCallback_UnitDef_getStorage;
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
	callback->UnitDef_getFrontToSpeed = &skirmishAiCallback_UnitDef_getFrontToSpeed;
	callback->UnitDef_getSpeedToFront = &skirmishAiCallback_UnitDef_getSpeedToFront;
	callback->UnitDef_getMyGravity = &skirmishAiCallback_UnitDef_getMyGravity;
	callback->UnitDef_getMaxBank = &skirmishAiCallback_UnitDef_getMaxBank;
	callback->UnitDef_getMaxPitch = &skirmishAiCallback_UnitDef_getMaxPitch;
	callback->UnitDef_getTurnRadius = &skirmishAiCallback_UnitDef_getTurnRadius;
	callback->UnitDef_getWantedHeight = &skirmishAiCallback_UnitDef_getWantedHeight;
	callback->UnitDef_getVerticalSpeed = &skirmishAiCallback_UnitDef_getVerticalSpeed;

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
	callback->UnitDef_canManualFire = &skirmishAiCallback_UnitDef_canManualFire;
	callback->UnitDef_isNeedGeo = &skirmishAiCallback_UnitDef_isNeedGeo;
	callback->UnitDef_isFeature = &skirmishAiCallback_UnitDef_isFeature;
	callback->UnitDef_isHideDamage = &skirmishAiCallback_UnitDef_isHideDamage;
	callback->UnitDef_isShowPlayerName = &skirmishAiCallback_UnitDef_isShowPlayerName;
	callback->UnitDef_isAbleToResurrect = &skirmishAiCallback_UnitDef_isAbleToResurrect;
	callback->UnitDef_isAbleToCapture = &skirmishAiCallback_UnitDef_isAbleToCapture;
	callback->UnitDef_getHighTrajectoryType = &skirmishAiCallback_UnitDef_getHighTrajectoryType;
	callback->UnitDef_getNoChaseCategory = &skirmishAiCallback_UnitDef_getNoChaseCategory;
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
	callback->UnitDef_getMaxThisUnit = &skirmishAiCallback_UnitDef_getMaxThisUnit;
	callback->UnitDef_getDecoyDef = &skirmishAiCallback_UnitDef_getDecoyDef;
	callback->UnitDef_isDontLand = &skirmishAiCallback_UnitDef_isDontLand;
	callback->UnitDef_getShieldDef = &skirmishAiCallback_UnitDef_getShieldDef;
	callback->UnitDef_getStockpileDef = &skirmishAiCallback_UnitDef_getStockpileDef;
	callback->UnitDef_getBuildOptions = &skirmishAiCallback_UnitDef_getBuildOptions;
	callback->UnitDef_getCustomParams = &skirmishAiCallback_UnitDef_getCustomParams;
	callback->UnitDef_isMoveDataAvailable = &skirmishAiCallback_UnitDef_isMoveDataAvailable;
	callback->UnitDef_MoveData_getXSize = &skirmishAiCallback_UnitDef_MoveData_getXSize;
	callback->UnitDef_MoveData_getZSize = &skirmishAiCallback_UnitDef_MoveData_getZSize;
	callback->UnitDef_MoveData_getDepth = &skirmishAiCallback_UnitDef_MoveData_getDepth;
	callback->UnitDef_MoveData_getMaxSlope = &skirmishAiCallback_UnitDef_MoveData_getMaxSlope;
	callback->UnitDef_MoveData_getSlopeMod = &skirmishAiCallback_UnitDef_MoveData_getSlopeMod;
	callback->UnitDef_MoveData_getDepthMod = &skirmishAiCallback_UnitDef_MoveData_getDepthMod;
	callback->UnitDef_MoveData_getPathType = &skirmishAiCallback_UnitDef_MoveData_getPathType;
	callback->UnitDef_MoveData_getCrushStrength = &skirmishAiCallback_UnitDef_MoveData_getCrushStrength;
	callback->UnitDef_MoveData_getSpeedModClass = &skirmishAiCallback_UnitDef_MoveData_getSpeedModClass;
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
	callback->Unit_getRulesParamFloat = &skirmishAiCallback_Unit_getRulesParamFloat;
	callback->Unit_getRulesParamString = &skirmishAiCallback_Unit_getRulesParamString;
	callback->Unit_getTeam = &skirmishAiCallback_Unit_getTeam;
	callback->Unit_getAllyTeam = &skirmishAiCallback_Unit_getAllyTeam;
	callback->Unit_getStockpile = &skirmishAiCallback_Unit_getStockpile;
	callback->Unit_getStockpileQueued = &skirmishAiCallback_Unit_getStockpileQueued;
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
	callback->Unit_getParalyzeDamage = &skirmishAiCallback_Unit_getParalyzeDamage;
	callback->Unit_getCaptureProgress = &skirmishAiCallback_Unit_getCaptureProgress;
	callback->Unit_getBuildProgress = &skirmishAiCallback_Unit_getBuildProgress;
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
	callback->Unit_getWeapons = &skirmishAiCallback_Unit_getWeapons;
	callback->Unit_getWeapon = &skirmishAiCallback_Unit_getWeapon;
	callback->Team_hasAIController = &skirmishAiCallback_Team_hasAIController;
	callback->getEnemyTeams = &skirmishAiCallback_getEnemyTeams;
	callback->getAllyTeams = &skirmishAiCallback_getAllyTeams;
	callback->Team_getRulesParamFloat = &skirmishAiCallback_Team_getRulesParamFloat;
	callback->Team_getRulesParamString = &skirmishAiCallback_Team_getRulesParamString;
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
	callback->Mod_getRadarMipLevel = &skirmishAiCallback_Mod_getRadarMipLevel;
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
	callback->Map_getAirLosMap = &skirmishAiCallback_Map_getAirLosMap;
	callback->Map_getRadarMap = &skirmishAiCallback_Map_getRadarMap;
	callback->Map_getSonarMap = &skirmishAiCallback_Map_getSonarMap;
	callback->Map_getSeismicMap = &skirmishAiCallback_Map_getSeismicMap;
	callback->Map_getJammerMap = &skirmishAiCallback_Map_getJammerMap;
	callback->Map_getSonarJammerMap = &skirmishAiCallback_Map_getSonarJammerMap;
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
	callback->Map_getWaterDamage = &skirmishAiCallback_Map_getWaterDamage;
	callback->Map_isDeformable = &skirmishAiCallback_Map_isDeformable;
	callback->Map_getHardness = &skirmishAiCallback_Map_getHardness;
	callback->Map_getHardnessModMap = &skirmishAiCallback_Map_getHardnessModMap;
	callback->Map_getSpeedModMap = &skirmishAiCallback_Map_getSpeedModMap;
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
	callback->FeatureDef_getXSize = &skirmishAiCallback_FeatureDef_getXSize;
	callback->FeatureDef_getZSize = &skirmishAiCallback_FeatureDef_getZSize;
	callback->FeatureDef_getCustomParams = &skirmishAiCallback_FeatureDef_getCustomParams;
	callback->getFeatures = &skirmishAiCallback_getFeatures;
	callback->getFeaturesIn = &skirmishAiCallback_getFeaturesIn;
	callback->Feature_getDef = &skirmishAiCallback_Feature_getDef;
	callback->Feature_getHealth = &skirmishAiCallback_Feature_getHealth;
	callback->Feature_getReclaimLeft = &skirmishAiCallback_Feature_getReclaimLeft;
	callback->Feature_getPosition = &skirmishAiCallback_Feature_getPosition;
	callback->Feature_getRulesParamFloat = &skirmishAiCallback_Feature_getRulesParamFloat;
	callback->Feature_getRulesParamString = &skirmishAiCallback_Feature_getRulesParamString;
	callback->getWeaponDefs = &skirmishAiCallback_getWeaponDefs;
	callback->getWeaponDefByName = &skirmishAiCallback_getWeaponDefByName;
	callback->WeaponDef_getName = &skirmishAiCallback_WeaponDef_getName;
	callback->WeaponDef_getType = &skirmishAiCallback_WeaponDef_getType;
	callback->WeaponDef_getDescription = &skirmishAiCallback_WeaponDef_getDescription;
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
	callback->WeaponDef_getUpTime = &skirmishAiCallback_WeaponDef_getUpTime;
	callback->WeaponDef_getFlightTime = &skirmishAiCallback_WeaponDef_getFlightTime;
	callback->WeaponDef_getCost = &skirmishAiCallback_WeaponDef_getCost;
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
	callback->WeaponDef_getDuration = &skirmishAiCallback_WeaponDef_getDuration;
	callback->WeaponDef_getFalloffRate = &skirmishAiCallback_WeaponDef_getFalloffRate;
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
	callback->Unit_Weapon_getDef = &skirmishAiCallback_Unit_Weapon_getDef;
	callback->Unit_Weapon_getReloadFrame = &skirmishAiCallback_Unit_Weapon_getReloadFrame;
	callback->Unit_Weapon_getReloadTime = &skirmishAiCallback_Unit_Weapon_getReloadTime;
	callback->Unit_Weapon_getRange = &skirmishAiCallback_Unit_Weapon_getRange;
	callback->Unit_Weapon_isShieldEnabled = &skirmishAiCallback_Unit_Weapon_isShieldEnabled;
	callback->Unit_Weapon_getShieldPower = &skirmishAiCallback_Unit_Weapon_getShieldPower;
	callback->Debug_GraphDrawer_isEnabled = &skirmishAiCallback_Debug_GraphDrawer_isEnabled;
}

SSkirmishAICallback* skirmishAiCallback_GetInstance(CSkirmishAIWrapper* ai)
{
	AI_LEGACY_CALLBACKS[ai->GetSkirmishAIID()].first  = CAICallback(ai->GetTeamId());
	AI_LEGACY_CALLBACKS[ai->GetSkirmishAIID()].second = CAICheats(ai); // NB: leaks <ai>

	AI_CHEAT_FLAGS[ai->GetSkirmishAIID()] = {false, false};
	AI_TEAM_IDS[ai->GetSkirmishAIID()] = ai->GetTeamId();

	skirmishAiCallback_init(&AI_CALLBACK_WRAPPERS[ai->GetSkirmishAIID()]);

	return &AI_CALLBACK_WRAPPERS[ai->GetSkirmishAIID()];
}

void skirmishAiCallback_Release(const CSkirmishAIWrapper* ai) {
	AI_LEGACY_CALLBACKS[ai->GetSkirmishAIID()].first  = {};
	AI_LEGACY_CALLBACKS[ai->GetSkirmishAIID()].second = {};

	AI_CHEAT_FLAGS[ai->GetSkirmishAIID()] = {false, false};
	AI_TEAM_IDS[ai->GetSkirmishAIID()] = -1;
}

void skirmishAiCallback_BlockOrders(const CSkirmishAIWrapper* ai)
{
	GetCallBack(ai->GetSkirmishAIID())->AllowOrders(false);
}

