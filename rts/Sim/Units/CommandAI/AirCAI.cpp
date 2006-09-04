#include "StdAfx.h"
#include "AirCAI.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Map/Ground.h"
#include "Game/GameHelper.h"
#include "Sim/Units/UnitHandler.h"
#include "ExternalAI/Group.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "Game/UI/InfoConsole.h"
#include "Game/UI/CursorIcons.h"
#include "Sim/Units/UnitDef.h"
#include "myMath.h"
#include "mmgr.h"

CAirCAI::CAirCAI(CUnit* owner)
: CCommandAI(owner), lastPC1(-1), lastPC2(-1)
{
	CommandDescription c;
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

	c.id = CMD_FIGHT;
	c.action="fight";
	c.type = CMDTYPE_ICON_MAP;
	c.name = "Fight";
	c.hotkey = "f";
	c.tooltip = "Fight: Order the aircraft to take action while moving to a position";
	possibleCommands.push_back(c);

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

void CAirCAI::GiveCommand(Command &c)
{
	if(c.id==CMD_AUTOREPAIRLEVEL){
		if(c.params.empty())
			return;
		switch((int)c.params[0]){
		case 0:
			((CAirMoveType*)owner->moveType)->repairBelowHealth=0;
			break;
		case 1:
			((CAirMoveType*)owner->moveType)->repairBelowHealth=0.3;
			break;
		case 2:
			((CAirMoveType*)owner->moveType)->repairBelowHealth=0.5;
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
		return;
	}
	if(!(c.options & SHIFT_KEY) && nonQueingCommands.find(c.id)==nonQueingCommands.end()){
		activeCommand=0;
		tempOrder=false;
	}
	if(c.id==CMD_AREA_ATTACK && c.params.size()<4){
		c.id=CMD_ATTACK;
	}
	CCommandAI::GiveCommand(c);
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

		if(owner->fireState==2 && owner->moveState!=0 && owner->maxRange>0){
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
			if(owner->moveState && owner->fireState==2 && owner->maxRange>0){
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
		}
		return;
	}

	if(myPlane->aircraftState==CAirMoveType::AIRCRAFT_LANDED && commandQue.front().id!=CMD_STOP){
		myPlane->SetState(CAirMoveType::AIRCRAFT_TAKEOFF);
	}

	if(myPlane->aircraftState==CAirMoveType::AIRCRAFT_LANDING && commandQue.front().id!=CMD_STOP){
		myPlane->SetState(CAirMoveType::AIRCRAFT_FLYING);
	}

	float3 curPos=owner->pos;

	Command& c=commandQue.front();
	switch(c.id){
	case CMD_STOP:{
		CCommandAI::SlowUpdate();
		break;}
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
		if(!(c.options&CONTROL_KEY)){
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
		float3 curPos=owner->pos;
		if(c.params.size()<3){		//this shouldnt happen but anyway ...
			info->AddLine("Error: got patrol cmd with less than 3 params on %s in aircai",
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
		if(tempOrder){
			tempOrder=false;
			inCommand=true;
		}
		goalPos=float3(c.params[0],c.params[1],c.params[2]);
		if(!inCommand){
			commandPos2=goalPos;
			inCommand=true;
		}
		commandPos1=curPos;
		if(c.params.size()<3){		//this shouldnt happen but anyway ...
			info->AddLine("Error: got fight cmd with less than 3 params on %s in aircai",
				owner->unitDef->humanName.c_str());
			return;
		}

		float testRad=1000*owner->moveState;
		CUnit* enemy = helper->GetClosestEnemyAircraft(owner->pos+owner->speed*10,testRad,owner->allyteam);
		if(owner->fireState==2 && owner->moveState!=0 && owner->maxRange>0){
			if(myPlane->isFighter
				&& enemy && !enemy->crashing
				&& (owner->moveState!=1 || LinePointDist(commandPos1,commandPos2,enemy->pos) < 1000))
			{
				Command nc,c3;
				c3.id = CMD_MOVE; //keep it from being pulled too far off the path
				float3 dir = goalPos-curPos;
				dir.Normalize();
				dir*=sqrtf(1024+owner->xsize*owner->xsize+owner->ysize*owner->ysize);
				dir+=curPos;
				c3.params.push_back(dir.x);
				c3.params.push_back(dir.y);
				c3.params.push_back(dir.z);
				c3.options = c.options|INTERNAL_ORDER;
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
			} else if(owner->maxRange>0){
				float testRad=500*owner->moveState;
				enemy=helper->GetClosestEnemyUnit(owner->pos+owner->speed*20,testRad,owner->allyteam);
				if(enemy && (owner->hasUWWeapons || !enemy->isUnderWater)  && !enemy->crashing
					&& (myPlane->isFighter || !enemy->unitDef->canfly))
				{
					if(owner->moveState!=1 || LinePointDist(commandPos1,commandPos2,enemy->pos) < 1000){
						Command nc,c3;
						c3.id = CMD_MOVE; //keep it from being pulled too far off the path
						float3 dir = goalPos-curPos;
						dir.Normalize();
						dir*=sqrtf(1024+owner->xsize*owner->xsize+owner->ysize*owner->ysize);
						dir+=curPos;
						c3.params.push_back(dir.x);
						c3.params.push_back(dir.y);
						c3.params.push_back(dir.z);
						c3.options = c.options|INTERNAL_ORDER;
						nc.id=CMD_ATTACK;
						nc.params.push_back(enemy->id);
						nc.options=c.options|INTERNAL_ORDER;
						commandQue.push_front(c3);
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
		}
		myPlane->goalPos=goalPos;

		if((owner->pos-goalPos).SqLength2D()<16000 || (owner->pos+owner->speed*8-goalPos).SqLength2D()<16000){
			FinishCommand();
		}
		return;}
	case CMD_ATTACK:
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
		if(uh->units[int(c.params[0])]!=0){
			CUnit* guarded=uh->units[int(c.params[0])];
			if(guarded->lastAttacker && guarded->lastAttack+40<gs->frameNum && owner->maxRange>0 && (owner->hasUWWeapons || !guarded->lastAttacker->isUnderWater)){
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

int CAirCAI::GetDefaultCmd(CUnit *pointed,CFeature* feature)
{
	if(pointed){
		if(!gs->Ally(gu->myAllyTeam,pointed->allyteam)){
			return CMD_ATTACK;
		} else {
			return CMD_MOVE;
		}
	}
	return CMD_MOVE;
}

void CAirCAI::DrawCommands(void)
{
	CAirMoveType* myPlane=(CAirMoveType*)owner->moveType;
	//CAircraft* myPlane=(CAircraft*)owner;
	float3 pos=owner->pos;
	glColor4f(1,1,1,0.4);
	glBegin(GL_LINE_STRIP);
	glVertexf3(pos);
	deque<Command>::iterator ci;
	for(ci=commandQue.begin();ci!=commandQue.end();++ci){
		switch(ci->id){
		case CMD_MOVE:
			pos=float3(ci->params[0],ci->params[1],ci->params[2]);
			glColor4f(0.5,1,0.5,0.4);
			glVertexf3(pos);
			cursorIcons->AddIcon(ci->id, pos);
			break;
		case CMD_FIGHT:
		case CMD_PATROL:
			pos=float3(ci->params[0],ci->params[1],ci->params[2]);
			glColor4f(0.5,0.5,1,0.4);
			glVertexf3(pos);
			cursorIcons->AddIcon(ci->id, pos);
			break;
		case CMD_ATTACK:
			if(ci->params.size()==1){
				if(uh->units[int(ci->params[0])]!=0)
					pos=helper->GetUnitErrorPos(uh->units[int(ci->params[0])],owner->allyteam);
			} else {
				pos=float3(ci->params[0],ci->params[1],ci->params[2]);
			}
			glColor4f(1,0.5,0.5,0.4);
			glVertexf3(pos);
			cursorIcons->AddIcon(ci->id, pos);
			break;
		case CMD_AREA_ATTACK:
			pos=float3(ci->params[0],ci->params[1],ci->params[2]);
			glColor4f(1,0.5,0.5,0.4);
			glVertexf3(pos);
			glEnd();
			glSurfaceCircle(pos, ci->params[3], 20);
			glBegin(GL_LINE_STRIP);
			glVertexf3(pos);
			cursorIcons->AddIcon(ci->id, pos);
			break;
		case CMD_GUARD:
			if(uh->units[int(ci->params[0])]!=0)
				pos=uh->units[int(ci->params[0])]->pos;
			glColor4f(0.3,0.3,1,0.4);
			glVertexf3(pos);
			cursorIcons->AddIcon(ci->id, pos);
			break;
		}
	}
	glEnd();
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
