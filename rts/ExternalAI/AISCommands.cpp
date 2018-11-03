/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Interface/AISCommands.h"
#include "Sim/Units/CommandAI/Command.h"

#include <climits>
#include <cstdlib>

int toInternalUnitCommandTopic(int aiCmdTopic, const void* sUnitCommandData) {
	int internalUnitCommandTopic;

	switch (aiCmdTopic) {
		case COMMAND_UNIT_BUILD:
		{
			const SBuildUnitCommand* cmd = reinterpret_cast<const SBuildUnitCommand*>(sUnitCommandData);
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
			const SCustomUnitCommand* cmd = reinterpret_cast<const SCustomUnitCommand*>(sUnitCommandData);
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

	const int internalUnitCmdTopic = engineCmd->GetID();
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
			if (engineCmd->GetNumParams() < 3) {
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
		case CMD_LOAD_UNITS:
		{
			if (engineCmd->GetNumParams() < 3) {
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
			if (engineCmd->GetNumParams() < 3 || engineCmd->GetNumParams() == 5) {
				if (engineCmd->GetParam(0) < maxUnits) {
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
			if (engineCmd->GetNumParams() < 3) {
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
			if (engineCmd->GetNumParams() < 3) {
				aiCommandTopic = COMMAND_UNIT_RESURRECT;
			} else {
				aiCommandTopic = COMMAND_UNIT_RESURRECT_AREA;
			}
			break;
		}
		case CMD_CAPTURE:
		{
			if (engineCmd->GetNumParams() < 3 || engineCmd->GetNumParams() == 5) {
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

bool newCommand(void* sUnitCommandData, int sCommandId, int maxUnits, Command* c) {
	switch (sCommandId) {
		case COMMAND_UNIT_BUILD: {
			SBuildUnitCommand* cmd = static_cast<SBuildUnitCommand*>(sUnitCommandData);
			*c = Command(-cmd->toBuildUnitDefId, cmd->options);
			c->SetTimeOut(cmd->timeOut);

			if (cmd->buildPos_posF3 != nullptr)
				c->PushPos(cmd->buildPos_posF3);

			if (cmd->facing != UNIT_COMMAND_BUILD_NO_FACING)
				c->PushParam(cmd->facing);

		} break;

		case COMMAND_UNIT_STOP: {
			SStopUnitCommand* cmd = static_cast<SStopUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_STOP, cmd->options);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_WAIT: {
			SWaitUnitCommand* cmd = static_cast<SWaitUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_WAIT, cmd->options);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_WAIT_TIME: {
			STimeWaitUnitCommand* cmd = static_cast<STimeWaitUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_TIMEWAIT, cmd->options, cmd->time);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_WAIT_DEATH: {
			SDeathWaitUnitCommand* cmd = static_cast<SDeathWaitUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_DEATHWAIT, cmd->options, cmd->toDieUnitId);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_WAIT_SQUAD: {
			SSquadWaitUnitCommand* cmd = static_cast<SSquadWaitUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_SQUADWAIT, cmd->options, cmd->numUnits);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_WAIT_GATHER: {
			SGatherWaitUnitCommand* cmd = static_cast<SGatherWaitUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_GATHERWAIT, cmd->options);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_MOVE: {
			SMoveUnitCommand* cmd = static_cast<SMoveUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_MOVE, cmd->options);
			c->SetTimeOut(cmd->timeOut);
			c->PushPos(cmd->toPos_posF3);
		} break;

		case COMMAND_UNIT_PATROL: {
			SPatrolUnitCommand* cmd = static_cast<SPatrolUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_PATROL, cmd->options);
			c->SetTimeOut(cmd->timeOut);
			c->PushPos(cmd->toPos_posF3);
		} break;

		case COMMAND_UNIT_FIGHT: {
			SFightUnitCommand* cmd = static_cast<SFightUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_FIGHT, cmd->options);
			c->SetTimeOut(cmd->timeOut);
			c->PushPos(cmd->toPos_posF3);
		} break;

		case COMMAND_UNIT_ATTACK: {
			SAttackUnitCommand* cmd = static_cast<SAttackUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_ATTACK, cmd->options, cmd->toAttackUnitId);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_ATTACK_AREA: {
			SAttackAreaUnitCommand* cmd = static_cast<SAttackAreaUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_ATTACK, cmd->options);
			c->SetTimeOut(cmd->timeOut);
			c->PushPos(cmd->toAttackPos_posF3);
			c->PushParam(cmd->radius);
		} break;

		case COMMAND_UNIT_GUARD: {
			SGuardUnitCommand* cmd = static_cast<SGuardUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_GUARD, cmd->options, cmd->toGuardUnitId);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_GROUP_ADD: {
			SGroupAddUnitCommand* cmd = static_cast<SGroupAddUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_GROUPADD, cmd->options, cmd->toGroupId);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_GROUP_CLEAR: {
			SGroupClearUnitCommand* cmd = static_cast<SGroupClearUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_GROUPCLEAR, cmd->options);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_REPAIR: {
			SRepairUnitCommand* cmd = static_cast<SRepairUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_REPAIR, cmd->options, cmd->toRepairUnitId);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_SET_FIRE_STATE: {
			SSetFireStateUnitCommand* cmd = static_cast<SSetFireStateUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_FIRE_STATE, cmd->options, cmd->fireState);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_SET_MOVE_STATE: {
			SSetMoveStateUnitCommand* cmd = static_cast<SSetMoveStateUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_MOVE_STATE, cmd->options, cmd->moveState);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_SET_BASE: {
			SSetBaseUnitCommand* cmd = static_cast<SSetBaseUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_SETBASE, cmd->options);
			c->SetTimeOut(cmd->timeOut);
			c->PushPos(cmd->basePos_posF3);
		} break;

		case COMMAND_UNIT_SELF_DESTROY: {
			SSelfDestroyUnitCommand* cmd = static_cast<SSelfDestroyUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_SELFD, cmd->options);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_LOAD_UNITS: {
			SLoadUnitsUnitCommand* cmd = static_cast<SLoadUnitsUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_LOAD_UNITS, cmd->options);
			c->SetTimeOut(cmd->timeOut);

			for (int i = 0; i < cmd->toLoadUnitIds_size; ++i) {
				c->PushParam(cmd->toLoadUnitIds[i]);
			}
		} break;

		case COMMAND_UNIT_LOAD_UNITS_AREA: {
			SLoadUnitsAreaUnitCommand* cmd = static_cast<SLoadUnitsAreaUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_LOAD_UNITS, cmd->options);
			c->SetTimeOut(cmd->timeOut);
			c->PushPos(cmd->pos_posF3);
			c->PushParam(cmd->radius);
		} break;

		case COMMAND_UNIT_LOAD_ONTO: {
			SLoadOntoUnitCommand* cmd = static_cast<SLoadOntoUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_LOAD_ONTO, cmd->options, cmd->transporterUnitId);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_UNLOAD_UNITS_AREA: {
			SUnloadUnitsAreaUnitCommand* cmd = static_cast<SUnloadUnitsAreaUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_UNLOAD_UNITS, cmd->options);
			c->SetTimeOut(cmd->timeOut);
			c->PushPos(cmd->toPos_posF3);
			c->PushParam(cmd->radius);
		} break;

		case COMMAND_UNIT_UNLOAD_UNIT: {
			SUnloadUnitCommand* cmd = static_cast<SUnloadUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_UNLOAD_UNIT, cmd->options);
			c->SetTimeOut(cmd->timeOut);
			c->PushPos(cmd->toPos_posF3);
			c->PushParam(cmd->toUnloadUnitId);
		} break;

		case COMMAND_UNIT_SET_ON_OFF: {
			SSetOnOffUnitCommand* cmd = static_cast<SSetOnOffUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_ONOFF, cmd->options, cmd->on ? 1 : 0);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_RECLAIM_UNIT: {
			SReclaimUnitUnitCommand* cmd = static_cast<SReclaimUnitUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_RECLAIM, cmd->options, cmd->toReclaimUnitId);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_RECLAIM_FEATURE: {
			SReclaimFeatureUnitCommand* cmd = static_cast<SReclaimFeatureUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_RECLAIM, cmd->options, maxUnits + cmd->toReclaimFeatureId);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_RECLAIM_AREA: {
			SReclaimAreaUnitCommand* cmd = static_cast<SReclaimAreaUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_RECLAIM, cmd->options);
			c->SetTimeOut(cmd->timeOut);
			c->PushPos(cmd->pos_posF3);
			c->PushParam(cmd->radius);
		} break;

		case COMMAND_UNIT_CLOAK: {
			SCloakUnitCommand* cmd = static_cast<SCloakUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_CLOAK, cmd->options, cmd->cloak ? 1 : 0);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_STOCKPILE: {
			SStockpileUnitCommand* cmd = static_cast<SStockpileUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_STOCKPILE, cmd->options);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_D_GUN: {
			// FIXME
			SDGunUnitCommand* cmd = static_cast<SDGunUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_MANUALFIRE, cmd->options, cmd->toAttackUnitId);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_D_GUN_POS: {
			// FIXME
			SDGunPosUnitCommand* cmd = static_cast<SDGunPosUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_MANUALFIRE, cmd->options);
			c->SetTimeOut(cmd->timeOut);
			c->PushPos(cmd->pos_posF3);
		} break;

		case COMMAND_UNIT_RESTORE_AREA: {
			SRestoreAreaUnitCommand* cmd = static_cast<SRestoreAreaUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_RESTORE, cmd->options);
			c->SetTimeOut(cmd->timeOut);
			c->PushPos(cmd->pos_posF3);
			c->PushParam(cmd->radius);
		} break;

		case COMMAND_UNIT_SET_REPEAT: {
			SSetRepeatUnitCommand* cmd = static_cast<SSetRepeatUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_REPEAT, cmd->options, cmd->repeat ? 1 : 0);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_SET_TRAJECTORY: {
			SSetTrajectoryUnitCommand* cmd = static_cast<SSetTrajectoryUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_TRAJECTORY, cmd->options, cmd->trajectory);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_RESURRECT: {
			SResurrectUnitCommand* cmd = static_cast<SResurrectUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_RESURRECT, cmd->options, cmd->toResurrectFeatureId);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_RESURRECT_AREA: {
			SResurrectAreaUnitCommand* cmd = static_cast<SResurrectAreaUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_RESURRECT, cmd->options);
			c->SetTimeOut(cmd->timeOut);
			c->PushPos(cmd->pos_posF3);
			c->PushParam(cmd->radius);
		} break;

		case COMMAND_UNIT_CAPTURE: {
			SCaptureUnitCommand* cmd = static_cast<SCaptureUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_CAPTURE, cmd->options, cmd->toCaptureUnitId);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_CAPTURE_AREA: {
			SCaptureAreaUnitCommand* cmd = static_cast<SCaptureAreaUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_CAPTURE, cmd->options);
			c->SetTimeOut(cmd->timeOut);
			c->PushPos(cmd->pos_posF3);
			c->PushParam(cmd->radius);
		} break;

		case COMMAND_UNIT_SET_AUTO_REPAIR_LEVEL: {
			SSetAutoRepairLevelUnitCommand* cmd = static_cast<SSetAutoRepairLevelUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_AUTOREPAIRLEVEL, cmd->options, cmd->autoRepairLevel);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_SET_IDLE_MODE: {
			SSetIdleModeUnitCommand* cmd = static_cast<SSetIdleModeUnitCommand*>(sUnitCommandData);
			*c = Command(CMD_IDLEMODE, cmd->options, cmd->idleMode);
			c->SetTimeOut(cmd->timeOut);
		} break;

		case COMMAND_UNIT_CUSTOM: {
			SCustomUnitCommand* cmd = static_cast<SCustomUnitCommand*>(sUnitCommandData);
			*c = Command(cmd->cmdId, cmd->options);
			c->SetTimeOut(cmd->timeOut);

			for (int i = 0; i < cmd->params_size; ++i) {
				c->PushParam(cmd->params[i]);
			}
		} break;

		default: {
			return false;
		} break;
	}

	return true;
}

