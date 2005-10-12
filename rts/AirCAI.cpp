#include "StdAfx.h"
#include "AirCAI.h"
#include "AirMoveType.h"
#include "Ground.h"
#include "GameHelper.h"
#include "UnitHandler.h"
#include "Group.h"
#include "myGL.h"
#include "InfoConsole.h"
#include "UnitDef.h"
//#include "mmgr.h"

CAirCAI::CAirCAI(CUnit* owner)
: CCommandAI(owner)
{
	CommandDescription c;
	c.id=CMD_MOVE;
	c.type=CMDTYPE_ICON_MAP;
	c.name="Move";
	c.key='M';
	c.tooltip="Move: Commands the aircraft to fly to the location";
	possibleCommands.push_back(c);

	c.id=CMD_PATROL;
	c.type=CMDTYPE_ICON_MAP;
	c.name="Patrol";
	c.key='P';
	c.tooltip="Patrol: Sets the aircraft to patrol a path to one or more waypoints";
	possibleCommands.push_back(c);

	c.id=CMD_AREA_ATTACK;
	c.type=CMDTYPE_ICON_AREA;
	c.name="Area attack";
	c.key='A';
	c.tooltip="Sets the aircraft to attack enemy units within a circle";
	possibleCommands.push_back(c);

	c.id=CMD_GUARD;
	c.type=CMDTYPE_ICON_UNIT;
	c.name="Guard";
	c.key='G';
	c.tooltip="Guard: Order a unit to guard another unit and attack units attacking it";
	possibleCommands.push_back(c);

	c.params.clear();
	c.id=CMD_AUTOREPAIRLEVEL;
	c.type=CMDTYPE_ICON_MODE;
	c.name="Repair level";
	c.params.push_back("1");
	c.params.push_back("LandAt 0");
	c.params.push_back("LandAt 30");
	c.params.push_back("LandAt 50");
	c.tooltip="Repair level: Sets at which health level an aircraft will try to find a repair pad";
	c.key=0;
	possibleCommands.push_back(c);
	nonQueingCommands.insert(CMD_AUTOREPAIRLEVEL);

	basePos=owner->pos;
	goalPos=owner->pos;

	tempOrder=false;
	targetAge=0;
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
				SNPRINTF(t,10,"%d",c.params[0]);
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

	if(commandQue.empty()){
		if(myPlane->aircraftState==CAirMoveType::AIRCRAFT_FLYING)
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
		if(tempOrder)
			tempOrder=false;
		float3 pos(c.params[0],c.params[1],c.params[2]);
		myPlane->goalPos=pos;
		if(!(c.options&CONTROL_KEY)){
			if(owner->fireState==2 && owner->moveState!=0 && owner->maxRange>0){
				if(myPlane->isFighter){
					float testRad=500*owner->moveState;
					CUnit* enemy=helper->GetClosestEnemyAircraft(owner->pos+owner->speed*20,testRad,owner->allyteam);
					if(enemy && ((enemy->unitDef->canfly && !enemy->crashing && myPlane->isFighter) || (!enemy->unitDef->canfly && (!myPlane->isFighter || owner->moveState==2)))){
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
				if((!myPlane->isFighter || owner->moveState==2) && owner->maxRange>0){
					float testRad=325*owner->moveState;
					CUnit* enemy=helper->GetClosestEnemyUnit(owner->pos+owner->speed*30,testRad,owner->allyteam);
					if(enemy && (owner->hasUWWeapons || !enemy->isUnderWater) && ((enemy->unitDef->canfly && !enemy->crashing && myPlane->isFighter) || (!enemy->unitDef->canfly && (!myPlane->isFighter || owner->moveState==2)))){
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
		if((curPos-pos).SqLength2D()<16000){
			FinishCommand();
		}
		break;}
	case CMD_PATROL:{
		if(tempOrder){
			tempOrder=false;
			inCommand=true;
		}
		if(!inCommand){
			float3 pos(c.params[0],c.params[1],c.params[2]);
			inCommand=true;
			Command co;
			co.id=CMD_PATROL;
			co.params.push_back(curPos.x);
			co.params.push_back(curPos.y);
			co.params.push_back(curPos.z);
			commandQue.push_front(co);
			activeCommand=1;
			patrolTime=3;
		}
		Command& c=commandQue[activeCommand];
		if(c.params.size()<3){
			info->AddLine("Patrol command with insufficient parameters ?");
			return;
		}
		goalPos=float3(c.params[0],c.params[1],c.params[2]);
		patrolTime++;

		if(owner->fireState==2 && owner->moveState!=0 && owner->maxRange>0){
			if(myPlane->isFighter){
				float testRad=1000*owner->moveState;
				CUnit* enemy=helper->GetClosestEnemyAircraft(owner->pos+owner->speed*10,testRad,owner->allyteam);
				if(enemy && !enemy->crashing){
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
			if((!myPlane->isFighter || owner->moveState==2) && owner->maxRange>0){
				float testRad=500*owner->moveState;
				CUnit* enemy=helper->GetClosestEnemyUnit(owner->pos+owner->speed*20,testRad,owner->allyteam);
				if(enemy && (owner->hasUWWeapons || !enemy->isUnderWater)  && !enemy->crashing && (myPlane->isFighter || !enemy->unitDef->canfly)){
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
		myPlane->goalPos=goalPos;

		if((owner->pos-goalPos).SqLength2D()<16000 || (owner->pos+owner->speed*8-goalPos).SqLength2D()<16000){
			if(owner->group)
				owner->group->CommandFinished(owner->id,CMD_PATROL);

			if((int)commandQue.size()<=activeCommand+1)
				activeCommand=0;
			else {
				if(commandQue[activeCommand+1].id!=CMD_PATROL)
					activeCommand=0;
				else
					activeCommand++;
			}
		}
		return;}
	case CMD_ATTACK:
		targetAge++;
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
			if(targetDied){
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
		if(!gs->allies[gu->myAllyTeam][pointed->allyteam]){
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
	float curHeight=pos.y-ground->GetHeight(pos.x,pos.z);
	deque<Command>::iterator ci;
	for(ci=commandQue.begin();ci!=commandQue.end();++ci){
		switch(ci->id){
		case CMD_MOVE:
			pos=float3(ci->params[0],ci->params[1]+curHeight,ci->params[2]);
			glColor4f(0.5,1,0.5,0.4);
			glVertexf3(pos);	
			break;
		case CMD_PATROL:
			pos=float3(ci->params[0],ci->params[1]+curHeight,ci->params[2]);
			glColor4f(0.5,0.5,1,0.4);
			glVertexf3(pos);	
			break;
		case CMD_ATTACK:
			if(ci->params.size()==1){
				if(uh->units[int(ci->params[0])]!=0)
					pos=helper->GetUnitErrorPos(uh->units[int(ci->params[0])],owner->allyteam);
			} else {
				pos=float3(ci->params[0],ci->params[1]+curHeight,ci->params[2]);
			}
			glColor4f(1,0.5,0.5,0.4);
			glVertexf3(pos);	
			break;
		case CMD_AREA_ATTACK:
			pos=float3(ci->params[0],ci->params[1]+curHeight,ci->params[2]);
			glColor4f(1,0.1,0.1,0.6);
			glVertexf3(pos);
			if(owner->userTarget){
				glEnd();
				glBegin(GL_LINES);
					glVertexf3(pos);
					glVertexf3(owner->userTarget->pos);
				glEnd();
				glBegin(GL_LINE_STRIP);
			} else if(owner->userAttackGround){
				glEnd();
				glBegin(GL_LINES);
					glVertexf3(pos);
					glVertexf3(owner->userAttackPos);
				glEnd();
				glBegin(GL_LINE_STRIP);
			}
			break;
		case CMD_GUARD:
			if(uh->units[int(ci->params[0])]!=0)
				pos=uh->units[int(ci->params[0])]->pos;
			glColor4f(0.3,0.3,1,0.4);
			glVertexf3(pos);	
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
