#include "StdAfx.h"
#include "MobileCAI.h"
#include "TransportCAI.h"
#include "LineDrawer.h"
#include "ExternalAI/Group.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Game/Team.h"
#include "Game/UI/CommandColors.h"
#include "Map/Ground.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/MoveTypes/TAAirMoveType.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/DGunWeapon.h"
#include "System/LogOutput.h"
#include "myMath.h"
#include "mmgr.h"

CR_BIND_DERIVED(CMobileCAI ,CCommandAI , );

CR_REG_METADATA(CMobileCAI, (
				CR_MEMBER(goalPos),
				CR_MEMBER(lastUserGoal),

				CR_MEMBER(lastIdleCheck),
				CR_MEMBER(tempOrder),

				CR_MEMBER(lastPC),

				CR_MEMBER(maxWantedSpeed),

				CR_MEMBER(lastBuggerOffTime),
				CR_MEMBER(buggerOffPos),
				CR_MEMBER(buggerOffRadius),

				CR_MEMBER(commandPos1),
				CR_MEMBER(commandPos2),

				CR_MEMBER(lastCloseInTry),

				CR_MEMBER(cancelDistance),
				CR_MEMBER(slowGuard),
				CR_MEMBER(moveDir),
				CR_RESERVED(16)
				));

CMobileCAI::CMobileCAI()
: CCommandAI(),
//	patrolTime(0),
	goalPos(-1,-1,-1),
	tempOrder(false),
	lastBuggerOffTime(-200),
	buggerOffPos(0,0,0),
	buggerOffRadius(0),
	maxWantedSpeed(0),
	lastIdleCheck(0),
	commandPos1(ZeroVector),
	commandPos2(ZeroVector),
	lastPC(-1),
	cancelDistance(1024),
	lastCloseInTry(-1),
	slowGuard(false),
	moveDir(gs->randFloat() > 0.5),
	lastUserGoal(0,0,0)
{}


CMobileCAI::CMobileCAI(CUnit* owner)
: CCommandAI(owner),
//	patrolTime(0),
	goalPos(-1,-1,-1),
	tempOrder(false),
	lastBuggerOffTime(-200),
	buggerOffPos(0,0,0),
	buggerOffRadius(0),
	maxWantedSpeed(0),
	lastIdleCheck(0),
	commandPos1(ZeroVector),
	commandPos2(ZeroVector),
	lastPC(-1),
	cancelDistance(1024),
	lastCloseInTry(-1),
	slowGuard(false),
	moveDir(gs->randFloat() > 0.5),
	lastUserGoal(owner->pos)
{
	CommandDescription c;

	c.id=CMD_LOAD_ONTO;
	c.action="loadonto";
	c.type=CMDTYPE_ICON_UNIT;
	c.name="Load units";
	c.mouseicon=c.name;
	c.tooltip="Sets the unit to load itself onto a transport";
	c.onlyKey = true;
	possibleCommands.push_back(c);
	c.onlyKey = false;

	if (owner->unitDef->canmove) {
		c.id=CMD_MOVE;
		c.action="move";
		c.type=CMDTYPE_ICON_FRONT;
		c.name="Move";
		c.mouseicon=c.name;
		c.tooltip="Move: Order the unit to move to a position";
		c.params.push_back("1000000"); // max distance
		possibleCommands.push_back(c);
		c.params.clear();
	}

	if(owner->unitDef->canPatrol){
		c.id=CMD_PATROL;
		c.action="patrol";
		c.type=CMDTYPE_ICON_MAP;
		c.name="Patrol";
		c.mouseicon=c.name;
		c.tooltip="Patrol: Order the unit to patrol to one or more waypoints";
		possibleCommands.push_back(c);
		c.params.clear();
	}

	if (owner->unitDef->canFight) {
		c.id = CMD_FIGHT;
		c.action="fight";
		c.type = CMDTYPE_ICON_FRONT;
		c.name = "Fight";
		c.mouseicon=c.name;
		c.tooltip = "Fight: Order the unit to take action while moving to a position";
		possibleCommands.push_back(c);
	}

	if(owner->unitDef->canGuard){
		c.id=CMD_GUARD;
		c.action="guard";
		c.type=CMDTYPE_ICON_UNIT;
		c.name="Guard";
		c.mouseicon=c.name;
		c.tooltip="Guard: Order a unit to guard another unit and attack units attacking it";
		possibleCommands.push_back(c);
	}

	if(owner->unitDef->canfly){
		c.params.clear();
		c.id=CMD_AUTOREPAIRLEVEL;
		c.action="autorepairlevel";
		c.type=CMDTYPE_ICON_MODE;
		c.name="Repair level";
		c.mouseicon=c.name;
		c.params.push_back("1");
		c.params.push_back("LandAt 0");
		c.params.push_back("LandAt 30");
		c.params.push_back("LandAt 50");
		c.params.push_back("LandAt 80");
		c.tooltip=
			"Repair level: Sets at which health level an aircraft will try to find a repair pad";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_AUTOREPAIRLEVEL);

		c.params.clear();
		c.id=CMD_IDLEMODE;
		c.action="idlemode";
		c.type=CMDTYPE_ICON_MODE;
		c.name="Land mode";
		c.mouseicon=c.name;
		c.params.push_back("1");
		c.params.push_back(" Fly ");
		c.params.push_back("Land");
		c.tooltip="Land mode: Sets what aircraft will do on idle";
		possibleCommands.push_back(c);
		nonQueingCommands.insert(CMD_IDLEMODE);
	}

	nonQueingCommands.insert(CMD_SET_WANTED_MAX_SPEED);
}


CMobileCAI::~CMobileCAI()
{

}

void CMobileCAI::GiveCommandReal(const Command &c)
{
	if (!AllowedCommand(c))
		return;
	if(owner->unitDef->canfly && c.id==CMD_AUTOREPAIRLEVEL){
		if (c.params.empty()) {
			return;
		}
		CTAAirMoveType* airMT;
		if (owner->usingScriptMoveType) {
			if(!dynamic_cast<CTAAirMoveType*>(owner->prevMoveType))
				return;
			airMT = (CTAAirMoveType*)owner->prevMoveType;
		} else {
			if(!dynamic_cast<CTAAirMoveType*>(owner->moveType))
				return;
			airMT = (CTAAirMoveType*)owner->moveType;
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
	if(owner->unitDef->canfly && c.id==CMD_IDLEMODE){
		if (c.params.empty()) {
			return;
		}
		CTAAirMoveType* airMT;
		if (owner->usingScriptMoveType) {
			if(!dynamic_cast<CTAAirMoveType*>(owner->prevMoveType))
				return;
			airMT = (CTAAirMoveType*)owner->prevMoveType;
		} else {
			if(!dynamic_cast<CTAAirMoveType*>(owner->moveType))
				return;
			airMT = (CTAAirMoveType*)owner->moveType;
		}
		switch((int)c.params[0]){
			case 0: { airMT->autoLand = false; break; }
			case 1: { airMT->autoLand = true; break; }
		}
		for(vector<CommandDescription>::iterator cdi = possibleCommands.begin();
				cdi != possibleCommands.end(); ++cdi){
			if(cdi->id==CMD_IDLEMODE){
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
		tempOrder=false;
		StopSlowGuard();
	}
	CCommandAI::GiveAllowedCommand(c);
}

void CMobileCAI::RefuelIfNeeded()
{
	if(!owner->moveType->reservedPad) {
		if(owner->currentFuel <= 0){
			StopMove();
			owner->userAttackGround=false;
			owner->userTarget=0;
			inCommand=false;
			CAirBaseHandler::LandingPad* lp = airBaseHandler->FindAirBase(
				owner, owner->unitDef->minAirBasePower);
			if(lp){
				owner->moveType->ReservePad(lp);
			} else {
				float3 landingPos = airBaseHandler->FindClosestAirBasePos(
						owner, owner->unitDef->minAirBasePower);
				if(landingPos != ZeroVector && owner->pos.distance2D(landingPos) > 800){
					SetGoal(landingPos,owner->pos);
					//myPlane->goalPos = landingPos;
				} else {
					owner->moveType->StopMoving();
				}
			}
		} else if(owner->currentFuel <
				(owner->moveType->repairBelowHealth * owner->unitDef->maxFuel)
				&& commandQue.empty() || commandQue.front().id == CMD_PATROL
				|| commandQue.front().id == CMD_FIGHT) {
			CAirBaseHandler::LandingPad* lp =
				airBaseHandler->FindAirBase(owner, owner->unitDef->minAirBasePower);
			if(lp) {
				StopMove();
				owner->userAttackGround = false;
				owner->userTarget = 0;
				inCommand = false;
				owner->moveType->ReservePad(lp);
			}
		} else if(owner->health < owner->maxHealth*owner->moveType->repairBelowHealth) {
			CAirBaseHandler::LandingPad* lp =
				airBaseHandler->FindAirBase(owner, owner->unitDef->minAirBasePower);
			if(lp) {
				owner->moveType->ReservePad(lp);
			}
		}
	}
}

void CMobileCAI::SlowUpdate()
{
	if(owner->unitDef->maxFuel>0 && dynamic_cast<AAirMoveType*>(owner->moveType)){
		RefuelIfNeeded();
	}

	if(!commandQue.empty() && commandQue.front().timeOut < gs->frameNum){
		StopMove();
		FinishCommand();
		return;
	}

	if(commandQue.empty()) {
//		if(!owner->ai || owner->ai->State() != CHasState::Active) {
			IdleCheck();
//		}
		//the attack order could terminate directly and thus cause a loop
		if(commandQue.empty() || commandQue.front().id == CMD_ATTACK) {
			return;
		}
	}

	// treat any following CMD_SET_WANTED_MAX_SPEED commands as options
	// to the current command  (and ignore them when it's their turn
	if (commandQue.size() >= 2 && !slowGuard) {
		CCommandQueue::iterator it = commandQue.begin();
		it++;
		const Command& c = *it;
		if ((c.id == CMD_SET_WANTED_MAX_SPEED) && (c.params.size() >= 1)) {
			const float defMaxSpeed = owner->maxSpeed;
			const float newMaxSpeed = std::min(c.params[0], defMaxSpeed);
			if (newMaxSpeed > 0)
				owner->moveType->SetMaxSpeed(newMaxSpeed);
		}
	}

	Execute();
}

/**
* @brief Executes the first command in the commandQue
*/
void CMobileCAI::Execute()
{
	Command& c = commandQue.front();
	switch (c.id) {
		case CMD_SET_WANTED_MAX_SPEED:	{ ExecuteSetWantedMaxSpeed(c); return; }
		case CMD_MOVE:      { ExecuteMove(c);	     return; }
		case CMD_PATROL:    { ExecutePatrol(c);		 return; }
		case CMD_FIGHT:     { ExecuteFight(c);		 return; }
		case CMD_GUARD:     { ExecuteGuard(c);		 return; }
		case CMD_LOAD_ONTO: { ExecuteLoadUnits(c); return; }
		default: {
		  CCommandAI::SlowUpdate();
		  return;
		}
	}
}

/**
* @brief executes the set wanted max speed command
*/
void CMobileCAI::ExecuteSetWantedMaxSpeed(Command &c)
{
	if (repeatOrders && (commandQue.size() >= 2) &&
			(commandQue.back().id != CMD_SET_WANTED_MAX_SPEED)) {
		commandQue.push_back(commandQue.front());
	}
	FinishCommand();
	SlowUpdate();
	return;
}

/**
* @brief executes the move command
*/
void CMobileCAI::ExecuteMove(Command &c)
{
	float3 pos = float3(c.params[0], c.params[1], c.params[2]);
	if(pos != goalPos){
		SetGoal(pos, owner->pos);
	}
	if((owner->pos - goalPos).SqLength2D() < cancelDistance ||
			owner->moveType->progressState == AMoveType::Failed){
		FinishCommand();
	}
	return;
}

void CMobileCAI::ExecuteLoadUnits(Command &c){
	CTransportUnit* tran = dynamic_cast<CTransportUnit*>(uh->units[(int)c.params[0]]);
	if(!tran){
		FinishCommand();
		return;
	}
	if(!inCommand){
		inCommand ^= true;
		Command newCommand;
		newCommand.id = CMD_LOAD_UNITS;
		newCommand.params.push_back(owner->id);
		newCommand.options = INTERNAL_ORDER | SHIFT_KEY;
		tran->commandAI->GiveCommandReal(newCommand);
	}
	if(owner->transporter) {
		FinishCommand();
		return;
	}
	CUnit *unit = uh->units[(int)c.params[0]];
	if (!unit) {
		return;
	}
	float3 pos = unit->pos;
	if((pos - goalPos).SqLength2D() > cancelDistance){
		SetGoal(pos, owner->pos);
	}
	if((owner->pos - goalPos).SqLength2D() < cancelDistance){
		StopMove();
	}
	if(owner->moveType->progressState == AMoveType::Failed){
	}
	return;
}

/**
* @brief Executes the Patrol command c
*/
void CMobileCAI::ExecutePatrol(Command &c)
{
	assert(owner->unitDef->canPatrol);
	assert(c.params.size() >= 3);
	Command temp;
	temp.id = CMD_FIGHT;
	temp.params.push_back(c.params[0]);
	temp.params.push_back(c.params[1]);
	temp.params.push_back(c.params[2]);
	temp.options = c.options | INTERNAL_ORDER;
	commandQue.push_back(c);
	commandQue.pop_front();
	commandQue.push_front(temp);
	if(owner->group){
		owner->group->CommandFinished(owner->id, CMD_PATROL);
	}
	ExecuteFight(temp);
	return;
}

/**
* @brief Executes the Fight command c
*/
void CMobileCAI::ExecuteFight(Command &c)
{
	assert((c.options & INTERNAL_ORDER) || owner->unitDef->canFight);
	if(c.params.size() == 1) {
		if(orderTarget && owner->weapons.size() > 0
				&& !owner->weapons.front()->AttackUnit(orderTarget, false)) {
			CUnit* newTarget = helper->GetClosestEnemyUnit(
				owner->pos, owner->maxRange, owner->allyteam);
			if(IsValidTarget(newTarget) && !owner->weapons.empty()
					&& owner->weapons.front()->AttackUnit(newTarget, false)) {
				c.params[0] = newTarget->id;
				inCommand = false;
			} else {
				owner->weapons.front()->AttackUnit(orderTarget, false);
			}
		}
		ExecuteAttack(c);
		return;
	}
	if(tempOrder){
		inCommand = true;
		tempOrder = false;
	}
	if(c.params.size()<3){		//this shouldnt happen but anyway ...
		logOutput.Print("Error: got fight cmd with less than 3 params on %s in mobilecai",
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
		// fight command finished by the 'if((curPos-pos).SqLength2D()<(64*64)){'
		// condition, but is actually updated correctly if you click somewhere
		// outside the area close to the line (for a new command).
		commandPos1 = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
		if ((owner->pos - commandPos1).SqLength2D() > (96 * 96)) {
			commandPos1 = owner->pos;
		}
	}
	float3 pos(c.params[0],c.params[1],c.params[2]);
	if(!inCommand){
		inCommand = true;
		commandPos2 = pos;
		lastUserGoal = commandPos2;
	}
	if(c.params.size() >= 6){
		pos = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
	}
	if(pos!=goalPos){
		SetGoal(pos, owner->pos);
	}

	if(owner->unitDef->canAttack && owner->fireState>=2){
		float3 curPosOnLine = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
		CUnit* enemy=helper->GetClosestEnemyUnit(
			curPosOnLine, owner->maxRange + 100 * owner->moveState * owner->moveState,
			owner->allyteam);
		if(IsValidTarget(enemy) && !owner->weapons.empty()) {
			Command c2;
			c2.id=CMD_FIGHT;
			c2.options=c.options|INTERNAL_ORDER;
			c2.params.push_back(enemy->id);
			PushOrUpdateReturnFight();
			commandQue.push_front(c2);
			inCommand=false;
			tempOrder=true;
			if(lastPC!=gs->frameNum){	//avoid infinite loops
				lastPC=gs->frameNum;
				SlowUpdate();
			}
			return;
		}
	}
	if((owner->pos - goalPos).SqLength2D() < (64 * 64)
			|| (owner->moveType->progressState == AMoveType::Failed)){
		FinishCommand();
	}
	return;
}

bool CMobileCAI::IsValidTarget(const CUnit* enemy) const {
	return enemy && (owner->hasUWWeapons || !enemy->isUnderWater)
		&& !(owner->unitDef->noChaseCategory & enemy->category)
		&& !enemy->neutral;
}

/**
* @brief Executes the guard command c
*/
void CMobileCAI::ExecuteGuard(Command &c)
{
	assert(owner->unitDef->canGuard);
	assert(!c.params.empty());
	if(int(c.params[0]) >= 0 && uh->units[int(c.params[0])] != NULL
			&& UpdateTargetLostTimer(int(c.params[0]))){
		CUnit* guarded = uh->units[int(c.params[0])];
		if(owner->unitDef->canAttack && guarded->lastAttack + 40 < gs->frameNum
				&& IsValidTarget(guarded->lastAttacker))
		{
			StopSlowGuard();
			Command nc;
			nc.id=CMD_ATTACK;
			nc.params.push_back(guarded->lastAttacker->id);
			nc.options = c.options;
			commandQue.push_front(nc);
			SlowUpdate();
			return;
		} else {
			//float3 dif = guarded->speed * guarded->frontdir;
			float3 dif = guarded->pos - owner->pos;
			dif.Normalize();
			float3 goal = guarded->pos - dif * (guarded->radius + owner->radius + 64);
			if((goalPos - goal).SqLength2D() > 1600
					|| (goalPos - owner->pos).SqLength2D()
						< (owner->maxSpeed*30 + 1 + SQUARE_SIZE*2)
						 * (owner->maxSpeed*30 + 1 + SQUARE_SIZE*2)){
				SetGoal(goal, owner->pos);
			}
			if((goal - owner->pos).SqLength2D() < 6400) {
				StartSlowGuard(guarded->maxSpeed);
				if((goal-owner->pos).SqLength2D() < 1800){
					StopMove();
					NonMoving();
				}
			} else {
				StopSlowGuard();
			}
		}
	} else {
		FinishCommand();
	}
	return;
}

/**
* @brief Executes the stop command c
*/
void CMobileCAI::ExecuteStop(Command &c)
{
	StopMove();
	return CCommandAI::ExecuteStop(c);
}

/**
* @brief Executes the DGun command c
*/
void CMobileCAI::ExecuteDGun(Command &c)
{
	if(uh->limitDgun && owner->unitDef->isCommander
			&& owner->pos.distance(gs->Team(owner->team)->startPos)>uh->dgunRadius){
		StopMove();
		return FinishCommand();
	}
	ExecuteAttack(c);
}




/**
* @brief Causes this CMobileCAI to execute the attack order c
*/
void CMobileCAI::ExecuteAttack(Command &c)
{
	assert(owner->unitDef->canAttack);

	// limit how far away we fly
	if (tempOrder && (owner->moveState < 2) && orderTarget
			&& LinePointDist(ClosestPointOnLine(commandPos1, commandPos2, owner->pos),
					commandPos2, orderTarget->pos)
			> (500*owner->moveState + owner->maxRange)) {
		StopMove();
		FinishCommand();
		return;
	}

	// check if we are in direct command of attacker
	if (!inCommand) {
		// don't start counting until the owner->AttackGround() order is given
		owner->commandShotCount = -1;

		if (c.params.size() == 1) {
			int unitID = int(c.params[0]);

			// check if we have valid target parameter and that we aren't attacking ourselves
			if (uh->units[unitID] != 0 && uh->units[unitID] != owner) {
				float3 fix = uh->units[unitID]->pos + owner->posErrorVector * 128;
				float3 diff = float3(fix - owner->pos).Normalize();
				if(owner->moveState > 0 || !tempOrder) {
					SetGoal(fix - diff*uh->units[unitID]->radius, owner->pos);
				}
				// get ID of attack-order target unit
				orderTarget = uh->units[unitID];
				AddDeathDependence(orderTarget);
				inCommand = true;
			}
			else {
				// unit may not fire on itself, cancel order
				StopMove();
				FinishCommand();
				return;
			}
		}
		else {
			// user gave force-fire attack command
			float3 pos(c.params[0], c.params[1], c.params[2]);
 			SetGoal(pos, owner->pos);
			inCommand = true;
		}
	}
	else if ((c.params.size() == 3) && (owner->commandShotCount > 0) && (commandQue.size() > 1)) {
		// the trailing CMD_SET_WANTED_MAX_SPEED in a command pair does not count
		if ((commandQue.size() > 2) || (commandQue.back().id != CMD_SET_WANTED_MAX_SPEED)) {
			StopMove();
			FinishCommand();
			return;
		}
	}

	// if our target is dead or we lost it then stop attacking
	// NOTE: unit should actually just continue to target area!
	if (targetDied || (c.params.size() == 1 && UpdateTargetLostTimer(int(c.params[0])) == 0)) {
		// cancel keeppointingto
		StopMove();
		FinishCommand();
		return;
	}


	// user clicked on enemy unit (note that we handle aircrafts slightly differently)
	if (orderTarget) {
		//bool b1 = owner->AttackUnit(orderTarget, c.id == CMD_DGUN);
		bool b2 = false;
		bool b3 = false;
		bool b4 = false;
		float edgeFactor = 0.f; // percent offset to target center
		float3 diff = owner->pos - orderTarget->pos;

		if (owner->weapons.size() > 0) {
			if (!(c.options & ALT_KEY) && SkipParalyzeTarget(orderTarget)) {
				StopMove();
				FinishCommand();
				return;
			}
			CWeapon* w = owner->weapons.front();
			// if we have at least one weapon then check if we
			// can hit target with our first (meanest) one
			b2 = w->TryTargetRotate(orderTarget, c.id == CMD_DGUN);
			b3 = (w->range - (w->relWeaponPos).Length())
					> (orderTarget->pos.distance(owner->pos));
			b4 = w->TryTargetHeading(GetHeadingFromVector(-diff.x, -diff.z),
					orderTarget->pos, orderTarget != NULL);
			edgeFactor = fabs(w->targetBorder);
		}

		double diffLength2d = diff.Length2D();

		// if w->AttackUnit() returned true then we are already
		// in range with our biggest weapon so stop moving
		// also make sure that we're not locked in close-in/in-range state loop
		// due to rotates invoked by in-range or out-of-range states
		if (b2) {
			if (!(tempOrder && owner->moveState == 0)
				&& (diffLength2d * 1.4f > owner->maxRange
					- orderTarget->speed.SqLength()
							/ owner->unitDef->maxAcc)
				&& b4 && diff.dot(orderTarget->speed) < 0)
			{
				SetGoal(owner->pos + (orderTarget->speed * 80), owner->pos,
						SQUARE_SIZE, orderTarget->speed.Length() * 1.1f);
			} else {
				StopMove();
				// FIXME kill magic frame number
				if (gs->frameNum > lastCloseInTry + MAX_CLOSE_IN_RETRY_TICKS) {
					owner->moveType->KeepPointingTo(orderTarget,
							std::min((float) (owner->losRadius * SQUARE_SIZE * 2),
									owner->maxRange * 0.9f), true);
				}
			}
			owner->AttackUnit(orderTarget, c.id == CMD_DGUN);
		}
		// if ((our movetype has type TAAirMoveType and length of 2D vector from us to target
		// less than 90% of our maximum range) OR squared length of 2D vector from us to target
		// less than 1024) then we are close enough
		else if(diffLength2d < (owner->maxRange * 0.9f)){
			if (dynamic_cast<CTAAirMoveType*>(owner->moveType)
					|| (diff.SqLength2D() < 1024))
			{
				StopMove();
				owner->moveType->KeepPointingTo(orderTarget,
						std::min((float) (owner->losRadius * SQUARE_SIZE * 2),
								owner->maxRange * 0.9f), true);
			} else if(tempOrder && owner->moveState == 0){
				SetGoal(lastUserGoal, owner->pos);
			}

			// if (((first weapon range minus first weapon length greater than distance to target)
			// and length of 2D vector from us to target less than 90% of our maximum range)
			// then we are close enough, but need to move sideways to get a shot.
			//assumption is flawed: The unit may be aiming or otherwise unable to shoot
			else if (owner->unitDef->strafeToAttack && b3 && diffLength2d < (owner->maxRange * 0.9f))
			{
				moveDir ^= (owner->moveType->progressState == AMoveType::Failed);
				float sin = moveDir ? 3.0/5 : -3.0/5;
				float cos = 4.0/5;
				float3 goalDiff(0, 0, 0);
				goalDiff.x = diff.dot(float3(cos, 0, -sin));
				goalDiff.z = diff.dot(float3(sin, 0, cos));
				goalDiff *= (diffLength2d < (owner->maxRange * 0.3f)) ? 1/cos : cos;
				goalDiff += orderTarget->pos;
				SetGoal(goalDiff, owner->pos);
			}
		}

		// if 2D distance of (target position plus attacker error vector times 128)
		// to goal position greater than
		// (10 plus 20% of 2D distance between attacker and target) then we need to close
		// in on target more
		else if ((orderTarget->pos + owner->posErrorVector * 128).distance2D(goalPos)
				> (10 + orderTarget->pos.distance2D(owner->pos) * 0.2f)) {
			// if the target isn't in LOS, go to its approximate position
			// otherwise try to go precisely to the target
			// this should fix issues with low range weapons (mainly melee)
			float3 fix = orderTarget->pos +
					(orderTarget->losStatus[owner->allyteam] & LOS_INLOS ?
						float3(0.f,0.f,0.f) :
						owner->posErrorVector * 128);
			float3 norm = float3(fix - owner->pos).Normalize();
			float3 goal = fix - norm*(orderTarget->radius*edgeFactor*0.8f);
			SetGoal(goal, owner->pos);
			if (lastCloseInTry < gs->frameNum + MAX_CLOSE_IN_RETRY_TICKS)
				lastCloseInTry = gs->frameNum;
		}
	}

	// user is attacking ground
	else {
		const float3 pos(c.params[0], c.params[1], c.params[2]);
		const float3 diff = owner->pos - pos;

		if (owner->weapons.size() > 0) {
			// if we have at least one weapon then check if
			// we can hit position with our first (assumed
			// to be meanest) one
			CWeapon* w = owner->weapons.front();

			// XXX hack - dgun overrides any checks
			if (c.id == CMD_DGUN) {
				float rr = owner->maxRange * owner->maxRange;

				for (vector<CWeapon*>::iterator it = owner->weapons.begin();
						it != owner->weapons.end(); ++it) {

					if (dynamic_cast<CDGunWeapon*>(*it))
						rr = (*it)->range * (*it)->range;
				}

				if (diff.SqLength() < rr) {
					StopMove();
					owner->AttackGround(pos, c.id == CMD_DGUN);
					owner->moveType->KeepPointingTo(pos, owner->maxRange * 0.9f, true);
				}
			} else {
				const bool inAngle = w->TryTargetRotate(pos, c.id == CMD_DGUN);
				const bool inRange = diff.Length2D() < (w->range - (w->relWeaponPos).Length2D());

				if (inAngle || inRange) {
					StopMove();
					owner->AttackGround(pos, c.id == CMD_DGUN);
					owner->moveType->KeepPointingTo(pos, owner->maxRange * 0.9f, true);
				}
			}
		}

		else if (diff.SqLength2D() < 1024) {
			StopMove();
			owner->moveType->KeepPointingTo(pos, owner->maxRange * 0.9f, true);
		}

		// if we are more than 10 units distant from target position then keeping moving closer
		else if (pos.distance2D(goalPos) > 10) {
			SetGoal(pos, owner->pos);
		}
	}
}




int CMobileCAI::GetDefaultCmd(CUnit* pointed, CFeature* feature)
{
	if (pointed) {
		if (!gs->Ally(gu->myAllyTeam,pointed->allyteam)) {
			if (owner->unitDef->canAttack) {
				return CMD_ATTACK;
			}
		} else {
			CTransportCAI* tran = dynamic_cast<CTransportCAI*>(pointed->commandAI);
			if(tran && tran->CanTransport(owner)) {
				return CMD_LOAD_ONTO;
			} else if (owner->unitDef->canGuard) {
				return CMD_GUARD;
			}
		}
	}
	return CMD_MOVE;
}

void CMobileCAI::SetGoal(const float3 &pos, const float3& curPos, float goalRadius)
{
	if (pos == goalPos)
		return;
	goalPos = pos;
	owner->moveType->StartMoving(pos, goalRadius);
}

void CMobileCAI::SetGoal(const float3 &pos, const float3& curPos, float goalRadius, float speed)
{
	if (pos == goalPos)
		return;
	goalPos = pos;
	owner->moveType->StartMoving(pos, goalRadius, speed);
}

void CMobileCAI::StopMove()
{
	owner->moveType->StopMoving();
	goalPos = owner->pos;
}

void CMobileCAI::DrawCommands(void)
{
	lineDrawer.StartPath(owner->midPos, cmdColors.start);

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
			case CMD_PATROL:{
				const float3 endPos(ci->params[0],ci->params[1],ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.patrol);
				break;
			}
			case CMD_FIGHT:{
				if(ci->params.size() != 1){
					const float3 endPos(ci->params[0],ci->params[1],ci->params[2]);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.fight);
					break;
				}
			}
			case CMD_ATTACK:
			case CMD_DGUN:{
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
			case CMD_GUARD:{
				const CUnit* unit = uh->units[int(ci->params[0])];
				if((unit != NULL) && isTrackable(unit)) {
					const float3 endPos =
						helper->GetUnitErrorPos(unit, owner->allyteam);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.guard);
				}
				break;
			}
			case CMD_LOAD_ONTO:{
				const CUnit* unit = uh->units[int(ci->params[0])];
				lineDrawer.DrawLineAndIcon(ci->id, unit->pos, cmdColors.load);
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
			default: {
				DrawDefaultCommand(*ci);
				break;
			}
		}
	}
	lineDrawer.FinishPath();
}


void CMobileCAI::BuggerOff(float3 pos, float radius)
{
	lastBuggerOffTime = gs->frameNum;
	buggerOffPos = pos;
	buggerOffRadius = radius + owner->radius;
}

void CMobileCAI::NonMoving(void)
{
	if (owner->usingScriptMoveType) {
		return;
	}
	if(lastBuggerOffTime>gs->frameNum-200){
		float3 dif=owner->pos-buggerOffPos;
		dif.y=0;
		float length=dif.Length();
		if(!length) {
			length=0.1f;
			dif = float3(0.1f, 0.0f, 0.0f);
		}
		if (length < buggerOffRadius) {
			float3 goalPos = buggerOffPos + dif * ((buggerOffRadius + 128) / length);
			Command c;
			c.id = CMD_MOVE;
			c.options = 0;//INTERNAL_ORDER;
			c.params.push_back(goalPos.x);
			c.params.push_back(goalPos.y);
			c.params.push_back(goalPos.z);
			c.timeOut = gs->frameNum + 40;
			commandQue.push_front(c);
			unimportantMove = true;
		}
	}
}

void CMobileCAI::FinishCommand(void)
{
	if(!(commandQue.front().options & INTERNAL_ORDER)){
		lastUserGoal=owner->pos;
	}
	StopSlowGuard();
	CCommandAI::FinishCommand();
}

void CMobileCAI::IdleCheck(void)
{
	if(owner->unitDef->canAttack && owner->moveState && owner->fireState
			&& !owner->weapons.empty() && owner->haveTarget) {
		if(!owner->userTarget) {
			owner->haveTarget = false;
		} else if(owner->pos.distance2D(owner->userTarget->pos) < 
				owner->maxRange + 200*owner->moveState*owner->moveState) {
			Command c;
			c.id = CMD_ATTACK;
			c.options=INTERNAL_ORDER;
			c.params.push_back(owner->userTarget->id);
			c.timeOut = gs->frameNum + 140;
			commandQue.push_front(c);
			return;
		}
	}
	if(owner->unitDef->canAttack && owner->moveState && owner->fireState
				&& !owner->weapons.empty() && !owner->haveTarget) {
		if(owner->lastAttacker && owner->lastAttack + 200 > gs->frameNum
				&& !(owner->unitDef->noChaseCategory & owner->lastAttacker->category)){
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
	if (owner->unitDef->canAttack && (gs->frameNum >= lastIdleCheck+10)
			&& owner->moveState && owner->fireState>=2 &&
			!owner->weapons.empty() && !owner->haveTarget)
	{
		CUnit* u = helper->GetClosestEnemyUnit(owner->pos,
				owner->maxRange + 150 * owner->moveState * owner->moveState, owner->allyteam);
		if(IsValidTarget(u)) {
			Command c;
			c.id=CMD_ATTACK;
			c.options=INTERNAL_ORDER;
			c.params.push_back(u->id);
			c.timeOut=gs->frameNum+140;
			commandQue.push_front(c);
			return;
		}
	}
	if (owner->usingScriptMoveType) {
		return;
	}
	lastIdleCheck = gs->frameNum;
	if (((owner->pos - lastUserGoal).SqLength2D() > 10000.0f) &&
	    !owner->haveTarget && !dynamic_cast<CTAAirMoveType*>(owner->moveType)) {
		//note that this is not internal order so that we dont keep generating
		//new orders if we cant get to that pos
		Command c;
		c.id=CMD_MOVE;
		c.options=0;
		c.params.push_back(lastUserGoal.x);
		c.params.push_back(lastUserGoal.y);
		c.params.push_back(lastUserGoal.z);
		commandQue.push_front(c);
		unimportantMove=true;
	} else {
		NonMoving();
	}
}

void CMobileCAI::StopSlowGuard(){
	if(slowGuard){
		slowGuard = false;
		if (owner->maxSpeed)
			owner->moveType->SetMaxSpeed(owner->maxSpeed);
	}
}

void CMobileCAI::StartSlowGuard(float speed){
	if(!slowGuard){
		slowGuard = true;
		//speed /= 30;
		if(owner->maxSpeed >= speed){
			if(!commandQue.empty()){
				Command currCommand = commandQue.front();
				if(commandQue.size() <= 1
						|| commandQue[1].id != CMD_SET_WANTED_MAX_SPEED
						|| commandQue[1].params[0] > speed){
					if (speed > 0)
						owner->moveType->SetMaxSpeed(speed);
				}
			}
		}
	}
}
