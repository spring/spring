#include "StdAfx.h"
#include "mmgr.h"

#include "AirCAI.h"
#include "LineDrawer.h"
#include "Sim/Units/Groups/Group.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Game/UI/CommandColors.h"
#include "Map/Ground.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "myMath.h"
#include "LogOutput.h"
#include <assert.h>
#include "GlobalUnsynced.h"
#include "Util.h"


CR_BIND_DERIVED(CAirCAI,CMobileCAI , );

CR_REG_METADATA(CAirCAI, (
				CR_MEMBER(basePos),
				CR_MEMBER(baseDir),

				CR_MEMBER(activeCommand),
				CR_MEMBER(targetAge),

				CR_MEMBER(lastPC1),
				CR_MEMBER(lastPC2),
				CR_RESERVED(16)
				));

CAirCAI::CAirCAI()
: CMobileCAI(), lastPC1(-1), lastPC2(-1)
{}

CAirCAI::CAirCAI(CUnit* owner)
: CMobileCAI(owner), lastPC1(-1), lastPC2(-1)
{
	cancelDistance = 16000;
	CommandDescription c;

	if(owner->unitDef->canAttack){
		c.id=CMD_AREA_ATTACK;
		c.action="areaattack";
		c.type=CMDTYPE_ICON_AREA;
		c.name="Area attack";
		c.mouseicon=c.name;
		c.tooltip="Sets the aircraft to attack enemy units within a circle";
		possibleCommands.push_back(c);
	}

	if(owner->unitDef->canLoopbackAttack){
		c.params.clear();
		c.id=CMD_LOOPBACKATTACK;
		c.action="loopbackattack";
		c.type=CMDTYPE_ICON_MODE;
		c.name="Loopback";
		c.mouseicon=c.name;
		c.params.push_back("0");
		c.params.push_back("Normal");
		c.params.push_back("Loopback");
		c.tooltip="Loopback attack: Sets if the aircraft should loopback after an attack instead of overflying target";
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

void CAirCAI::GiveCommandReal(const Command &c)
{
	// take care not to allow aircraft to be ordered to move out of the map
	if (c.id != CMD_MOVE && !AllowedCommand(c, true))
		return;

	else if (c.id == CMD_MOVE && c.params.size() >= 3 &&
			(c.params[0] < 0.f || c.params[2] < 0.f
			 || c.params[0] > gs->mapx*SQUARE_SIZE
			 || c.params[2] > gs->mapy*SQUARE_SIZE))
		return;

	if (c.id == CMD_SET_WANTED_MAX_SPEED) {
	  return;
	}

	if (c.id == CMD_AUTOREPAIRLEVEL) {
		if (c.params.empty()) {
			return;
		}
		CAirMoveType* airMT;
		if (owner->usingScriptMoveType) {
			airMT = (CAirMoveType*)owner->prevMoveType;
		} else {
			airMT = (CAirMoveType*)owner->moveType;
		}
		switch((int)c.params[0]){
			case 0: { airMT->repairBelowHealth = 0.0f; break; }
			case 1: { airMT->repairBelowHealth = 0.3f; break; }
			case 2: { airMT->repairBelowHealth = 0.5f; break; }
			case 3: { airMT->repairBelowHealth = 0.8f; break; }
		}
		for(vector<CommandDescription>::iterator cdi = possibleCommands.begin();
				cdi != possibleCommands.end(); ++cdi){
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

	if (c.id == CMD_IDLEMODE) {
		if (c.params.empty()) {
			return;
		}
		CAirMoveType* airMT;
		if (owner->usingScriptMoveType) {
			airMT = (CAirMoveType*)owner->prevMoveType;
		} else {
			airMT = (CAirMoveType*)owner->moveType;
		}
		switch((int)c.params[0]){
			case 0: { airMT->autoLand = false; break; }
			case 1: { airMT->autoLand = true;  break; }
		}
		for(vector<CommandDescription>::iterator cdi = possibleCommands.begin();
				cdi != possibleCommands.end(); ++cdi){
			if(cdi->id == CMD_IDLEMODE){
				char t[10];
				SNPRINTF(t, 10, "%d", (int)c.params[0]);
				cdi->params[0] = t;
				break;
			}
		}
		selectedUnits.PossibleCommandChange(owner);
		return;
	}

	if (c.id == CMD_LOOPBACKATTACK) {
		if (c.params.empty()) {
			return;
		}
		CAirMoveType* airMT;
		if (owner->usingScriptMoveType) {
			airMT = (CAirMoveType*)owner->prevMoveType;
		} else {
			airMT = (CAirMoveType*)owner->moveType;
		}
		switch((int)c.params[0]){
			case 0: { airMT->loopbackAttack = false; break; }
			case 1: { airMT->loopbackAttack = true;  break; }
		}
		for(vector<CommandDescription>::iterator cdi = possibleCommands.begin();
				cdi != possibleCommands.end(); ++cdi){
			if(cdi->id == CMD_LOOPBACKATTACK){
				char t[10];
				SNPRINTF(t, 10, "%d", (int)c.params[0]);
				cdi->params[0] = t;
				break;
			}
		}
		selectedUnits.PossibleCommandChange(owner);
		return;
	}

	if (!(c.options & SHIFT_KEY)
			&& nonQueingCommands.find(c.id) == nonQueingCommands.end()) {
		activeCommand=0;
		tempOrder=false;
	}

	if (c.id == CMD_AREA_ATTACK && c.params.size() < 4){
		Command c2 = c;
		c2.id = CMD_ATTACK;
		CCommandAI::GiveAllowedCommand(c2);
		return;
	}

	CCommandAI::GiveAllowedCommand(c);
}

void CAirCAI::SlowUpdate()
{
	if (!commandQue.empty() && commandQue.front().timeOut < gs->frameNum) {
		FinishCommand();
		return;
	}

	if (owner->usingScriptMoveType) {
		return; // avoid the invalid (CAirMoveType*) cast
	}

	AAirMoveType* myPlane=(AAirMoveType*) owner->moveType;

	bool wantToRefuel = LandRepairIfNeeded();
	if(!wantToRefuel && owner->unitDef->maxFuel > 0){
		wantToRefuel = RefuelIfNeeded();
	}

	if(commandQue.empty()){
		if(myPlane->aircraftState == AAirMoveType::AIRCRAFT_FLYING
				&& !owner->unitDef->DontLand() && myPlane->autoLand) {
			StopMove();
//			myPlane->SetState(AAirMoveType::AIRCRAFT_LANDING);
		}

		if(owner->unitDef->canAttack && owner->fireState>=2
				&& owner->moveState != 0 && owner->maxRange > 0){
			if(myPlane->IsFighter()){
				float testRad=1000 * owner->moveState;
				CUnit* enemy=helper->GetClosestEnemyAircraft(
					owner->pos + (owner->speed * 10), testRad, owner->allyteam);
				if(IsValidTarget(enemy)) {
					Command nc;
					nc.id = CMD_ATTACK;
					nc.params.push_back(enemy->id);
					nc.options = 0;
					commandQue.push_front(nc);
					inCommand = false;
					return;
				}
			}
			float testRad = 500 * owner->moveState;
			CUnit* enemy = helper->GetClosestEnemyUnit(
				owner->pos + (owner->speed * 20), testRad, owner->allyteam);
			if(IsValidTarget(enemy)) {
				Command nc;
				nc.id = CMD_ATTACK;
				nc.params.push_back(enemy->id);
				nc.options = 0;
				commandQue.push_front(nc);
				inCommand = false;
				return;
			}
		}
		return;
	}

	Command& c = commandQue.front();

	if (c.id == CMD_WAIT) {
		if ((myPlane->aircraftState == AAirMoveType::AIRCRAFT_FLYING)
		    && !owner->unitDef->DontLand() && myPlane->autoLand) {
			StopMove();
//			myPlane->SetState(AAirMoveType::AIRCRAFT_LANDING);
		}
		return;
	}

	if (c.id != CMD_STOP && c.id != CMD_AUTOREPAIRLEVEL
			&& c.id != CMD_IDLEMODE && c.id != CMD_SET_WANTED_MAX_SPEED) {
		myPlane->Takeoff();
	}

	if (wantToRefuel) {
		switch (c.id) {
			case CMD_AREA_ATTACK:
			case CMD_ATTACK:
			case CMD_FIGHT:
				return;
		}
	}

	switch(c.id){
		case CMD_AREA_ATTACK:{
			ExecuteAreaAttack(c);
			return;
		}
		default:{
			CMobileCAI::Execute();
			return;
		}
	}
}

/*
void CAirCAI::ExecuteMove(Command &c){
	if(tempOrder){
		tempOrder = false;
		inCommand = true;
	}
	if(!inCommand){
		inCommand=true;
		commandPos1=myPlane->owner->pos;
	}
	float3 pos(c.params[0], c.params[1], c.params[2]);
	commandPos2 = pos;
	myPlane->goalPos = pos;// This is not what we want move to do
	if(owner->unitDef->canAttack && !(c.options & CONTROL_KEY)){
		if(owner->fireState >= 2 && owner->moveState != 0 && owner->maxRange > 0){
			if(myPlane->isFighter){
				float testRad = 500 * owner->moveState;
				CUnit* enemy = helper->GetClosestEnemyAircraft(
					owner->pos+owner->speed * 20, testRad, owner->allyteam);
				if(enemy && ((enemy->unitDef->canfly && !enemy->crashing
						&& myPlane->isFighter) || (!enemy->unitDef->canfly
						&& (!myPlane->isFighter || owner->moveState == 2)))){
					if(owner->moveState != 1
							|| LinePointDist(commandPos1, commandPos2, enemy->pos) < 1000){
						Command nc;
						nc.id = CMD_ATTACK;
						nc.params.push_back(enemy->id);
						nc.options = c.options;
						commandQue.push_front(nc);
						tempOrder = true;
						inCommand = false;
						SlowUpdate();
						return;
					}
				}
			}
			if((!myPlane->isFighter || owner->moveState == 2) && owner->maxRange > 0){
				float testRad = 325 * owner->moveState;
				CUnit* enemy = helper->GetClosestEnemyUnit(
					owner->pos + owner->speed * 30, testRad, owner->allyteam);
				if(enemy && (owner->hasUWWeapons || !enemy->isUnderWater)
						&& ((enemy->unitDef->canfly && !enemy->crashing
						&& myPlane->isFighter) || (!enemy->unitDef->canfly
						&& (!myPlane->isFighter || owner->moveState==2)))){
					if(owner->moveState!=1 || LinePointDist(
							commandPos1, commandPos2, enemy->pos) < 1000){
						Command nc;
						nc.id = CMD_ATTACK;
						nc.params.push_back(enemy->id);
						nc.options = c.options;
						commandQue.push_front(nc);
						tempOrder = true;
						inCommand = false;
						SlowUpdate();
						return;
					}
				}
			}
		}
	}
	if((curPos - pos).SqLength2D() < 16000){
		FinishCommand();
	}
	return;
}*/

void CAirCAI::ExecuteFight(Command &c)
{
	assert((c.options & INTERNAL_ORDER) || owner->unitDef->canFight);
	AAirMoveType* myPlane = (AAirMoveType*) owner->moveType;
	if(tempOrder){
		tempOrder=false;
		inCommand=true;
	}
	if(c.params.size()<3){		//this shouldnt happen but anyway ...
		logOutput.Print("Error: got fight cmd with less than 3 params on %s in AirCAI",
			owner->unitDef->humanName.c_str());
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
		commandPos1 = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
		if ((owner->pos - commandPos1).SqLength2D() > (150 * 150)) {
			commandPos1 = owner->pos;
		}
	}
	goalPos = float3(c.params[0], c.params[1], c.params[2]);
	if(!inCommand){
		inCommand = true;
		commandPos2=goalPos;
	}
	if(c.params.size() >= 6){
		goalPos = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
	}

	// CMD_FIGHT is pretty useless if !canAttack but we try to honour the modders wishes anyway...
	if (owner->unitDef->canAttack && owner->fireState >= 2
			&& owner->moveState != 0 && owner->maxRange > 0) {
		float3 curPosOnLine = ClosestPointOnLine(commandPos1, commandPos2,
				owner->pos + owner->speed*10);
		float testRad = 1000 * owner->moveState;
		CUnit* enemy = NULL;
		if(myPlane->IsFighter()) {
			enemy = helper->GetClosestEnemyAircraft(curPosOnLine,
					testRad, owner->allyteam);
		}
		if(IsValidTarget(enemy) && (owner->moveState!=1
				|| LinePointDist(commandPos1, commandPos2, enemy->pos) < 1000))
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
			float3 curPosOnLine = ClosestPointOnLine(
				commandPos1, commandPos2, owner->pos + owner->speed * 20);
			float testRad = 500 * owner->moveState;
			enemy = helper->GetClosestEnemyUnit(curPosOnLine, testRad, owner->allyteam);
			if(IsValidTarget(enemy)) {
				Command nc;
				nc.id = CMD_ATTACK;
				nc.params.push_back(enemy->id);
				nc.options = c.options | INTERNAL_ORDER;
				PushOrUpdateReturnFight();
				commandQue.push_front(nc);
				tempOrder = true;
				inCommand = false;
				if(lastPC2 != gs->frameNum){	//avoid infinite loops
					lastPC2 = gs->frameNum;
					SlowUpdate();
				}
				return;
			}
		}
	}
	myPlane->goalPos = goalPos;

	CAirMoveType* airmt = dynamic_cast<CAirMoveType*>(myPlane);
	const float radius = airmt ? std::max(airmt->turnRadius + 2*SQUARE_SIZE, 128.f) : 127.f;

	// we're either circling or will get to the target in 8 frames
	if((owner->pos - goalPos).SqLength2D() < (radius * radius)
			|| (owner->pos + owner->speed*8 - goalPos).SqLength2D() < 127*127) {
		FinishCommand();
	}
	return;
}

void CAirCAI::ExecuteAttack(Command &c)
{
	assert(owner->unitDef->canAttack);
	targetAge++;

	if (tempOrder && owner->moveState == 1) {
		// limit how far away we fly
		if (orderTarget && LinePointDist(commandPos1, commandPos2, orderTarget->pos) > 1500) {
			owner->SetUserTarget(0);
			FinishCommand();
			return;
		}
	}

	if (inCommand) {
		if (targetDied || (c.params.size() == 1 && UpdateTargetLostTimer(int(c.params[0])) == 0)) {
			FinishCommand();
			return;
		}
		if ((c.params.size() == 3) && (owner->commandShotCount < 0) && (commandQue.size() > 1)) {
			owner->AttackGround(float3(c.params[0], c.params[1], c.params[2]), true);
			FinishCommand();
			return;
		}
		if (orderTarget) {
			if (orderTarget->unitDef->canfly && orderTarget->crashing) {
				owner->SetUserTarget(0);
				FinishCommand();
				return;
			}
			if (!(c.options & ALT_KEY) && SkipParalyzeTarget(orderTarget)) {
				owner->SetUserTarget(0);
				FinishCommand();
				return;
			}
		}
	} else {
		targetAge = 0;
		owner->commandShotCount = -1;

		if (c.params.size() == 1) {
			const unsigned int targetID = (unsigned int) c.params[0];
			const bool legalTarget      = (targetID < uh->MaxUnits());
			CUnit* targetUnit           = (legalTarget)? uh->units[targetID]: 0x0;

			if (legalTarget && targetUnit != 0x0 && targetUnit != owner) {
				orderTarget = targetUnit;
				owner->AttackUnit(orderTarget, false);
				AddDeathDependence(orderTarget);
				inCommand = true;
				SetGoal(orderTarget->pos, owner->pos, cancelDistance);
			} else {
				FinishCommand();
				return;
			}
		} else {
			float3 pos(c.params[0], c.params[1], c.params[2]);
			owner->AttackGround(pos, false);
			inCommand = true;
		}
	}

	return;
}

void CAirCAI::ExecuteAreaAttack(Command &c)
{
	assert(owner->unitDef->canAttack);
	AAirMoveType* myPlane = (AAirMoveType*) owner->moveType;
	if (targetDied){
		targetDied = false;
		inCommand = false;
	}
	const float3 pos(c.params[0], c.params[1], c.params[2]);
	const float radius = c.params[3];
	if (inCommand) {
		if (myPlane->aircraftState == AAirMoveType::AIRCRAFT_LANDED)
			inCommand = false;
		if (orderTarget && orderTarget->pos.SqDistance2D(pos) > Square(radius)) {
			inCommand = false;
			DeleteDeathDependence(orderTarget);
			orderTarget = 0;
		}
		if (owner->commandShotCount < 0) {
			if ((c.params.size() == 4) && (commandQue.size() > 1)) {
				owner->AttackUnit(0, true);
				FinishCommand();
			}
			else if (owner->userAttackGround) {
				// reset the attack position after each run
				float3 attackPos = pos + (gs->randVector() * radius);
				attackPos.y = ground->GetHeight(attackPos.x, attackPos.z);
				owner->AttackGround(attackPos, false);
				owner->commandShotCount = 0;
			}
		}
	} else {
		owner->commandShotCount = -1;

		if (myPlane->aircraftState != AAirMoveType::AIRCRAFT_LANDED) {
			inCommand = true;
			std::vector<int> eu;
			helper->GetEnemyUnits(pos, radius, owner->allyteam, eu);

			if (eu.empty()) {
				float3 attackPos = pos + gs->randVector() * radius;
				attackPos.y = ground->GetHeight(attackPos.x, attackPos.z);
				owner->AttackGround(attackPos, false);
			} else {
				// the range of randFloat() is inclusive of 1.0f
				int num = (int) (gs->randFloat() * (eu.size() - 1));
				orderTarget = uh->units[eu[num]];
				owner->AttackUnit(orderTarget, false);
				AddDeathDependence(orderTarget);
			}
		}
	}
}

void CAirCAI::ExecuteGuard(Command &c)
{
	assert(owner->unitDef->canGuard);
	if (int(c.params[0]) >= 0 && uh->units[int(c.params[0])] != NULL
			&& UpdateTargetLostTimer(int(c.params[0]))) {
		CUnit* guarded = uh->units[int(c.params[0])];
		if(owner->unitDef->canAttack && guarded->lastAttack + 40 < gs->frameNum
				&& owner->maxRange > 0 && IsValidTarget(guarded->lastAttacker))
		{
			Command nc;
			nc.id = CMD_ATTACK;
			nc.params.push_back(guarded->lastAttacker->id);
			nc.options = c.options | INTERNAL_ORDER;
			commandQue.push_front(nc);
			SlowUpdate();
			return;
		} else {
			Command c2;
			c2.id = CMD_MOVE;
			c2.options = c.options | INTERNAL_ORDER;
			c2.params.push_back(guarded->pos.x);
			c2.params.push_back(guarded->pos.y);
			c2.params.push_back(guarded->pos.z);
			c2.timeOut = gs->frameNum + 60;
			commandQue.push_front(c2);
			return;
		}
	} else {
		FinishCommand();
	}
}

int CAirCAI::GetDefaultCmd(CUnit* pointed, CFeature* feature)
{
	if (pointed) {
		if (!teamHandler->Ally(gu->myAllyTeam, pointed->allyteam)) {
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

bool CAirCAI::IsValidTarget(const CUnit* enemy) const {
	return CMobileCAI::IsValidTarget(enemy) && !enemy->crashing
		&& (((CAirMoveType*)owner->moveType)->isFighter || !enemy->unitDef->canfly);
}

void CAirCAI::DrawCommands(void)
{
	lineDrawer.StartPath(owner->drawMidPos, cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	CCommandQueue::iterator ci;
	for(ci=commandQue.begin();ci!=commandQue.end();++ci){
		switch(ci->id){
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
					const CUnit* unit = uh->units[int(ci->params[0])];
					if((unit != NULL) && isTrackable(unit)) {
						const float3 endPos =
							helper->GetUnitErrorPos(unit, owner->allyteam);
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
				const CUnit* unit = uh->units[int(ci->params[0])];
				if((unit != NULL) && isTrackable(unit)) {
					const float3 endPos =
						helper->GetUnitErrorPos(unit, owner->allyteam);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.guard);
				}
				break;
			}
			case CMD_WAIT:{
				DrawWaitIcon(*ci);
				break;
			}
			case CMD_SELFD:{
				lineDrawer.DrawIconAtLastPos(ci->id);
				break;
			}
			default:
				DrawDefaultCommand(*ci);
				break;
		}
	}
	lineDrawer.FinishPath();
}

void CAirCAI::FinishCommand(void)
{
	targetAge=0;
	CCommandAI::FinishCommand();
}

void CAirCAI::BuggerOff(float3 pos, float radius)
{
	AAirMoveType* myPlane = dynamic_cast<AAirMoveType*>(owner->moveType);
	if(myPlane) {
		myPlane->Takeoff();
	} else {
		CMobileCAI::BuggerOff(pos, radius);
	}
}

void CAirCAI::SetGoal(const float3 &pos, const float3 &curPos, float goalRadius){
	owner->moveType->SetGoal(pos);
	CMobileCAI::SetGoal(pos, curPos, goalRadius);
}
