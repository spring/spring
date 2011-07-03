/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Interface/AISCommands.h"

#include <limits.h>
#include <stdlib.h>

void initSUnitCommand(void* sUnitCommand) {

	struct SStopUnitCommand* scmd = (struct SStopUnitCommand*) sUnitCommand;

	scmd->unitId = -1;
	scmd->groupId = -1;
	scmd->options = 0;
	scmd->timeOut = INT_MAX;
}


#ifdef __cplusplus
#ifdef    BUILDING_AI
#include "LegacyCpp/Command.h"
#define CMD_MANUALFIRE CMD_DGUN
using namespace springLegacyAI;
#else  // BUILDING_AI
#include "Sim/Units/CommandAI/Command.h"
#endif // BUILDING_AI

void freeSUnitCommand(void* sCommandData, int sCommandId) {

	switch (sCommandId) {
		case COMMAND_UNIT_LOAD_UNITS:
		{
			struct SLoadUnitsUnitCommand* cmd = (struct SLoadUnitsUnitCommand*) sCommandData;
			FREE(cmd->toLoadUnitIds);
			break;
		}
		case COMMAND_UNIT_MOVE:
		{
			struct SMoveUnitCommand* cmd = (struct SMoveUnitCommand*) sCommandData;
			FREE(cmd->toPos_posF3);
			break;
		}
		case COMMAND_UNIT_PATROL:
		{
			struct SPatrolUnitCommand* cmd = (struct SPatrolUnitCommand*) sCommandData;
			FREE(cmd->toPos_posF3);
			break;
		}
		case COMMAND_UNIT_FIGHT:
		{
			struct SFightUnitCommand* cmd = (struct SFightUnitCommand*) sCommandData;
			FREE(cmd->toPos_posF3);
			break;
		}
		case COMMAND_UNIT_ATTACK_AREA:
		{
			struct SAttackAreaUnitCommand* cmd = (struct SAttackAreaUnitCommand*) sCommandData;
			FREE(cmd->toAttackPos_posF3);
			break;
		}
		case COMMAND_UNIT_SET_BASE:
		{
			struct SSetBaseUnitCommand* cmd = (struct SSetBaseUnitCommand*) sCommandData;
			FREE(cmd->basePos_posF3);
			break;
			}
		case COMMAND_UNIT_LOAD_UNITS_AREA:
		{
			struct SLoadUnitsAreaUnitCommand* cmd = (struct SLoadUnitsAreaUnitCommand*) sCommandData;
			FREE(cmd->pos_posF3);
			break;
			}
		case COMMAND_UNIT_UNLOAD_UNIT:
		{
			struct SUnloadUnitCommand* cmd = (struct SUnloadUnitCommand*) sCommandData;
			FREE(cmd->toPos_posF3);
			break;
		}
		case COMMAND_UNIT_UNLOAD_UNITS_AREA:
		{
			struct SUnloadUnitsAreaUnitCommand* cmd = (struct SUnloadUnitsAreaUnitCommand*) sCommandData;
			FREE(cmd->toPos_posF3);
			break;
		}
		case COMMAND_UNIT_RECLAIM_AREA:
		{
			struct SReclaimAreaUnitCommand* cmd = (struct SReclaimAreaUnitCommand*) sCommandData;
			FREE(cmd->pos_posF3);
			break;
		}
		case COMMAND_UNIT_D_GUN_POS:
		{
			struct SDGunPosUnitCommand* cmd = (struct SDGunPosUnitCommand*) sCommandData;
			FREE(cmd->pos_posF3);
			break;
		}
		case COMMAND_UNIT_RESTORE_AREA:
		{
			struct SRestoreAreaUnitCommand* cmd = (struct SRestoreAreaUnitCommand*) sCommandData;
			FREE(cmd->pos_posF3);
			break;
		}
		case COMMAND_UNIT_RESURRECT_AREA:
		{
			struct SResurrectAreaUnitCommand* cmd = (struct SResurrectAreaUnitCommand*) sCommandData;
			FREE(cmd->pos_posF3);
			break;
		}
		case COMMAND_UNIT_CAPTURE_AREA:
		{
			struct SCaptureAreaUnitCommand* cmd = (struct SCaptureAreaUnitCommand*) sCommandData;
			FREE(cmd->pos_posF3);
			break;
		}
		case COMMAND_UNIT_BUILD:
		{
			struct SBuildUnitCommand* cmd = (struct SBuildUnitCommand*) sCommandData;
			FREE(cmd->buildPos_posF3);
			break;
		}
		case COMMAND_UNIT_CUSTOM:
		{
			struct SCustomUnitCommand* cmd = (struct SCustomUnitCommand*) sCommandData;
			FREE(cmd->params);
			break;
		}
	}

	FREE(sCommandData);
}

static float* allocFloatArr3(const std::vector<float>& from, const size_t firstValIndex = 0) {

	float* to = (float*) calloc(3, sizeof(float));

	to[0] = from[firstValIndex + 0];
	to[1] = from[firstValIndex + 1];
	to[2] = from[firstValIndex + 2];

	return to;
}

void* mallocSUnitCommand(int unitId, int groupId, const Command* c, int* sCommandId, int maxUnits) {

	int aiCmdId = extractAICommandTopic(c, maxUnits);
	void* sCommandData;

	switch (aiCmdId) {
		case COMMAND_UNIT_STOP:
		{
			SStopUnitCommand* cmd = (SStopUnitCommand*) malloc(sizeof (SStopUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_WAIT:
		{
			SWaitUnitCommand* cmd = (SWaitUnitCommand*) malloc(sizeof (SWaitUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_WAIT_TIME:
		{
			STimeWaitUnitCommand* cmd = (STimeWaitUnitCommand*) malloc(sizeof (STimeWaitUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->time = c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_WAIT_DEATH:
		{
			SDeathWaitUnitCommand* cmd = (SDeathWaitUnitCommand*) malloc(sizeof (SDeathWaitUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toDieUnitId = (int) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_WAIT_SQUAD:
		{
			SSquadWaitUnitCommand* cmd = (SSquadWaitUnitCommand*) malloc(sizeof (SSquadWaitUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->numUnits = (int) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_WAIT_GATHER:
		{
			SGatherWaitUnitCommand* cmd = (SGatherWaitUnitCommand*) malloc(sizeof (SGatherWaitUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_MOVE:
		{
			SMoveUnitCommand* cmd = (SMoveUnitCommand*) malloc(sizeof (SMoveUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toPos_posF3 = allocFloatArr3(c->params, 0);

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_PATROL:
		{
			SPatrolUnitCommand* cmd = (SPatrolUnitCommand*) malloc(sizeof (SPatrolUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toPos_posF3 = allocFloatArr3(c->params, 0);

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_FIGHT:
		{
			SFightUnitCommand* cmd = (SFightUnitCommand*) malloc(sizeof (SFightUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toPos_posF3 = allocFloatArr3(c->params, 0);

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_ATTACK:
		{
			SAttackUnitCommand* cmd = (SAttackUnitCommand*) malloc(sizeof (SAttackUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toAttackUnitId = (int) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_ATTACK_AREA:
		{
			float radius = 0.0f;
			if (c->params.size() >= 4) radius = c->params[3];
			SAttackAreaUnitCommand* cmd = (SAttackAreaUnitCommand*) malloc(sizeof (SAttackAreaUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toAttackPos_posF3 = allocFloatArr3(c->params, 0);
			cmd->radius = radius;

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_GUARD:
		{
			SGuardUnitCommand* cmd = (SGuardUnitCommand*) malloc(sizeof (SGuardUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toGuardUnitId = (int) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_AI_SELECT:
		{
			SAiSelectUnitCommand* cmd = (SAiSelectUnitCommand*) malloc(sizeof (SAiSelectUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_GROUP_ADD:
		{
			SGroupAddUnitCommand* cmd = (SGroupAddUnitCommand*) malloc(sizeof (SGroupAddUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toGroupId = (int) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_GROUP_CLEAR:
		{
			SGroupClearUnitCommand* cmd = (SGroupClearUnitCommand*) malloc(sizeof (SGroupClearUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_REPAIR:
		{
			SRepairUnitCommand* cmd = (SRepairUnitCommand*) malloc(sizeof (SRepairUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toRepairUnitId = (int) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_SET_FIRE_STATE:
		{
			SSetFireStateUnitCommand* cmd = (SSetFireStateUnitCommand*) malloc(sizeof (SSetFireStateUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->fireState = (int) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_SET_MOVE_STATE:
		{
			SSetMoveStateUnitCommand* cmd = (SSetMoveStateUnitCommand*) malloc(sizeof (SSetMoveStateUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->moveState = (int) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_SET_BASE:
		{
			SSetBaseUnitCommand* cmd = (SSetBaseUnitCommand*) malloc(sizeof (SSetBaseUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->basePos_posF3 = allocFloatArr3(c->params, 0);

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_SELF_DESTROY:
		{
			SSelfDestroyUnitCommand* cmd = (SSelfDestroyUnitCommand*) malloc(sizeof (SSelfDestroyUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_SET_WANTED_MAX_SPEED:
		{
			SSetWantedMaxSpeedUnitCommand* cmd = (SSetWantedMaxSpeedUnitCommand*) malloc(sizeof (SSetWantedMaxSpeedUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->wantedMaxSpeed = c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_LOAD_UNITS:
		{
			//int numToLoadUnits = 1;
			const int toLoadUnitIds_size = c->params.size();

			SLoadUnitsUnitCommand* cmd = (SLoadUnitsUnitCommand*) malloc(sizeof (SLoadUnitsUnitCommand));
			cmd->unitId  = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;

			cmd->toLoadUnitIds_size = toLoadUnitIds_size;
			cmd->toLoadUnitIds      = (int*) calloc(toLoadUnitIds_size, sizeof(int));
			int u;
			for (u=0; u < toLoadUnitIds_size; ++u) {
				cmd->toLoadUnitIds[u] = (int) c->params.at(u);
			}

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_LOAD_UNITS_AREA:
		{
			float radius = 0.0f;
			if (c->params.size() >= 4) radius = c->params[3];
			SLoadUnitsAreaUnitCommand* cmd = (SLoadUnitsAreaUnitCommand*) malloc(sizeof (SLoadUnitsAreaUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->pos_posF3 = allocFloatArr3(c->params, 0);
			cmd->radius = radius;

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_LOAD_ONTO:
		{
			SLoadOntoUnitCommand* cmd = (SLoadOntoUnitCommand*) malloc(sizeof (SLoadOntoUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->transporterUnitId = (int) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_UNLOAD_UNIT:
		{
			SUnloadUnitCommand* cmd = (SUnloadUnitCommand*) malloc(sizeof (SUnloadUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toPos_posF3 = allocFloatArr3(c->params, 0);
			cmd->toUnloadUnitId = (int) c->params[3];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_UNLOAD_UNITS_AREA:
		{
			float radius = 0.0f;
			if (c->params.size() >= 4) radius = c->params[3];
			SUnloadUnitsAreaUnitCommand* cmd = (SUnloadUnitsAreaUnitCommand*) malloc(sizeof (SUnloadUnitsAreaUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toPos_posF3 = allocFloatArr3(c->params, 0);
			cmd->radius = radius;

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_SET_ON_OFF:
		{
			SSetOnOffUnitCommand* cmd = (SSetOnOffUnitCommand*) malloc(sizeof (SSetOnOffUnitCommand));
			cmd->unitId  = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->on      = (bool) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_RECLAIM_UNIT:
		{
			SReclaimUnitUnitCommand* cmd = (SReclaimUnitUnitCommand*) malloc(sizeof (SReclaimUnitUnitCommand));
			cmd->unitId          = unitId;
			cmd->groupId         = groupId;
			cmd->options         = c->options;
			cmd->timeOut         = c->timeOut;
			cmd->toReclaimUnitId = (int) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_RECLAIM_FEATURE:
		{
			SReclaimFeatureUnitCommand* cmd = (SReclaimFeatureUnitCommand*) malloc(sizeof (SReclaimFeatureUnitCommand));
			cmd->unitId             = unitId;
			cmd->groupId            = groupId;
			cmd->options            = c->options;
			cmd->timeOut            = c->timeOut;
			cmd->toReclaimFeatureId = ((int) c->params[0]) - maxUnits;

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_RECLAIM_AREA:
		{
			float radius = 0.0f;
			if (c->params.size() >= 4) radius = c->params[3];
			SReclaimAreaUnitCommand* cmd = (SReclaimAreaUnitCommand*) malloc(sizeof (SReclaimAreaUnitCommand));
			cmd->unitId    = unitId;
			cmd->groupId   = groupId;
			cmd->options   = c->options;
			cmd->timeOut   = c->timeOut;
			cmd->pos_posF3 = allocFloatArr3(c->params, 0);
			cmd->radius    = radius;

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_CLOAK:
		{
			SCloakUnitCommand* cmd = (SCloakUnitCommand*) malloc(sizeof (SCloakUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->cloak = (bool) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_STOCKPILE:
		{
			SStockpileUnitCommand* cmd = (SStockpileUnitCommand*) malloc(sizeof (SStockpileUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_D_GUN:
		{
			SDGunUnitCommand* cmd = (SDGunUnitCommand*) malloc(sizeof (SDGunUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toAttackUnitId = (int) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_D_GUN_POS:
		{
			SDGunPosUnitCommand* cmd = (SDGunPosUnitCommand*) malloc(sizeof (SDGunPosUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->pos_posF3 = allocFloatArr3(c->params, 0);

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_RESTORE_AREA:
		{
			float radius = 0.0f;
			if (c->params.size() >= 4) radius = c->params[3];
			SRestoreAreaUnitCommand* cmd = (SRestoreAreaUnitCommand*) malloc(sizeof (SRestoreAreaUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->pos_posF3 = allocFloatArr3(c->params, 0);
			cmd->radius = radius;

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_SET_REPEAT:
		{
			SSetRepeatUnitCommand* cmd = (SSetRepeatUnitCommand*) malloc(sizeof (SSetRepeatUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->repeat = (bool) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_SET_TRAJECTORY:
		{
			SSetTrajectoryUnitCommand* cmd = (SSetTrajectoryUnitCommand*) malloc(sizeof (SSetTrajectoryUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->trajectory = (int) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_RESURRECT:
		{
			SResurrectUnitCommand* cmd = (SResurrectUnitCommand*) malloc(sizeof (SResurrectUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toResurrectFeatureId = (int) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_RESURRECT_AREA:
		{
			float radius = 0.0f;
			if (c->params.size() >= 4) radius = c->params[3];
			SResurrectAreaUnitCommand* cmd = (SResurrectAreaUnitCommand*) malloc(sizeof (SResurrectAreaUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->pos_posF3 = allocFloatArr3(c->params, 0);
			cmd->radius = radius;

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_CAPTURE:
		{
			SCaptureUnitCommand* cmd = (SCaptureUnitCommand*) malloc(sizeof (SCaptureUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toCaptureUnitId = (int) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_CAPTURE_AREA:
		{
			float radius = 0.0f;
			if (c->params.size() >= 4) radius = c->params[3];
			SCaptureAreaUnitCommand* cmd = (SCaptureAreaUnitCommand*) malloc(sizeof (SCaptureAreaUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->pos_posF3 = allocFloatArr3(c->params, 0);
			cmd->radius = radius;

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_SET_AUTO_REPAIR_LEVEL:
		{
			SSetAutoRepairLevelUnitCommand* cmd = (SSetAutoRepairLevelUnitCommand*) malloc(sizeof (SSetAutoRepairLevelUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->autoRepairLevel = (int) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_SET_IDLE_MODE:
		{
			SSetIdleModeUnitCommand* cmd = (SSetIdleModeUnitCommand*) malloc(sizeof (SSetIdleModeUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->idleMode = (int) c->params[0];

			sCommandData = cmd;
			break;
		}
		case COMMAND_UNIT_BUILD:
		{
			int toBuildUnitDefId = -c->GetID();
			int facing = UNIT_COMMAND_BUILD_NO_FACING;
			if (c->params.size() >= 4) facing = c->params[3];
			SBuildUnitCommand* cmd = (SBuildUnitCommand*) malloc(sizeof (SBuildUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toBuildUnitDefId = toBuildUnitDefId;
			if (c->params.size() >= 3) {
				cmd->buildPos_posF3 = allocFloatArr3(c->params, 0);
			} else {
				cmd->buildPos_posF3 = NULL;
			}
			cmd->facing = facing;

			sCommandData = cmd;
			break;
		}
		default:
		case COMMAND_UNIT_CUSTOM:
		{
			const int& cmdId         = c->GetID();
			const size_t params_size = c->params.size();
			SCustomUnitCommand* cmd  = (SCustomUnitCommand*) malloc(sizeof (SCustomUnitCommand));
			cmd->unitId      = unitId;
			cmd->groupId     = groupId;
			cmd->options     = c->options;
			cmd->timeOut     = c->timeOut;
			cmd->cmdId       = cmdId;
			cmd->params_size = params_size;
			cmd->params      = (float*) calloc(params_size, sizeof(float));
			int p;
			for (p=0; p < params_size; ++p) {
				cmd->params[p] = c->params.at(p);
			}

			sCommandData = cmd;
			break;
		}

	}

	*sCommandId = aiCmdId;

	return sCommandData;
}


int toInternalUnitCommandTopic(int aiCmdTopic, const void* sUnitCommandData) {

	int internalUnitCommandTopic;

	switch (aiCmdTopic) {
		case COMMAND_UNIT_BUILD:
		{
			SBuildUnitCommand* cmd = (SBuildUnitCommand*) sUnitCommandData;
			internalUnitCommandTopic = -cmd->toBuildUnitDefId;
			break;
		}
		case COMMAND_UNIT_STOP:
		{
			internalUnitCommandTopic = CMD_STOP;
			break;
		}
		case COMMAND_UNIT_WAIT:
		{
			internalUnitCommandTopic = CMD_WAIT;
			break;
		}
		case COMMAND_UNIT_WAIT_TIME:
		{
			internalUnitCommandTopic = CMD_TIMEWAIT;
			break;
		}
		case COMMAND_UNIT_WAIT_DEATH:
		{
			internalUnitCommandTopic = CMD_DEATHWAIT;
			break;
		}
		case COMMAND_UNIT_WAIT_SQUAD:
		{
			internalUnitCommandTopic = CMD_SQUADWAIT;
			break;
		}
		case COMMAND_UNIT_WAIT_GATHER:
		{
			internalUnitCommandTopic = CMD_GATHERWAIT;
			break;
		}
		case COMMAND_UNIT_MOVE:
		{
			internalUnitCommandTopic = CMD_MOVE;
			break;
		}
		case COMMAND_UNIT_PATROL:
		{
			internalUnitCommandTopic = CMD_PATROL;
			break;
		}
		case COMMAND_UNIT_FIGHT:
		{
			internalUnitCommandTopic = CMD_FIGHT;
			break;
		}
		case COMMAND_UNIT_ATTACK:
		{
			internalUnitCommandTopic = CMD_ATTACK;
			break;
		}
		case COMMAND_UNIT_ATTACK_AREA:
		{
			internalUnitCommandTopic = CMD_ATTACK;
			break;
		}
		case COMMAND_UNIT_GUARD:
		{
			internalUnitCommandTopic = CMD_GUARD;
			break;
		}
		case COMMAND_UNIT_AI_SELECT:
		{
			internalUnitCommandTopic = CMD_AISELECT;
			break;
		}
		case COMMAND_UNIT_GROUP_ADD:
		{
			internalUnitCommandTopic = CMD_GROUPADD;
			break;
		}
		case COMMAND_UNIT_GROUP_CLEAR:
		{
			internalUnitCommandTopic = CMD_GROUPCLEAR;
			break;
		}
		case COMMAND_UNIT_REPAIR:
		{
			internalUnitCommandTopic = CMD_REPAIR;
			break;
		}
		case COMMAND_UNIT_SET_FIRE_STATE:
		{
			internalUnitCommandTopic = CMD_FIRE_STATE;
			break;
		}
		case COMMAND_UNIT_SET_MOVE_STATE:
		{
			internalUnitCommandTopic = CMD_MOVE_STATE;
			break;
		}
		case COMMAND_UNIT_SET_BASE:
		{
			internalUnitCommandTopic = CMD_SETBASE;
			break;
		}
		case COMMAND_UNIT_SELF_DESTROY:
		{
			internalUnitCommandTopic = CMD_SELFD;
			break;
		}
		case COMMAND_UNIT_SET_WANTED_MAX_SPEED:
		{
			internalUnitCommandTopic = CMD_SET_WANTED_MAX_SPEED;
			break;
		}
		case COMMAND_UNIT_LOAD_UNITS:
		{
			internalUnitCommandTopic = CMD_LOAD_UNITS;
			break;
		}
		case COMMAND_UNIT_LOAD_UNITS_AREA:
		{
			internalUnitCommandTopic = CMD_LOAD_UNITS;
			break;
		}
		case COMMAND_UNIT_LOAD_ONTO:
		{
			internalUnitCommandTopic = CMD_LOAD_ONTO;
			break;
		}
		case COMMAND_UNIT_UNLOAD_UNITS_AREA:
		{
			internalUnitCommandTopic = CMD_UNLOAD_UNITS;
			break;
		}
		case COMMAND_UNIT_UNLOAD_UNIT:
		{
			internalUnitCommandTopic = CMD_UNLOAD_UNIT;
			break;
		}
		case COMMAND_UNIT_SET_ON_OFF:
		{
			internalUnitCommandTopic = CMD_ONOFF;
			break;
		}
		case COMMAND_UNIT_RECLAIM_UNIT:
		{
			internalUnitCommandTopic = CMD_RECLAIM;
			break;
		}
		case COMMAND_UNIT_RECLAIM_FEATURE:
		{
			internalUnitCommandTopic = CMD_RECLAIM;
			break;
		}
		case COMMAND_UNIT_RECLAIM_AREA:
		{
			internalUnitCommandTopic = CMD_RECLAIM;
			break;
		}
		case COMMAND_UNIT_CLOAK:
		{
			internalUnitCommandTopic = CMD_CLOAK;
			break;
		}
		case COMMAND_UNIT_STOCKPILE:
		{
			internalUnitCommandTopic = CMD_STOCKPILE;
			break;
		}

		case COMMAND_UNIT_D_GUN:
		{
			// FIXME
			internalUnitCommandTopic = CMD_MANUALFIRE;
			break;
		}
		case COMMAND_UNIT_D_GUN_POS:
		{
			// FIXME
			internalUnitCommandTopic = CMD_MANUALFIRE;
			break;
		}

		case COMMAND_UNIT_RESTORE_AREA:
		{
			internalUnitCommandTopic = CMD_RESTORE;
			break;
		}
		case COMMAND_UNIT_SET_REPEAT:
		{
			internalUnitCommandTopic = CMD_REPEAT;
			break;
		}
		case COMMAND_UNIT_SET_TRAJECTORY:
		{
			internalUnitCommandTopic = CMD_TRAJECTORY;
			break;
		}
		case COMMAND_UNIT_RESURRECT:
		{
			internalUnitCommandTopic = CMD_RESURRECT;
			break;
		}
		case COMMAND_UNIT_RESURRECT_AREA:
		{
			internalUnitCommandTopic = CMD_RESURRECT;
			break;
		}
		case COMMAND_UNIT_CAPTURE:
		{
			internalUnitCommandTopic = CMD_CAPTURE;
			break;
		}
		case COMMAND_UNIT_CAPTURE_AREA:
		{
			internalUnitCommandTopic = CMD_CAPTURE;
			break;
		}
		case COMMAND_UNIT_SET_AUTO_REPAIR_LEVEL:
		{
			internalUnitCommandTopic = CMD_AUTOREPAIRLEVEL;
			break;
		}
		case COMMAND_UNIT_SET_IDLE_MODE:
		{
			internalUnitCommandTopic = CMD_IDLEMODE;
			break;
		}
		case COMMAND_UNIT_CUSTOM:
		{
			SCustomUnitCommand* cmd = (SCustomUnitCommand*) sUnitCommandData;
			internalUnitCommandTopic = cmd->cmdId;
			break;
		}
		default:
		{
			internalUnitCommandTopic = -1;
		}
	}

	return internalUnitCommandTopic;
}


int extractAICommandTopic(const Command* engineCmd, int maxUnits) {

	const int& internalUnitCmdTopic = engineCmd->GetID();
	int aiCommandTopic;

	switch (internalUnitCmdTopic) {
		case CMD_STOP:
		{
			aiCommandTopic = COMMAND_UNIT_STOP;
			break;
		}
		case CMD_WAIT:
		{
			aiCommandTopic = COMMAND_UNIT_WAIT;
			break;
		}
		case CMD_TIMEWAIT:
		{
			aiCommandTopic = COMMAND_UNIT_WAIT_TIME;
			break;
		}
		case CMD_DEATHWAIT:
		{
			aiCommandTopic = COMMAND_UNIT_WAIT_DEATH;
			break;
		}
		case CMD_SQUADWAIT:
		{
			aiCommandTopic = COMMAND_UNIT_WAIT_SQUAD;
			break;
		}
		case CMD_GATHERWAIT:
		{
			aiCommandTopic = COMMAND_UNIT_WAIT_GATHER;
			break;
		}
		case CMD_MOVE:
		{
			aiCommandTopic = COMMAND_UNIT_MOVE;
			break;
		}
		case CMD_PATROL:
		{
			aiCommandTopic = COMMAND_UNIT_PATROL;
			break;
		}
		case CMD_FIGHT:
		{
			aiCommandTopic = COMMAND_UNIT_FIGHT;
			break;
		}
		case CMD_ATTACK:
		{
			if (engineCmd->params.size() < 3) {
				aiCommandTopic = COMMAND_UNIT_ATTACK;
			} else {
				aiCommandTopic = COMMAND_UNIT_ATTACK_AREA;
			}
			break;
		}
		case CMD_GUARD:
		{
			aiCommandTopic = COMMAND_UNIT_GUARD;
			break;
		}
		case CMD_AISELECT:
		{
			aiCommandTopic = COMMAND_UNIT_AI_SELECT;
			break;
		}
		case CMD_GROUPADD:
		{
			aiCommandTopic = COMMAND_UNIT_GROUP_ADD;
			break;
		}
		case CMD_GROUPCLEAR:
		{
			aiCommandTopic = COMMAND_UNIT_GROUP_CLEAR;
			break;
		}
		case CMD_REPAIR:
		{
			aiCommandTopic = COMMAND_UNIT_REPAIR;
			break;
		}
		case CMD_FIRE_STATE:
		{
			aiCommandTopic = COMMAND_UNIT_SET_FIRE_STATE;
			break;
		}
		case CMD_MOVE_STATE:
		{
			aiCommandTopic = COMMAND_UNIT_SET_MOVE_STATE;
			break;
		}
		case CMD_SETBASE:
		{
			aiCommandTopic = COMMAND_UNIT_SET_BASE;
			break;
		}
		case CMD_SELFD:
		{
			aiCommandTopic = COMMAND_UNIT_SELF_DESTROY;
			break;
		}
		case CMD_SET_WANTED_MAX_SPEED:
		{
			aiCommandTopic = COMMAND_UNIT_SET_WANTED_MAX_SPEED;
			break;
		}
		case CMD_LOAD_UNITS:
		{
			if (engineCmd->params.size() < 3) {
				aiCommandTopic = COMMAND_UNIT_LOAD_UNITS;
			} else {
				aiCommandTopic = COMMAND_UNIT_LOAD_UNITS_AREA;
			}
			break;
		}
		case CMD_LOAD_ONTO:
		{
			aiCommandTopic = COMMAND_UNIT_LOAD_ONTO;
			break;
		}
		case CMD_UNLOAD_UNITS:
		{
			aiCommandTopic = COMMAND_UNIT_UNLOAD_UNITS_AREA;
			break;
		}
		case CMD_UNLOAD_UNIT:
		{
			aiCommandTopic = COMMAND_UNIT_UNLOAD_UNIT;
			break;
		}
		case CMD_ONOFF:
		{
			aiCommandTopic = COMMAND_UNIT_SET_ON_OFF;
			break;
		}
		case CMD_RECLAIM:
		{
			if (engineCmd->params.size() < 3 || engineCmd->params.size() == 5) {
				if (engineCmd->params[0] < maxUnits) {
					aiCommandTopic = COMMAND_UNIT_RECLAIM_UNIT;
				} else {
					aiCommandTopic = COMMAND_UNIT_RECLAIM_FEATURE;
				}
			} else {
				aiCommandTopic = COMMAND_UNIT_RECLAIM_AREA;
			}
			break;
		}
		case CMD_CLOAK:
		{
			aiCommandTopic = COMMAND_UNIT_CLOAK;
			break;
		}
		case CMD_STOCKPILE:
		{
			aiCommandTopic = COMMAND_UNIT_STOCKPILE;
			break;
		}
		case CMD_MANUALFIRE:
		{
			// FIXME
			if (engineCmd->params.size() < 3) {
				aiCommandTopic = COMMAND_UNIT_D_GUN;
			} else {
				aiCommandTopic = COMMAND_UNIT_D_GUN_POS;
			}
			break;
		}
		case CMD_RESTORE:
		{
			aiCommandTopic = COMMAND_UNIT_RESTORE_AREA;
			break;
		}
		case CMD_REPEAT:
		{
			aiCommandTopic = COMMAND_UNIT_SET_REPEAT;
			break;
		}
		case CMD_TRAJECTORY:
		{
			aiCommandTopic = COMMAND_UNIT_SET_TRAJECTORY;
			break;
		}
		case CMD_RESURRECT:
		{
			if (engineCmd->params.size() < 3) {
				aiCommandTopic = COMMAND_UNIT_RESURRECT;
			} else {
				aiCommandTopic = COMMAND_UNIT_RESURRECT_AREA;
			}
			break;
		}
		case CMD_CAPTURE:
		{
			if (engineCmd->params.size() < 3 || engineCmd->params.size() == 5) {
				aiCommandTopic = COMMAND_UNIT_CAPTURE;
			} else {
				aiCommandTopic = COMMAND_UNIT_CAPTURE_AREA;
			}
			break;
		}
		case CMD_AUTOREPAIRLEVEL:
		{
			aiCommandTopic = COMMAND_UNIT_SET_AUTO_REPAIR_LEVEL;
			break;
		}
		case CMD_IDLEMODE:
		{
			aiCommandTopic = COMMAND_UNIT_SET_IDLE_MODE;
			break;
		}
		default:
		{
			if (internalUnitCmdTopic < 0) {
				aiCommandTopic = COMMAND_UNIT_BUILD;
			} else {
				aiCommandTopic = COMMAND_UNIT_CUSTOM;
			}
			break;
		}
	}

	return aiCommandTopic;
}

Command* newCommand(void* sUnitCommandData, int sCommandId, int maxUnits) {

	Command* c = NULL;

	switch (sCommandId) {
		case COMMAND_UNIT_BUILD:
		{
			SBuildUnitCommand* cmd = (SBuildUnitCommand*) sUnitCommandData;
			c = new Command(-cmd->toBuildUnitDefId, cmd->options);
			c->timeOut = cmd->timeOut;

			if (cmd->buildPos_posF3 != NULL) {
				c->params.push_back(cmd->buildPos_posF3[0]);
				c->params.push_back(cmd->buildPos_posF3[1]);
				c->params.push_back(cmd->buildPos_posF3[2]);
			}
			if (cmd->facing != UNIT_COMMAND_BUILD_NO_FACING) c->params.push_back(cmd->facing);
			break;
		}
		case COMMAND_UNIT_STOP:
		{
			SStopUnitCommand* cmd = (SStopUnitCommand*) sUnitCommandData;
			c = new Command(CMD_STOP, cmd->options);
			c->timeOut = cmd->timeOut;
			break;
		}
		case COMMAND_UNIT_WAIT:
		{
			SWaitUnitCommand* cmd = (SWaitUnitCommand*) sUnitCommandData;
			c = new Command(CMD_WAIT, cmd->options);
			c->timeOut = cmd->timeOut;
			break;
		}
		case COMMAND_UNIT_WAIT_TIME:
		{
			STimeWaitUnitCommand* cmd = (STimeWaitUnitCommand*) sUnitCommandData;
			c = new Command(CMD_TIMEWAIT, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->time);
			break;
		}
		case COMMAND_UNIT_WAIT_DEATH:
		{
			SDeathWaitUnitCommand* cmd = (SDeathWaitUnitCommand*) sUnitCommandData;
			c = new Command(CMD_DEATHWAIT, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toDieUnitId);
			break;
		}
		case COMMAND_UNIT_WAIT_SQUAD:
		{
			SSquadWaitUnitCommand* cmd = (SSquadWaitUnitCommand*) sUnitCommandData;
			c = new Command(CMD_SQUADWAIT, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->numUnits);
			break;
		}
		case COMMAND_UNIT_WAIT_GATHER:
		{
			SGatherWaitUnitCommand* cmd = (SGatherWaitUnitCommand*) sUnitCommandData;
			c = new Command(CMD_GATHERWAIT, cmd->options);
			c->timeOut = cmd->timeOut;
			break;
		}
		case COMMAND_UNIT_MOVE:
		{
			SMoveUnitCommand* cmd = (SMoveUnitCommand*) sUnitCommandData;
			c = new Command(CMD_MOVE, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toPos_posF3[0]);
			c->params.push_back(cmd->toPos_posF3[1]);
			c->params.push_back(cmd->toPos_posF3[2]);
			break;
		}
		case COMMAND_UNIT_PATROL:
		{
			SPatrolUnitCommand* cmd = (SPatrolUnitCommand*) sUnitCommandData;
			c = new Command(CMD_PATROL, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toPos_posF3[0]);
			c->params.push_back(cmd->toPos_posF3[1]);
			c->params.push_back(cmd->toPos_posF3[2]);
			break;
		}
		case COMMAND_UNIT_FIGHT:
		{
			SFightUnitCommand* cmd = (SFightUnitCommand*) sUnitCommandData;
			c = new Command(CMD_FIGHT, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toPos_posF3[0]);
			c->params.push_back(cmd->toPos_posF3[1]);
			c->params.push_back(cmd->toPos_posF3[2]);
			break;
		}
		case COMMAND_UNIT_ATTACK:
		{
			SAttackUnitCommand* cmd = (SAttackUnitCommand*) sUnitCommandData;
			c = new Command(CMD_ATTACK, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toAttackUnitId);
			break;
		}
		case COMMAND_UNIT_ATTACK_AREA:
		{
			SAttackAreaUnitCommand* cmd = (SAttackAreaUnitCommand*) sUnitCommandData;
			c = new Command(CMD_ATTACK, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toAttackPos_posF3[0]);
			c->params.push_back(cmd->toAttackPos_posF3[1]);
			c->params.push_back(cmd->toAttackPos_posF3[2]);
			c->params.push_back(cmd->radius);
			break;
		}
		case COMMAND_UNIT_GUARD:
		{
			SGuardUnitCommand* cmd = (SGuardUnitCommand*) sUnitCommandData;
			c = new Command(CMD_GUARD, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toGuardUnitId);
			break;
		}
		case COMMAND_UNIT_GROUP_ADD:
		{
			SGroupAddUnitCommand* cmd = (SGroupAddUnitCommand*) sUnitCommandData;
			c = new Command(CMD_GROUPADD, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toGroupId);
			break;
		}
		case COMMAND_UNIT_GROUP_CLEAR:
		{
			SGroupClearUnitCommand* cmd = (SGroupClearUnitCommand*) sUnitCommandData;
			c = new Command(CMD_GROUPCLEAR, cmd->options);
			c->timeOut = cmd->timeOut;
			break;
		}
		case COMMAND_UNIT_REPAIR:
		{
			SRepairUnitCommand* cmd = (SRepairUnitCommand*) sUnitCommandData;
			c = new Command(CMD_REPAIR, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toRepairUnitId);
			break;
		}
		case COMMAND_UNIT_SET_FIRE_STATE:
		{
			SSetFireStateUnitCommand* cmd = (SSetFireStateUnitCommand*) sUnitCommandData;
			c = new Command(CMD_FIRE_STATE, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->fireState);
			break;
		}
		case COMMAND_UNIT_SET_MOVE_STATE:
		{
			SSetMoveStateUnitCommand* cmd = (SSetMoveStateUnitCommand*) sUnitCommandData;
			c = new Command(CMD_MOVE_STATE, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->moveState);
			break;
		}
		case COMMAND_UNIT_SET_BASE:
		{
			SSetBaseUnitCommand* cmd = (SSetBaseUnitCommand*) sUnitCommandData;
			c = new Command(CMD_SETBASE, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->basePos_posF3[0]);
			c->params.push_back(cmd->basePos_posF3[1]);
			c->params.push_back(cmd->basePos_posF3[2]);
			break;
		}
		case COMMAND_UNIT_SELF_DESTROY:
		{
			SSelfDestroyUnitCommand* cmd = (SSelfDestroyUnitCommand*) sUnitCommandData;
			c = new Command(CMD_SELFD, cmd->options);
			c->timeOut = cmd->timeOut;
			break;
		}
		case COMMAND_UNIT_SET_WANTED_MAX_SPEED:
		{
			SSetWantedMaxSpeedUnitCommand* cmd = (SSetWantedMaxSpeedUnitCommand*) sUnitCommandData;
			c = new Command(CMD_SET_WANTED_MAX_SPEED, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->wantedMaxSpeed);
			break;
		}
		case COMMAND_UNIT_LOAD_UNITS:
		{
			SLoadUnitsUnitCommand* cmd = (SLoadUnitsUnitCommand*) sUnitCommandData;
			c = new Command(CMD_LOAD_UNITS, cmd->options);
			c->timeOut = cmd->timeOut;

			for (int i = 0; i < cmd->toLoadUnitIds_size; ++i) {
				c->params.push_back(cmd->toLoadUnitIds[i]);
			}
			break;
		}
		case COMMAND_UNIT_LOAD_UNITS_AREA:
		{
			SLoadUnitsAreaUnitCommand* cmd = (SLoadUnitsAreaUnitCommand*) sUnitCommandData;
			c = new Command(CMD_LOAD_UNITS, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->pos_posF3[0]);
			c->params.push_back(cmd->pos_posF3[1]);
			c->params.push_back(cmd->pos_posF3[2]);
			c->params.push_back(cmd->radius);
			break;
		}
		case COMMAND_UNIT_LOAD_ONTO:
		{
			SLoadOntoUnitCommand* cmd = (SLoadOntoUnitCommand*) sUnitCommandData;
			c = new Command(CMD_LOAD_ONTO, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->transporterUnitId);
			break;
		}
		case COMMAND_UNIT_UNLOAD_UNITS_AREA:
		{
			SUnloadUnitsAreaUnitCommand* cmd = (SUnloadUnitsAreaUnitCommand*) sUnitCommandData;
			c = new Command(CMD_UNLOAD_UNITS, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toPos_posF3[0]);
			c->params.push_back(cmd->toPos_posF3[1]);
			c->params.push_back(cmd->toPos_posF3[2]);
			c->params.push_back(cmd->radius);
			break;
		}
		case COMMAND_UNIT_UNLOAD_UNIT:
		{
			SUnloadUnitCommand* cmd = (SUnloadUnitCommand*) sUnitCommandData;
			c = new Command(CMD_UNLOAD_UNIT, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toPos_posF3[0]);
			c->params.push_back(cmd->toPos_posF3[1]);
			c->params.push_back(cmd->toPos_posF3[2]);
			c->params.push_back(cmd->toUnloadUnitId);
			break;
		}
		case COMMAND_UNIT_SET_ON_OFF:
		{
			SSetOnOffUnitCommand* cmd = (SSetOnOffUnitCommand*) sUnitCommandData;
			c = new Command(CMD_ONOFF, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->on ? 1 : 0);
			break;
		}
		case COMMAND_UNIT_RECLAIM_UNIT:
		{
			SReclaimUnitUnitCommand* cmd = (SReclaimUnitUnitCommand*) sUnitCommandData;
			c = new Command(CMD_RECLAIM, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toReclaimUnitId);
			break;
		}
		case COMMAND_UNIT_RECLAIM_FEATURE:
		{
			SReclaimFeatureUnitCommand* cmd = (SReclaimFeatureUnitCommand*) sUnitCommandData;
			c = new Command(CMD_RECLAIM, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(maxUnits + cmd->toReclaimFeatureId);
			break;
		}
		case COMMAND_UNIT_RECLAIM_AREA:
		{
			SReclaimAreaUnitCommand* cmd = (SReclaimAreaUnitCommand*) sUnitCommandData;
			c = new Command(CMD_RECLAIM, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->pos_posF3[0]);
			c->params.push_back(cmd->pos_posF3[1]);
			c->params.push_back(cmd->pos_posF3[2]);
			c->params.push_back(cmd->radius);
			break;
		}
		case COMMAND_UNIT_CLOAK:
		{
			SCloakUnitCommand* cmd = (SCloakUnitCommand*) sUnitCommandData;
			c = new Command(CMD_CLOAK, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->cloak ? 1 : 0);
			break;
		}
		case COMMAND_UNIT_STOCKPILE:
		{
			SStockpileUnitCommand* cmd = (SStockpileUnitCommand*) sUnitCommandData;
			c = new Command(CMD_STOCKPILE, cmd->options);
			c->timeOut = cmd->timeOut;
			break;
		}

		case COMMAND_UNIT_D_GUN:
		{
			// FIXME
			SDGunUnitCommand* cmd = (SDGunUnitCommand*) sUnitCommandData;
			c = new Command(CMD_MANUALFIRE, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toAttackUnitId);
			break;
		}
		case COMMAND_UNIT_D_GUN_POS:
		{
			// FIXME
			SDGunPosUnitCommand* cmd = (SDGunPosUnitCommand*) sUnitCommandData;
			c = new Command(CMD_MANUALFIRE, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->pos_posF3[0]);
			c->params.push_back(cmd->pos_posF3[1]);
			c->params.push_back(cmd->pos_posF3[2]);
			break;
		}

		case COMMAND_UNIT_RESTORE_AREA:
		{
			SRestoreAreaUnitCommand* cmd = (SRestoreAreaUnitCommand*) sUnitCommandData;
			c = new Command(CMD_RESTORE, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->pos_posF3[0]);
			c->params.push_back(cmd->pos_posF3[1]);
			c->params.push_back(cmd->pos_posF3[2]);
			c->params.push_back(cmd->radius);
			break;
		}
		case COMMAND_UNIT_SET_REPEAT:
		{
			SSetRepeatUnitCommand* cmd = (SSetRepeatUnitCommand*) sUnitCommandData;
			c = new Command(CMD_REPEAT, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->repeat ? 1 : 0);
			break;
		}
		case COMMAND_UNIT_SET_TRAJECTORY:
		{
			SSetTrajectoryUnitCommand* cmd = (SSetTrajectoryUnitCommand*) sUnitCommandData;
			c = new Command(CMD_TRAJECTORY, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->trajectory);
			break;
		}
		case COMMAND_UNIT_RESURRECT:
		{
			SResurrectUnitCommand* cmd = (SResurrectUnitCommand*) sUnitCommandData;
			c = new Command(CMD_RESURRECT, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toResurrectFeatureId);
			break;
		}
		case COMMAND_UNIT_RESURRECT_AREA:
		{
			SResurrectAreaUnitCommand* cmd = (SResurrectAreaUnitCommand*) sUnitCommandData;
			c = new Command(CMD_RESURRECT, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->pos_posF3[0]);
			c->params.push_back(cmd->pos_posF3[1]);
			c->params.push_back(cmd->pos_posF3[2]);
			c->params.push_back(cmd->radius);
			break;
		}
		case COMMAND_UNIT_CAPTURE:
		{
			SCaptureUnitCommand* cmd = (SCaptureUnitCommand*) sUnitCommandData;
			c = new Command(CMD_CAPTURE, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toCaptureUnitId);
			break;
		}
		case COMMAND_UNIT_CAPTURE_AREA:
		{
			SCaptureAreaUnitCommand* cmd = (SCaptureAreaUnitCommand*) sUnitCommandData;
			c = new Command(CMD_CAPTURE, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->pos_posF3[0]);
			c->params.push_back(cmd->pos_posF3[1]);
			c->params.push_back(cmd->pos_posF3[2]);
			c->params.push_back(cmd->radius);
			break;
		}
		case COMMAND_UNIT_SET_AUTO_REPAIR_LEVEL:
		{
			SSetAutoRepairLevelUnitCommand* cmd = (SSetAutoRepairLevelUnitCommand*) sUnitCommandData;
			c = new Command(CMD_AUTOREPAIRLEVEL, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->autoRepairLevel);
			break;
		}
		case COMMAND_UNIT_SET_IDLE_MODE:
		{
			SSetIdleModeUnitCommand* cmd = (SSetIdleModeUnitCommand*) sUnitCommandData;
			c = new Command(CMD_IDLEMODE, cmd->options);
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->idleMode);
			break;
		}
		case COMMAND_UNIT_CUSTOM:
		{
			SCustomUnitCommand* cmd = (SCustomUnitCommand*) sUnitCommandData;
			c = new Command(cmd->cmdId, cmd->options);
			c->timeOut = cmd->timeOut;

			for (int i = 0; i < cmd->params_size; ++i) {
				c->params.push_back(cmd->params[i]);
			}
			break;
		}
		default:
		{
			delete c;
			c = NULL;
		}
	}

	return c;
}

#endif // __cplusplus
