#include "StdAfx.h"
#include "AirCAI.h"
#include "LineDrawer.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Map/Ground.h"
#include "Sim/Units/UnitHandler.h"
#include "ExternalAI/Group.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/CursorIcons.h"
#include "LogOutput.h"
#include "Sim/Units/UnitDef.h"
#include "myMath.h"
#include "mmgr.h"

CAirCAI::CAirCAI(CUnit* owner)
: CCommandAI(owner), lastPC1(-1), lastPC2(-1)
{
	CommandDescription c;

	// aircraft can always move...
	c.id=CMD_MOVE;
	c.action="move";
	c.type=CMDTYPE_ICON_MAP;
	c.name="Move";
	c.hotkey="m";
	c.tooltip="Move: Commands the aircraft to fly to the location";
	possibleCommands.push_back(c);

	if(owner->unitDef->canPatrol){
		c.id=CMD_PATROL;
		c.action="patrol";
		c.type=CMDTYPE_ICON_MAP;
		c.name="Patrol";
		c.hotkey="p";
		c.tooltip="Patrol: Sets the aircraft to patrol a path to one or more waypoints";
		possibleCommands.push_back(c);
	}

	if (owner->unitDef->canFight) {
		c.id = CMD_FIGHT;
		c.action="fight";
		c.type = CMDTYPE_ICON_MAP;
		c.name = "Fight";
		c.hotkey = "f";
		c.tooltip = "Fight: Order the aircraft to take action while moving to a position";
		possibleCommands.push_back(c);
	}

	if(owner->unitDef->canAttack){
		c.id=CMD_AREA_ATTACK;
		c.action="areaattack";
		c.type=CMDTYPE_ICON_AREA;
		c.name="Area attack";
		c.hotkey="a";
		c.tooltip="Sets the aircraft to attack enemy units within a circle";
		possibleCommands.push_back(c);
	}

	if(owner->unitDef->canGuard){
		c.id=CMD_GUARD;
		c.action="guard";
		c.type=CMDTYPE_ICON_UNIT;
		c.name="Guard";
		c.hotkey="g";
		c.tooltip="Guard: Order a unit to guard another unit and attack units attacking it";
		possibleCommands.push_back(c);
	}

	c.params.clear();
	c.id=CMD_AUTOREPAIRLEVEL;
	c.action="autorepairlevel";
	c.type=CMDTYPE_ICON_MODE;
	c.name="Repair level";
	c.params.push_back("1");
	c.params.push_back("LandAt 0");
	c.params.push_back("LandAt 30");
	c.params.push_back("LandAt 50");
	c.tooltip="Repair level: Sets at which health level an aircraft will try to find a repair pad";
	c.hotkey="";
	possibleCommands.push_back(c);
	nonQueingCommands.insert(CMD_AUTOREPAIRLEVEL);

	if(owner->unitDef->canLoopbackAttack){
		c.params.clear();
		c.id=CMD_LOOPBACKATTACK;
		c.action="loopbackattack";
		c.type=CMDTYPE_ICON_MODE;
		c.name="Loopback";
		c.params.push_back("0");
		c.params.push_back("Normal");
		c.params.push_back("Loopback");
		c.tooltip="Loopback attack: Sets if the aircraft should loopback after an attack instead of overflying target";
		c.hotkey="";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_LOOPBACKATTACK);
	}

	basePos=owner->pos;
	goalPos=owner->pos;

	tempOrder=false;
	targetAge=0;
	commandPos1=ZeroVector;
	commandPos2=ZeroVector;
}

CAirCAI::~CAirCAI(void)
{
}

void CAirCAI::GiveCommand(const Command &c)
{
	if (c.id != CMD_MOVE && !AllowedCommand(c))
		return;

	if (c.id==CMD_SET_WANTED_MAX_SPEED) {
	  return;
	}
	
	if(c.id==CMD_AUTOREPAIRLEVEL){
		if(c.params.empty())
			return;
		switch((int)c.params[0]){
		case 0:
			((CAirMoveType*)owner->moveType)->repairBelowHealth=0;
			break;
		case 1:
			((CAirMoveType*)owner->moveType)->repairBelowHealth=0.3f;
			break;
		case 2:
			((CAirMoveType*)owner->moveType)->repairBelowHealth=0.5f;
			break;
		}
		for(vector<CommandDescription>::iterator cdi=possibleCommands.begin();cdi!=possibleCommands.end();++cdi){
			if(cdi->id==CMD_AUTOREPAIRLEVEL){
				char t[10];
				SNPRINTF(t,10,"%d", (int)c.params[0]);
				cdi->params[0]=t;
				break;
			}
		}
		selectedUnits.PossibleCommandChange(owner);
		return;
	}
	if(c.id==CMD_LOOPBACKATTACK){
		if(c.params.empty())
			return;
		switch((int)c.params[0]){
		case 0:
			((CAirMoveType*)owner->moveType)->loopbackAttack=false;
			break;
		case 1:
			((CAirMoveType*)owner->moveType)->loopbackAttack=true;
			break;
		}
		for(vector<CommandDescription>::iterator cdi=possibleCommands.begin();cdi!=possibleCommands.end();++cdi){
			if(cdi->id==CMD_LOOPBACKATTACK){
				char t[10];
				SNPRINTF(t,10,"%d", (int)c.params[0]);
				cdi->params[0]=t;
				break;
			}
		}
		selectedUnits.PossibleCommandChange(owner);
		return;
	}
	if(!(c.options & SHIFT_KEY) && nonQueingCommands.find(c.id)==nonQueingCommands.end()){
		activeCommand=0;
		tempOrder=false;
	}
	if(c.id==CMD_AREA_ATTACK && c.params.size()<4){
		Command c2 = c;
		c2.id=CMD_ATTACK;
		CCommandAI::GiveAllowedCommand(c2);
		return;
	}

	CCommandAI::GiveAllowedCommand(c);
}

void CAirCAI::SlowUpdate()
{
	if(!commandQue.empty() && commandQue.front().timeOut<gs->frameNum){
		FinishCommand();
		return;
	}

	CAirMoveType* myPlane=(CAirMoveType*)owner->moveType;

	if(owner->unitDef->maxFuel>0){
		if(myPlane->reservedPad){
			return;
		}else{
			if(owner->currentFuel <= 0){
				owner->userAttackGround=false;
				owner->userTarget=0;
				inCommand=false;

				CAirBaseHandler::LandingPad* lp=airBaseHandler->FindAirBase(owner,8000,owner->unitDef->minAirBasePower);
				if(lp){
					myPlane->AddDeathDependence(lp);
					myPlane->reservedPad=lp;
					myPlane->padStatus=0;
					myPlane->oldGoalPos=myPlane->goalPos;
					return;
				}
				float3 landingPos = airBaseHandler->FindClosestAirBasePos(owner,8000,owner->unitDef->minAirBasePower);
				if(landingPos != ZeroVector && owner->pos.distance2D(landingPos) > 300){
					if(myPlane->aircraftState == CAirMoveType::AIRCRAFT_LANDED && owner->pos.distance2D(landingPos) > 800)
						myPlane->SetState(CAirMoveType::AIRCRAFT_TAKEOFF);	
					myPlane->goalPos=landingPos;		
				} else {
					if(myPlane->aircraftState == CAirMoveType::AIRCRAFT_FLYING)
						myPlane->SetState(CAirMoveType::AIRCRAFT_LANDING);	
				}
				return;
			}
			if(owner->currentFuel < myPlane->repairBelowHealth*owner->unitDef->maxFuel){
				if(commandQue.empty() || commandQue.front().id==CMD_PATROL){
					CAirBaseHandler::LandingPad* lp=airBaseHandler->FindAirBase(owner,8000,owner->unitDef->minAirBasePower);
					if(lp){
						owner->userAttackGround=false;
						owner->userTarget=0;
						inCommand=false;
						myPlane->AddDeathDependence(lp);
						myPlane->reservedPad=lp;
						myPlane->padStatus=0;
						myPlane->oldGoalPos=myPlane->goalPos;
						if(myPlane->aircraftState==CAirMoveType::AIRCRAFT_LANDED){
							myPlane->SetState(CAirMoveType::AIRCRAFT_TAKEOFF);
						}
						return;
					}		
				}
			}
		}
	}
	
	if(commandQue.empty()){
		if(myPlane->aircraftState==CAirMoveType::AIRCRAFT_FLYING && !owner->unitDef->DontLand ())
			myPlane->SetState(CAirMoveType::AIRCRAFT_LANDING);

		if(owner->unitDef->canAttack && owner->fireState==2 && owner->moveState!=0 && owner->maxRange>0){
			if(myPlane->isFighter){
				float testRad=1000*owner->moveState;
				CUnit* enemy=helper->GetClosestEnemyAircraft(owner->pos+owner->speed*10,testRad,owner->allyteam);
				if(enemy && !enemy->crashing){
					Command nc;
					nc.id=CMD_ATTACK;
					nc.params.push_back(enemy->id);
					nc.options=0;
					commandQue.push_front(nc);
					inCommand=false;
					return;
				}
			}
			float testRad=500*owner->moveState;
			CUnit* enemy=helper->GetClosestEnemyUnit(owner->pos+owner->speed*20,testRad,owner->allyteam);
			if(enemy && (owner->hasUWWeapons || !enemy->isUnderWater)  && !enemy->crashing && (myPlane->isFighter || !enemy->unitDef->canfly)){
				Command nc;
				nc.id=CMD_ATTACK;
				nc.params.push_back(enemy->id);
				nc.options=0;
				commandQue.push_front(nc);
				inCommand=false;
				return;
			}
		}
		return;
	}

	Command& c = commandQue.front();

	if (c.id == CMD_WAIT) {
		if ((myPlane->aircraftState == CAirMoveType::AIRCRAFT_FLYING)
		    && !owner->unitDef->DontLand()) {
			myPlane->SetState(CAirMoveType::AIRCRAFT_LANDING);
		}
		return;
	}

	if (c.id != CMD_STOP) {
		if (myPlane->aircraftState == CAirMoveType::AIRCRAFT_LANDED) {
			myPlane->SetState(CAirMoveType::AIRCRAFT_TAKEOFF);
		}
		if (myPlane->aircraftState == CAirMoveType::AIRCRAFT_LANDING) {
			myPlane->SetState(CAirMoveType::AIRCRAFT_FLYING);
		}
	}

	float3 curPos=owner->pos;

	switch(c.id){
	case CMD_STOP:{
		CCommandAI::SlowUpdate();
		break;
	}
	case CMD_MOVE:{
		if(tempOrder){
			tempOrder=false;
			inCommand=true;
		}
		if(!inCommand){
			inCommand=true;
			commandPos1=myPlane->owner->pos;
		}
		float3 pos(c.params[0],c.params[1],c.params[2]);
		commandPos2=pos;
		myPlane->goalPos=pos;
		if(owner->unitDef->canAttack && !(c.options&CONTROL_KEY)){
			if(owner->fireState==2 && owner->moveState!=0 && owner->maxRange>0){
				if(myPlane->isFighter){
					float testRad=500*owner->moveState;
					CUnit* enemy=helper->GetClosestEnemyAircraft(owner->pos+owner->speed*20,testRad,owner->allyteam);
					if(enemy && ((enemy->unitDef->canfly && !enemy->crashing && myPlane->isFighter) || (!enemy->unitDef->canfly && (!myPlane->isFighter || owner->moveState==2)))){
						if(owner->moveState!=1 || LinePointDist(commandPos1,commandPos2,enemy->pos) < 1000){
							Command nc;
							nc.id=CMD_ATTACK;
							nc.params.push_back(enemy->id);
							nc.options=c.options;
							commandQue.push_front(nc);
							tempOrder=true;
							inCommand=false;
							SlowUpdate();
							return;
						}
					}
				}
				if((!myPlane->isFighter || owner->moveState==2) && owner->maxRange>0){
					float testRad=325*owner->moveState;
					CUnit* enemy=helper->GetClosestEnemyUnit(owner->pos+owner->speed*30,testRad,owner->allyteam);
					if(enemy && (owner->hasUWWeapons || !enemy->isUnderWater) && ((enemy->unitDef->canfly && !enemy->crashing && myPlane->isFighter) || (!enemy->unitDef->canfly && (!myPlane->isFighter || owner->moveState==2)))){
						if(owner->moveState!=1 || LinePointDist(commandPos1,commandPos2,enemy->pos) < 1000){
							Command nc;
							nc.id=CMD_ATTACK;
							nc.params.push_back(enemy->id);
							nc.options=c.options;
							commandQue.push_front(nc);
							tempOrder=true;
							inCommand=false;
							SlowUpdate();
							return;
						}
					}
				}
			}
		}
		if((curPos-pos).SqLength2D()<16000){
			FinishCommand();
		}
		break;}
	case CMD_PATROL:{
		assert(owner->unitDef->canPatrol);
		float3 curPos=owner->pos;
		if(c.params.size()<3){		//this shouldnt happen but anyway ...
			logOutput.Print("Error: got patrol cmd with less than 3 params on %s in aircai",
				owner->unitDef->humanName.c_str());
			return;
		}
		Command temp;
		temp.id = CMD_FIGHT;
		temp.params.push_back(c.params[0]);
		temp.params.push_back(c.params[1]);
		temp.params.push_back(c.params[2]);
		temp.options = c.options|INTERNAL_ORDER;
		commandQue.push_back(c);
		commandQue.pop_front();
		commandQue.push_front(temp);
		if(owner->group){
			owner->group->CommandFinished(owner->id,CMD_PATROL);
		}
		SlowUpdate();
		break;}
	case CMD_FIGHT:{
		assert((c.options & INTERNAL_ORDER) || owner->unitDef->canFight);
		if(tempOrder){
			tempOrder=false;
			inCommand=true;
		}
		if(c.params.size()<3){		//this shouldnt happen but anyway ...
			logOutput.Print("Error: got fight cmd with less than 3 params on %s in AirCAI", owner->unitDef->humanName.c_str());
			return;
		}
		if(c.params.size() >= 6){
			if(!inCommand){
				commandPos1 = float3(c.params[3],c.params[4],c.params[5]);
			}
		} else {
			// Some hackery to make sure the line (commandPos1,commandPos2) is NOT
			// rotated (only shortened) if we reach this because the previous return
			// fight command finished by the 'if((curPos-pos).SqLength2D()<(127*127)){'
			// condition, but is actually updated correctly if you click somewhere
			// outside the area close to the line (for a new command).
			commandPos1 = ClosestPointOnLine(commandPos1, commandPos2, curPos);
			if ((curPos-commandPos1).SqLength2D()>(150*150)) {
				commandPos1 = curPos;
			}
		}
		goalPos=float3(c.params[0],c.params[1],c.params[2]);
		if(!inCommand){
			inCommand = true;
			commandPos2=goalPos;
		}

		// CMD_FIGHT is pretty useless if !canAttack but we try to honour the modders wishes anyway...
		if (owner->unitDef->canAttack && owner->fireState == 2 && owner->moveState != 0 && owner->maxRange > 0) {
			float3 curPosOnLine = ClosestPointOnLine(commandPos1, commandPos2, owner->pos+owner->speed*10);
			float testRad=1000*owner->moveState;
			CUnit* enemy = helper->GetClosestEnemyAircraft(curPosOnLine,testRad,owner->allyteam);
			if(myPlane->isFighter
				&& enemy && !enemy->crashing
				&& (owner->moveState!=1 || LinePointDist(commandPos1,commandPos2,enemy->pos) < 1000))
			{
				Command nc;
				nc.id=CMD_ATTACK;
				nc.params.push_back(enemy->id);
				nc.options=c.options|INTERNAL_ORDER;
				commandQue.push_front(nc);
				tempOrder=true;
				inCommand=false;
				if(lastPC1!=gs->frameNum){	//avoid infinite loops
					lastPC1=gs->frameNum;
					SlowUpdate();
				}
				return;
			} else {
				float3 curPosOnLine = ClosestPointOnLine(commandPos1, commandPos2, owner->pos+owner->speed*20);
				float testRad=500*owner->moveState;
				enemy=helper->GetClosestEnemyUnit(curPosOnLine,testRad,owner->allyteam);
				if(enemy && (owner->hasUWWeapons || !enemy->isUnderWater)  && !enemy->crashing
					&& (myPlane->isFighter || !enemy->unitDef->canfly))
				{
					Command nc;
					nc.id=CMD_ATTACK;
					nc.params.push_back(enemy->id);
					nc.options=c.options|INTERNAL_ORDER;
					PushOrUpdateReturnFight();
					commandQue.push_front(nc);
					tempOrder=true;
					inCommand=false;
					if(lastPC2!=gs->frameNum){	//avoid infinite loops
						lastPC2=gs->frameNum;
						SlowUpdate();
					}
					return;
				}
			}
		}
		myPlane->goalPos=goalPos;

		if((owner->pos-goalPos).SqLength2D()<(127*127) || (owner->pos+owner->speed*8-goalPos).SqLength2D()<(127*127)){
			FinishCommand();
		}
		return;}
	case CMD_ATTACK:
		assert(owner->unitDef->canAttack);
		targetAge++;
		if(tempOrder && owner->moveState==1){		//limit how far away we fly
			if(orderTarget && LinePointDist(commandPos1,commandPos2,orderTarget->pos) > 1500){
				owner->SetUserTarget(0);
				FinishCommand();
				break;
			}
		}
		if(tempOrder && myPlane->isFighter && orderTarget){
			if(owner->fireState==2 && owner->moveState!=0){
				CUnit* enemy=helper->GetClosestEnemyAircraft(owner->pos+owner->speed*50,800,owner->allyteam);
				if(enemy && (!orderTarget->unitDef->canfly || (orderTarget->pos-owner->pos+owner->speed*50).Length()+100*gs->randFloat()*40-targetAge < (enemy->pos-owner->pos+owner->speed*50).Length())){
					c.params.clear();
					c.params.push_back(enemy->id);
					inCommand=false;
				}
			}
		}
		if(inCommand){
			if(targetDied || (c.params.size() == 1 && uh->units[int(c.params[0])] && !(uh->units[int(c.params[0])]->losStatus[owner->allyteam] & LOS_INRADAR))){
				FinishCommand();
				break;
			}
			if ((c.params.size() == 3) && (owner->commandShotCount > 0) && (commandQue.size() > 1)) {
				owner->AttackUnit(0,true); 
				FinishCommand();
			  break;
			}    
			if(orderTarget && orderTarget->unitDef->canfly && orderTarget->crashing){
				owner->SetUserTarget(0);
				FinishCommand();
				break;
			}
			if(orderTarget){
//				myPlane->goalPos=orderTarget->pos;
			} else {
//				float3 pos(c.params[0],c.params[1],c.params[2]);
//				myPlane->goalPos=pos;
			}
		} else {
			targetAge=0;
			if(c.params.size()==1){
				if(uh->units[int(c.params[0])]!=0 && uh->units[int(c.params[0])]!=owner){
					orderTarget=uh->units[int(c.params[0])];
					owner->AttackUnit(orderTarget,false);
					AddDeathDependence(orderTarget);
					inCommand=true;
				} else {
					FinishCommand();
					break;
				}
			} else {
				float3 pos(c.params[0],c.params[1],c.params[2]);
				owner->AttackGround(pos,false);
				inCommand=true;
			}
		}
		break;
	case CMD_AREA_ATTACK:{
		assert(owner->unitDef->canAttack);
		if(targetDied){
			targetDied=false;
			inCommand=false;
		}
		float3 pos(c.params[0],c.params[1],c.params[2]);
		float radius=c.params[3];
		if(inCommand){
			if(myPlane->aircraftState==CAirMoveType::AIRCRAFT_LANDED)
				inCommand=false;
			if(orderTarget && orderTarget->pos.distance2D(pos)>radius){
				inCommand=false;
				DeleteDeathDependence(orderTarget);
				orderTarget=0;
			}
			if ((c.params.size() == 4) && (owner->commandShotCount > 0) && (commandQue.size() > 1)) {
				owner->AttackUnit(0,true); 
				FinishCommand();
			}
		} else {
			if(myPlane->aircraftState!=CAirMoveType::AIRCRAFT_LANDED){
				inCommand=true;
				std::vector<int> eu;
				helper->GetEnemyUnits(pos,radius,owner->allyteam,eu);
				if(eu.empty()){
					float3 attackPos=pos+gs->randVector()*radius;
					attackPos.y=ground->GetHeight(attackPos.x,attackPos.z);
					owner->AttackGround(attackPos,false);
				} else {
					int num=(int) (gs->randFloat()*eu.size());
					orderTarget=uh->units[eu[num]];
					owner->AttackUnit(orderTarget,false);
					AddDeathDependence(orderTarget);
				}
			}
		}
		break;}
	case CMD_GUARD:
		assert(owner->unitDef->canGuard);
		if(uh->units[int(c.params[0])]!=0){
			CUnit* guarded=uh->units[int(c.params[0])];
			if(owner->unitDef->canAttack && guarded->lastAttacker && guarded->lastAttack+40<gs->frameNum && owner->maxRange>0 && (owner->hasUWWeapons || !guarded->lastAttacker->isUnderWater)){
				Command nc;
				nc.id=CMD_ATTACK;
				nc.params.push_back(guarded->lastAttacker->id);
				nc.options=c.options;
				commandQue.push_front(nc);
				SlowUpdate();
				return;
			} else {
				Command c2;
				c2.id=CMD_MOVE;
				c2.options=c.options;
				c2.params.push_back(guarded->pos.x);
				c2.params.push_back(guarded->pos.y);
				c2.params.push_back(guarded->pos.z);
				c2.timeOut=gs->frameNum+60;
				commandQue.push_front(c2);
				return;
			}
		} else {
			FinishCommand();
		}
		break;
	default:
		CCommandAI::SlowUpdate();
		break;
	}
}

int CAirCAI::GetDefaultCmd(CUnit* pointed, CFeature* feature)
{
	if (pointed) {
		if (!gs->Ally(gu->myAllyTeam, pointed->allyteam)) {
			if (owner->unitDef->canAttack) {
				return CMD_ATTACK;
			}
		} else {
			if (owner->unitDef->canGuard) {
				return CMD_GUARD;
			}
		}
	}
	return CMD_MOVE;
}

void CAirCAI::DrawCommands(void)
{
	lineDrawer.StartPath(owner->pos, cmdColors.start);

	deque<Command>::iterator ci;
	for(ci=commandQue.begin();ci!=commandQue.end();++ci){
		switch(ci->id){
			case CMD_WAIT:{
				lineDrawer.DrawIconAtLastPos(ci->id);
				break;
			}
			case CMD_MOVE:{
				const float3 endPos(ci->params[0],ci->params[1],ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.move);
				break;
			}
			case CMD_FIGHT:{
				const float3 endPos(ci->params[0],ci->params[1],ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.fight);
				break;
			}
			case CMD_PATROL:{
				const float3 endPos(ci->params[0],ci->params[1],ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.patrol);
				break;
			}
			case CMD_ATTACK:{
				if(ci->params.size()==1){
					if(uh->units[int(ci->params[0])]!=0){
						const float3 endPos =
							helper->GetUnitErrorPos(uh->units[int(ci->params[0])],owner->allyteam);
						lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.attack);
					}
				} else {
					const float3 endPos(ci->params[0],ci->params[1],ci->params[2]);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.attack);
				}
				break;
			}
			case CMD_AREA_ATTACK:{
				const float3 endPos(ci->params[0],ci->params[1],ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.attack);
				lineDrawer.Break(endPos, cmdColors.attack);
				glSurfaceCircle(endPos, ci->params[3], 20);
				lineDrawer.RestartSameColor();
				break;
			}
			case CMD_GUARD:{
				if(uh->units[int(ci->params[0])]!=0){
					const float3 endPos = uh->units[int(ci->params[0])]->pos;
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.guard);
				}
				break;
			}
		}
	}
	lineDrawer.FinishPath();
}

void CAirCAI::StopMove()
{
	CAirMoveType* myPlane = (CAirMoveType*)owner->moveType;
	if((myPlane->aircraftState == CAirMoveType::AIRCRAFT_FLYING)
	   && !owner->unitDef->DontLand()) {
		myPlane->SetState(CAirMoveType::AIRCRAFT_LANDING);
	}
}

void CAirCAI::FinishCommand(void)
{
	targetAge=0;
	CCommandAI::FinishCommand();
}

void CAirCAI::BuggerOff(float3 pos, float radius)
{
	CAirMoveType* myPlane=(CAirMoveType*)owner->moveType;
	if(myPlane->aircraftState==CAirMoveType::AIRCRAFT_LANDED){
		myPlane->SetState(CAirMoveType::AIRCRAFT_TAKEOFF);
	}
}
