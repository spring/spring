/*
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

#include "SAICallback.h"

#include "IGlobalAICallback.h"
#include "IAICallback.h"
#include "IAICheats.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "AISCommands.h"


IGlobalAICallback* team_globalCallback[MAX_AIS];
IAICallback* team_callback[MAX_AIS];
bool team_cheatingEnabled[MAX_AIS];
IAICheats* team_cheatCallback[MAX_AIS];

int fillCMap(const std::map<std::string,std::string>* map, const char* cMap[][2]) {
	std::map<std::string,std::string>::const_iterator it;
	int i;
	for (i=0, it=map->begin(); it != map->end(); ++i, it++) {
		cMap[i][0] = it->first.c_str();
		cMap[i][1] = it->second.c_str();
	}
	return i;
}
void toFloatArr(const SAIFloat3* color, float alpha, float arrColor[4]) {
	arrColor[0] = color->x;
	arrColor[1] = color->y;
	arrColor[2] = color->z;
	arrColor[3] = alpha;
}
void fillVector(std::vector<int>* vector_unitIds, int* unitIds, int numUnitIds) {
	for (int i=0; i < numUnitIds; ++i) {
		vector_unitIds->push_back(unitIds[i]);
	}
}

const UnitDef* getUnitDefById(int teamId, int unitDefId) {
	AIHCGetUnitDefById cmd = {unitDefId, NULL};
	int ret = team_callback[teamId]->HandleCommand(AIHCGetUnitDefByIdId, &cmd);
	if (ret == 1) {
		return cmd.ret;
	} else {
		return NULL;
	}
}
const WeaponDef* getWeaponDefById(int teamId, int weaponDefId) {
	AIHCGetWeaponDefById cmd = {weaponDefId, NULL};
	int ret = team_callback[teamId]->HandleCommand(AIHCGetWeaponDefByIdId, &cmd);
	if (ret == 1) {
		return cmd.ret;
	} else {
		return NULL;
	}
}
const FeatureDef* getFeatureDefById(int teamId, int featureDefId) {
	AIHCGetFeatureDefById cmd = {featureDefId, NULL};
	int ret = team_callback[teamId]->HandleCommand(AIHCGetFeatureDefByIdId, &cmd);
	if (ret == 1) {
		return cmd.ret;
	} else {
		return NULL;
	}
}

int wrapper_HandleCommand(IAICallback* clb, IAICheats* clbCheat, int cmdId, void* cmdData) {
	
	int ret;
	
	if (clbCheat != NULL) {
		ret = clbCheat->HandleCommand(cmdId, cmdData);
	} else {
		ret = clb->HandleCommand(cmdId, cmdData);
	}
	
	return ret;
}

int _handleCommand(int teamId, int toId, int commandId, int commandTopic, void* commandData) {

	int ret = 0;

	IAICallback* clb = team_callback[teamId];
	IAICheats* clbCheat = team_cheatCallback[teamId]; // if this is != NULL, cheating is enabled

		
	switch (commandTopic) {
			
		case COMMAND_CHEATS_SET_MY_HANDICAP:
		{
			SSetMyHandicapCheatCommand* cmd = (SSetMyHandicapCheatCommand*) commandData;
			if (clbCheat != NULL) {
				clbCheat->SetMyHandicap(cmd->handicap);
				ret = 0;
			} else {
				ret = -1;
			}
			break;
		}
		case COMMAND_CHEATS_GIVE_ME_METAL:
		{
			SGiveMeMetalCheatCommand* cmd = (SGiveMeMetalCheatCommand*) commandData;
			if (clbCheat != NULL) {
				clbCheat->GiveMeMetal(cmd->amount);
				ret = 0;
			} else {
				ret = -1;
			}
			break;
		}
		case COMMAND_CHEATS_GIVE_ME_ENERGY:
		{
			SGiveMeEnergyCheatCommand* cmd = (SGiveMeEnergyCheatCommand*) commandData;
			if (clbCheat != NULL) {
				clbCheat->GiveMeEnergy(cmd->amount);
				ret = 0;
			} else {
				ret = -1;
			}
			break;
		}
		case COMMAND_CHEATS_GIVE_ME_NEW_UNIT:
		{
			SGiveMeNewUnitCheatCommand* cmd = (SGiveMeNewUnitCheatCommand*) commandData;
			if (clbCheat != NULL) {
				cmd->ret_newUnitId = clbCheat->CreateUnit(getUnitDefById(teamId, cmd->unitDefId)->name.c_str(), float3(cmd->pos));
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
			SRemovePointDrawCommand* cmd = (SRemovePointDrawCommand*) commandData;
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
			SSendTextMessageCommand* cmd = (SSendTextMessageCommand*) commandData;
			clb->SendTextMsg(cmd->text, cmd->zone);
			break;
		}
		case COMMAND_SET_LAST_POS_MESSAGE:
		{
			SSetLastPosMessageCommand* cmd = (SSetLastPosMessageCommand*) commandData;
			clb->SetLastMsgPos(cmd->pos);
			break;
		}
		case COMMAND_SEND_RESOURCES:
		{
			SSendResourcesCommand* cmd = (SSendResourcesCommand*) commandData;
			cmd->ret_isExecuted = clb->SendResources(cmd->mAmount, cmd->eAmount, cmd->receivingTeam);
			break;
		}

		case COMMAND_SEND_UNITS:
		{
			SSendUnitsCommand* cmd = (SSendUnitsCommand*) commandData;
			std::vector<int> vector_unitIds;
			fillVector(&vector_unitIds, cmd->unitIds, cmd->numUnitIds);
			cmd->ret_sentUnits = clb->SendUnits(vector_unitIds, cmd->receivingTeam);
			break;
		}

		case COMMAND_SHARED_MEM_AREA_CREATE:
		{
			SCreateSharedMemAreaCommand* cmd = (SCreateSharedMemAreaCommand*) commandData;
			cmd->ret_sharedMemArea = clb->CreateSharedMemArea(cmd->name, cmd->size);
			break;
		}
		case COMMAND_SHARED_MEM_AREA_RELEASE:
		{
			SReleaseSharedMemAreaCommand* cmd = (SReleaseSharedMemAreaCommand*) commandData;
			clb->ReleasedSharedMemArea(cmd->name);
			break;
		}

		case COMMAND_GROUP_CREATE:
		{
			SCreateGroupCommand* cmd = (SCreateGroupCommand*) commandData;
			cmd->ret_groupId = clb->CreateGroup(cmd->libraryName, cmd->aiNumber);
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
			cmd->ret_isExecuted = clb->AddUnitToGroup(cmd->unitId, cmd->groupId);
			break;
		}
		case COMMAND_GROUP_REMOVE_UNIT:
		{
			SRemoveUnitFromGroupCommand* cmd = (SRemoveUnitFromGroupCommand*) commandData;
			cmd->ret_isExecuted = clb->RemoveUnitFromGroup(cmd->unitId);
			break;
		}
		case COMMAND_PATH_INIT:
		{
			SInitPathCommand* cmd = (SInitPathCommand*) commandData;
			cmd->ret_pathId = clb->InitPath(float3(cmd->start), float3(cmd->end), cmd->pathType);
			break;
		}
		case COMMAND_PATH_GET_APPROXIMATE_LENGTH:
		{
			SGetApproximateLengthPathCommand* cmd = (SGetApproximateLengthPathCommand*) commandData;
			cmd->ret_approximatePathLength = clb->GetPathLength(float3(cmd->start), float3(cmd->end), cmd->pathType);
			break;
		}
		case COMMAND_PATH_GET_NEXT_WAYPOINT:
		{
			SGetNextWaypointPathCommand* cmd = (SGetNextWaypointPathCommand*) commandData;
			cmd->ret_nextWaypoint = clb->GetNextWaypoint(cmd->pathId).toSAIFloat3();
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
			cmd->ret_outData = clb->CallLuaRules(cmd->data, cmd->inSize, cmd->outSize);
			break;
		}


		case COMMAND_DRAWER_ADD_NOTIFICATION:
		{
			SAddNotificationDrawerCommand* cmd = (SAddNotificationDrawerCommand*) commandData;
			clb->AddNotification(float3(cmd->pos), float3(cmd->color), cmd->alpha);
			break;
		}
		case COMMAND_DRAWER_PATH_START:
		{
			SStartPathDrawerCommand* cmd = (SStartPathDrawerCommand*) commandData;
			float arrColor[4];
			toFloatArr(&cmd->color, cmd->alpha, arrColor);
			clb->LineDrawerStartPath(float3(cmd->pos), arrColor);
			break;
		}
		case COMMAND_DRAWER_PATH_FINISH:
		{
			//SFinishPathDrawerCommand* cmd = (SFinishPathDrawerCommand*) commandData;
			clb->LineDrawerFinishPath();
			break;
		}
		case COMMAND_DRAWER_PATH_DRAW_LINE:
		{
			SDrawLinePathDrawerCommand* cmd = (SDrawLinePathDrawerCommand*) commandData;
			float arrColor[4];
			toFloatArr(&cmd->color, cmd->alpha, arrColor);
			clb->LineDrawerDrawLine(float3(cmd->endPos), arrColor);
			break;
		}
		case COMMAND_DRAWER_PATH_DRAW_LINE_AND_ICON:
		{
			SDrawLineAndIconPathDrawerCommand* cmd = (SDrawLineAndIconPathDrawerCommand*) commandData;
			float arrColor[4];
			toFloatArr(&cmd->color, cmd->alpha, arrColor);
			clb->LineDrawerDrawLineAndIcon(cmd->cmdId, float3(cmd->endPos), arrColor);
			break;
		}
		case COMMAND_DRAWER_PATH_DRAW_ICON_AT_LAST_POS:
		{
			SDrawIconAtLastPosPathDrawerCommand* cmd = (SDrawIconAtLastPosPathDrawerCommand*) commandData;
			clb->LineDrawerDrawIconAtLastPos(cmd->cmdId);
			break;
		}
		case COMMAND_DRAWER_PATH_BREAK:
		{
			SBreakPathDrawerCommand* cmd = (SBreakPathDrawerCommand*) commandData;
			float arrColor[4];
			toFloatArr(&cmd->color, cmd->alpha, arrColor);
			clb->LineDrawerBreak(float3(cmd->endPos), arrColor);
			break;
		}
		case COMMAND_DRAWER_PATH_RESTART:
		{
			SRestartPathDrawerCommand* cmd = (SRestartPathDrawerCommand*) commandData;
			if (cmd->sameColor) {
				clb->LineDrawerRestartSameColor();
			} else {
				clb->LineDrawerRestart();
			}
			break;
		}
		case COMMAND_DRAWER_FIGURE_CREATE_SPLINE:
		{
			SCreateSplineFigureDrawerCommand* cmd = (SCreateSplineFigureDrawerCommand*) commandData;
			cmd->ret_newFigureGroupId = clb->CreateSplineFigure(float3(cmd->pos1), float3(cmd->pos2), float3(cmd->pos3), float3(cmd->pos4), cmd->width, cmd->arrow, cmd->lifeTime, cmd->figureGroupId);
			break;
		}
		case COMMAND_DRAWER_FIGURE_CREATE_LINE:
		{
			SCreateLineFigureDrawerCommand* cmd = (SCreateLineFigureDrawerCommand*) commandData;
			cmd->ret_newFigureGroupId = clb->CreateLineFigure(float3(cmd->pos1), float3(cmd->pos2), cmd->width, cmd->arrow, cmd->lifeTime, cmd->figureGroupId);
			break;
		}
		case COMMAND_DRAWER_FIGURE_SET_COLOR:
		{
			SSetColorFigureDrawerCommand* cmd = (SSetColorFigureDrawerCommand*) commandData;
			clb->SetFigureColor(cmd->figureGroupId, cmd->color.x, cmd->color.y, cmd->color.z, cmd->alpha);
			break;
		}
		case COMMAND_DRAWER_FIGURE_DELETE:
		{
			SDeleteFigureDrawerCommand* cmd = (SDeleteFigureDrawerCommand*) commandData;
			clb->DeleteFigureGroup(cmd->figureGroupId);
			break;
		}
		case COMMAND_DRAWER_DRAW_UNIT:
		{
			SDrawUnitDrawerCommand* cmd = (SDrawUnitDrawerCommand*) commandData;
			clb->DrawUnit(getUnitDefById(teamId, cmd->toDrawUnitDefId)->name.c_str(), float3(cmd->pos), cmd->rotation, cmd->lifeTime, cmd->teamId, cmd->transparent, cmd->drawBorder, cmd->facing);
			break;
		}

/*
		case COMMAND_UNIT_BUILD:
		{
			SBuildUnitCommand* cmd = (SBuildUnitCommand*) commandData;
			Command c;
			c.id = -cmd->toBuildUnitDefId;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->buildPos.x);
			c.params.push_back(cmd->buildPos.y);
			c.params.push_back(cmd->buildPos.z);
			if (cmd->facing != UNIT_COMMAND_BUILD_NO_FACING) c.params.push_back(cmd->facing);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_STOP:
		{
			SStopUnitCommand* cmd = (SStopUnitCommand*) commandData;
			Command c;
			c.id = CMD_STOP;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
//		case COMMAND_UNIT_Insert:
//		{
//			SInsertUnitCommand* cmd = (SInsertUnitCommand*) commandData;
//			Command c;
//			c.id = CMD_Insert;
//			c.options = cmd->options;
//			c.timeOut = cmd->timeOut;
//			c.params.push_back(cmd->toPos.x);
//			c.params.push_back(cmd->toPos.y);
//			c.params.push_back(cmd->toPos.z);
//			if (cmd->unitId >= 0) {
//				ret = clb->GiveOrder(cmd->unitId, &c);
//			} else {
//				ret = clb->GiveGroupOrder(cmd->groupId, &c);
//			}
//			break;
//		}
//		case COMMAND_UNIT_Remove:
//		{
//			SRemoveUnitCommand* cmd = (SRemoveUnitCommand*) commandData;
//			Command c;
//			c.id = CMD_Remove;
//			c.options = cmd->options;
//			c.timeOut = cmd->timeOut;
//			c.params.push_back(cmd->toPos.x);
//			c.params.push_back(cmd->toPos.y);
//			c.params.push_back(cmd->toPos.z);
//			if (cmd->unitId >= 0) {
//				ret = clb->GiveOrder(cmd->unitId, &c);
//			} else {
//				ret = clb->GiveGroupOrder(cmd->groupId, &c);
//			}
//			break;
//		}
		case COMMAND_UNIT_WAIT:
		{
			SWaitUnitCommand* cmd = (SWaitUnitCommand*) commandData;
			Command c;
			c.id = CMD_WAIT;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_WAIT_TIME:
		{
			STimeWaitUnitCommand* cmd = (STimeWaitUnitCommand*) commandData;
			Command c;
			c.id = CMD_WAITCODE_TIMEWAIT;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->time);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_WAIT_DEATH:
		{
			SDeathWaitUnitCommand* cmd = (SDeathWaitUnitCommand*) commandData;
			Command c;
			c.id = CMD_WAITCODE_DEATHWAIT;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->toDieUnitId);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_WAIT_SQUAD:
		{
			SSquadWaitUnitCommand* cmd = (SSquadWaitUnitCommand*) commandData;
			Command c;
			c.id = CMD_WAITCODE_SQUADWAIT;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->numUnits);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_WAIT_GATHER:
		{
			SGatherWaitUnitCommand* cmd = (SGatherWaitUnitCommand*) commandData;
			Command c;
			c.id = CMD_WAITCODE_GATHERWAIT;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_MOVE:
		{
			SMoveUnitCommand* cmd = (SMoveUnitCommand*) commandData;
			Command c;
			c.id = CMD_MOVE;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->toPos.x);
			c.params.push_back(cmd->toPos.y);
			c.params.push_back(cmd->toPos.z);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_PATROL:
		{
			SPatrolUnitCommand* cmd = (SPatrolUnitCommand*) commandData;
			Command c;
			c.id = CMD_PATROL;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->toPos.x);
			c.params.push_back(cmd->toPos.y);
			c.params.push_back(cmd->toPos.z);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_FIGHT:
		{
			SFightUnitCommand* cmd = (SFightUnitCommand*) commandData;
			Command c;
			c.id = CMD_FIGHT;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->toPos.x);
			c.params.push_back(cmd->toPos.y);
			c.params.push_back(cmd->toPos.z);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_ATTACK:
		{
			SAttackUnitCommand* cmd = (SAttackUnitCommand*) commandData;
			Command c;
			c.id = CMD_ATTACK;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->toAttackUnitId);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
//		case COMMAND_UNIT_AttackPos:
//		{
//			SAttackPosUnitCommand* cmd = (SAttackPosUnitCommand*) commandData;
//			Command c;
//			c.id = CMD_AttackPos;
//			c.options = cmd->options;
//			c.timeOut = cmd->timeOut;
//			c.params.push_back(cmd->toPos.x);
//			c.params.push_back(cmd->toPos.y);
//			c.params.push_back(cmd->toPos.z);
//			if (cmd->unitId >= 0) {
//				ret = clb->GiveOrder(cmd->unitId, &c);
//			} else {
//				ret = clb->GiveGroupOrder(cmd->groupId, &c);
//			}
//			break;
//		}
		case COMMAND_UNIT_ATTACK_AREA:
		{
			SAttackAreaUnitCommand* cmd = (SAttackAreaUnitCommand*) commandData;
			Command c;
			c.id = CMD_ATTACK;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->toAttackPos.x);
			c.params.push_back(cmd->toAttackPos.y);
			c.params.push_back(cmd->toAttackPos.z);
			c.params.push_back(cmd->radius);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_GUARD:
		{
			SGuardUnitCommand* cmd = (SGuardUnitCommand*) commandData;
			Command c;
			c.id = CMD_GUARD;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->toGuardUnitId);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_AI_SELECT:
		{
			SAiSelectUnitCommand* cmd = (SAiSelectUnitCommand*) commandData;
			Command c;
			c.id = CMD_AISELECT;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
//			c.params.push_back(cmd->toPos.x);
//			c.params.push_back(cmd->toPos.y);
//			c.params.push_back(cmd->toPos.z);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
//		case COMMAND_UNIT_GroupSelect:
//		{
//			SGroupSelectUnitCommand* cmd = (SGroupSelectUnitCommand*) commandData;
//			Command c;
//			c.id = CMD_GroupSelect;
//			c.options = cmd->options;
//			c.timeOut = cmd->timeOut;
//			c.params.push_back(cmd->toPos.x);
//			c.params.push_back(cmd->toPos.y);
//			c.params.push_back(cmd->toPos.z);
//			if (cmd->unitId >= 0) {
//				ret = clb->GiveOrder(cmd->unitId, &c);
//			} else {
//				ret = clb->GiveGroupOrder(cmd->groupId, &c);
//			}
//			break;
//		}
		case COMMAND_UNIT_GROUP_ADD:
		{
			SGroupAddUnitCommand* cmd = (SGroupAddUnitCommand*) commandData;
			Command c;
			c.id = CMD_GROUPADD;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->toGroupId);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_GROUP_CLEAR:
		{
			SGroupClearUnitCommand* cmd = (SGroupClearUnitCommand*) commandData;
			Command c;
			c.id = CMD_GROUPCLEAR;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_REPAIR:
		{
			SRepairUnitCommand* cmd = (SRepairUnitCommand*) commandData;
			Command c;
			c.id = CMD_REPAIR;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->toRepairUnitId);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_SET_FIRE_STATE:
		{
			SSetFireStateUnitCommand* cmd = (SSetFireStateUnitCommand*) commandData;
			Command c;
			c.id = CMD_FIRE_STATE;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->fireState);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_SET_MOVE_STATE:
		{
			SSetMoveStateUnitCommand* cmd = (SSetMoveStateUnitCommand*) commandData;
			Command c;
			c.id = CMD_MOVE_STATE;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->moveState);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_SET_BASE:
		{
			SSetBaseUnitCommand* cmd = (SSetBaseUnitCommand*) commandData;
			Command c;
			c.id = CMD_SETBASE;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->basePos.x);
			c.params.push_back(cmd->basePos.y);
			c.params.push_back(cmd->basePos.z);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
//		case COMMAND_UNIT_Internal:
//		{
//			SInternalUnitCommand* cmd = (SInternalUnitCommand*) commandData;
//			Command c;
//			c.id = CMD_Internal;
//			c.options = cmd->options;
//			c.timeOut = cmd->timeOut;
//			c.params.push_back(cmd->toPos.x);
//			c.params.push_back(cmd->toPos.y);
//			c.params.push_back(cmd->toPos.z);
//			if (cmd->unitId >= 0) {
//				ret = clb->GiveOrder(cmd->unitId, &c);
//			} else {
//				ret = clb->GiveGroupOrder(cmd->groupId, &c);
//			}
//			break;
//		}
		case COMMAND_UNIT_SELF_DESTROY:
		{
			SSelfDestroyUnitCommand* cmd = (SSelfDestroyUnitCommand*) commandData;
			Command c;
			c.id = CMD_SELFD;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_SET_WANTED_MAX_SPEED:
		{
			SSetWantedMaxSpeedUnitCommand* cmd = (SSetWantedMaxSpeedUnitCommand*) commandData;
			Command c;
			c.id = CMD_SET_WANTED_MAX_SPEED;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->wantedMaxSpeed);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_LOAD_UNITS:
		{
			SLoadUnitsUnitCommand* cmd = (SLoadUnitsUnitCommand*) commandData;
			Command c;
			c.id = CMD_LOAD_UNITS;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			for (int i=0; i < cmd->numToLoadUnits; ++i) {
				c.params.push_back(cmd->toLoadUnitIds[i]);
			}
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_LOAD_UNITS_AREA:
		{
			SLoadUnitsAreaUnitCommand* cmd = (SLoadUnitsAreaUnitCommand*) commandData;
			Command c;
			c.id = CMD_LOAD_UNITS;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->pos.x);
			c.params.push_back(cmd->pos.y);
			c.params.push_back(cmd->pos.z);
			c.params.push_back(cmd->radius);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_LOAD_ONTO:
		{
			SLoadOntoUnitCommand* cmd = (SLoadOntoUnitCommand*) commandData;
			Command c;
			c.id = CMD_LOAD_ONTO;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->transporterUnitId);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_UNLOAD_UNITS_AREA:
		{
			SUnloadUnitsAreaUnitCommand* cmd = (SUnloadUnitsAreaUnitCommand*) commandData;
			Command c;
			c.id = CMD_UNLOAD_UNITS;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->toPos.x);
			c.params.push_back(cmd->toPos.y);
			c.params.push_back(cmd->toPos.z);
			c.params.push_back(cmd->radius);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_UNLOAD_UNIT:
		{
			SUnloadUnitCommand* cmd = (SUnloadUnitCommand*) commandData;
			Command c;
			c.id = CMD_UNLOAD_UNIT;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->toPos.x);
			c.params.push_back(cmd->toPos.y);
			c.params.push_back(cmd->toPos.z);
			c.params.push_back(cmd->toUnloadUnitId);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_SET_ON_OFF:
		{
			SSetOnOffUnitCommand* cmd = (SSetOnOffUnitCommand*) commandData;
			Command c;
			c.id = CMD_ONOFF;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->on);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_RECLAIM:
		{
			SReclaimUnitCommand* cmd = (SReclaimUnitCommand*) commandData;
			Command c;
			c.id = CMD_RECLAIM;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->toReclaimUnitIdOrFeatureId);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_RECLAIM_AREA:
		{
			SReclaimAreaUnitCommand* cmd = (SReclaimAreaUnitCommand*) commandData;
			Command c;
			c.id = CMD_RECLAIM;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->pos.x);
			c.params.push_back(cmd->pos.y);
			c.params.push_back(cmd->pos.z);
			c.params.push_back(cmd->radius);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_CLOAK:
		{
			SCloakUnitCommand* cmd = (SCloakUnitCommand*) commandData;
			Command c;
			c.id = CMD_CLOAK;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->cloak);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_STOCKPILE:
		{
			SStockpileUnitCommand* cmd = (SStockpileUnitCommand*) commandData;
			Command c;
			c.id = CMD_STOCKPILE;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_D_GUN:
		{
			SDGunUnitCommand* cmd = (SDGunUnitCommand*) commandData;
			Command c;
			c.id = CMD_DGUN;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->toAttackUnitId);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_D_GUN_POS:
		{
			SDGunPosUnitCommand* cmd = (SDGunPosUnitCommand*) commandData;
			Command c;
			c.id = CMD_DGUN;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->pos.x);
			c.params.push_back(cmd->pos.y);
			c.params.push_back(cmd->pos.z);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_RESTORE_AREA:
		{
			SRestoreAreaUnitCommand* cmd = (SRestoreAreaUnitCommand*) commandData;
			Command c;
			c.id = CMD_RESTORE;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->pos.x);
			c.params.push_back(cmd->pos.y);
			c.params.push_back(cmd->pos.z);
			c.params.push_back(cmd->radius);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_SET_REPEAT:
		{
			SSetRepeatUnitCommand* cmd = (SSetRepeatUnitCommand*) commandData;
			Command c;
			c.id = CMD_REPEAT;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->repeat);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_SET_TRAJECTORY:
		{
			SSetTrajectoryUnitCommand* cmd = (SSetTrajectoryUnitCommand*) commandData;
			Command c;
			c.id = CMD_TRAJECTORY;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->trajectory);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_RESURRECT:
		{
			SResurrectUnitCommand* cmd = (SResurrectUnitCommand*) commandData;
			Command c;
			c.id = CMD_RESURRECT;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->toResurrectFeatureId);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_RESURRECT_AREA:
		{
			SResurrectAreaUnitCommand* cmd = (SResurrectAreaUnitCommand*) commandData;
			Command c;
			c.id = CMD_RESURRECT;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->pos.x);
			c.params.push_back(cmd->pos.y);
			c.params.push_back(cmd->pos.z);
			c.params.push_back(cmd->radius);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_CAPTURE:
		{
			SCaptureUnitCommand* cmd = (SCaptureUnitCommand*) commandData;
			Command c;
			c.id = CMD_CAPTURE;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->toCaptureUnitId);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_CAPTURE_AREA:
		{
			SCaptureAreaUnitCommand* cmd = (SCaptureAreaUnitCommand*) commandData;
			Command c;
			c.id = CMD_CAPTURE;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->pos.x);
			c.params.push_back(cmd->pos.y);
			c.params.push_back(cmd->pos.z);
			c.params.push_back(cmd->radius);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_SET_AUTO_REPAIR_LEVEL:
		{
			SSetAutoRepairLevelUnitCommand* cmd = (SSetAutoRepairLevelUnitCommand*) commandData;
			Command c;
			c.id = CMD_AUTOREPAIRLEVEL;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->autoRepairLevel);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
//		case COMMAND_UNIT_AttackLoopback:
//		{
//			SAttackLoopbackUnitCommand* cmd = (SAttackLoopbackUnitCommand*) commandData;
//			Command c;
//			c.id = CMD_AttackLoopback;
//			c.options = cmd->options;
//			c.timeOut = cmd->timeOut;
//			c.params.push_back(cmd->toPos.x);
//			c.params.push_back(cmd->toPos.y);
//			c.params.push_back(cmd->toPos.z);
//			if (cmd->unitId >= 0) {
//				ret = clb->GiveOrder(cmd->unitId, &c);
//			} else {
//				ret = clb->GiveGroupOrder(cmd->groupId, &c);
//			}
//			break;
//		}
		case COMMAND_UNIT_SET_IDLE_MODE:
		{
			SSetIdleModeUnitCommand* cmd = (SSetIdleModeUnitCommand*) commandData;
			Command c;
			c.id = CMD_IDLEMODE;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			c.params.push_back(cmd->idleMode);
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
		case COMMAND_UNIT_CUSTOM:
		{
			SCustomUnitCommand* cmd = (SCustomUnitCommand*) commandData;
			Command c;
			c.id = cmd->cmdId;
			c.options = cmd->options;
			c.timeOut = cmd->timeOut;
			
			for (int i=0; i < cmd->numParams; ++i) {
				c.params.push_back(cmd->params[i]);
			}
			
			if (cmd->unitId >= 0) {
				ret = clb->GiveOrder(cmd->unitId, &c);
			} else {
				ret = clb->GiveGroupOrder(cmd->groupId, &c);
			}
			break;
		}
*/
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
bool _Cheats_isEnabled(int teamId) {
	team_cheatCallback[teamId] = NULL;
	if (team_cheatingEnabled[teamId]) {
		team_cheatCallback[teamId] = team_globalCallback[teamId]->GetCheatInterface();
	}
	return team_cheatCallback[teamId] != NULL;
}

bool _Cheats_setEnabled(int teamId, bool enabled) {
//	bool isEnabled = _Cheats_isEnabled(teamId);
//	
//	if (enabled != isEnabled) {
//		if (enable) {
//			team_cheatCallback[teamId] = team_globalCallback[teamId]->GetCheatInterface();
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
	return enabled == _Cheats_isEnabled(teamId);
}

bool _Cheats_setEventsEnabled(int teamId, bool enabled) {
	if (_Cheats_isEnabled(teamId)) {
		team_cheatCallback[teamId]->EnableCheatEvents(enabled);
		return true;
	} else {
		return false;
	}
}

bool _Cheats_isOnlyPassive(int teamId) {
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->OnlyPassiveCheats();
	} else {
		return true; //TODO: is this correct?
	}
}

int _Game_getAiInterfaceVersion(int teamId) {
	IAICallback* clb = team_callback[teamId]; return wrapper_HandleCommand(clb, NULL, AIHCQuerySubVersionId, NULL);
}

bool _Map_isPosInCamera(int teamId, SAIFloat3 pos, float radius) {
	IAICallback* clb = team_callback[teamId]; return clb->PosInCamera(SAIFloat3(pos), radius);
}

int _Game_getCurrentFrame(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetCurrentFrame();
}

int _Game_getMyTeam(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMyTeam();
}

int _Game_getMyAllyTeam(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMyAllyTeam();
}

int _Game_getPlayerTeam(int teamId, int player) {
	IAICallback* clb = team_callback[teamId]; return clb->GetPlayerTeam(player);
}

const char* _Game_getTeamSide(int teamId, int team) {
	IAICallback* clb = team_callback[teamId]; return clb->GetTeamSide(team);
}





int _WeaponDef_STATIC_getNumDamageTypes(int teamId) {
    int numDamageTypes;
	IAICallback* clb = team_callback[teamId];
    bool fetchOk = clb->GetValue(AIVAL_NUMDAMAGETYPES, &numDamageTypes);
    if (!fetchOk) {
        numDamageTypes = -1;
    }
    return numDamageTypes;
}
unsigned int _Map_getChecksum(int teamId) {
    unsigned int checksum;
	IAICallback* clb = team_callback[teamId];
    bool fetchOk = clb->GetValue(AIVAL_MAP_CHECKSUM, &checksum);
    if (!fetchOk) {
        checksum = -1;
    }
    return checksum;
}

bool _Game_isExceptionHandlingEnabled(int teamId) {
    bool exceptionHandlingEnabled;
	IAICallback* clb = team_callback[teamId];
    bool fetchOk = clb->GetValue(AIVAL_EXCEPTION_HANDLING, &exceptionHandlingEnabled);
    if (!fetchOk) {
        exceptionHandlingEnabled = false;
    }
    return exceptionHandlingEnabled;
}
bool _Game_isDebugModeEnabled(int teamId) {
    bool debugModeEnabled;
	IAICallback* clb = team_callback[teamId];
    bool fetchOk = clb->GetValue(AIVAL_DEBUG_MODE, &debugModeEnabled);
    if (!fetchOk) {
        debugModeEnabled = false;
    }
    return debugModeEnabled;
}
int _Game_getMode(int teamId) {
    int mode;
	IAICallback* clb = team_callback[teamId];
    bool fetchOk = clb->GetValue(AIVAL_GAME_MODE, &mode);
    if (!fetchOk) {
        mode = -1;
    }
    return mode;
}
bool _Game_isPaused(int teamId) {
    bool paused;
	IAICallback* clb = team_callback[teamId];
    bool fetchOk = clb->GetValue(AIVAL_GAME_PAUSED, &paused);
    if (!fetchOk) {
        paused = false;
    }
    return paused;
}
float _Game_getSpeedFactor(int teamId) {
    float speedFactor;
	IAICallback* clb = team_callback[teamId];
    bool fetchOk = clb->GetValue(AIVAL_GAME_SPEED_FACTOR, &speedFactor);
    if (!fetchOk) {
        speedFactor = false;
    }
    return speedFactor;
}

float _Gui_getViewRange(int teamId) {
    float viewRange;
	IAICallback* clb = team_callback[teamId];
    bool fetchOk = clb->GetValue(AIVAL_GUI_VIEW_RANGE, &viewRange);
    if (!fetchOk) {
        viewRange = false;
    }
    return viewRange;
}
float _Gui_getScreenX(int teamId) {
    float screenX;
	IAICallback* clb = team_callback[teamId];
    bool fetchOk = clb->GetValue(AIVAL_GUI_SCREENX, &screenX);
    if (!fetchOk) {
        screenX = false;
    }
    return screenX;
}
float _Gui_getScreenY(int teamId) {
    float screenY;
	IAICallback* clb = team_callback[teamId];
    bool fetchOk = clb->GetValue(AIVAL_GUI_SCREENY, &screenY);
    if (!fetchOk) {
        screenY = false;
    }
    return screenY;
}
SAIFloat3 _Gui_Camera_getDirection(int teamId) {
    float3 cameraDir;
	IAICallback* clb = team_callback[teamId];
    bool fetchOk = clb->GetValue(AIVAL_GUI_CAMERA_DIR, &cameraDir);
    if (!fetchOk) {
        cameraDir = float3(1.0f, 0.0f, 0.0f);
    }
    return cameraDir.toSAIFloat3();
}
SAIFloat3 _Gui_Camera_getPosition(int teamId) {
    float3 cameraPosition;
	IAICallback* clb = team_callback[teamId];
    bool fetchOk = clb->GetValue(AIVAL_GUI_CAMERA_POS, &cameraPosition);
    if (!fetchOk) {
        cameraPosition = float3(1.0f, 0.0f, 0.0f);
    }
    return cameraPosition.toSAIFloat3();
}

bool _File_locateForReading(int teamId, char* filename) {
	IAICallback* clb = team_callback[teamId];
    return clb->GetValue(AIVAL_LOCATE_FILE_R, filename);
}
bool _File_locateForWriting(int teamId, char* filename) {
	IAICallback* clb = team_callback[teamId];
    return clb->GetValue(AIVAL_LOCATE_FILE_W, filename);
}

int _Unit_STATIC_getLimit(int teamId) {
    int unitLimit;
	IAICallback* clb = team_callback[teamId];
    bool fetchOk = clb->GetValue(AIVAL_UNIT_LIMIT, &unitLimit);
    if (!fetchOk) {
        unitLimit = -1;
    }
    return unitLimit;
}
const char* _Game_getSetupScript(int teamId) {
    const char* setupScript;
	IAICallback* clb = team_callback[teamId];
    bool fetchOk = clb->GetValue(AIVAL_SCRIPT, &setupScript);
    if (!fetchOk) {
        return NULL;
    }
    return setupScript;
}







//########### BEGINN UnitDef
int _UnitDef_STATIC_getIds(int teamId, int* list) {
	IAICallback* clb = team_callback[teamId];
	int numUnitDefs = clb->GetNumUnitDefs();
	const UnitDef** defList = (const UnitDef**) new UnitDef*[numUnitDefs];
	clb->GetUnitDefList(defList);
	int i;
	for (i=0; i < numUnitDefs; ++i) {
		list[i] = defList[i]->id;
	}
	delete [] defList;
	return numUnitDefs;
}
int _UnitDef_STATIC_getIdByName(int teamId, const char* unitName) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetUnitDef(unitName)->id;
}

float _UnitDef_getHeight(int teamId, int unitDefId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetUnitDefHeight(unitDefId);
}
float _UnitDef_getRadius(int teamId, int unitDefId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetUnitDefRadius(unitDefId);
}

bool _UnitDef_isValid(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->valid;}
const char* _UnitDef_getName(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->name.c_str();}
const char* _UnitDef_getHumanName(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->humanName.c_str();}
const char* _UnitDef_getFilename(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->filename.c_str();}
int _UnitDef_getId(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->id;}
int _UnitDef_getAiHint(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->aihint;}
int _UnitDef_getCobID(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->cobID;}
int _UnitDef_getTechLevel(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->techLevel;}
const char* _UnitDef_getGaia(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->gaia.c_str();}
float _UnitDef_getMetalUpkeep(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->metalUpkeep;}
float _UnitDef_getEnergyUpkeep(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->energyUpkeep;}
float _UnitDef_getMetalMake(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->metalMake;}
float _UnitDef_getMakesMetal(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->makesMetal;}
float _UnitDef_getEnergyMake(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->energyMake;}
float _UnitDef_getMetalCost(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->metalCost;}
float _UnitDef_getEnergyCost(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->energyCost;}
float _UnitDef_getBuildTime(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildTime;}
float _UnitDef_getExtractsMetal(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->extractsMetal;}
float _UnitDef_getExtractRange(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->extractRange;}
float _UnitDef_getWindGenerator(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->windGenerator;}
float _UnitDef_getTidalGenerator(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->tidalGenerator;}
float _UnitDef_getMetalStorage(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->metalStorage;}
float _UnitDef_getEnergyStorage(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->energyStorage;}
float _UnitDef_getAutoHeal(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->autoHeal;}
float _UnitDef_getIdleAutoHeal(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->idleAutoHeal;}
int _UnitDef_getIdleTime(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->idleTime;}
float _UnitDef_getPower(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->power;}
float _UnitDef_getHealth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->health;}
unsigned int _UnitDef_getCategory(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->category;}
float _UnitDef_getSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->speed;}
float _UnitDef_getTurnRate(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->turnRate;}
int _UnitDef_getMoveType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->moveType;}
bool _UnitDef_isUpright(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->upright;}
bool _UnitDef_isCollide(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->collide;}
float _UnitDef_getControlRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->controlRadius;}
float _UnitDef_getLosRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->losRadius;}
float _UnitDef_getAirLosRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->airLosRadius;}
float _UnitDef_getLosHeight(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->losHeight;}
int _UnitDef_getRadarRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->radarRadius;}
int _UnitDef_getSonarRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->sonarRadius;}
int _UnitDef_getJammerRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->jammerRadius;}
int _UnitDef_getSonarJamRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->sonarJamRadius;}
int _UnitDef_getSeismicRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->seismicRadius;}
float _UnitDef_getSeismicSignature(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->seismicSignature;}
bool _UnitDef_isStealth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->stealth;}
bool _UnitDef_isSonarStealth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->sonarStealth;}
bool _UnitDef_isBuildRange3D(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildRange3D;}
float _UnitDef_getBuildDistance(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildDistance;}
float _UnitDef_getBuildSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildSpeed;}
float _UnitDef_getReclaimSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->reclaimSpeed;}
float _UnitDef_getRepairSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->repairSpeed;}
float _UnitDef_getMaxRepairSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxRepairSpeed;}
float _UnitDef_getResurrectSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->resurrectSpeed;}
float _UnitDef_getCaptureSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->captureSpeed;}
float _UnitDef_getTerraformSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->terraformSpeed;}
float _UnitDef_getMass(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->mass;}
bool _UnitDef_isPushResistant(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->pushResistant;}
bool _UnitDef_isStrafeToAttack(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->strafeToAttack;}
float _UnitDef_getMinCollisionSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->minCollisionSpeed;}
float _UnitDef_getSlideTolerance(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->slideTolerance;}
float _UnitDef_getMaxSlope(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxSlope;}
float _UnitDef_getMaxHeightDif(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxHeightDif;}
float _UnitDef_getMinWaterDepth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->minWaterDepth;}
float _UnitDef_getWaterline(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->waterline;}
float _UnitDef_getMaxWaterDepth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxWaterDepth;}
float _UnitDef_getArmoredMultiple(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->armoredMultiple;}
int _UnitDef_getArmorType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->armorType;}
int _UnitDef_getFlankingBonusMode(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flankingBonusMode;}
SAIFloat3 _UnitDef_getFlankingBonusDir(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flankingBonusDir.toSAIFloat3();}
float _UnitDef_getFlankingBonusMax(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flankingBonusMax;}
float _UnitDef_getFlankingBonusMin(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flankingBonusMin;}
float _UnitDef_getFlankingBonusMobilityAdd(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flankingBonusMobilityAdd;}
const char* _UnitDef_getCollisionVolumeType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->collisionVolumeType.c_str();}
SAIFloat3 _UnitDef_getCollisionVolumeScales(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->collisionVolumeScales.toSAIFloat3();}
SAIFloat3 _UnitDef_getCollisionVolumeOffsets(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->collisionVolumeOffsets.toSAIFloat3();}
int _UnitDef_getCollisionVolumeTest(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->collisionVolumeTest;}
float _UnitDef_getMaxWeaponRange(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxWeaponRange;}
const char* _UnitDef_getType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->type.c_str();}
const char* _UnitDef_getTooltip(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->tooltip.c_str();}
const char* _UnitDef_getWreckName(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->wreckName.c_str();}
const char* _UnitDef_getDeathExplosion(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->deathExplosion.c_str();}
const char* _UnitDef_getSelfDExplosion(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->selfDExplosion.c_str();}
const char* _UnitDef_getTedClassString(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->TEDClassString.c_str();}
const char* _UnitDef_getCategoryString(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->categoryString.c_str();}
bool _UnitDef_isCanSelfD(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canSelfD;}
int _UnitDef_getSelfDCountdown(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->selfDCountdown;}
bool _UnitDef_isCanSubmerge(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canSubmerge;}
bool _UnitDef_isCanFly(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canfly;}
bool _UnitDef_isCanMove(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canmove;}
bool _UnitDef_isCanHover(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canhover;}
bool _UnitDef_isFloater(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->floater;}
bool _UnitDef_isBuilder(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->builder;}
bool _UnitDef_isActivateWhenBuilt(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->activateWhenBuilt;}
bool _UnitDef_isOnOffable(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->onoffable;}
bool _UnitDef_isFullHealthFactory(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->fullHealthFactory;}
bool _UnitDef_isFactoryHeadingTakeoff(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->factoryHeadingTakeoff;}
bool _UnitDef_isReclaimable(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->reclaimable;}
bool _UnitDef_isCapturable(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->capturable;}
bool _UnitDef_isCanRestore(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canRestore;}
bool _UnitDef_isCanRepair(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canRepair;}
bool _UnitDef_isCanSelfRepair(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canSelfRepair;}
bool _UnitDef_isCanReclaim(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canReclaim;}
bool _UnitDef_isCanAttack(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canAttack;}
bool _UnitDef_isCanPatrol(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canPatrol;}
bool _UnitDef_isCanFight(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canFight;}
bool _UnitDef_isCanGuard(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canGuard;}
bool _UnitDef_isCanBuild(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canBuild;}
bool _UnitDef_isCanAssist(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canAssist;}
bool _UnitDef_isCanBeAssisted(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canBeAssisted;}
bool _UnitDef_isCanRepeat(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canRepeat;}
bool _UnitDef_isCanFireControl(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canFireControl;}
int _UnitDef_getFireState(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->fireState;}
int _UnitDef_getMoveState(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->moveState;}
float _UnitDef_getWingDrag(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->wingDrag;}
float _UnitDef_getWingAngle(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->wingAngle;}
float _UnitDef_getDrag(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->drag;}
float _UnitDef_getFrontToSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->frontToSpeed;}
float _UnitDef_getSpeedToFront(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->speedToFront;}
float _UnitDef_getMyGravity(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->myGravity;}
float _UnitDef_getMaxBank(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxBank;}
float _UnitDef_getMaxPitch(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxPitch;}
float _UnitDef_getTurnRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->turnRadius;}
float _UnitDef_getWantedHeight(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->wantedHeight;}
float _UnitDef_getVerticalSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->verticalSpeed;}
bool _UnitDef_isCanCrash(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canCrash;}
bool _UnitDef_isHoverAttack(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->hoverAttack;}
bool _UnitDef_isAirStrafe(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->airStrafe;}
float _UnitDef_getDlHoverFactor(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->dlHoverFactor;}
float _UnitDef_getMaxAcceleration(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxAcc;}
float _UnitDef_getMaxDeceleration(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxDec;}
float _UnitDef_getMaxAileron(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxAileron;}
float _UnitDef_getMaxElevator(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxElevator;}
float _UnitDef_getMaxRudder(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxRudder;}
//const unsigned char** _UnitDef_getYardMaps(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->yardmaps;}
int _UnitDef_getXSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->xsize;}
int _UnitDef_getYSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->ysize;}
int _UnitDef_getBuildAngle(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildangle;}
float _UnitDef_getLoadingRadius(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->loadingRadius;}
float _UnitDef_getUnloadSpread(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->unloadSpread;}
int _UnitDef_getTransportCapacity(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->transportCapacity;}
int _UnitDef_getTransportSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->transportSize;}
int _UnitDef_getMinTransportSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->minTransportSize;}
bool _UnitDef_isAirBase(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->isAirBase;}
float _UnitDef_getTransportMass(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->transportMass;}
float _UnitDef_getMinTransportMass(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->minTransportMass;}
bool _UnitDef_isHoldSteady(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->holdSteady;}
bool _UnitDef_isReleaseHeld(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->releaseHeld;}
bool _UnitDef_isCantBeTransported(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->cantBeTransported;}
bool _UnitDef_isTransportByEnemy(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->transportByEnemy;}
int _UnitDef_getTransportUnloadMethod(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->transportUnloadMethod;}
float _UnitDef_getFallSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->fallSpeed;}
float _UnitDef_getUnitFallSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->unitFallSpeed;}
bool _UnitDef_isCanCloak(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canCloak;}
bool _UnitDef_isStartCloaked(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->startCloaked;}
float _UnitDef_getCloakCost(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->cloakCost;}
float _UnitDef_getCloakCostMoving(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->cloakCostMoving;}
float _UnitDef_getDecloakDistance(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->decloakDistance;}
bool _UnitDef_isDecloakSpherical(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->decloakSpherical;}
bool _UnitDef_isDecloakOnFire(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->decloakOnFire;}
bool _UnitDef_isCanKamikaze(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canKamikaze;}
float _UnitDef_getKamikazeDist(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->kamikazeDist;}
bool _UnitDef_isTargetingFacility(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->targfac;}
bool _UnitDef_isCanDGun(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canDGun;}
bool _UnitDef_isNeedGeo(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->needGeo;}
bool _UnitDef_isFeature(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->isFeature;}
bool _UnitDef_isHideDamage(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->hideDamage;}
bool _UnitDef_isCommander(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->isCommander;}
bool _UnitDef_isShowPlayerName(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->showPlayerName;}
bool _UnitDef_isCanResurrect(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canResurrect;}
bool _UnitDef_isCanCapture(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canCapture;}
int _UnitDef_getHighTrajectoryType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->highTrajectoryType;}
unsigned int _UnitDef_getNoChaseCategory(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->noChaseCategory;}
bool _UnitDef_isLeaveTracks(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->leaveTracks;}
float _UnitDef_getTrackWidth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->trackWidth;}
float _UnitDef_getTrackOffset(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->trackOffset;}
float _UnitDef_getTrackStrength(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->trackStrength;}
float _UnitDef_getTrackStretch(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->trackStretch;}
int _UnitDef_getTrackType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->trackType;}
bool _UnitDef_isCanDropFlare(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canDropFlare;}
float _UnitDef_getFlareReloadTime(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareReloadTime;}
float _UnitDef_getFlareEfficiency(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareEfficiency;}
float _UnitDef_getFlareDelay(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareDelay;}
SAIFloat3 _UnitDef_getFlareDropVector(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareDropVector.toSAIFloat3();}
int _UnitDef_getFlareTime(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareTime;}
int _UnitDef_getFlareSalvoSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareSalvoSize;}
int _UnitDef_getFlareSalvoDelay(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->flareSalvoDelay;}
bool _UnitDef_isSmoothAnim(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->smoothAnim;}
bool _UnitDef_isMetalMaker(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->isMetalMaker;}
bool _UnitDef_isCanLoopbackAttack(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->canLoopbackAttack;}
bool _UnitDef_isLevelGround(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->levelGround;}
bool _UnitDef_isUseBuildingGroundDecal(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->useBuildingGroundDecal;}
int _UnitDef_getBuildingDecalType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildingDecalType;}
int _UnitDef_getBuildingDecalSizeX(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildingDecalSizeX;}
int _UnitDef_getBuildingDecalSizeY(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildingDecalSizeY;}
float _UnitDef_getBuildingDecalDecaySpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildingDecalDecaySpeed;}
bool _UnitDef_isFirePlatform(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->isFirePlatform;}
float _UnitDef_getMaxFuel(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxFuel;}
float _UnitDef_getRefuelTime(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->refuelTime;}
float _UnitDef_getMinAirBasePower(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->minAirBasePower;}
int _UnitDef_getMaxThisUnit(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->maxThisUnit;}
int _UnitDef_getDecoyDefId(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->decoyDef->id;}
bool _UnitDef_isDontLand(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->DontLand();}
int _UnitDef_getShieldWeaponDefId(int teamId, int unitDefId) {
	const WeaponDef* wd = getUnitDefById(teamId, unitDefId)->shieldWeaponDef;
	if (wd == NULL)
		return -1;
	else
		return wd->id;
}
int _UnitDef_getStockpileWeaponDefId(int teamId, int unitDefId) {
	const WeaponDef* wd = getUnitDefById(teamId, unitDefId)->stockpileWeaponDef;
	if (wd == NULL)
		return -1;
	else
		return wd->id;
}
int _UnitDef_getNumBuildOptions(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->buildOptions.size();}
int _UnitDef_getBuildOptions(int teamId, int unitDefId, int* unitDefIds) {
	const std::map<int,std::string>* bo = &(getUnitDefById(teamId, unitDefId)->buildOptions);
	int numBo = bo->size();
	std::map<int,std::string>::const_iterator bb;
	int b;
	for (b=0, bb = bo->begin(); bb != bo->end(); ++b, bb++) {
		unitDefIds[b] = _UnitDef_STATIC_getIdByName(teamId, bb->second.c_str());
	}
	return numBo;
}
int _UnitDef_getNumCustomParams(int teamId, int unitDefId) {
	return getUnitDefById(teamId, unitDefId)->customParams.size();
}
int _UnitDef_getCustomParams(int teamId, int unitDefId, const char* map[][2]) {
	return fillCMap(&(getUnitDefById(teamId, unitDefId)->customParams), map);
}
bool _UnitDef_hasMoveData(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata != NULL;}
int _UnitDef_MoveData_getMoveType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->moveType;}
int _UnitDef_MoveData_getMoveFamily(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->moveFamily;}
int _UnitDef_MoveData_getSize(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->size;}
float _UnitDef_MoveData_getDepth(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->depth;}
float _UnitDef_MoveData_getMaxSlope(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->maxSlope;}
float _UnitDef_MoveData_getSlopeMod(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->slopeMod;}
float _UnitDef_MoveData_getDepthMod(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->depthMod;}
int _UnitDef_MoveData_getPathType(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->pathType;}
float _UnitDef_MoveData_getCrushStrength(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->crushStrength;}
float _UnitDef_MoveData_getMaxSpeed(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->maxSpeed;}
short _UnitDef_MoveData_getMaxTurnRate(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->maxTurnRate;}
float _UnitDef_MoveData_getMaxAcceleration(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->maxAcceleration;}
float _UnitDef_MoveData_getMaxBreaking(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->maxBreaking;}
bool _UnitDef_MoveData_isSubMarine(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->movedata->subMarine;}
int _UnitDef_getNumUnitDefWeapons(int teamId, int unitDefId) {return getUnitDefById(teamId, unitDefId)->weapons.size();}
const char* _UnitDef_UnitDefWeapon_getName(int teamId, int unitDefId, int weaponIndex) {
	return getUnitDefById(teamId, unitDefId)->weapons.at(weaponIndex).name.c_str();
}
int _UnitDef_UnitDefWeapon_getWeaponDefId(int teamId, int unitDefId, int weaponIndex) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponIndex).def->id;}
int _UnitDef_UnitDefWeapon_getSlavedTo(int teamId, int unitDefId, int weaponIndex) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponIndex).slavedTo;}
SAIFloat3 _UnitDef_UnitDefWeapon_getMainDir(int teamId, int unitDefId, int weaponIndex) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponIndex).mainDir.toSAIFloat3();}
float _UnitDef_UnitDefWeapon_getMaxAngleDif(int teamId, int unitDefId, int weaponIndex) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponIndex).maxAngleDif;}
float _UnitDef_UnitDefWeapon_getFuelUsage(int teamId, int unitDefId, int weaponIndex) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponIndex).fuelUsage;}
unsigned int _UnitDef_UnitDefWeapon_getBadTargetCat(int teamId, int unitDefId, int weaponIndex) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponIndex).badTargetCat;}
unsigned int _UnitDef_UnitDefWeapon_getOnlyTargetCat(int teamId, int unitDefId, int weaponIndex) {return getUnitDefById(teamId, unitDefId)->weapons.at(weaponIndex).onlyTargetCat;}
//########### END UnitDef





//########### BEGINN Unit
int _Unit_getDefId(int teamId, int unitId) {
	const UnitDef* unitDef;
	if (_Cheats_isEnabled(teamId)) {
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
int _Unit_getAiHint(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetUnitAiHint(unitId);
}
int _Unit_getGroup(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetUnitGroup(unitId);
}
int _Unit_getTeam(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetUnitTeam(unitId);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitTeam(unitId);
	} else {
		return team_callback[teamId]->GetUnitTeam(unitId);
	}
}
int _Unit_getAllyTeam(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetUnitAllyTeam(unitId);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitAllyTeam(unitId);
	} else {
		return team_callback[teamId]->GetUnitAllyTeam(unitId);
	}
}
int _Unit_getNumSupportedCommands(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetUnitCommands(unitId)->size();
}
int _Unit_SupportedCommands_getId(int teamId, int unitId, int* ids) {
	IAICallback* clb = team_callback[teamId];
	const std::vector<CommandDescription>* comDescs = clb->GetUnitCommands(unitId);
	int numComDescs = comDescs->size();
	for (int i=0; i < numComDescs; i++) {
		ids[i] = comDescs->at(i).id;
	}
	return numComDescs;
}
int _Unit_SupportedCommands_getName(int teamId, int unitId, const char** names) {
	IAICallback* clb = team_callback[teamId];
	const std::vector<CommandDescription>* comDescs = clb->GetUnitCommands(unitId);
	int numComDescs = comDescs->size();
	for (int i=0; i < numComDescs; i++) {
		names[i] = comDescs->at(i).name.c_str();
	}
	return numComDescs;
}
int _Unit_SupportedCommands_getToolTip(int teamId, int unitId, const char** toolTips) {
	IAICallback* clb = team_callback[teamId];
	const std::vector<CommandDescription>* comDescs = clb->GetUnitCommands(unitId);
	int numComDescs = comDescs->size();
	for (int i=0; i < numComDescs; i++) {
		toolTips[i] = comDescs->at(i).tooltip.c_str();
	}
	return numComDescs;
}
int _Unit_SupportedCommands_isShowUnique(int teamId, int unitId, bool* showUniques) {
	IAICallback* clb = team_callback[teamId];
	const std::vector<CommandDescription>* comDescs = clb->GetUnitCommands(unitId);
	int numComDescs = comDescs->size();
	for (int i=0; i < numComDescs; i++) {
		showUniques[i] = comDescs->at(i).showUnique;
	}
	return numComDescs;
}
int _Unit_SupportedCommands_isDisabled(int teamId, int unitId, bool* disableds) {
	IAICallback* clb = team_callback[teamId];
	const std::vector<CommandDescription>* comDescs = clb->GetUnitCommands(unitId);
	int numComDescs = comDescs->size();
	for (int i=0; i < numComDescs; i++) {
		disableds[i] = comDescs->at(i).disabled;
	}
	return numComDescs;
}
int _Unit_SupportedCommands_getNumParams(int teamId, int unitId, int* numParams) {
	IAICallback* clb = team_callback[teamId];
	const std::vector<CommandDescription>* comDescs = clb->GetUnitCommands(unitId);
	int numComDescs = comDescs->size();
	for (int i=0; i < numComDescs; i++) {
		numParams[i] = comDescs->at(i).params.size();
	}
	return numComDescs;
}
int _Unit_SupportedCommands_getParams(int teamId, int unitId, const char*** params) {
	IAICallback* clb = team_callback[teamId];
	const std::vector<CommandDescription>* comDescs = clb->GetUnitCommands(unitId);
	int numComDescs = comDescs->size();
	for (int c=0; c < numComDescs; c++) {
		const std::vector<std::string> ps = comDescs->at(c).params;
		for (int p=0; p < ps.size(); p++) {
			params[c][p] = ps.at(p).c_str();
		}
	}
	return numComDescs;
}

int _Unit_getStockpile(int teamId, int unitId) {
    IAICallback* clb = team_callback[teamId];
    int stockpile;
    bool fetchOk = clb->GetProperty(AIVAL_STOCKPILED, unitId, &stockpile);
    if (!fetchOk) {
        stockpile = -1;
    }
    return stockpile;
}
int _Unit_getStockpileQueued(int teamId, int unitId) {
    IAICallback* clb = team_callback[teamId];
    int stockpileQueue;
    bool fetchOk = clb->GetProperty(AIVAL_STOCKPILE_QUED, unitId, &stockpileQueue);
    if (!fetchOk) {
        stockpileQueue = -1;
    }
    return stockpileQueue;
}
float _Unit_getCurrentFuel(int teamId, int unitId) {
    IAICallback* clb = team_callback[teamId];
    float currentFuel;
    bool fetchOk = clb->GetProperty(AIVAL_CURRENT_FUEL, unitId, &currentFuel);
    if (!fetchOk) {
        currentFuel = -1.0f;
    }
    return currentFuel;
}
float _Unit_getMaxSpeed(int teamId, int unitId) {
    IAICallback* clb = team_callback[teamId];
    float maxSpeed;
    bool fetchOk = clb->GetProperty(AIVAL_UNIT_MAXSPEED, unitId, &maxSpeed);
    if (!fetchOk) {
        maxSpeed = -1.0f;
    }
    return maxSpeed;
}

float _Unit_getMaxRange(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetUnitMaxRange(unitId);
}

float _Unit_getMaxHealth(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetUnitMaxHealth(unitId);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitMaxHealth(unitId);
	} else {
		return team_callback[teamId]->GetUnitMaxHealth(unitId);
	}
}

int _Unit_getNumCurrentCommands(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId];
//	return clb->GetCurrentUnitCommands(unitId)->size();
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId)->size();
	} else {
		return team_callback[teamId]->GetCurrentUnitCommands(unitId)->size();
	}
}
int _Unit_CurrentCommands_getType(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId];
//	CCommandQueue::QueueType qt = clb->GetCurrentUnitCommands(unitId)->GetType();
//	return qt;
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId)->GetType();
	} else {
		return team_callback[teamId]->GetCurrentUnitCommands(unitId)->GetType();
	}
}
int _Unit_CurrentCommands_getIds(int teamId, int unitId, int* ids) {
//	IAICallback* clb = team_callback[teamId];
//	const CCommandQueue* cc = clb->GetCurrentUnitCommands(unitId);
	const CCommandQueue* cc = NULL;
	if (_Cheats_isEnabled(teamId)) {
		cc = team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId);
	} else {
		cc = team_callback[teamId]->GetCurrentUnitCommands(unitId);
	}
	int numCommands = cc->size();
	for (int c=0; c < numCommands; ++c) {
		ids[c] = cc->at(c).id;
	}
	return numCommands;
}
int _Unit_CurrentCommands_getOptions(int teamId, int unitId, unsigned char* options) {
//	IAICallback* clb = team_callback[teamId];
//	const CCommandQueue* cc = clb->GetCurrentUnitCommands(unitId);
	const CCommandQueue* cc = NULL;
	if (_Cheats_isEnabled(teamId)) {
		cc = team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId);
	} else {
		cc = team_callback[teamId]->GetCurrentUnitCommands(unitId);
	}
	int numCommands = cc->size();
	for (int c=0; c < numCommands; ++c) {
		options[c] = cc->at(c).options;
	}
	return numCommands;
}
int _Unit_CurrentCommands_getTag(int teamId, int unitId, unsigned int* tags) {
//	IAICallback* clb = team_callback[teamId];
//	const CCommandQueue* cc = clb->GetCurrentUnitCommands(unitId);
	const CCommandQueue* cc = NULL;
	if (_Cheats_isEnabled(teamId)) {
		cc = team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId);
	} else {
		cc = team_callback[teamId]->GetCurrentUnitCommands(unitId);
	}
	int numCommands = cc->size();
	for (int c=0; c < numCommands; ++c) {
		tags[c] = cc->at(c).tag;
	}
	return numCommands;
}
int _Unit_CurrentCommands_getTimeOut(int teamId, int unitId, int* timeOuts) {
//	IAICallback* clb = team_callback[teamId];
//	const CCommandQueue* cc = clb->GetCurrentUnitCommands(unitId);
	const CCommandQueue* cc = NULL;
	if (_Cheats_isEnabled(teamId)) {
		cc = team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId);
	} else {
		cc = team_callback[teamId]->GetCurrentUnitCommands(unitId);
	}
	int numCommands = cc->size();
	for (int c=0; c < numCommands; ++c) {
		timeOuts[c] = cc->at(c).timeOut;
	}
	return numCommands;
}
int _Unit_CurrentCommands_getNumParams(int teamId, int unitId, int* numParams) {
//	IAICallback* clb = team_callback[teamId];
//	const CCommandQueue* cc = clb->GetCurrentUnitCommands(unitId);
	const CCommandQueue* cc = NULL;
	if (_Cheats_isEnabled(teamId)) {
		cc = team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId);
	} else {
		cc = team_callback[teamId]->GetCurrentUnitCommands(unitId);
	}
	int numCommands = cc->size();
	for (int c=0; c < numCommands; ++c) {
		numParams[c] = cc->at(c).params.size();
	}
	return numCommands;
}
int _Unit_CurrentCommands_getParams(int teamId, int unitId, float** params) {
//	IAICallback* clb = team_callback[teamId];
//	const CCommandQueue* cc = clb->GetCurrentUnitCommands(unitId);
	const CCommandQueue* cc = NULL;
	if (_Cheats_isEnabled(teamId)) {
		cc = team_cheatCallback[teamId]->GetCurrentUnitCommands(unitId);
	} else {
		cc = team_callback[teamId]->GetCurrentUnitCommands(unitId);
	}
	int numCommands = cc->size();
	for (int c=0; c < numCommands; c++) {
		const std::vector<float> ps = cc->at(c).params;
		for (int p=0; p < ps.size(); p++) {
			params[c][p] = ps.at(p);
		}
	}
	return numCommands;
}

float _Unit_getExperience(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetUnitExperience(unitId);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitExperience(unitId);
	} else {
		return team_callback[teamId]->GetUnitExperience(unitId);
	}
}

float _Unit_getHealth(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetUnitHealth(unitId);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitHealth(unitId);
	} else {
		return team_callback[teamId]->GetUnitHealth(unitId);
	}
}
float _Unit_getSpeed(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetUnitSpeed(unitId);
}
float _Unit_getPower(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId];return clb->GetUnitPower(unitId);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitPower(unitId);
	} else {
		return team_callback[teamId]->GetUnitPower(unitId);
	}
}
SAIFloat3 _Unit_getPos(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId];
//	return clb->GetUnitPos(unitId).toSAIFloat3();
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetUnitPos(unitId).toSAIFloat3();
	} else {
		return team_callback[teamId]->GetUnitPos(unitId).toSAIFloat3();
	}
}
float _Unit_ResourceInfo_Metal_getUse(int teamId, int unitId) {
	
	int res = -1.0F;
	UnitResourceInfo resourceInfo;
	
//	IAICallback* clb = team_callback[teamId];
//	bool fetchOk = clb->GetUnitResourceInfo(unitId, &resourceInfo);
	bool fetchOk;
	if (_Cheats_isEnabled(teamId)) {
		fetchOk = team_cheatCallback[teamId]->GetUnitResourceInfo(unitId, &resourceInfo);
	} else {
		fetchOk = team_callback[teamId]->GetUnitResourceInfo(unitId, &resourceInfo);
	}
	if (fetchOk) {
		res = resourceInfo.metalUse;
	}
	
	return res;
}
float _Unit_ResourceInfo_Metal_getMake(int teamId, int unitId) {
	
	int res = -1.0F;
	UnitResourceInfo resourceInfo;
	
//	IAICallback* clb = team_callback[teamId];
//	bool fetchOk = clb->GetUnitResourceInfo(unitId, &resourceInfo);
	bool fetchOk;
	if (_Cheats_isEnabled(teamId)) {
		fetchOk = team_cheatCallback[teamId]->GetUnitResourceInfo(unitId, &resourceInfo);
	} else {
		fetchOk = team_callback[teamId]->GetUnitResourceInfo(unitId, &resourceInfo);
	}
	if (fetchOk) {
		res = resourceInfo.metalMake;
	}
	
	return res;
}
float _Unit_ResourceInfo_Energy_getUse(int teamId, int unitId) {
	
	int res = -1.0F;
	UnitResourceInfo resourceInfo;
	
//	IAICallback* clb = team_callback[teamId];
//	bool fetchOk = clb->GetUnitResourceInfo(unitId, &resourceInfo);
	bool fetchOk;
	if (_Cheats_isEnabled(teamId)) {
		fetchOk = team_cheatCallback[teamId]->GetUnitResourceInfo(unitId, &resourceInfo);
	} else {
		fetchOk = team_callback[teamId]->GetUnitResourceInfo(unitId, &resourceInfo);
	}
	if (fetchOk) {
		res = resourceInfo.energyUse;
	}
	
	return res;
}
float _Unit_ResourceInfo_Energy_getMake(int teamId, int unitId) {
	
	int res = -1.0F;
	UnitResourceInfo resourceInfo;
	
//	IAICallback* clb = team_callback[teamId];
//	bool fetchOk = clb->GetUnitResourceInfo(unitId, &resourceInfo);
	bool fetchOk;
	if (_Cheats_isEnabled(teamId)) {
		fetchOk = team_cheatCallback[teamId]->GetUnitResourceInfo(unitId, &resourceInfo);
	} else {
		fetchOk = team_callback[teamId]->GetUnitResourceInfo(unitId, &resourceInfo);
	}
	if (fetchOk) {
		res = resourceInfo.energyMake;
	}
	
	return res;
}
bool _Unit_isActivated(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->IsUnitActivated(unitId);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->IsUnitActivated(unitId);
	} else {
		return team_callback[teamId]->IsUnitActivated(unitId);
	}
}
bool _Unit_isBeingBuilt(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->UnitBeingBuilt(unitId);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->UnitBeingBuilt(unitId);
	} else {
		return team_callback[teamId]->UnitBeingBuilt(unitId);
	}
}
bool _Unit_isCloaked(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->IsUnitCloaked(unitId);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->IsUnitCloaked(unitId);
	} else {
		return team_callback[teamId]->IsUnitCloaked(unitId);
	}
}
bool _Unit_isParalyzed(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->IsUnitParalyzed(unitId);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->IsUnitParalyzed(unitId);
	} else {
		return team_callback[teamId]->IsUnitParalyzed(unitId);
	}
}
bool _Unit_isNeutral(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->IsUnitNeutral(unitId);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->IsUnitNeutral(unitId);
	} else {
		return team_callback[teamId]->IsUnitNeutral(unitId);
	}
}
int _Unit_getBuildingFacing(int teamId, int unitId) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetBuildingFacing(unitId);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetBuildingFacing(unitId);
	} else {
		return team_callback[teamId]->GetBuildingFacing(unitId);
	}
}
//########### END Unit

int _Unit_STATIC_getEnemies(int teamId, int* unitIds) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetEnemyUnits(unitIds);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetEnemyUnits(unitIds);
	} else {
		return team_callback[teamId]->GetEnemyUnits(unitIds);
	}
}

int _Unit_STATIC_getEnemiesIn(int teamId, int* unitIds, SAIFloat3 pos, float radius) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetEnemyUnits(unitIds, float3(pos), radius);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetEnemyUnits(unitIds, float3(pos), radius);
	} else {
		return team_callback[teamId]->GetEnemyUnits(unitIds, float3(pos), radius);
	}
}

int _Unit_STATIC_getEnemiesInRadarAndLos(int teamId, int* unitIds) {
	IAICallback* clb = team_callback[teamId]; return clb->GetEnemyUnitsInRadarAndLos(unitIds);
}

int _Unit_STATIC_getFriendlies(int teamId, int* unitIds) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFriendlyUnits(unitIds);
}

int _Unit_STATIC_getFriendliesIn(int teamId, int* unitIds, SAIFloat3 pos, float radius) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFriendlyUnits(unitIds, float3(pos), radius);
}

int _Unit_STATIC_getNeutrals(int teamId, int* unitIds) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetNeutralUnits(unitIds);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetNeutralUnits(unitIds);
	} else {
		return team_callback[teamId]->GetNeutralUnits(unitIds);
	}
}

int _Unit_STATIC_getNeutralsIn(int teamId, int* unitIds, SAIFloat3 pos, float radius) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetNeutralUnits(unitIds, float3(pos), radius);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetNeutralUnits(unitIds, float3(pos), radius);
	} else {
		return team_callback[teamId]->GetNeutralUnits(unitIds, float3(pos), radius);
	}
}

const char* _Mod_getName(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetModName();
}

//########### BEGINN Map
int _Map_getWidth(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMapWidth();
}

int _Map_getHeight(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMapHeight();
}

const float* _Map_getHeightMap(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetHeightMap();
}

float _Map_getMinHeight(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMinHeight();
}

float _Map_getMaxHeight(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMaxHeight();
}

const float* _Map_getSlopeMap(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetSlopeMap();
}

const unsigned short* _Map_getLosMap(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetLosMap();
}

const unsigned short* _Map_getRadarMap(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetRadarMap();
}

const unsigned short* _Map_getJammerMap(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetJammerMap();
}

const unsigned char* _Map_getMetalMap(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMetalMap();
}

const char* _Map_getName(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMapName();
}

float _Map_getElevationAt(int teamId, float x, float z) {
	IAICallback* clb = team_callback[teamId]; return clb->GetElevation(x, z);
}

float _Map_getMaxMetal(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMaxMetal();
}

float _Map_getExtractorRadius(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetExtractorRadius();
}

float _Map_getMinWind(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMinWind();
}

float _Map_getMaxWind(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMaxWind();
}

float _Map_getTidalStrength(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetTidalStrength();
}

float _Map_getGravity(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetGravity();
}

bool _Map_canBuildAt(int teamId, int unitDefId, SAIFloat3 pos, int facing) {
	IAICallback* clb = team_callback[teamId];
	const UnitDef* unitDef = getUnitDefById(teamId, unitDefId);
	return clb->CanBuildAt(unitDef, pos, facing);
}

SAIFloat3 _Map_findClosestBuildSite(int teamId, int unitDefId, SAIFloat3 pos, float searchRadius, int minDist, int facing) {
	IAICallback* clb = team_callback[teamId];
	const UnitDef* unitDef = getUnitDefById(teamId, unitDefId);
	return clb->ClosestBuildSite(unitDef, pos, searchRadius, minDist, facing).toSAIFloat3();
}

int _Map_getPoints(int teamId, SAIFloat3* positions, unsigned char** colors, const char** labels, int maxPoints) {
	IAICallback* clb = team_callback[teamId];
	PointMarker* pm = new PointMarker[maxPoints];
	int numPoints = clb->GetMapPoints(pm, maxPoints);
	for (int i=0; i < numPoints; ++i) {
		positions[i] = pm[i].pos.toSAIFloat3();
		colors[i] = pm[i].color;
		labels[i] = pm[i].label;
	}
	delete [] pm;
	return numPoints;
}

int _Map_getLines(int teamId, SAIFloat3* firstPositions, SAIFloat3* secondPositions, unsigned char** colors, int maxLines) {
	IAICallback* clb = team_callback[teamId];
	LineMarker* lm = new LineMarker[maxLines];
	int numLines = clb->GetMapLines(lm, maxLines);
	for (int i=0; i < numLines; ++i) {
		firstPositions[i] = lm[i].pos.toSAIFloat3();
		secondPositions[i] = lm[i].pos2.toSAIFloat3();
		colors[i] = lm[i].color;
	}
	delete [] lm;
	return numLines;
}
//########### END Map


/*
bool _getProperty(int teamId, int id, int property, void* dst) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetProperty(id, property, dst);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetProperty(id, property, dst);
	} else {
		return team_callback[teamId]->GetProperty(id, property, dst);
	}
}

bool _getValue(int teamId, int id, void* dst) {
//	IAICallback* clb = team_callback[teamId]; return clb->GetValue(id, dst);
	if (_Cheats_isEnabled(teamId)) {
		return team_cheatCallback[teamId]->GetValue(id, dst);
	} else {
		return team_callback[teamId]->GetValue(id, dst);
	}
}
*/

int _File_getSize(int teamId, const char* filename) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFileSize(filename);
}

bool _File_getContent(int teamId, const char* filename, void* buffer, int bufferLen) {
	IAICallback* clb = team_callback[teamId]; return clb->ReadFile(filename, buffer, bufferLen);
}

int _Unit_STATIC_getSelected(int teamId, int* units) {
	IAICallback* clb = team_callback[teamId]; return clb->GetSelectedUnits(units);
}

SAIFloat3 _Map_getMousePos(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMousePos().toSAIFloat3();
}

float _ResourceInfo_Metal_getCurrent(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMetal();
}

float _ResourceInfo_Metal_getIncome(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMetalIncome();
}

float _ResourceInfo_Metal_getUsage(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMetalUsage();
}

float _ResourceInfo_Metal_getStorage(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetMetalStorage();
}

float _ResourceInfo_Energy_getCurrent(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetEnergy();
}

float _ResourceInfo_Energy_getIncome(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetEnergyIncome();
}

float _ResourceInfo_Energy_getUsage(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetEnergyUsage();
}

float _ResourceInfo_Energy_getStorage(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetEnergyStorage();
}

int _Feature_STATIC_getIds(int teamId, int *featureIds, int max) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFeatures(featureIds, max);
}

int _Feature_STATIC_getIdsIn(int teamId, int *featureIds, int max, SAIFloat3 pos, float radius) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFeatures(featureIds, max, pos, radius);
}

//########### BEGINN FeatureDef
const char* _FeatureDef_getName(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->myName.c_str();}
const char* _FeatureDef_getDescription(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->description.c_str();}
const char* _FeatureDef_getFilename(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->filename.c_str();}
int _FeatureDef_getId(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->id;}
float _FeatureDef_getMetal(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->metal;}
float _FeatureDef_getEnergy(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->energy;}
float _FeatureDef_getMaxHealth(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->maxHealth;}
float _FeatureDef_getReclaimTime(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->reclaimTime;}
float _FeatureDef_getMass(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->mass;}
const char* _FeatureDef_getCollisionVolumeType(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->collisionVolumeType.c_str();}	
SAIFloat3 _FeatureDef_getCollisionVolumeScales(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->collisionVolumeScales.toSAIFloat3();}		
SAIFloat3 _FeatureDef_getCollisionVolumeOffsets(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->collisionVolumeOffsets.toSAIFloat3();}		
int _FeatureDef_getCollisionVolumeTest(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->collisionVolumeTest;}			
bool _FeatureDef_isUpright(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->upright;}
int _FeatureDef_getDrawType(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->drawType;}
const char* _FeatureDef_getModelName(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->modelname.c_str();}
int _FeatureDef_getModelType(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->modelType;}
bool _FeatureDef_isDestructable(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->destructable;}
bool _FeatureDef_isReclaimable(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->reclaimable;}
bool _FeatureDef_isBlocking(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->blocking;}
bool _FeatureDef_isBurnable(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->burnable;}
bool _FeatureDef_isFloating(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->floating;}
bool _FeatureDef_isNoSelect(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->noSelect;}
bool _FeatureDef_isGeoThermal(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->geoThermal;}
const char* _FeatureDef_getDeathFeature(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->deathFeature.c_str();}
int _FeatureDef_getXsize(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->xsize;}
int _FeatureDef_getYsize(int teamId, int featureDefId) {return getFeatureDefById(teamId, featureDefId)->ysize;}
int _FeatureDef_getNumCustomParams(int teamId, int featureDefId) {
	return getFeatureDefById(teamId, featureDefId)->customParams.size();
}
int _FeatureDef_getCustomParams(int teamId, int featureDefId, const char* map[][2]) {
	return fillCMap(&(getFeatureDefById(teamId, featureDefId)->customParams), map);
}
//########### END FeatureDef

int _Feature_getDefId(int teamId, int featureId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetFeatureDef(featureId)->id;
}

float _Feature_getHealth(int teamId, int feature) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFeatureHealth(feature);
}

float _Feature_getReclaimLeft(int teamId, int feature) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFeatureReclaimLeft(feature);
}

SAIFloat3 _Feature_getPos(int teamId, int feature) {
	IAICallback* clb = team_callback[teamId]; return clb->GetFeaturePos(feature).toSAIFloat3();
}

int _UnitDef_STATIC_getNumIds(int teamId) {
	IAICallback* clb = team_callback[teamId]; return clb->GetNumUnitDefs();
}

//########### BEGINN WeaponDef
int _WeaponDef_STATIC_getIdByName(int teamId, const char* weaponDefName) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetWeapon(weaponDefName)->id;
}

const char* _WeaponDef_getName(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->name.c_str();
}
const char* _WeaponDef_getType(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->type.c_str();
}
const char* _WeaponDef_getDescription(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->description.c_str();}
const char* _WeaponDef_getFilename(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->filename.c_str();}
const char* _WeaponDef_getCegTag(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->cegTag.c_str();}
float _WeaponDef_getRange(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->range;}
float _WeaponDef_getHeightMod(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->heightmod;}
float _WeaponDef_getAccuracy(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->accuracy;}
float _WeaponDef_getSprayAngle(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->sprayAngle;}
float _WeaponDef_getMovingAccuracy(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->movingAccuracy;}
float _WeaponDef_getTargetMoveError(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->targetMoveError;}
float _WeaponDef_getLeadLimit(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->leadLimit;}
float _WeaponDef_getLeadBonus(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->leadBonus;}
float _WeaponDef_getPredictBoost(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->predictBoost;}
int _WeaponDef_Damages_getParalyzeDamageTime(int teamId, int weaponDefId) {
	DamageArray da = getWeaponDefById(teamId, weaponDefId)->damages;
	return da.paralyzeDamageTime;
}
float _WeaponDef_Damages_getImpulseFactor(int teamId, int weaponDefId) {
	DamageArray da = getWeaponDefById(teamId, weaponDefId)->damages;
	return da.impulseFactor;
}
float _WeaponDef_Damages_getImpulseBoost(int teamId, int weaponDefId) {
	DamageArray da = getWeaponDefById(teamId, weaponDefId)->damages;
	return da.impulseBoost;
}
float _WeaponDef_Damages_getCraterMult(int teamId, int weaponDefId) {
	DamageArray da = getWeaponDefById(teamId, weaponDefId)->damages;
	return da.craterMult;
}
float _WeaponDef_Damages_getCraterBoost(int teamId, int weaponDefId) {
	DamageArray da = getWeaponDefById(teamId, weaponDefId)->damages;
	return da.craterBoost;
}
int _WeaponDef_Damages_getNumTypes(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->damages.GetNumTypes();
}
void _WeaponDef_Damages_getTypeDamages(int teamId, int weaponDefId, float* typeDamages) {
	int numTypes = _WeaponDef_Damages_getNumTypes(teamId, weaponDefId);
	const WeaponDef* weaponDef = getWeaponDefById(teamId, weaponDefId);
	for (int i=0; i < numTypes; ++i) {
		typeDamages[i] = weaponDef->damages[i];
	}
}
float _WeaponDef_getAreaOfEffect(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->areaOfEffect;}
bool _WeaponDef_isNoSelfDamage(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->noSelfDamage;}
float _WeaponDef_getFireStarter(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->fireStarter;}
float _WeaponDef_getEdgeEffectiveness(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->edgeEffectiveness;}
float _WeaponDef_getSize(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->size;}
float _WeaponDef_getSizeGrowth(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->sizeGrowth;}
float _WeaponDef_getCollisionSize(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->collisionSize;}
int _WeaponDef_getSalvoSize(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->salvosize;}
float _WeaponDef_getSalvoDelay(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->salvodelay;}
float _WeaponDef_getReload(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->reload;}
float _WeaponDef_getBeamTime(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->beamtime;}
bool _WeaponDef_isBeamBurst(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->beamburst;}
bool _WeaponDef_isWaterBounce(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->waterBounce;}
bool _WeaponDef_isGroundBounce(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->groundBounce;}
float _WeaponDef_getBounceRebound(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->bounceRebound;}
float _WeaponDef_getBounceSlip(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->bounceSlip;}
int _WeaponDef_getNumBounce(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->numBounce;}
float _WeaponDef_getMaxAngle(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->maxAngle;}
float _WeaponDef_getRestTime(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->restTime;}
float _WeaponDef_getUpTime(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->uptime;}
int _WeaponDef_getFlightTime(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->flighttime;}
float _WeaponDef_getMetalCost(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->metalcost;}
float _WeaponDef_getEnergyCost(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->energycost;}
float _WeaponDef_getSupplyCost(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->supplycost;}
int _WeaponDef_getProjectilesPerShot(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->projectilespershot;}
int _WeaponDef_getId(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->id;}
int _WeaponDef_getTdfId(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->tdfId;}
bool _WeaponDef_isTurret(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->turret;}
bool _WeaponDef_isOnlyForward(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->onlyForward;}
bool _WeaponDef_isFixedLauncher(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->fixedLauncher;}
bool _WeaponDef_isWaterWeapon(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->waterweapon;}
bool _WeaponDef_isFireSubmersed(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->fireSubmersed;}
bool _WeaponDef_isSubMissile(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->submissile;}
bool _WeaponDef_isTracks(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->tracks;}
bool _WeaponDef_isDropped(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dropped;}
bool _WeaponDef_isParalyzer(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->paralyzer;}
bool _WeaponDef_isImpactOnly(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->impactOnly;}
bool _WeaponDef_isNoAutoTarget(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->noAutoTarget;}
bool _WeaponDef_isManualFire(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->manualfire;}
int _WeaponDef_getInterceptor(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->interceptor;}
int _WeaponDef_getTargetable(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->targetable;}
bool _WeaponDef_isStockpileable(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->stockpile;}
float _WeaponDef_getCoverageRange(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->coverageRange;}
float _WeaponDef_getIntensity(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->intensity;}
float _WeaponDef_getThickness(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->thickness;}
float _WeaponDef_getLaserFlareSize(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->laserflaresize;}
float _WeaponDef_getCoreThickness(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->corethickness;}
float _WeaponDef_getDuration(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->duration;}
int _WeaponDef_getLodDistance(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->lodDistance;}
float _WeaponDef_getFalloffRate(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->falloffRate;}
int _WeaponDef_getGraphicsType(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->graphicsType;}
bool _WeaponDef_isSoundTrigger(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->soundTrigger;}
bool _WeaponDef_isSelfExplode(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->selfExplode;}
bool _WeaponDef_isGravityAffected(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->gravityAffected;}
int _WeaponDef_getHighTrajectory(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->highTrajectory;}
float _WeaponDef_getMyGravity(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->myGravity;}
bool _WeaponDef_isTwoPhase(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->twophase;}
bool _WeaponDef_isGuided(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->guided;}
bool _WeaponDef_isVLaunched(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->vlaunch;}
bool _WeaponDef_isSelfPropelled(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->selfprop;}
bool _WeaponDef_isNoExplode(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->noExplode;}
float _WeaponDef_getStartVelocity(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->startvelocity;}
float _WeaponDef_getWeaponAcceleration(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->weaponacceleration;}
float _WeaponDef_getTurnRate(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->turnrate;}
float _WeaponDef_getMaxVelocity(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->maxvelocity;}
float _WeaponDef_getProjectileSpeed(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->projectilespeed;}
float _WeaponDef_getExplosionSpeed(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->explosionSpeed;}
unsigned int _WeaponDef_getOnlyTargetCategory(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->onlyTargetCategory;}
float _WeaponDef_getWobble(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->wobble;}
float _WeaponDef_getDance(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dance;}
float _WeaponDef_getTrajectoryHeight(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->trajectoryHeight;}
bool _WeaponDef_isLargeBeamLaser(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->largeBeamLaser;}
bool _WeaponDef_isShield(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->isShield;}
bool _WeaponDef_isShieldRepulser(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldRepulser;}
bool _WeaponDef_isSmartShield(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->smartShield;}
bool _WeaponDef_isExteriorShield(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->exteriorShield;}
bool _WeaponDef_isVisibleShield(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->visibleShield;}
bool _WeaponDef_isVisibleShieldRepulse(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->visibleShieldRepulse;}
int _WeaponDef_getVisibleShieldHitFrames(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->visibleShieldHitFrames;}
float _WeaponDef_getShieldEnergyUse(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldEnergyUse;}
float _WeaponDef_getShieldRadius(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldRadius;}
float _WeaponDef_getShieldForce(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldForce;}
float _WeaponDef_getShieldMaxSpeed(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldMaxSpeed;}
float _WeaponDef_getShieldPower(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldPower;}
float _WeaponDef_getShieldPowerRegen(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldPowerRegen;}
float _WeaponDef_getShieldPowerRegenEnergy(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldPowerRegenEnergy;}
float _WeaponDef_getShieldStartingPower(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldStartingPower;}
int _WeaponDef_getShieldRechargeDelay(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldRechargeDelay;}
SAIFloat3 _WeaponDef_getShieldGoodColor(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldGoodColor.toSAIFloat3();}
SAIFloat3 _WeaponDef_getShieldBadColor(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldBadColor.toSAIFloat3();}
float _WeaponDef_getShieldAlpha(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldAlpha;}
unsigned int _WeaponDef_getShieldInterceptType(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->shieldInterceptType;}
unsigned int _WeaponDef_getInterceptedByShieldType(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->interceptedByShieldType;}
bool _WeaponDef_isAvoidFriendly(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->avoidFriendly;}
bool _WeaponDef_isAvoidFeature(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->avoidFeature;}
bool _WeaponDef_isAvoidNeutral(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->avoidNeutral;}
float _WeaponDef_getTargetBorder(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->targetBorder;}
float _WeaponDef_getCylinderTargetting(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->cylinderTargetting;}
float _WeaponDef_getMinIntensity(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->minIntensity;}
float _WeaponDef_getHeightBoostFactor(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->heightBoostFactor;}
float _WeaponDef_getProximityPriority(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->proximityPriority;}
unsigned int _WeaponDef_getCollisionFlags(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->collisionFlags;}
bool _WeaponDef_isSweepFire(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->sweepFire;}
bool _WeaponDef_isCanAttackGround(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->canAttackGround;}
float _WeaponDef_getCameraShake(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->cameraShake;}
float _WeaponDef_getDynDamageExp(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dynDamageExp;}
float _WeaponDef_getDynDamageMin(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dynDamageMin;}
float _WeaponDef_getDynDamageRange(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dynDamageRange;}
bool _WeaponDef_isDynDamageInverted(int teamId, int weaponDefId) {return getWeaponDefById(teamId, weaponDefId)->dynDamageInverted;}
int _WeaponDef_getNumCustomParams(int teamId, int weaponDefId) {
	return getWeaponDefById(teamId, weaponDefId)->customParams.size();
}
int _WeaponDef_getCustomParams(int teamId, int weaponDefId, const char* map[][2]) {
	return fillCMap(&(getWeaponDefById(teamId, weaponDefId)->customParams), map);
}
//########### END WeaponDef


SAIFloat3 _Map_getStartPos(int teamId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetStartPos()->toSAIFloat3();
}











/*
void _sendTextMsg(int teamId, const char* text, int zone) {
	IAICallback* clb = team_callback[teamId]; clb->SendTextMsg(text, zone);
}

void _setLastMsgPos(int teamId, SAIFloat3 pos) {
	IAICallback* clb = team_callback[teamId]; clb->SetLastMsgPos(pos);
}

void _addNotification(int teamId, SAIFloat3 pos, SAIFloat3 color, float alpha) {
	IAICallback* clb = team_callback[teamId]; clb->AddNotification(pos, color, alpha);
}

bool _sendResources(int teamId, float mAmount, float eAmount, int receivingTeam) {
	IAICallback* clb = team_callback[teamId]; return clb->SendResources(mAmount, eAmount, receivingTeam);
}

int _sendUnits(int teamId, int numUnitIds, const int* unitIds, int receivingTeam) {
	IAICallback* clb = team_callback[teamId];
	std::vector<int> unitIdVector(numUnitIds);
	int i;
	for (i=0; i < numUnitIds; ++i) {
		unitIdVector.push_back(unitIds[i]);
	}
	return clb->SendUnits(unitIdVector, receivingTeam);
}

void* _createSharedMemArea(int teamId, char* name, int size) {
	IAICallback* clb = team_callback[teamId]; return clb->CreateSharedMemArea(name, size);
}

void _releasedSharedMemArea(int teamId, char* name) {
	IAICallback* clb = team_callback[teamId]; clb->ReleasedSharedMemArea(name);
}

int _Group_create(int teamId, char* dll, unsigned aiNumber) {
	IAICallback* clb = team_callback[teamId]; return clb->CreateGroup(dll, aiNumber);
}

void _Group_erase(int teamId, int groupId) {
	IAICallback* clb = team_callback[teamId]; clb->EraseGroup(groupId);
}
*/

int _Group_getNumSupportedCommands(int teamId, int groupId) {
	IAICallback* clb = team_callback[teamId];
	return clb->GetGroupCommands(groupId)->size();
}
int _Group_SupportedCommands_getId(int teamId, int groupId, int* ids) {
	IAICallback* clb = team_callback[teamId];
	const std::vector<CommandDescription>* comDescs = clb->GetGroupCommands(groupId);
	int numComDescs = comDescs->size();
	for (int i=0; i < numComDescs; i++) {
		ids[i] = comDescs->at(i).id;
	}
	return numComDescs;
}
int _Group_SupportedCommands_getName(int teamId, int groupId, const char** names) {
	IAICallback* clb = team_callback[teamId];
	const std::vector<CommandDescription>* comDescs = clb->GetGroupCommands(groupId);
	int numComDescs = comDescs->size();
	for (int i=0; i < numComDescs; i++) {
		names[i] = comDescs->at(i).name.c_str();
	}
	return numComDescs;
}
int _Group_SupportedCommands_getToolTip(int teamId, int groupId, const char** toolTips) {
	IAICallback* clb = team_callback[teamId];
	const std::vector<CommandDescription>* comDescs = clb->GetGroupCommands(groupId);
	int numComDescs = comDescs->size();
	for (int i=0; i < numComDescs; i++) {
		toolTips[i] = comDescs->at(i).tooltip.c_str();
	}
	return numComDescs;
}
int _Group_SupportedCommands_isShowUnique(int teamId, int groupId, bool* showUniques) {
	IAICallback* clb = team_callback[teamId];
	const std::vector<CommandDescription>* comDescs = clb->GetGroupCommands(groupId);
	int numComDescs = comDescs->size();
	for (int i=0; i < numComDescs; i++) {
		showUniques[i] = comDescs->at(i).showUnique;
	}
	return numComDescs;
}
int _Group_SupportedCommands_isDisabled(int teamId, int groupId, bool* disableds) {
	IAICallback* clb = team_callback[teamId];
	const std::vector<CommandDescription>* comDescs = clb->GetGroupCommands(groupId);
	int numComDescs = comDescs->size();
	for (int i=0; i < numComDescs; i++) {
		disableds[i] = comDescs->at(i).disabled;
	}
	return numComDescs;
}
int _Group_SupportedCommands_getNumParams(int teamId, int groupId, int* numParams) {
	IAICallback* clb = team_callback[teamId];
	const std::vector<CommandDescription>* comDescs = clb->GetGroupCommands(groupId);
	int numComDescs = comDescs->size();
	for (int i=0; i < numComDescs; i++) {
		numParams[i] = comDescs->at(i).params.size();
	}
	return numComDescs;
}
int _Group_SupportedCommands_getParams(int teamId, int groupId, const char*** params) {
	IAICallback* clb = team_callback[teamId];
	const std::vector<CommandDescription>* comDescs = clb->GetGroupCommands(groupId);
	int numComDescs = comDescs->size();
	for (int c=0; c < numComDescs; c++) {
		const std::vector<std::string> ps = comDescs->at(c).params;
		for (int p=0; p < ps.size(); p++) {
			params[c][p] = ps.at(p).c_str();
		}
	}
	return numComDescs;
}

/*
bool _Unit_addToGroup(int teamId, int unitId, int groupId) {
	IAICallback* clb = team_callback[teamId]; return clb->AddUnitToGroup(unitId, groupId);
}

bool _Unit_removeFromGroup(int teamId, int unitId) {
	IAICallback* clb = team_callback[teamId]; return clb->RemoveUnitFromGroup(unitId);
}
*/

//int _Group_giveOrder(int teamId, int unitId, SAICommand* c) {
//	IAICallback* clb = team_callback[teamId]; return clb->GiveGroupOrder(unitId, c);
//}
//
//int _Unit_giveOrder(int teamId, int unitId, SAICommand* c) {
//	IAICallback* clb = team_callback[teamId]; return clb->GiveOrder(unitId, c);
//}
/*

int _initPath(int teamId, SAIFloat3 start, SAIFloat3 end, int pathType) {
	IAICallback* clb = team_callback[teamId]; return clb->InitPath(start, end, pathType);
}

void _freePath(int teamId, int pathid) {
	IAICallback* clb = team_callback[teamId]; clb->FreePath(pathid);
}

void _LineDrawer_startPath(int teamId, SAIFloat3 pos, const float* color) {
	IAICallback* clb = team_callback[teamId]; clb->LineDrawerStartPath(pos, color);
}

void _LineDrawer_finishPath(int teamId) {
	IAICallback* clb = team_callback[teamId]; clb->LineDrawerFinishPath();
}

void _LineDrawer_drawLine(int teamId, SAIFloat3 endPos, const float* color) {
	IAICallback* clb = team_callback[teamId]; clb->LineDrawerDrawLine(endPos, color);
}

void _LineDrawer_drawLineAndIcon(int teamId, int cmdID, SAIFloat3 endPos, const float* color) {
	IAICallback* clb = team_callback[teamId]; clb->LineDrawerDrawLineAndIcon(cmdID, endPos, color);
}

void _LineDrawer_drawIconAtLastPos(int teamId, int cmdID) {
	IAICallback* clb = team_callback[teamId]; clb->LineDrawerDrawIconAtLastPos(cmdID);
}

void _LineDrawer_break(int teamId, SAIFloat3 endPos, const float* color) {
	IAICallback* clb = team_callback[teamId]; clb->LineDrawerBreak(endPos, color);
}

void _LineDrawer_restart(int teamId) {
	IAICallback* clb = team_callback[teamId]; clb->LineDrawerRestart();
}

void _LineDrawer_restartSameColor(int teamId) {
	IAICallback* clb = team_callback[teamId]; clb->LineDrawerRestartSameColor();
}

int _createSplineFigure(int teamId, SAIFloat3 pos1, SAIFloat3 pos2, SAIFloat3 pos3, SAIFloat3 pos4, float width, int arrow, int lifetime, int figureGroupId) {
	IAICallback* clb = team_callback[teamId]; return clb->CreateSplineFigure(pos1, pos2, pos3, pos4, width, arrow, lifetime, figureGroupId);
}

int _createLineFigure(int teamId, SAIFloat3 pos1, SAIFloat3 pos2, float width, int arrow, int lifetime, int figureGroupId) {
	IAICallback* clb = team_callback[teamId]; return clb->CreateLineFigure(pos1, pos2, width, arrow, lifetime, figureGroupId);
}

void _setFigureColor(int teamId, int group, float red, float green, float blue, float alpha) {
	IAICallback* clb = team_callback[teamId]; clb->SetFigureColor(group, red, green, blue, alpha);
}

void _deleteFigureGroup(int teamId, int figureGroupId) {
	IAICallback* clb = team_callback[teamId]; clb->DeleteFigureGroup(figureGroupId);
}

void _drawUnit(int teamId, const char* name, SAIFloat3 pos, float rotation, int lifetime, int team, bool transparent, bool drawBorder, int facing) {
	IAICallback* clb = team_callback[teamId]; clb->DrawUnit(name, pos, rotation, lifetime, team, transparent, drawBorder, facing);
}
*/

//int _handleCommand(int teamId, int commandId, void* data) {
//	IAICallback* clb = team_callback[teamId]; return wrapper_HandleCommand(clb, clbCheat, commandId, data);
//}

/*
bool _readFile(int teamId, const char* name, void* buffer, int bufferLen) {
	IAICallback* clb = team_callback[teamId]; return clb->ReadFile(name, buffer, bufferLen);
}

const char* _callLuaRules(int teamId, const char* data, int inSize, int* outSize) {
	IAICallback* clb = team_callback[teamId]; return clb->CallLuaRules(data, inSize, outSize);
}
*/
//##############################################################################




SAICallback* initSAICallback(int teamId, IGlobalAICallback* aiGlobalCallback) {
	
	SAICallback* sAICallback = new SAICallback();

	sAICallback->handleCommand = _handleCommand;
	sAICallback->Game_getCurrentFrame = _Game_getCurrentFrame;
	sAICallback->Game_getAiInterfaceVersion = _Game_getAiInterfaceVersion;
	sAICallback->Game_getMyTeam = _Game_getMyTeam;
	sAICallback->Game_getMyAllyTeam = _Game_getMyAllyTeam;
	sAICallback->Game_getPlayerTeam = _Game_getPlayerTeam;
	sAICallback->Game_getTeamSide = _Game_getTeamSide;
	sAICallback->WeaponDef_STATIC_getNumDamageTypes = _WeaponDef_STATIC_getNumDamageTypes;
	sAICallback->Map_getChecksum = _Map_getChecksum;
	sAICallback->Game_isExceptionHandlingEnabled = _Game_isExceptionHandlingEnabled;
	sAICallback->Game_isDebugModeEnabled = _Game_isDebugModeEnabled;
	sAICallback->Game_getMode = _Game_getMode;
	sAICallback->Game_isPaused = _Game_isPaused;
	sAICallback->Game_getSpeedFactor = _Game_getSpeedFactor;
	sAICallback->Gui_getViewRange = _Gui_getViewRange;
	sAICallback->Gui_getScreenX = _Gui_getScreenX;
	sAICallback->Gui_getScreenY = _Gui_getScreenY;
	sAICallback->Gui_Camera_getDirection = _Gui_Camera_getDirection;
	sAICallback->Gui_Camera_getPosition = _Gui_Camera_getPosition;
	sAICallback->File_locateForReading = _File_locateForReading;
	sAICallback->File_locateForWriting = _File_locateForWriting;
	sAICallback->Unit_STATIC_getLimit = _Unit_STATIC_getLimit;
	sAICallback->Game_getSetupScript = _Game_getSetupScript;
	sAICallback->Cheats_isEnabled = _Cheats_isEnabled;
	sAICallback->Cheats_setEnabled = _Cheats_setEnabled;
	sAICallback->Cheats_setEventsEnabled = _Cheats_setEventsEnabled;
	sAICallback->Cheats_isOnlyPassive = _Cheats_isOnlyPassive;
	sAICallback->ResourceInfo_Metal_getCurrent = _ResourceInfo_Metal_getCurrent;
	sAICallback->ResourceInfo_Metal_getIncome = _ResourceInfo_Metal_getIncome;
	sAICallback->ResourceInfo_Metal_getUsage = _ResourceInfo_Metal_getUsage;
	sAICallback->ResourceInfo_Metal_getStorage = _ResourceInfo_Metal_getStorage;
	sAICallback->ResourceInfo_Energy_getCurrent = _ResourceInfo_Energy_getCurrent;
	sAICallback->ResourceInfo_Energy_getIncome = _ResourceInfo_Energy_getIncome;
	sAICallback->ResourceInfo_Energy_getUsage = _ResourceInfo_Energy_getUsage;
	sAICallback->ResourceInfo_Energy_getStorage = _ResourceInfo_Energy_getStorage;
	sAICallback->File_getSize = _File_getSize;
	sAICallback->File_getContent = _File_getContent;
	sAICallback->UnitDef_STATIC_getIds = _UnitDef_STATIC_getIds;
	sAICallback->UnitDef_STATIC_getNumIds = _UnitDef_STATIC_getNumIds;
	sAICallback->UnitDef_STATIC_getIdByName = _UnitDef_STATIC_getIdByName;
	sAICallback->UnitDef_getHeight = _UnitDef_getHeight;
	sAICallback->UnitDef_getRadius = _UnitDef_getRadius;
	sAICallback->UnitDef_isValid = _UnitDef_isValid;
	sAICallback->UnitDef_getName = _UnitDef_getName;
	sAICallback->UnitDef_getHumanName = _UnitDef_getHumanName;
	sAICallback->UnitDef_getFilename = _UnitDef_getFilename;
	sAICallback->UnitDef_getId = _UnitDef_getId;
	sAICallback->UnitDef_getAiHint = _UnitDef_getAiHint;
	sAICallback->UnitDef_getCobID = _UnitDef_getCobID;
	sAICallback->UnitDef_getTechLevel = _UnitDef_getTechLevel;
	sAICallback->UnitDef_getGaia = _UnitDef_getGaia;
	sAICallback->UnitDef_getMetalUpkeep = _UnitDef_getMetalUpkeep;
	sAICallback->UnitDef_getEnergyUpkeep = _UnitDef_getEnergyUpkeep;
	sAICallback->UnitDef_getMetalMake = _UnitDef_getMetalMake;
	sAICallback->UnitDef_getMakesMetal = _UnitDef_getMakesMetal;
	sAICallback->UnitDef_getEnergyMake = _UnitDef_getEnergyMake;
	sAICallback->UnitDef_getMetalCost = _UnitDef_getMetalCost;
	sAICallback->UnitDef_getEnergyCost = _UnitDef_getEnergyCost;
	sAICallback->UnitDef_getBuildTime = _UnitDef_getBuildTime;
	sAICallback->UnitDef_getExtractsMetal = _UnitDef_getExtractsMetal;
	sAICallback->UnitDef_getExtractRange = _UnitDef_getExtractRange;
	sAICallback->UnitDef_getWindGenerator = _UnitDef_getWindGenerator;
	sAICallback->UnitDef_getTidalGenerator = _UnitDef_getTidalGenerator;
	sAICallback->UnitDef_getMetalStorage = _UnitDef_getMetalStorage;
	sAICallback->UnitDef_getEnergyStorage = _UnitDef_getEnergyStorage;
	sAICallback->UnitDef_getAutoHeal = _UnitDef_getAutoHeal;
	sAICallback->UnitDef_getIdleAutoHeal = _UnitDef_getIdleAutoHeal;
	sAICallback->UnitDef_getIdleTime = _UnitDef_getIdleTime;
	sAICallback->UnitDef_getPower = _UnitDef_getPower;
	sAICallback->UnitDef_getHealth = _UnitDef_getHealth;
	sAICallback->UnitDef_getCategory = _UnitDef_getCategory;
	sAICallback->UnitDef_getSpeed = _UnitDef_getSpeed;
	sAICallback->UnitDef_getTurnRate = _UnitDef_getTurnRate;
	sAICallback->UnitDef_getMoveType = _UnitDef_getMoveType;
	sAICallback->UnitDef_isUpright = _UnitDef_isUpright;
	sAICallback->UnitDef_isCollide = _UnitDef_isCollide;
	sAICallback->UnitDef_getControlRadius = _UnitDef_getControlRadius;
	sAICallback->UnitDef_getLosRadius = _UnitDef_getLosRadius;
	sAICallback->UnitDef_getAirLosRadius = _UnitDef_getAirLosRadius;
	sAICallback->UnitDef_getLosHeight = _UnitDef_getLosHeight;
	sAICallback->UnitDef_getRadarRadius = _UnitDef_getRadarRadius;
	sAICallback->UnitDef_getSonarRadius = _UnitDef_getSonarRadius;
	sAICallback->UnitDef_getJammerRadius = _UnitDef_getJammerRadius;
	sAICallback->UnitDef_getSonarJamRadius = _UnitDef_getSonarJamRadius;
	sAICallback->UnitDef_getSeismicRadius = _UnitDef_getSeismicRadius;
	sAICallback->UnitDef_getSeismicSignature = _UnitDef_getSeismicSignature;
	sAICallback->UnitDef_isStealth = _UnitDef_isStealth;
	sAICallback->UnitDef_isSonarStealth = _UnitDef_isSonarStealth;
	sAICallback->UnitDef_isBuildRange3D = _UnitDef_isBuildRange3D;
	sAICallback->UnitDef_getBuildDistance = _UnitDef_getBuildDistance;
	sAICallback->UnitDef_getBuildSpeed = _UnitDef_getBuildSpeed;
	sAICallback->UnitDef_getReclaimSpeed = _UnitDef_getReclaimSpeed;
	sAICallback->UnitDef_getRepairSpeed = _UnitDef_getRepairSpeed;
	sAICallback->UnitDef_getMaxRepairSpeed = _UnitDef_getMaxRepairSpeed;
	sAICallback->UnitDef_getResurrectSpeed = _UnitDef_getResurrectSpeed;
	sAICallback->UnitDef_getCaptureSpeed = _UnitDef_getCaptureSpeed;
	sAICallback->UnitDef_getTerraformSpeed = _UnitDef_getTerraformSpeed;
	sAICallback->UnitDef_getMass = _UnitDef_getMass;
	sAICallback->UnitDef_isPushResistant = _UnitDef_isPushResistant;
	sAICallback->UnitDef_isStrafeToAttack = _UnitDef_isStrafeToAttack;
	sAICallback->UnitDef_getMinCollisionSpeed = _UnitDef_getMinCollisionSpeed;
	sAICallback->UnitDef_getSlideTolerance = _UnitDef_getSlideTolerance;
	sAICallback->UnitDef_getMaxSlope = _UnitDef_getMaxSlope;
	sAICallback->UnitDef_getMaxHeightDif = _UnitDef_getMaxHeightDif;
	sAICallback->UnitDef_getMinWaterDepth = _UnitDef_getMinWaterDepth;
	sAICallback->UnitDef_getWaterline = _UnitDef_getWaterline;
	sAICallback->UnitDef_getMaxWaterDepth = _UnitDef_getMaxWaterDepth;
	sAICallback->UnitDef_getArmoredMultiple = _UnitDef_getArmoredMultiple;
	sAICallback->UnitDef_getArmorType = _UnitDef_getArmorType;
	sAICallback->UnitDef_getFlankingBonusMode = _UnitDef_getFlankingBonusMode;
	sAICallback->UnitDef_getFlankingBonusDir = _UnitDef_getFlankingBonusDir;
	sAICallback->UnitDef_getFlankingBonusMax = _UnitDef_getFlankingBonusMax;
	sAICallback->UnitDef_getFlankingBonusMin = _UnitDef_getFlankingBonusMin;
	sAICallback->UnitDef_getFlankingBonusMobilityAdd = _UnitDef_getFlankingBonusMobilityAdd;
	sAICallback->UnitDef_getCollisionVolumeType = _UnitDef_getCollisionVolumeType;
	sAICallback->UnitDef_getCollisionVolumeScales = _UnitDef_getCollisionVolumeScales;
	sAICallback->UnitDef_getCollisionVolumeOffsets = _UnitDef_getCollisionVolumeOffsets;
	sAICallback->UnitDef_getCollisionVolumeTest = _UnitDef_getCollisionVolumeTest;
	sAICallback->UnitDef_getMaxWeaponRange = _UnitDef_getMaxWeaponRange;
	sAICallback->UnitDef_getType = _UnitDef_getType;
	sAICallback->UnitDef_getTooltip = _UnitDef_getTooltip;
	sAICallback->UnitDef_getWreckName = _UnitDef_getWreckName;
	sAICallback->UnitDef_getDeathExplosion = _UnitDef_getDeathExplosion;
	sAICallback->UnitDef_getSelfDExplosion = _UnitDef_getSelfDExplosion;
	sAICallback->UnitDef_getTedClassString = _UnitDef_getTedClassString;
	sAICallback->UnitDef_getCategoryString = _UnitDef_getCategoryString;
	sAICallback->UnitDef_isCanSelfD = _UnitDef_isCanSelfD;
	sAICallback->UnitDef_getSelfDCountdown = _UnitDef_getSelfDCountdown;
	sAICallback->UnitDef_isCanSubmerge = _UnitDef_isCanSubmerge;
	sAICallback->UnitDef_isCanFly = _UnitDef_isCanFly;
	sAICallback->UnitDef_isCanMove = _UnitDef_isCanMove;
	sAICallback->UnitDef_isCanHover = _UnitDef_isCanHover;
	sAICallback->UnitDef_isFloater = _UnitDef_isFloater;
	sAICallback->UnitDef_isBuilder = _UnitDef_isBuilder;
	sAICallback->UnitDef_isActivateWhenBuilt = _UnitDef_isActivateWhenBuilt;
	sAICallback->UnitDef_isOnOffable = _UnitDef_isOnOffable;
	sAICallback->UnitDef_isFullHealthFactory = _UnitDef_isFullHealthFactory;
	sAICallback->UnitDef_isFactoryHeadingTakeoff = _UnitDef_isFactoryHeadingTakeoff;
	sAICallback->UnitDef_isReclaimable = _UnitDef_isReclaimable;
	sAICallback->UnitDef_isCapturable = _UnitDef_isCapturable;
	sAICallback->UnitDef_isCanRestore = _UnitDef_isCanRestore;
	sAICallback->UnitDef_isCanRepair = _UnitDef_isCanRepair;
	sAICallback->UnitDef_isCanSelfRepair = _UnitDef_isCanSelfRepair;
	sAICallback->UnitDef_isCanReclaim = _UnitDef_isCanReclaim;
	sAICallback->UnitDef_isCanAttack = _UnitDef_isCanAttack;
	sAICallback->UnitDef_isCanPatrol = _UnitDef_isCanPatrol;
	sAICallback->UnitDef_isCanFight = _UnitDef_isCanFight;
	sAICallback->UnitDef_isCanGuard = _UnitDef_isCanGuard;
	sAICallback->UnitDef_isCanBuild = _UnitDef_isCanBuild;
	sAICallback->UnitDef_isCanAssist = _UnitDef_isCanAssist;
	sAICallback->UnitDef_isCanBeAssisted = _UnitDef_isCanBeAssisted;
	sAICallback->UnitDef_isCanRepeat = _UnitDef_isCanRepeat;
	sAICallback->UnitDef_isCanFireControl = _UnitDef_isCanFireControl;
	sAICallback->UnitDef_getFireState = _UnitDef_getFireState;
	sAICallback->UnitDef_getMoveState = _UnitDef_getMoveState;
	sAICallback->UnitDef_getWingDrag = _UnitDef_getWingDrag;
	sAICallback->UnitDef_getWingAngle = _UnitDef_getWingAngle;
	sAICallback->UnitDef_getDrag = _UnitDef_getDrag;
	sAICallback->UnitDef_getFrontToSpeed = _UnitDef_getFrontToSpeed;
	sAICallback->UnitDef_getSpeedToFront = _UnitDef_getSpeedToFront;
	sAICallback->UnitDef_getMyGravity = _UnitDef_getMyGravity;
	sAICallback->UnitDef_getMaxBank = _UnitDef_getMaxBank;
	sAICallback->UnitDef_getMaxPitch = _UnitDef_getMaxPitch;
	sAICallback->UnitDef_getTurnRadius = _UnitDef_getTurnRadius;
	sAICallback->UnitDef_getWantedHeight = _UnitDef_getWantedHeight;
	sAICallback->UnitDef_getVerticalSpeed = _UnitDef_getVerticalSpeed;
	sAICallback->UnitDef_isCanCrash = _UnitDef_isCanCrash;
	sAICallback->UnitDef_isHoverAttack = _UnitDef_isHoverAttack;
	sAICallback->UnitDef_isAirStrafe = _UnitDef_isAirStrafe;
	sAICallback->UnitDef_getDlHoverFactor = _UnitDef_getDlHoverFactor;
	sAICallback->UnitDef_getMaxAcceleration = _UnitDef_getMaxAcceleration;
	sAICallback->UnitDef_getMaxDeceleration = _UnitDef_getMaxDeceleration;
	sAICallback->UnitDef_getMaxAileron = _UnitDef_getMaxAileron;
	sAICallback->UnitDef_getMaxElevator = _UnitDef_getMaxElevator;
	sAICallback->UnitDef_getMaxRudder = _UnitDef_getMaxRudder;
	sAICallback->UnitDef_getXSize = _UnitDef_getXSize;
	sAICallback->UnitDef_getYSize = _UnitDef_getYSize;
	sAICallback->UnitDef_getBuildAngle = _UnitDef_getBuildAngle;
	sAICallback->UnitDef_getLoadingRadius = _UnitDef_getLoadingRadius;
	sAICallback->UnitDef_getUnloadSpread = _UnitDef_getUnloadSpread;
	sAICallback->UnitDef_getTransportCapacity = _UnitDef_getTransportCapacity;
	sAICallback->UnitDef_getTransportSize = _UnitDef_getTransportSize;
	sAICallback->UnitDef_getMinTransportSize = _UnitDef_getMinTransportSize;
	sAICallback->UnitDef_isAirBase = _UnitDef_isAirBase;
	sAICallback->UnitDef_getTransportMass = _UnitDef_getTransportMass;
	sAICallback->UnitDef_getMinTransportMass = _UnitDef_getMinTransportMass;
	sAICallback->UnitDef_isHoldSteady = _UnitDef_isHoldSteady;
	sAICallback->UnitDef_isReleaseHeld = _UnitDef_isReleaseHeld;
	sAICallback->UnitDef_isCantBeTransported = _UnitDef_isCantBeTransported;
	sAICallback->UnitDef_isTransportByEnemy = _UnitDef_isTransportByEnemy;
	sAICallback->UnitDef_getTransportUnloadMethod = _UnitDef_getTransportUnloadMethod;
	sAICallback->UnitDef_getFallSpeed = _UnitDef_getFallSpeed;
	sAICallback->UnitDef_getUnitFallSpeed = _UnitDef_getUnitFallSpeed;
	sAICallback->UnitDef_isCanCloak = _UnitDef_isCanCloak;
	sAICallback->UnitDef_isStartCloaked = _UnitDef_isStartCloaked;
	sAICallback->UnitDef_getCloakCost = _UnitDef_getCloakCost;
	sAICallback->UnitDef_getCloakCostMoving = _UnitDef_getCloakCostMoving;
	sAICallback->UnitDef_getDecloakDistance = _UnitDef_getDecloakDistance;
	sAICallback->UnitDef_isDecloakSpherical = _UnitDef_isDecloakSpherical;
	sAICallback->UnitDef_isDecloakOnFire = _UnitDef_isDecloakOnFire;
	sAICallback->UnitDef_isCanKamikaze = _UnitDef_isCanKamikaze;
	sAICallback->UnitDef_getKamikazeDist = _UnitDef_getKamikazeDist;
	sAICallback->UnitDef_isTargetingFacility = _UnitDef_isTargetingFacility;
	sAICallback->UnitDef_isCanDGun = _UnitDef_isCanDGun;
	sAICallback->UnitDef_isNeedGeo = _UnitDef_isNeedGeo;
	sAICallback->UnitDef_isFeature = _UnitDef_isFeature;
	sAICallback->UnitDef_isHideDamage = _UnitDef_isHideDamage;
	sAICallback->UnitDef_isCommander = _UnitDef_isCommander;
	sAICallback->UnitDef_isShowPlayerName = _UnitDef_isShowPlayerName;
	sAICallback->UnitDef_isCanResurrect = _UnitDef_isCanResurrect;
	sAICallback->UnitDef_isCanCapture = _UnitDef_isCanCapture;
	sAICallback->UnitDef_getHighTrajectoryType = _UnitDef_getHighTrajectoryType;
	sAICallback->UnitDef_getNoChaseCategory = _UnitDef_getNoChaseCategory;
	sAICallback->UnitDef_isLeaveTracks = _UnitDef_isLeaveTracks;
	sAICallback->UnitDef_getTrackWidth = _UnitDef_getTrackWidth;
	sAICallback->UnitDef_getTrackOffset = _UnitDef_getTrackOffset;
	sAICallback->UnitDef_getTrackStrength = _UnitDef_getTrackStrength;
	sAICallback->UnitDef_getTrackStretch = _UnitDef_getTrackStretch;
	sAICallback->UnitDef_getTrackType = _UnitDef_getTrackType;
	sAICallback->UnitDef_isCanDropFlare = _UnitDef_isCanDropFlare;
	sAICallback->UnitDef_getFlareReloadTime = _UnitDef_getFlareReloadTime;
	sAICallback->UnitDef_getFlareEfficiency = _UnitDef_getFlareEfficiency;
	sAICallback->UnitDef_getFlareDelay = _UnitDef_getFlareDelay;
	sAICallback->UnitDef_getFlareDropVector = _UnitDef_getFlareDropVector;
	sAICallback->UnitDef_getFlareTime = _UnitDef_getFlareTime;
	sAICallback->UnitDef_getFlareSalvoSize = _UnitDef_getFlareSalvoSize;
	sAICallback->UnitDef_getFlareSalvoDelay = _UnitDef_getFlareSalvoDelay;
	sAICallback->UnitDef_isSmoothAnim = _UnitDef_isSmoothAnim;
	sAICallback->UnitDef_isMetalMaker = _UnitDef_isMetalMaker;
	sAICallback->UnitDef_isCanLoopbackAttack = _UnitDef_isCanLoopbackAttack;
	sAICallback->UnitDef_isLevelGround = _UnitDef_isLevelGround;
	sAICallback->UnitDef_isUseBuildingGroundDecal = _UnitDef_isUseBuildingGroundDecal;
	sAICallback->UnitDef_getBuildingDecalType = _UnitDef_getBuildingDecalType;
	sAICallback->UnitDef_getBuildingDecalSizeX = _UnitDef_getBuildingDecalSizeX;
	sAICallback->UnitDef_getBuildingDecalSizeY = _UnitDef_getBuildingDecalSizeY;
	sAICallback->UnitDef_getBuildingDecalDecaySpeed = _UnitDef_getBuildingDecalDecaySpeed;
	sAICallback->UnitDef_isFirePlatform = _UnitDef_isFirePlatform;
	sAICallback->UnitDef_getMaxFuel = _UnitDef_getMaxFuel;
	sAICallback->UnitDef_getRefuelTime = _UnitDef_getRefuelTime;
	sAICallback->UnitDef_getMinAirBasePower = _UnitDef_getMinAirBasePower;
	sAICallback->UnitDef_getMaxThisUnit = _UnitDef_getMaxThisUnit;
	sAICallback->UnitDef_getDecoyDefId = _UnitDef_getDecoyDefId;
	sAICallback->UnitDef_isDontLand = _UnitDef_isDontLand;
	sAICallback->UnitDef_getShieldWeaponDefId = _UnitDef_getShieldWeaponDefId;
	sAICallback->UnitDef_getStockpileWeaponDefId = _UnitDef_getStockpileWeaponDefId;
	sAICallback->UnitDef_getNumBuildOptions = _UnitDef_getNumBuildOptions;
	sAICallback->UnitDef_getBuildOptions = _UnitDef_getBuildOptions;
	sAICallback->UnitDef_getNumCustomParams = _UnitDef_getNumCustomParams;
	sAICallback->UnitDef_getCustomParams = _UnitDef_getCustomParams;
	sAICallback->UnitDef_hasMoveData = _UnitDef_hasMoveData;
	sAICallback->UnitDef_MoveData_getMoveType = _UnitDef_MoveData_getMoveType;
	sAICallback->UnitDef_MoveData_getMoveFamily = _UnitDef_MoveData_getMoveFamily;
	sAICallback->UnitDef_MoveData_getSize = _UnitDef_MoveData_getSize;
	sAICallback->UnitDef_MoveData_getDepth = _UnitDef_MoveData_getDepth;
	sAICallback->UnitDef_MoveData_getMaxSlope = _UnitDef_MoveData_getMaxSlope;
	sAICallback->UnitDef_MoveData_getSlopeMod = _UnitDef_MoveData_getSlopeMod;
	sAICallback->UnitDef_MoveData_getDepthMod = _UnitDef_MoveData_getDepthMod;
	sAICallback->UnitDef_MoveData_getPathType = _UnitDef_MoveData_getPathType;
	sAICallback->UnitDef_MoveData_getCrushStrength = _UnitDef_MoveData_getCrushStrength;
	sAICallback->UnitDef_MoveData_getMaxSpeed = _UnitDef_MoveData_getMaxSpeed;
	sAICallback->UnitDef_MoveData_getMaxTurnRate = _UnitDef_MoveData_getMaxTurnRate;
	sAICallback->UnitDef_MoveData_getMaxAcceleration = _UnitDef_MoveData_getMaxAcceleration;
	sAICallback->UnitDef_MoveData_getMaxBreaking = _UnitDef_MoveData_getMaxBreaking;
	sAICallback->UnitDef_MoveData_isSubMarine = _UnitDef_MoveData_isSubMarine;
	sAICallback->UnitDef_getNumUnitDefWeapons = _UnitDef_getNumUnitDefWeapons;
	sAICallback->UnitDef_UnitDefWeapon_getName = _UnitDef_UnitDefWeapon_getName;
	sAICallback->UnitDef_UnitDefWeapon_getWeaponDefId = _UnitDef_UnitDefWeapon_getWeaponDefId;
	sAICallback->UnitDef_UnitDefWeapon_getSlavedTo = _UnitDef_UnitDefWeapon_getSlavedTo;
	sAICallback->UnitDef_UnitDefWeapon_getMainDir = _UnitDef_UnitDefWeapon_getMainDir;
	sAICallback->UnitDef_UnitDefWeapon_getMaxAngleDif = _UnitDef_UnitDefWeapon_getMaxAngleDif;
	sAICallback->UnitDef_UnitDefWeapon_getFuelUsage = _UnitDef_UnitDefWeapon_getFuelUsage;
	sAICallback->UnitDef_UnitDefWeapon_getBadTargetCat = _UnitDef_UnitDefWeapon_getBadTargetCat;
	sAICallback->UnitDef_UnitDefWeapon_getOnlyTargetCat = _UnitDef_UnitDefWeapon_getOnlyTargetCat;
	sAICallback->Unit_STATIC_getEnemies = _Unit_STATIC_getEnemies;
	sAICallback->Unit_STATIC_getEnemiesIn = _Unit_STATIC_getEnemiesIn;
	sAICallback->Unit_STATIC_getEnemiesInRadarAndLos = _Unit_STATIC_getEnemiesInRadarAndLos;
	sAICallback->Unit_STATIC_getFriendlies = _Unit_STATIC_getFriendlies;
	sAICallback->Unit_STATIC_getFriendliesIn = _Unit_STATIC_getFriendliesIn;
	sAICallback->Unit_STATIC_getNeutrals = _Unit_STATIC_getNeutrals;
	sAICallback->Unit_STATIC_getNeutralsIn = _Unit_STATIC_getNeutralsIn;
	sAICallback->Unit_STATIC_getSelected = _Unit_STATIC_getSelected;
	sAICallback->Unit_getDefId = _Unit_getDefId;
	sAICallback->Unit_getAiHint = _Unit_getAiHint;
	sAICallback->Unit_getTeam = _Unit_getTeam;
	sAICallback->Unit_getAllyTeam = _Unit_getAllyTeam;
	sAICallback->Unit_getStockpile = _Unit_getStockpile;
	sAICallback->Unit_getStockpileQueued = _Unit_getStockpileQueued;
	sAICallback->Unit_getCurrentFuel = _Unit_getCurrentFuel;
	sAICallback->Unit_getMaxSpeed = _Unit_getMaxSpeed;
	sAICallback->Unit_getMaxRange = _Unit_getMaxRange;
	sAICallback->Unit_getMaxHealth = _Unit_getMaxHealth;
	sAICallback->Unit_getExperience = _Unit_getExperience;
	sAICallback->Unit_getGroup = _Unit_getGroup;
	sAICallback->Unit_getNumCurrentCommands = _Unit_getNumCurrentCommands;
	sAICallback->Unit_CurrentCommands_getType = _Unit_CurrentCommands_getType;
	sAICallback->Unit_CurrentCommands_getIds = _Unit_CurrentCommands_getIds;
	sAICallback->Unit_CurrentCommands_getOptions = _Unit_CurrentCommands_getOptions;
	sAICallback->Unit_CurrentCommands_getTag = _Unit_CurrentCommands_getTag;
	sAICallback->Unit_CurrentCommands_getTimeOut = _Unit_CurrentCommands_getTimeOut;
	sAICallback->Unit_CurrentCommands_getNumParams = _Unit_CurrentCommands_getNumParams;
	sAICallback->Unit_CurrentCommands_getParams = _Unit_CurrentCommands_getParams;
	sAICallback->Unit_getNumSupportedCommands = _Unit_getNumSupportedCommands;
	sAICallback->Unit_SupportedCommands_getId = _Unit_SupportedCommands_getId;
	sAICallback->Unit_SupportedCommands_getName = _Unit_SupportedCommands_getName;
	sAICallback->Unit_SupportedCommands_getToolTip = _Unit_SupportedCommands_getToolTip;
	sAICallback->Unit_SupportedCommands_isShowUnique = _Unit_SupportedCommands_isShowUnique;
	sAICallback->Unit_SupportedCommands_isDisabled = _Unit_SupportedCommands_isDisabled;
	sAICallback->Unit_SupportedCommands_getNumParams = _Unit_SupportedCommands_getNumParams;
	sAICallback->Unit_SupportedCommands_getParams = _Unit_SupportedCommands_getParams;
	sAICallback->Unit_getHealth = _Unit_getHealth;
	sAICallback->Unit_getSpeed = _Unit_getSpeed;
	sAICallback->Unit_getPower = _Unit_getPower;
	sAICallback->Unit_ResourceInfo_Metal_getUse = _Unit_ResourceInfo_Metal_getUse;
	sAICallback->Unit_ResourceInfo_Metal_getMake = _Unit_ResourceInfo_Metal_getMake;
	sAICallback->Unit_ResourceInfo_Energy_getUse = _Unit_ResourceInfo_Energy_getUse;
	sAICallback->Unit_ResourceInfo_Energy_getMake = _Unit_ResourceInfo_Energy_getMake;
	sAICallback->Unit_getPos = _Unit_getPos;
	sAICallback->Unit_isActivated = _Unit_isActivated;
	sAICallback->Unit_isBeingBuilt = _Unit_isBeingBuilt;
	sAICallback->Unit_isCloaked = _Unit_isCloaked;
	sAICallback->Unit_isParalyzed = _Unit_isParalyzed;
	sAICallback->Unit_isNeutral = _Unit_isNeutral;
	sAICallback->Unit_getBuildingFacing = _Unit_getBuildingFacing;
	sAICallback->Group_getNumSupportedCommands = _Group_getNumSupportedCommands;
	sAICallback->Group_SupportedCommands_getId = _Group_SupportedCommands_getId;
	sAICallback->Group_SupportedCommands_getName = _Group_SupportedCommands_getName;
	sAICallback->Group_SupportedCommands_getToolTip = _Group_SupportedCommands_getToolTip;
	sAICallback->Group_SupportedCommands_isShowUnique = _Group_SupportedCommands_isShowUnique;
	sAICallback->Group_SupportedCommands_isDisabled = _Group_SupportedCommands_isDisabled;
	sAICallback->Group_SupportedCommands_getNumParams = _Group_SupportedCommands_getNumParams;
	sAICallback->Group_SupportedCommands_getParams = _Group_SupportedCommands_getParams;
	sAICallback->Mod_getName = _Mod_getName;
	sAICallback->Map_getStartPos = _Map_getStartPos;
	sAICallback->Map_getMousePos = _Map_getMousePos;
	sAICallback->Map_isPosInCamera = _Map_isPosInCamera;
	sAICallback->Map_getWidth = _Map_getWidth;
	sAICallback->Map_getHeight = _Map_getHeight;
	sAICallback->Map_getHeightMap = _Map_getHeightMap;
	sAICallback->Map_getMinHeight = _Map_getMinHeight;
	sAICallback->Map_getMaxHeight = _Map_getMaxHeight;
	sAICallback->Map_getSlopeMap = _Map_getSlopeMap;
	sAICallback->Map_getLosMap = _Map_getLosMap;
	sAICallback->Map_getRadarMap = _Map_getRadarMap;
	sAICallback->Map_getJammerMap = _Map_getJammerMap;
	sAICallback->Map_getMetalMap = _Map_getMetalMap;
	sAICallback->Map_getName = _Map_getName;
	sAICallback->Map_getElevationAt = _Map_getElevationAt;
	sAICallback->Map_getMaxMetal = _Map_getMaxMetal;
	sAICallback->Map_getExtractorRadius = _Map_getExtractorRadius;
	sAICallback->Map_getMinWind = _Map_getMinWind;
	sAICallback->Map_getMaxWind = _Map_getMaxWind;
	sAICallback->Map_getTidalStrength = _Map_getTidalStrength;
	sAICallback->Map_getGravity = _Map_getGravity;
	sAICallback->Map_getPoints = _Map_getPoints;
	sAICallback->Map_getLines = _Map_getLines;
	sAICallback->Map_canBuildAt = _Map_canBuildAt;
	sAICallback->Map_findClosestBuildSite = _Map_findClosestBuildSite;
	sAICallback->FeatureDef_getName = _FeatureDef_getName;
	sAICallback->FeatureDef_getDescription = _FeatureDef_getDescription;
	sAICallback->FeatureDef_getFilename = _FeatureDef_getFilename;
	sAICallback->FeatureDef_getId = _FeatureDef_getId;
	sAICallback->FeatureDef_getMetal = _FeatureDef_getMetal;
	sAICallback->FeatureDef_getEnergy = _FeatureDef_getEnergy;
	sAICallback->FeatureDef_getMaxHealth = _FeatureDef_getMaxHealth;
	sAICallback->FeatureDef_getReclaimTime = _FeatureDef_getReclaimTime;
	sAICallback->FeatureDef_getMass = _FeatureDef_getMass;
	sAICallback->FeatureDef_getCollisionVolumeType = _FeatureDef_getCollisionVolumeType;
	sAICallback->FeatureDef_getCollisionVolumeScales = _FeatureDef_getCollisionVolumeScales;
	sAICallback->FeatureDef_getCollisionVolumeOffsets = _FeatureDef_getCollisionVolumeOffsets;
	sAICallback->FeatureDef_getCollisionVolumeTest = _FeatureDef_getCollisionVolumeTest;
	sAICallback->FeatureDef_isUpright = _FeatureDef_isUpright;
	sAICallback->FeatureDef_getDrawType = _FeatureDef_getDrawType;
	sAICallback->FeatureDef_getModelName = _FeatureDef_getModelName;
	sAICallback->FeatureDef_getModelType = _FeatureDef_getModelType;
	sAICallback->FeatureDef_isDestructable = _FeatureDef_isDestructable;
	sAICallback->FeatureDef_isReclaimable = _FeatureDef_isReclaimable;
	sAICallback->FeatureDef_isBlocking = _FeatureDef_isBlocking;
	sAICallback->FeatureDef_isBurnable = _FeatureDef_isBurnable;
	sAICallback->FeatureDef_isFloating = _FeatureDef_isFloating;
	sAICallback->FeatureDef_isNoSelect = _FeatureDef_isNoSelect;
	sAICallback->FeatureDef_isGeoThermal = _FeatureDef_isGeoThermal;
	sAICallback->FeatureDef_getDeathFeature = _FeatureDef_getDeathFeature;
	sAICallback->FeatureDef_getXsize = _FeatureDef_getXsize;
	sAICallback->FeatureDef_getYsize = _FeatureDef_getYsize;
	sAICallback->FeatureDef_getNumCustomParams = _FeatureDef_getNumCustomParams;
	sAICallback->FeatureDef_getCustomParams = _FeatureDef_getCustomParams;
	sAICallback->Feature_STATIC_getIds = _Feature_STATIC_getIds;
	sAICallback->Feature_STATIC_getIdsIn = _Feature_STATIC_getIdsIn;
	sAICallback->Feature_getDefId = _Feature_getDefId;
	sAICallback->Feature_getHealth = _Feature_getHealth;
	sAICallback->Feature_getReclaimLeft = _Feature_getReclaimLeft;
	sAICallback->Feature_getPos = _Feature_getPos;
	sAICallback->WeaponDef_STATIC_getIdByName = _WeaponDef_STATIC_getIdByName;
	sAICallback->WeaponDef_getName = _WeaponDef_getName;
	sAICallback->WeaponDef_getType = _WeaponDef_getType;
	sAICallback->WeaponDef_getDescription = _WeaponDef_getDescription;
	sAICallback->WeaponDef_getFilename = _WeaponDef_getFilename;
	sAICallback->WeaponDef_getCegTag = _WeaponDef_getCegTag;
	sAICallback->WeaponDef_getRange = _WeaponDef_getRange;
	sAICallback->WeaponDef_getHeightMod = _WeaponDef_getHeightMod;
	sAICallback->WeaponDef_getAccuracy = _WeaponDef_getAccuracy;
	sAICallback->WeaponDef_getSprayAngle = _WeaponDef_getSprayAngle;
	sAICallback->WeaponDef_getMovingAccuracy = _WeaponDef_getMovingAccuracy;
	sAICallback->WeaponDef_getTargetMoveError = _WeaponDef_getTargetMoveError;
	sAICallback->WeaponDef_getLeadLimit = _WeaponDef_getLeadLimit;
	sAICallback->WeaponDef_getLeadBonus = _WeaponDef_getLeadBonus;
	sAICallback->WeaponDef_getPredictBoost = _WeaponDef_getPredictBoost;
	sAICallback->WeaponDef_Damages_getParalyzeDamageTime = _WeaponDef_Damages_getParalyzeDamageTime;
	sAICallback->WeaponDef_Damages_getImpulseFactor = _WeaponDef_Damages_getImpulseFactor;
	sAICallback->WeaponDef_Damages_getImpulseBoost = _WeaponDef_Damages_getImpulseBoost;
	sAICallback->WeaponDef_Damages_getCraterMult = _WeaponDef_Damages_getCraterMult;
	sAICallback->WeaponDef_Damages_getCraterBoost = _WeaponDef_Damages_getCraterBoost;
	sAICallback->WeaponDef_Damages_getNumTypes = _WeaponDef_Damages_getNumTypes;
	sAICallback->WeaponDef_Damages_getTypeDamages = _WeaponDef_Damages_getTypeDamages;
	sAICallback->WeaponDef_getAreaOfEffect = _WeaponDef_getAreaOfEffect;
	sAICallback->WeaponDef_isNoSelfDamage = _WeaponDef_isNoSelfDamage;
	sAICallback->WeaponDef_getFireStarter = _WeaponDef_getFireStarter;
	sAICallback->WeaponDef_getEdgeEffectiveness = _WeaponDef_getEdgeEffectiveness;
	sAICallback->WeaponDef_getSize = _WeaponDef_getSize;
	sAICallback->WeaponDef_getSizeGrowth = _WeaponDef_getSizeGrowth;
	sAICallback->WeaponDef_getCollisionSize = _WeaponDef_getCollisionSize;
	sAICallback->WeaponDef_getSalvoSize = _WeaponDef_getSalvoSize;
	sAICallback->WeaponDef_getSalvoDelay = _WeaponDef_getSalvoDelay;
	sAICallback->WeaponDef_getReload = _WeaponDef_getReload;
	sAICallback->WeaponDef_getBeamTime = _WeaponDef_getBeamTime;
	sAICallback->WeaponDef_isBeamBurst = _WeaponDef_isBeamBurst;
	sAICallback->WeaponDef_isWaterBounce = _WeaponDef_isWaterBounce;
	sAICallback->WeaponDef_isGroundBounce = _WeaponDef_isGroundBounce;
	sAICallback->WeaponDef_getBounceRebound = _WeaponDef_getBounceRebound;
	sAICallback->WeaponDef_getBounceSlip = _WeaponDef_getBounceSlip;
	sAICallback->WeaponDef_getNumBounce = _WeaponDef_getNumBounce;
	sAICallback->WeaponDef_getMaxAngle = _WeaponDef_getMaxAngle;
	sAICallback->WeaponDef_getRestTime = _WeaponDef_getRestTime;
	sAICallback->WeaponDef_getUpTime = _WeaponDef_getUpTime;
	sAICallback->WeaponDef_getFlightTime = _WeaponDef_getFlightTime;
	sAICallback->WeaponDef_getMetalCost = _WeaponDef_getMetalCost;
	sAICallback->WeaponDef_getEnergyCost = _WeaponDef_getEnergyCost;
	sAICallback->WeaponDef_getSupplyCost = _WeaponDef_getSupplyCost;
	sAICallback->WeaponDef_getProjectilesPerShot = _WeaponDef_getProjectilesPerShot;
	sAICallback->WeaponDef_getId = _WeaponDef_getId;
	sAICallback->WeaponDef_getTdfId = _WeaponDef_getTdfId;
	sAICallback->WeaponDef_isTurret = _WeaponDef_isTurret;
	sAICallback->WeaponDef_isOnlyForward = _WeaponDef_isOnlyForward;
	sAICallback->WeaponDef_isFixedLauncher = _WeaponDef_isFixedLauncher;
	sAICallback->WeaponDef_isWaterWeapon = _WeaponDef_isWaterWeapon;
	sAICallback->WeaponDef_isFireSubmersed = _WeaponDef_isFireSubmersed;
	sAICallback->WeaponDef_isSubMissile = _WeaponDef_isSubMissile;
	sAICallback->WeaponDef_isTracks = _WeaponDef_isTracks;
	sAICallback->WeaponDef_isDropped = _WeaponDef_isDropped;
	sAICallback->WeaponDef_isParalyzer = _WeaponDef_isParalyzer;
	sAICallback->WeaponDef_isImpactOnly = _WeaponDef_isImpactOnly;
	sAICallback->WeaponDef_isNoAutoTarget = _WeaponDef_isNoAutoTarget;
	sAICallback->WeaponDef_isManualFire = _WeaponDef_isManualFire;
	sAICallback->WeaponDef_getInterceptor = _WeaponDef_getInterceptor;
	sAICallback->WeaponDef_getTargetable = _WeaponDef_getTargetable;
	sAICallback->WeaponDef_isStockpileable = _WeaponDef_isStockpileable;
	sAICallback->WeaponDef_getCoverageRange = _WeaponDef_getCoverageRange;
	sAICallback->WeaponDef_getIntensity = _WeaponDef_getIntensity;
	sAICallback->WeaponDef_getThickness = _WeaponDef_getThickness;
	sAICallback->WeaponDef_getLaserFlareSize = _WeaponDef_getLaserFlareSize;
	sAICallback->WeaponDef_getCoreThickness = _WeaponDef_getCoreThickness;
	sAICallback->WeaponDef_getDuration = _WeaponDef_getDuration;
	sAICallback->WeaponDef_getLodDistance = _WeaponDef_getLodDistance;
	sAICallback->WeaponDef_getFalloffRate = _WeaponDef_getFalloffRate;
	sAICallback->WeaponDef_getGraphicsType = _WeaponDef_getGraphicsType;
	sAICallback->WeaponDef_isSoundTrigger = _WeaponDef_isSoundTrigger;
	sAICallback->WeaponDef_isSelfExplode = _WeaponDef_isSelfExplode;
	sAICallback->WeaponDef_isGravityAffected = _WeaponDef_isGravityAffected;
	sAICallback->WeaponDef_getHighTrajectory = _WeaponDef_getHighTrajectory;
	sAICallback->WeaponDef_getMyGravity = _WeaponDef_getMyGravity;
	sAICallback->WeaponDef_isTwoPhase = _WeaponDef_isTwoPhase;
	sAICallback->WeaponDef_isGuided = _WeaponDef_isGuided;
	sAICallback->WeaponDef_isVLaunched = _WeaponDef_isVLaunched;
	sAICallback->WeaponDef_isSelfPropelled = _WeaponDef_isSelfPropelled;
	sAICallback->WeaponDef_isNoExplode = _WeaponDef_isNoExplode;
	sAICallback->WeaponDef_getStartVelocity = _WeaponDef_getStartVelocity;
	sAICallback->WeaponDef_getWeaponAcceleration = _WeaponDef_getWeaponAcceleration;
	sAICallback->WeaponDef_getTurnRate = _WeaponDef_getTurnRate;
	sAICallback->WeaponDef_getMaxVelocity = _WeaponDef_getMaxVelocity;
	sAICallback->WeaponDef_getProjectileSpeed = _WeaponDef_getProjectileSpeed;
	sAICallback->WeaponDef_getExplosionSpeed = _WeaponDef_getExplosionSpeed;
	sAICallback->WeaponDef_getOnlyTargetCategory = _WeaponDef_getOnlyTargetCategory;
	sAICallback->WeaponDef_getWobble = _WeaponDef_getWobble;
	sAICallback->WeaponDef_getDance = _WeaponDef_getDance;
	sAICallback->WeaponDef_getTrajectoryHeight = _WeaponDef_getTrajectoryHeight;
	sAICallback->WeaponDef_isLargeBeamLaser = _WeaponDef_isLargeBeamLaser;
	sAICallback->WeaponDef_isShield = _WeaponDef_isShield;
	sAICallback->WeaponDef_isShieldRepulser = _WeaponDef_isShieldRepulser;
	sAICallback->WeaponDef_isSmartShield = _WeaponDef_isSmartShield;
	sAICallback->WeaponDef_isExteriorShield = _WeaponDef_isExteriorShield;
	sAICallback->WeaponDef_isVisibleShield = _WeaponDef_isVisibleShield;
	sAICallback->WeaponDef_isVisibleShieldRepulse = _WeaponDef_isVisibleShieldRepulse;
	sAICallback->WeaponDef_getVisibleShieldHitFrames = _WeaponDef_getVisibleShieldHitFrames;
	sAICallback->WeaponDef_getShieldEnergyUse = _WeaponDef_getShieldEnergyUse;
	sAICallback->WeaponDef_getShieldRadius = _WeaponDef_getShieldRadius;
	sAICallback->WeaponDef_getShieldForce = _WeaponDef_getShieldForce;
	sAICallback->WeaponDef_getShieldMaxSpeed = _WeaponDef_getShieldMaxSpeed;
	sAICallback->WeaponDef_getShieldPower = _WeaponDef_getShieldPower;
	sAICallback->WeaponDef_getShieldPowerRegen = _WeaponDef_getShieldPowerRegen;
	sAICallback->WeaponDef_getShieldPowerRegenEnergy = _WeaponDef_getShieldPowerRegenEnergy;
	sAICallback->WeaponDef_getShieldStartingPower = _WeaponDef_getShieldStartingPower;
	sAICallback->WeaponDef_getShieldRechargeDelay = _WeaponDef_getShieldRechargeDelay;
	sAICallback->WeaponDef_getShieldGoodColor = _WeaponDef_getShieldGoodColor;
	sAICallback->WeaponDef_getShieldBadColor = _WeaponDef_getShieldBadColor;
	sAICallback->WeaponDef_getShieldAlpha = _WeaponDef_getShieldAlpha;
	sAICallback->WeaponDef_getShieldInterceptType = _WeaponDef_getShieldInterceptType;
	sAICallback->WeaponDef_getInterceptedByShieldType = _WeaponDef_getInterceptedByShieldType;
	sAICallback->WeaponDef_isAvoidFriendly = _WeaponDef_isAvoidFriendly;
	sAICallback->WeaponDef_isAvoidFeature = _WeaponDef_isAvoidFeature;
	sAICallback->WeaponDef_isAvoidNeutral = _WeaponDef_isAvoidNeutral;
	sAICallback->WeaponDef_getTargetBorder = _WeaponDef_getTargetBorder;
	sAICallback->WeaponDef_getCylinderTargetting = _WeaponDef_getCylinderTargetting;
	sAICallback->WeaponDef_getMinIntensity = _WeaponDef_getMinIntensity;
	sAICallback->WeaponDef_getHeightBoostFactor = _WeaponDef_getHeightBoostFactor;
	sAICallback->WeaponDef_getProximityPriority = _WeaponDef_getProximityPriority;
	sAICallback->WeaponDef_getCollisionFlags = _WeaponDef_getCollisionFlags;
	sAICallback->WeaponDef_isSweepFire = _WeaponDef_isSweepFire;
	sAICallback->WeaponDef_isCanAttackGround = _WeaponDef_isCanAttackGround;
	sAICallback->WeaponDef_getCameraShake = _WeaponDef_getCameraShake;
	sAICallback->WeaponDef_getDynDamageExp = _WeaponDef_getDynDamageExp;
	sAICallback->WeaponDef_getDynDamageMin = _WeaponDef_getDynDamageMin;
	sAICallback->WeaponDef_getDynDamageRange = _WeaponDef_getDynDamageRange;
	sAICallback->WeaponDef_isDynDamageInverted = _WeaponDef_isDynDamageInverted;
	sAICallback->WeaponDef_getNumCustomParams = _WeaponDef_getNumCustomParams;
	sAICallback->WeaponDef_getCustomParams = _WeaponDef_getCustomParams;
	
	team_globalCallback[teamId] = aiGlobalCallback;
//	team_callback[teamId] = aiCallback;
	team_callback[teamId] = aiGlobalCallback->GetAICallback();
	
	return sAICallback;
}
