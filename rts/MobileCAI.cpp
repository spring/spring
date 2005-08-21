#include "StdAfx.h"
#include "MobileCAI.h"
#include "Ground.h"
#include "GameHelper.h"
#include "UnitHandler.h"
#include "Group.h"
#include "myGL.h"
#include "Unit.h"
#include "MoveType.h"
#include "InfoConsole.h"
#include "UnitDef.h"
#include "Weapon.h"
#include "TAAirMoveType.h"
//#include "mmgr.h"

CMobileCAI::CMobileCAI(CUnit* owner)
: CCommandAI(owner),
	activeCommand(0),
	patrolTime(0),
	goalPos(-1,-1,-1),
	tempOrder(false),
	patrolGoal(1,1,1),
	lastBuggerOffTime(-200),
	buggerOffPos(0,0,0),
	buggerOffRadius(0),
	maxWantedSpeed(0),
	lastIdleCheck(0)
{
	lastUserGoal=owner->pos;

	CommandDescription c;
	c.id=CMD_MOVE;
	c.type=CMDTYPE_ICON_MAP;
	c.name="Move";
	c.key='M';
	c.tooltip="Move: Order the unit to move to a position";
	possibleCommands.push_back(c);

	c.params.clear();
	c.id=CMD_PATROL;
	c.type=CMDTYPE_ICON_MAP;
	c.name="Patrol";
	c.key='P';
	c.tooltip="Patrol: Order the unit to patrol to one or more waypoints";
	possibleCommands.push_back(c);

	c.id=CMD_GUARD;
	c.type=CMDTYPE_ICON_UNIT;
	c.name="Guard";
	c.key='G';
	c.tooltip="Guard: Order a unit to guard another unit and attack units attacking it";
	possibleCommands.push_back(c);

	nonQueingCommands.insert(CMD_SET_WANTED_MAX_SPEED);
}

CMobileCAI::~CMobileCAI()
{

}

void CMobileCAI::GiveCommand(Command &c)
{
	if(c.id == CMD_SET_WANTED_MAX_SPEED) {
		//owner->moveType->SetWantedMaxSpeed(*c.params.begin());
		maxWantedSpeed = *c.params.begin();
		return;
	}

	if(!(c.options & SHIFT_KEY) && nonQueingCommands.find(c.id)==nonQueingCommands.end()){
		activeCommand=0;
		tempOrder=false;
	}
	CCommandAI::GiveCommand(c);
}

void CMobileCAI::SlowUpdate()
{
	if(!commandQue.empty() && commandQue.front().timeOut<gs->frameNum){
		StopMove();
		FinishCommand();
		return;
	}

	if(commandQue.empty()) {
//		if(!owner->ai || owner->ai->State() != CHasState::Active) {
			IdleCheck();
//		}
		if(commandQue.empty() || commandQue.front().id==CMD_ATTACK)	//the attack order could terminate directly and thus cause a loop
			return;
	}

	float3 curPos=owner->pos;

	Command& c=commandQue.front();
	switch(c.id){
	case CMD_STOP:{
		StopMove();
		CCommandAI::SlowUpdate();
		break;}
	case CMD_MOVE:{
		float3 pos(c.params[0],c.params[1],c.params[2]);
		if(!(pos==goalPos)){
			SetGoal(pos,curPos);
		}
		if((curPos-goalPos).SqLength2D()<1024 || owner->moveType->progressState==CMoveType::Failed){
			FinishCommand();
		}
		break;}
	case CMD_PATROL:{
		if(tempOrder){
			tempOrder=false;
			inCommand=true;
			patrolTime=1;
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
			patrolTime=1;
		}
		Command& c=commandQue[activeCommand];
		if(c.params.size()<3){		//this shouldnt happen but anyway ...
			info->AddLine("Error: got patrol cmd with less than 3 params on %s in mobilecai",owner->unitDef->humanName.c_str());
			return;
		}
		patrolGoal=float3(c.params[0],c.params[1],c.params[2]);
		patrolTime++;

		if(!(patrolTime&1) && owner->fireState==2){
			CUnit* enemy=helper->GetClosestEnemyUnit(curPos,owner->maxRange+100*owner->moveState*owner->moveState,owner->allyteam);
			if(enemy && (owner->hasUWWeapons || !enemy->isUnderWater) && !(owner->unitDef->noChaseCategory & enemy->category) && !owner->weapons.empty()){			//todo: make sure they dont stray to far from path
				Command c2;
				c2.id=CMD_ATTACK;
				c2.options=c.options;
				c2.params.push_back(enemy->id);
				commandQue.push_front(c2);
				inCommand=false;
				tempOrder=true;
				SlowUpdate();				//if attack can finish immidiatly this could lead to an infinite loop so make sure it doesnt
				return;
			}
		}
		if((curPos-patrolGoal).SqLength2D()<4096 || owner->moveType->progressState==CMoveType::Failed){
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
			Command& c=commandQue[activeCommand];
			patrolGoal=float3(c.params[0],c.params[1],c.params[2]);
		}
		if((patrolGoal!=goalPos)){
			SetGoal(patrolGoal,curPos);
		}
		return;}
	case CMD_ATTACK:
	case CMD_DGUN:
		if(!inCommand){
			if(c.params.size()==1){
				if(uh->units[int(c.params[0])]!=0 && uh->units[int(c.params[0])]!=owner){
					float3 fix=uh->units[int(c.params[0])]->pos+owner->posErrorVector*128;
					SetGoal(fix,curPos);
					orderTarget=uh->units[int(c.params[0])];
					AddDeathDependence(orderTarget);
					inCommand=true;
				} else {
					StopMove();		//cancel keeppointingto
					FinishCommand();
					break;
				}
			} else {
				float3 pos(c.params[0],c.params[1],c.params[2]);
				SetGoal(pos,curPos);
				inCommand=true;
			}
		}
		if(targetDied){
			StopMove();		//cancel keeppointingto
			FinishCommand();
			break;
		}
		if(orderTarget){
			//note that we handle aircrafts slightly differently
			if(((owner->AttackUnit(orderTarget, c.id==CMD_DGUN) || dynamic_cast<CTAAirMoveType*>(owner->moveType)) && (owner->pos-orderTarget->pos).Length2D()<owner->maxRange*0.9) || (owner->pos-orderTarget->pos).SqLength2D()<1024){
				//					if(owner->isMoving) {
				StopMove();
				owner->moveType->KeepPointingTo(orderTarget->pos, min((double)(owner->losRadius*SQUARE_SIZE*2),owner->maxRange*0.9), true);
				//					}
			} else {
				if((orderTarget->pos+owner->posErrorVector*128).distance2D(goalPos)>10+orderTarget->pos.distance2D(owner->pos)*0.2)
				{
					float3 fix=orderTarget->pos+owner->posErrorVector*128;
					SetGoal(fix,curPos);
				}
			}
		} else {
			float3 pos(c.params[0],c.params[1],c.params[2]);
			if(owner->AttackGround(pos,c.id==CMD_DGUN) || (owner->pos-pos).SqLength2D()<1024) {
				StopMove();
				owner->moveType->KeepPointingTo(pos, owner->maxRange*0.9, true);
			} else {
				if(pos.distance2D(goalPos)>10)
					SetGoal(pos,curPos);					
			}
		}
		break;
	case CMD_GUARD:
		if(uh->units[int(c.params[0])]!=0){
			CUnit* guarded=uh->units[int(c.params[0])];
			if(guarded->lastAttacker && guarded->lastAttack+40<gs->frameNum && (owner->hasUWWeapons || !guarded->lastAttacker->isUnderWater)){
				Command nc;
				nc.id=CMD_ATTACK;
				nc.params.push_back(guarded->lastAttacker->id);
				nc.options=c.options;
				commandQue.push_front(nc);
				SlowUpdate();
				return;
			} else {
				float3 dif=guarded->pos-curPos;
				dif.Normalize();
				float3 goal=guarded->pos-dif*(guarded->radius+owner->radius+64);
				if((goal-curPos).SqLength2D()<8000){
					StopMove();
					NonMoving();
				}else{
					if((goalPos-goal).SqLength2D()>3600)
						SetGoal(goal,curPos);
				}
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

int CMobileCAI::GetDefaultCmd(CUnit *pointed,CFeature* feature)
{
	if(pointed){
		if(!gs->allies[gu->myAllyTeam][pointed->allyteam]){
			return CMD_ATTACK;
		} else {
			return CMD_GUARD;
		}
	}
	return CMD_MOVE;
}

void CMobileCAI::SetGoal(float3 &pos,float3& curPos, float goalRadius)
{
	if(pos==goalPos)
		return;
	goalPos=pos;
	owner->moveType->StartMoving(pos, goalRadius);
}

void CMobileCAI::StopMove()
{
	owner->moveType->StopMoving();
	goalPos=owner->pos;
}

void CMobileCAI::DrawCommands(void)
{
	float3 pos=owner->midPos;
	glColor4f(1,1,1,0.4);
	glBegin(GL_LINE_STRIP);
	glVertexf3(pos);
	deque<Command>::iterator ci;
	for(ci=commandQue.begin();ci!=commandQue.end();++ci){
		bool draw=false;
		switch(ci->id){
		case CMD_MOVE:
			pos=float3(ci->params[0],ci->params[1],ci->params[2]);
			glColor4f(0.5,1,0.5,0.4);
			draw=true;
			break;
		case CMD_PATROL:
			pos=float3(ci->params[0],ci->params[1],ci->params[2]);
			glColor4f(0.5,0.5,1,0.4);
			draw=true;
			break;
		case CMD_ATTACK:
		case CMD_DGUN:
			if(ci->params.size()==1){
				if(uh->units[int(ci->params[0])]!=0)
					pos=helper->GetUnitErrorPos(uh->units[int(ci->params[0])],owner->allyteam);
			} else {
				pos=float3(ci->params[0],ci->params[1],ci->params[2]);
			}
			glColor4f(1,0.5,0.5,0.4);
			draw=true;
			break;
		case CMD_GUARD:
			if(uh->units[int(ci->params[0])]!=0)
				pos=helper->GetUnitErrorPos(uh->units[int(ci->params[0])],owner->allyteam);
			glColor4f(0.3,0.3,1,0.4);
			draw=true;
			break;
		}
		if(draw){
			glVertexf3(pos);	
		}
	}
	glEnd();
}

void CMobileCAI::BuggerOff(float3 pos, float radius)
{
	lastBuggerOffTime=gs->frameNum;
	buggerOffPos=pos;
	buggerOffRadius=radius+owner->radius;
}

void CMobileCAI::NonMoving(void)
{
	if(lastBuggerOffTime>gs->frameNum-200){
		float3 dif=owner->pos-buggerOffPos;
		dif.y=0;
		float length=dif.Length();
		if(length<buggerOffRadius){
			float3 goalPos=buggerOffPos+dif*((buggerOffRadius+128)/length);
			Command c;
			c.id=CMD_MOVE;
			c.options=0;//INTERNAL_ORDER;
			c.params.push_back(goalPos.x);
			c.params.push_back(goalPos.y);
			c.params.push_back(goalPos.z);
			c.timeOut=gs->frameNum+40;
			commandQue.push_front(c);
			unimportantMove=true;
		}
	}
}

void CMobileCAI::FinishCommand(void)
{
	if(!(commandQue.front().options & INTERNAL_ORDER)){
		lastUserGoal=owner->pos;
//		info->AddLine("Reseting user goal");
	}

	CCommandAI::FinishCommand();
}

void CMobileCAI::IdleCheck(void)
{
	if(owner->moveState && owner->fireState && !owner->weapons.empty() && (!owner->haveTarget || owner->weapons[0]->onlyForward)){
		if(owner->lastAttacker && owner->lastAttack+100>gs->frameNum && !(owner->unitDef->noChaseCategory & owner->lastAttacker->category)){
			float3 apos=owner->lastAttacker->pos;
			float dist=apos.distance2D(owner->pos);
			if(dist<owner->maxRange+200*owner->moveState*owner->moveState){
				Command c;
				c.id=CMD_ATTACK;
				c.options=INTERNAL_ORDER;
				c.params.push_back(owner->lastAttacker->id);
				c.timeOut=gs->frameNum+140;
				commandQue.push_front(c);
				return;
			}
		}
	}
	if((gs->frameNum!=lastIdleCheck+16) && owner->moveState && owner->fireState==2 && !owner->weapons.empty() && (!owner->haveTarget || owner->weapons[0]->onlyForward)){
		CUnit* u=helper->GetClosestEnemyUnit(owner->pos,owner->maxRange+300*owner->moveState,owner->allyteam);
		if(u && !(owner->unitDef->noChaseCategory & u->category)){
			Command c;
			c.id=CMD_ATTACK;
			c.options=INTERNAL_ORDER;
			c.params.push_back(u->id);
			c.timeOut=gs->frameNum+140;
			commandQue.push_front(c);
			return;
		}
	}
	lastIdleCheck=gs->frameNum;
	if((owner->pos-lastUserGoal).SqLength2D()>10000 && !owner->haveTarget && !dynamic_cast<CTAAirMoveType*>(owner->moveType)){
		Command c;
		c.id=CMD_MOVE;
		c.options=0;		//note that this is not internal order so that we dont keep generating new orders if we cant get to that pos
		c.params.push_back(lastUserGoal.x);
		c.params.push_back(lastUserGoal.y);
		c.params.push_back(lastUserGoal.z);
		commandQue.push_front(c);
		unimportantMove=true;
	} else {
		NonMoving();
	}
}
