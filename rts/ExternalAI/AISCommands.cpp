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
#include "Sim/Units/CommandAI/Command.h"

void freeSUnitCommand(void* sCommandData, int sCommandId) {

	switch (sCommandId) {
		case COMMAND_UNIT_LOAD_UNITS:
		{
			struct SLoadUnitsUnitCommand* cmd = (struct SLoadUnitsUnitCommand*) sCommandData;
			free(cmd->toLoadUnitIds);
			cmd->toLoadUnitIds = NULL;
			break;
		}
		case COMMAND_UNIT_CUSTOM:
		{
			struct SCustomUnitCommand* cmd = (struct SCustomUnitCommand*) sCommandData;
			free(cmd->params);
			cmd->params = NULL;
			break;
		}
	}

	free(sCommandData);
	sCommandData = NULL;
}

void* mallocSUnitCommand(int unitId, int groupId, const Command* c, int sCommandId[0]) {

	int sCmdId = COMMAND_NULL;
	void* sCommandData;

	switch (c->id) {
		case CMD_STOP:
		{
			SStopUnitCommand* cmd = (SStopUnitCommand*) malloc(sizeof (SStopUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_STOP;
			break;
		}
		case CMD_WAIT:
		{
			SWaitUnitCommand* cmd = (SWaitUnitCommand*) malloc(sizeof (SWaitUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_WAIT;
			break;
		}
		case CMD_TIMEWAIT:
		{
			STimeWaitUnitCommand* cmd = (STimeWaitUnitCommand*) malloc(sizeof (STimeWaitUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->time = c->params[0];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_WAIT_TIME;
			break;
		}
		case CMD_DEATHWAIT:
		{
			SDeathWaitUnitCommand* cmd = (SDeathWaitUnitCommand*) malloc(sizeof (SDeathWaitUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toDieUnitId = (int) c->params[0];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_WAIT_DEATH;
			break;
		}
		case CMD_SQUADWAIT:
		{
			SSquadWaitUnitCommand* cmd = (SSquadWaitUnitCommand*) malloc(sizeof (SSquadWaitUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->numUnits = (int) c->params[0];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_WAIT_SQUAD;
			break;
		}
		case CMD_GATHERWAIT:
		{
			SGatherWaitUnitCommand* cmd = (SGatherWaitUnitCommand*) malloc(sizeof (SGatherWaitUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_WAIT_GATHER;
			break;
		}
		case CMD_MOVE:
		{
			SAIFloat3 toPos = {c->params[0], c->params[1], c->params[2]};
			SMoveUnitCommand* cmd = (SMoveUnitCommand*) malloc(sizeof (SMoveUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toPos = toPos;

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_MOVE;
			break;
		}
		case CMD_PATROL:
		{
			SAIFloat3 toPos = {c->params[0], c->params[1], c->params[2]};
			SPatrolUnitCommand* cmd = (SPatrolUnitCommand*) malloc(sizeof (SPatrolUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toPos = toPos;

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_PATROL;
			break;
		}
		case CMD_FIGHT:
		{
			SAIFloat3 toPos = {c->params[0], c->params[1], c->params[2]};
			SFightUnitCommand* cmd = (SFightUnitCommand*) malloc(sizeof (SFightUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toPos = toPos;

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_FIGHT;
			break;
		}
		case CMD_ATTACK:
		{
			if (c->params.size() < 3) {
				SAttackUnitCommand* cmd = (SAttackUnitCommand*) malloc(sizeof (SAttackUnitCommand));
				cmd->unitId = unitId;
				cmd->groupId = groupId;
				cmd->options = c->options;
				cmd->timeOut = c->timeOut;
				cmd->toAttackUnitId = (int) c->params[0];

				sCommandData = cmd;
				sCmdId = COMMAND_UNIT_ATTACK;
			} else {
				SAIFloat3 toAttackPos = {c->params[0], c->params[1], c->params[2]};
				float radius = 0.0f;
				if (c->params.size() >= 4) radius = c->params[3];
				SAttackAreaUnitCommand* cmd = (SAttackAreaUnitCommand*) malloc(sizeof (SAttackAreaUnitCommand));
				cmd->unitId = unitId;
				cmd->groupId = groupId;
				cmd->options = c->options;
				cmd->timeOut = c->timeOut;
				cmd->toAttackPos = toAttackPos;
				cmd->radius = radius;

				sCommandData = cmd;
				sCmdId = COMMAND_UNIT_ATTACK_AREA;
			}
			break;
		}
		case CMD_GUARD:
		{
			SGuardUnitCommand* cmd = (SGuardUnitCommand*) malloc(sizeof (SGuardUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toGuardUnitId = (int) c->params[0];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_GUARD;
			break;
		}
		case CMD_AISELECT:
		{
			SAiSelectUnitCommand* cmd = (SAiSelectUnitCommand*) malloc(sizeof (SAiSelectUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_AI_SELECT;
			break;
		}
		case CMD_GROUPADD:
		{
			SGroupAddUnitCommand* cmd = (SGroupAddUnitCommand*) malloc(sizeof (SGroupAddUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toGroupId = (int) c->params[0];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_GROUP_ADD;
			break;
		}
		case CMD_GROUPCLEAR:
		{
			SGroupClearUnitCommand* cmd = (SGroupClearUnitCommand*) malloc(sizeof (SGroupClearUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_GROUP_CLEAR;
			break;
		}
		case CMD_REPAIR:
		{
			SRepairUnitCommand* cmd = (SRepairUnitCommand*) malloc(sizeof (SRepairUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toRepairUnitId = (int) c->params[0];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_REPAIR;
			break;
		}
		case CMD_FIRE_STATE:
		{
			SSetFireStateUnitCommand* cmd = (SSetFireStateUnitCommand*) malloc(sizeof (SSetFireStateUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->fireState = (int) c->params[0];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_SET_FIRE_STATE;
			break;
		}
		case CMD_MOVE_STATE:
		{
			SSetMoveStateUnitCommand* cmd = (SSetMoveStateUnitCommand*) malloc(sizeof (SSetMoveStateUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->moveState = (int) c->params[0];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_SET_MOVE_STATE;
			break;
		}
		case CMD_SETBASE:
		{
			SAIFloat3 basePos = {c->params[0], c->params[1], c->params[2]};
			SSetBaseUnitCommand* cmd = (SSetBaseUnitCommand*) malloc(sizeof (SSetBaseUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->basePos = basePos;

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_SET_BASE;
			break;
		}
		case CMD_SELFD:
		{
			SSelfDestroyUnitCommand* cmd = (SSelfDestroyUnitCommand*) malloc(sizeof (SSelfDestroyUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_SELF_DESTROY;
			break;
		}
		case CMD_SET_WANTED_MAX_SPEED:
		{
			SSetWantedMaxSpeedUnitCommand* cmd = (SSetWantedMaxSpeedUnitCommand*) malloc(sizeof (SSetWantedMaxSpeedUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->wantedMaxSpeed = c->params[0];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_SET_WANTED_MAX_SPEED;
			break;
		}
		case CMD_LOAD_UNITS:
		{
			if (c->params.size() < 3) {
				//int numToLoadUnits = 1;
				int numToLoadUnits = c->params.size();

				SLoadUnitsUnitCommand* cmd = (SLoadUnitsUnitCommand*) malloc(sizeof (SLoadUnitsUnitCommand));
				cmd->unitId = unitId;
				cmd->groupId = groupId;
				cmd->options = c->options;
				cmd->timeOut = c->timeOut;

				cmd->numToLoadUnits = numToLoadUnits;
				cmd->toLoadUnitIds = (int*) calloc(numToLoadUnits, sizeof(int));
				int i;
				for (i=0; i < numToLoadUnits; ++i) {
					cmd->toLoadUnitIds[i] = (int) c->params.at(i);
				}

				sCommandData = cmd;
				sCmdId = COMMAND_UNIT_LOAD_UNITS;
			} else {
				SAIFloat3 pos = {c->params[0], c->params[1], c->params[2]};
				float radius = 0.0f;
				if (c->params.size() >= 4) radius = c->params[3];
				SLoadUnitsAreaUnitCommand* cmd = (SLoadUnitsAreaUnitCommand*) malloc(sizeof (SLoadUnitsAreaUnitCommand));
				cmd->unitId = unitId;
				cmd->groupId = groupId;
				cmd->options = c->options;
				cmd->timeOut = c->timeOut;
				cmd->pos = pos;
				cmd->radius = radius;

				sCommandData = cmd;
				sCmdId = COMMAND_UNIT_LOAD_UNITS_AREA;
			}
			break;
		}
		case CMD_LOAD_ONTO:
		{
			SLoadOntoUnitCommand* cmd = (SLoadOntoUnitCommand*) malloc(sizeof (SLoadOntoUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->transporterUnitId = (int) c->params[0];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_LOAD_ONTO;
			break;
		}
		case CMD_UNLOAD_UNIT:
		{
			SAIFloat3 toPos = {c->params[0], c->params[1], c->params[2]};
			SUnloadUnitCommand* cmd = (SUnloadUnitCommand*) malloc(sizeof (SUnloadUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toPos = toPos;
			cmd->toUnloadUnitId = (int) c->params[3];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_UNLOAD_UNIT;
			break;
		}
		case CMD_UNLOAD_UNITS:
		{
			SAIFloat3 toPos = {c->params[0], c->params[1], c->params[2]};
			float radius = 0.0f;
			if (c->params.size() >= 4) radius = c->params[3];
			SUnloadUnitsAreaUnitCommand* cmd = (SUnloadUnitsAreaUnitCommand*) malloc(sizeof (SUnloadUnitsAreaUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->toPos = toPos;
			cmd->radius = radius;

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_UNLOAD_UNITS_AREA;
			break;
		}
		case CMD_ONOFF:
		{
			SSetOnOffUnitCommand* cmd = (SSetOnOffUnitCommand*) malloc(sizeof (SSetOnOffUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->on = (bool) c->params[0];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_SET_ON_OFF;
			break;
		}
		case CMD_RECLAIM:
		{
			if (c->params.size() < 3) {
				SReclaimUnitCommand* cmd = (SReclaimUnitCommand*) malloc(sizeof (SReclaimUnitCommand));
				cmd->unitId = unitId;
				cmd->groupId = groupId;
				cmd->options = c->options;
				cmd->timeOut = c->timeOut;
				cmd->toReclaimUnitIdOrFeatureId = (int) c->params[0];

				sCommandData = cmd;
				sCmdId = COMMAND_UNIT_RECLAIM;
			} else {
				SAIFloat3 pos = {c->params[0], c->params[1], c->params[2]};
				float radius = 0.0f;
				if (c->params.size() >= 4) radius = c->params[3];
				SReclaimAreaUnitCommand* cmd = (SReclaimAreaUnitCommand*) malloc(sizeof (SReclaimAreaUnitCommand));
				cmd->unitId = unitId;
				cmd->groupId = groupId;
				cmd->options = c->options;
				cmd->timeOut = c->timeOut;
				cmd->pos = pos;
				cmd->radius = radius;

				sCommandData = cmd;
				sCmdId = COMMAND_UNIT_RECLAIM_AREA;
			}
			break;
		}
		case CMD_CLOAK:
		{
			SCloakUnitCommand* cmd = (SCloakUnitCommand*) malloc(sizeof (SCloakUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->cloak = (bool) c->params[0];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_CLOAK;
			break;
		}
		case CMD_STOCKPILE:
		{
			SStockpileUnitCommand* cmd = (SStockpileUnitCommand*) malloc(sizeof (SStockpileUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_STOCKPILE;
			break;
		}
		case CMD_DGUN:
		{
			if (c->params.size() < 3) {
				SDGunUnitCommand* cmd = (SDGunUnitCommand*) malloc(sizeof (SDGunUnitCommand));
				cmd->unitId = unitId;
				cmd->groupId = groupId;
				cmd->options = c->options;
				cmd->timeOut = c->timeOut;
				cmd->toAttackUnitId = (int) c->params[0];

				sCommandData = cmd;
				sCmdId = COMMAND_UNIT_D_GUN;
			} else {
				SAIFloat3 pos = {c->params[0], c->params[1], c->params[2]};
				SDGunPosUnitCommand* cmd = (SDGunPosUnitCommand*) malloc(sizeof (SDGunPosUnitCommand));
				cmd->unitId = unitId;
				cmd->groupId = groupId;
				cmd->options = c->options;
				cmd->timeOut = c->timeOut;
				cmd->pos = pos;

				sCommandData = cmd;
				sCmdId = COMMAND_UNIT_D_GUN_POS;
			}
			break;
		}
		case CMD_RESTORE:
		{
			SAIFloat3 pos = {c->params[0], c->params[1], c->params[2]};
			float radius = 0.0f;
			if (c->params.size() >= 4) radius = c->params[3];
			SRestoreAreaUnitCommand* cmd = (SRestoreAreaUnitCommand*) malloc(sizeof (SRestoreAreaUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->pos = pos;
			cmd->radius = radius;

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_RESTORE_AREA;
			break;
		}
		case CMD_REPEAT:
		{
			SSetRepeatUnitCommand* cmd = (SSetRepeatUnitCommand*) malloc(sizeof (SSetRepeatUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->repeat = (bool) c->params[0];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_SET_REPEAT;
			break;
		}
		case CMD_TRAJECTORY:
		{
			SSetTrajectoryUnitCommand* cmd = (SSetTrajectoryUnitCommand*) malloc(sizeof (SSetTrajectoryUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->trajectory = (int) c->params[0];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_SET_TRAJECTORY;
			break;
		}
		case CMD_RESURRECT:
		{
			if (c->params.size() < 3) {
				SResurrectUnitCommand* cmd = (SResurrectUnitCommand*) malloc(sizeof (SResurrectUnitCommand));
				cmd->unitId = unitId;
				cmd->groupId = groupId;
				cmd->options = c->options;
				cmd->timeOut = c->timeOut;
				cmd->toResurrectFeatureId = (int) c->params[0];

				sCommandData = cmd;
				sCmdId = COMMAND_UNIT_RESURRECT;
			} else {
				SAIFloat3 pos = {c->params[0], c->params[1], c->params[2]};
				float radius = 0.0f;
				if (c->params.size() >= 4) radius = c->params[3];
				SResurrectAreaUnitCommand* cmd = (SResurrectAreaUnitCommand*) malloc(sizeof (SResurrectAreaUnitCommand));
				cmd->unitId = unitId;
				cmd->groupId = groupId;
				cmd->options = c->options;
				cmd->timeOut = c->timeOut;
				cmd->pos = pos;
				cmd->radius = radius;

				sCommandData = cmd;
				sCmdId = COMMAND_UNIT_RESURRECT_AREA;
			}
			break;
		}
		case CMD_CAPTURE:
		{
			if (c->params.size() < 3) {
				SCaptureUnitCommand* cmd = (SCaptureUnitCommand*) malloc(sizeof (SCaptureUnitCommand));
				cmd->unitId = unitId;
				cmd->groupId = groupId;
				cmd->options = c->options;
				cmd->timeOut = c->timeOut;
				cmd->toCaptureUnitId = (int) c->params[0];

				sCommandData = cmd;
				sCmdId = COMMAND_UNIT_CAPTURE;
			} else {
				SAIFloat3 pos = {c->params[0], c->params[1], c->params[2]};
				float radius = 0.0f;
				if (c->params.size() >= 4) radius = c->params[3];
				SCaptureAreaUnitCommand* cmd = (SCaptureAreaUnitCommand*) malloc(sizeof (SCaptureAreaUnitCommand));
				cmd->unitId = unitId;
				cmd->groupId = groupId;
				cmd->options = c->options;
				cmd->timeOut = c->timeOut;
				cmd->pos = pos;
				cmd->radius = radius;

				sCommandData = cmd;
				sCmdId = COMMAND_UNIT_CAPTURE_AREA;
			}
			break;
		}
		case CMD_AUTOREPAIRLEVEL:
		{
			SSetAutoRepairLevelUnitCommand* cmd = (SSetAutoRepairLevelUnitCommand*) malloc(sizeof (SSetAutoRepairLevelUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->autoRepairLevel = (int) c->params[0];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_SET_AUTO_REPAIR_LEVEL;
			break;
		}
		case CMD_IDLEMODE:
		{
			SSetIdleModeUnitCommand* cmd = (SSetIdleModeUnitCommand*) malloc(sizeof (SSetIdleModeUnitCommand));
			cmd->unitId = unitId;
			cmd->groupId = groupId;
			cmd->options = c->options;
			cmd->timeOut = c->timeOut;
			cmd->idleMode = (int) c->params[0];

			sCommandData = cmd;
			sCmdId = COMMAND_UNIT_SET_IDLE_MODE;
			break;
		}
		default:
		{
			if (c->id < 0) { // CMD_BUILD
				int toBuildUnitDefId = -c->id;
				SAIFloat3 buildPos = {-1.0, -1.0, -1.0};
				if (c->params.size() >= 3) {
					buildPos.x = c->params[0];
					buildPos.y = c->params[1];
					buildPos.z = c->params[2];
				}
				int facing = UNIT_COMMAND_BUILD_NO_FACING;
				if (c->params.size() >= 4) facing = c->params[3];
				SBuildUnitCommand* cmd = (SBuildUnitCommand*) malloc(sizeof (SBuildUnitCommand));
				cmd->unitId = unitId;
				cmd->groupId = groupId;
				cmd->options = c->options;
				cmd->timeOut = c->timeOut;
				cmd->toBuildUnitDefId = toBuildUnitDefId;
				cmd->buildPos = buildPos;
				cmd->facing = facing;

				sCommandData = cmd;
				sCmdId = COMMAND_UNIT_BUILD;
			} else { // CMD_CUSTOM
				int cmdId = c->id;
				int numParams = c->params.size();
				SCustomUnitCommand* cmd = (SCustomUnitCommand*) malloc(sizeof (SCustomUnitCommand));
				cmd->unitId = unitId;
				cmd->groupId = groupId;
				cmd->options = c->options;
				cmd->timeOut = c->timeOut;
				cmd->cmdId = cmdId;
				cmd->numParams = numParams;
				cmd->params = (float*) calloc(numParams, sizeof(float));
				int i;
				for (i=0; i < numParams; ++i) {
					cmd->params[i] = c->params.at(i);
				}

				sCommandData = cmd;
				sCmdId = COMMAND_UNIT_CUSTOM;
			}
			break;
		}

	}

	*sCommandId = sCmdId;
	return sCommandData;
}



Command* newCommand(void* sUnitCommandData, int sCommandId) {

	Command* c = new Command();

	switch (sCommandId) {
		case COMMAND_UNIT_BUILD:
		{
			SBuildUnitCommand* cmd = (SBuildUnitCommand*) sUnitCommandData;
			c->id = -cmd->toBuildUnitDefId;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			if (cmd->buildPos.x != -1.0
					|| cmd->buildPos.y != -1.0
					|| cmd->buildPos.z != -1.0) {
				c->params.push_back(cmd->buildPos.x);
				c->params.push_back(cmd->buildPos.y);
				c->params.push_back(cmd->buildPos.z);
			}
			if (cmd->facing != UNIT_COMMAND_BUILD_NO_FACING) c->params.push_back(cmd->facing);
			break;
		}
		case COMMAND_UNIT_STOP:
		{
			SStopUnitCommand* cmd = (SStopUnitCommand*) sUnitCommandData;
			c->id = CMD_STOP;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;
			break;
		}
		case COMMAND_UNIT_WAIT:
		{
			SWaitUnitCommand* cmd = (SWaitUnitCommand*) sUnitCommandData;
			c->id = CMD_WAIT;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;
			break;
		}
		case COMMAND_UNIT_WAIT_TIME:
		{
			STimeWaitUnitCommand* cmd = (STimeWaitUnitCommand*) sUnitCommandData;
			c->id = CMD_WAITCODE_TIMEWAIT;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->time);
			break;
		}
		case COMMAND_UNIT_WAIT_DEATH:
		{
			SDeathWaitUnitCommand* cmd = (SDeathWaitUnitCommand*) sUnitCommandData;
			c->id = CMD_WAITCODE_DEATHWAIT;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toDieUnitId);
			break;
		}
		case COMMAND_UNIT_WAIT_SQUAD:
		{
			SSquadWaitUnitCommand* cmd = (SSquadWaitUnitCommand*) sUnitCommandData;
			c->id = CMD_WAITCODE_SQUADWAIT;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->numUnits);
			break;
		}
		case COMMAND_UNIT_WAIT_GATHER:
		{
			SGatherWaitUnitCommand* cmd = (SGatherWaitUnitCommand*) sUnitCommandData;
			c->id = CMD_WAITCODE_GATHERWAIT;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;
			break;
		}
		case COMMAND_UNIT_MOVE:
		{
			SMoveUnitCommand* cmd = (SMoveUnitCommand*) sUnitCommandData;
			c->id = CMD_MOVE;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toPos.x);
			c->params.push_back(cmd->toPos.y);
			c->params.push_back(cmd->toPos.z);
			break;
		}
		case COMMAND_UNIT_PATROL:
		{
			SPatrolUnitCommand* cmd = (SPatrolUnitCommand*) sUnitCommandData;
			c->id = CMD_PATROL;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toPos.x);
			c->params.push_back(cmd->toPos.y);
			c->params.push_back(cmd->toPos.z);
			break;
		}
		case COMMAND_UNIT_FIGHT:
		{
			SFightUnitCommand* cmd = (SFightUnitCommand*) sUnitCommandData;
			c->id = CMD_FIGHT;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toPos.x);
			c->params.push_back(cmd->toPos.y);
			c->params.push_back(cmd->toPos.z);
			break;
		}
		case COMMAND_UNIT_ATTACK:
		{
			SAttackUnitCommand* cmd = (SAttackUnitCommand*) sUnitCommandData;
			c->id = CMD_ATTACK;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toAttackUnitId);
			break;
		}
		case COMMAND_UNIT_ATTACK_AREA:
		{
			SAttackAreaUnitCommand* cmd = (SAttackAreaUnitCommand*) sUnitCommandData;
			c->id = CMD_ATTACK;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toAttackPos.x);
			c->params.push_back(cmd->toAttackPos.y);
			c->params.push_back(cmd->toAttackPos.z);
			c->params.push_back(cmd->radius);
			break;
		}
		case COMMAND_UNIT_GUARD:
		{
			SGuardUnitCommand* cmd = (SGuardUnitCommand*) sUnitCommandData;
			c->id = CMD_GUARD;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toGuardUnitId);
			break;
		}
		case COMMAND_UNIT_AI_SELECT:
		{
			SAiSelectUnitCommand* cmd = (SAiSelectUnitCommand*) sUnitCommandData;
			c->id = CMD_AISELECT;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;
			break;
		}
		case COMMAND_UNIT_GROUP_ADD:
		{
			SGroupAddUnitCommand* cmd = (SGroupAddUnitCommand*) sUnitCommandData;
			c->id = CMD_GROUPADD;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toGroupId);
			break;
		}
		case COMMAND_UNIT_GROUP_CLEAR:
		{
			SGroupClearUnitCommand* cmd = (SGroupClearUnitCommand*) sUnitCommandData;
			c->id = CMD_GROUPCLEAR;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;
			break;
		}
		case COMMAND_UNIT_REPAIR:
		{
			SRepairUnitCommand* cmd = (SRepairUnitCommand*) sUnitCommandData;
			c->id = CMD_REPAIR;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toRepairUnitId);
			break;
		}
		case COMMAND_UNIT_SET_FIRE_STATE:
		{
			SSetFireStateUnitCommand* cmd = (SSetFireStateUnitCommand*) sUnitCommandData;
			c->id = CMD_FIRE_STATE;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->fireState);
			break;
		}
		case COMMAND_UNIT_SET_MOVE_STATE:
		{
			SSetMoveStateUnitCommand* cmd = (SSetMoveStateUnitCommand*) sUnitCommandData;
			c->id = CMD_MOVE_STATE;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->moveState);
			break;
		}
		case COMMAND_UNIT_SET_BASE:
		{
			SSetBaseUnitCommand* cmd = (SSetBaseUnitCommand*) sUnitCommandData;
			c->id = CMD_SETBASE;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->basePos.x);
			c->params.push_back(cmd->basePos.y);
			c->params.push_back(cmd->basePos.z);
			break;
		}
		case COMMAND_UNIT_SELF_DESTROY:
		{
			SSelfDestroyUnitCommand* cmd = (SSelfDestroyUnitCommand*) sUnitCommandData;
			c->id = CMD_SELFD;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;
			break;
		}
		case COMMAND_UNIT_SET_WANTED_MAX_SPEED:
		{
			SSetWantedMaxSpeedUnitCommand* cmd = (SSetWantedMaxSpeedUnitCommand*) sUnitCommandData;
			c->id = CMD_SET_WANTED_MAX_SPEED;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->wantedMaxSpeed);
			break;
		}
		case COMMAND_UNIT_LOAD_UNITS:
		{
			SLoadUnitsUnitCommand* cmd = (SLoadUnitsUnitCommand*) sUnitCommandData;
			c->id = CMD_LOAD_UNITS;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			for (int i = 0; i < cmd->numToLoadUnits; ++i) {
				c->params.push_back(cmd->toLoadUnitIds[i]);
			}
			break;
		}
		case COMMAND_UNIT_LOAD_UNITS_AREA:
		{
			SLoadUnitsAreaUnitCommand* cmd = (SLoadUnitsAreaUnitCommand*) sUnitCommandData;
			c->id = CMD_LOAD_UNITS;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->pos.x);
			c->params.push_back(cmd->pos.y);
			c->params.push_back(cmd->pos.z);
			c->params.push_back(cmd->radius);
			break;
		}
		case COMMAND_UNIT_LOAD_ONTO:
		{
			SLoadOntoUnitCommand* cmd = (SLoadOntoUnitCommand*) sUnitCommandData;
			c->id = CMD_LOAD_ONTO;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->transporterUnitId);
			break;
		}
		case COMMAND_UNIT_UNLOAD_UNITS_AREA:
		{
			SUnloadUnitsAreaUnitCommand* cmd = (SUnloadUnitsAreaUnitCommand*) sUnitCommandData;
			c->id = CMD_UNLOAD_UNITS;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toPos.x);
			c->params.push_back(cmd->toPos.y);
			c->params.push_back(cmd->toPos.z);
			c->params.push_back(cmd->radius);
			break;
		}
		case COMMAND_UNIT_UNLOAD_UNIT:
		{
			SUnloadUnitCommand* cmd = (SUnloadUnitCommand*) sUnitCommandData;
			c->id = CMD_UNLOAD_UNIT;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toPos.x);
			c->params.push_back(cmd->toPos.y);
			c->params.push_back(cmd->toPos.z);
			c->params.push_back(cmd->toUnloadUnitId);
			break;
		}
		case COMMAND_UNIT_SET_ON_OFF:
		{
			SSetOnOffUnitCommand* cmd = (SSetOnOffUnitCommand*) sUnitCommandData;
			c->id = CMD_ONOFF;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->on);
			break;
		}
		case COMMAND_UNIT_RECLAIM:
		{
			SReclaimUnitCommand* cmd = (SReclaimUnitCommand*) sUnitCommandData;
			c->id = CMD_RECLAIM;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toReclaimUnitIdOrFeatureId);
			break;
		}
		case COMMAND_UNIT_RECLAIM_AREA:
		{
			SReclaimAreaUnitCommand* cmd = (SReclaimAreaUnitCommand*) sUnitCommandData;
			c->id = CMD_RECLAIM;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->pos.x);
			c->params.push_back(cmd->pos.y);
			c->params.push_back(cmd->pos.z);
			c->params.push_back(cmd->radius);
			break;
		}
		case COMMAND_UNIT_CLOAK:
		{
			SCloakUnitCommand* cmd = (SCloakUnitCommand*) sUnitCommandData;
			c->id = CMD_CLOAK;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->cloak);
			break;
		}
		case COMMAND_UNIT_STOCKPILE:
		{
			SStockpileUnitCommand* cmd = (SStockpileUnitCommand*) sUnitCommandData;
			c->id = CMD_STOCKPILE;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;
			break;
		}
		case COMMAND_UNIT_D_GUN:
		{
			SDGunUnitCommand* cmd = (SDGunUnitCommand*) sUnitCommandData;
			c->id = CMD_DGUN;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toAttackUnitId);
			break;
		}
		case COMMAND_UNIT_D_GUN_POS:
		{
			SDGunPosUnitCommand* cmd = (SDGunPosUnitCommand*) sUnitCommandData;
			c->id = CMD_DGUN;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->pos.x);
			c->params.push_back(cmd->pos.y);
			c->params.push_back(cmd->pos.z);
			break;
		}
		case COMMAND_UNIT_RESTORE_AREA:
		{
			SRestoreAreaUnitCommand* cmd = (SRestoreAreaUnitCommand*) sUnitCommandData;
			c->id = CMD_RESTORE;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->pos.x);
			c->params.push_back(cmd->pos.y);
			c->params.push_back(cmd->pos.z);
			c->params.push_back(cmd->radius);
			break;
		}
		case COMMAND_UNIT_SET_REPEAT:
		{
			SSetRepeatUnitCommand* cmd = (SSetRepeatUnitCommand*) sUnitCommandData;
			c->id = CMD_REPEAT;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->repeat);
			break;
		}
		case COMMAND_UNIT_SET_TRAJECTORY:
		{
			SSetTrajectoryUnitCommand* cmd = (SSetTrajectoryUnitCommand*) sUnitCommandData;
			c->id = CMD_TRAJECTORY;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->trajectory);
			break;
		}
		case COMMAND_UNIT_RESURRECT:
		{
			SResurrectUnitCommand* cmd = (SResurrectUnitCommand*) sUnitCommandData;
			c->id = CMD_RESURRECT;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toResurrectFeatureId);
			break;
		}
		case COMMAND_UNIT_RESURRECT_AREA:
		{
			SResurrectAreaUnitCommand* cmd = (SResurrectAreaUnitCommand*) sUnitCommandData;
			c->id = CMD_RESURRECT;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->pos.x);
			c->params.push_back(cmd->pos.y);
			c->params.push_back(cmd->pos.z);
			c->params.push_back(cmd->radius);
			break;
		}
		case COMMAND_UNIT_CAPTURE:
		{
			SCaptureUnitCommand* cmd = (SCaptureUnitCommand*) sUnitCommandData;
			c->id = CMD_CAPTURE;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->toCaptureUnitId);
			break;
		}
		case COMMAND_UNIT_CAPTURE_AREA:
		{
			SCaptureAreaUnitCommand* cmd = (SCaptureAreaUnitCommand*) sUnitCommandData;
			c->id = CMD_CAPTURE;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->pos.x);
			c->params.push_back(cmd->pos.y);
			c->params.push_back(cmd->pos.z);
			c->params.push_back(cmd->radius);
			break;
		}
		case COMMAND_UNIT_SET_AUTO_REPAIR_LEVEL:
		{
			SSetAutoRepairLevelUnitCommand* cmd = (SSetAutoRepairLevelUnitCommand*) sUnitCommandData;
			c->id = CMD_AUTOREPAIRLEVEL;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->autoRepairLevel);
			break;
		}
		case COMMAND_UNIT_SET_IDLE_MODE:
		{
			SSetIdleModeUnitCommand* cmd = (SSetIdleModeUnitCommand*) sUnitCommandData;
			c->id = CMD_IDLEMODE;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			c->params.push_back(cmd->idleMode);
			break;
		}
		case COMMAND_UNIT_CUSTOM:
		{
			SCustomUnitCommand* cmd = (SCustomUnitCommand*) sUnitCommandData;
			c->id = cmd->cmdId;
			c->options = cmd->options;
			c->timeOut = cmd->timeOut;

			for (int i = 0; i < cmd->numParams; ++i) {
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
