/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"

#include "MobileCAI.h"
#include "TransportCAI.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnits.h"
#include "Map/Ground.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/HoverAirMoveType.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Groups/Group.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/Util.h"
#include <assert.h>

#define BUGGER_OFF_TTL 200
#define MAX_CLOSE_IN_RETRY_TICKS 30


CR_BIND_DERIVED(CMobileCAI ,CCommandAI , );

CR_REG_METADATA(CMobileCAI, (
				CR_MEMBER(goalPos),
				CR_MEMBER(lastBuggerGoalPos),
				CR_MEMBER(lastUserGoal),

				CR_MEMBER(lastIdleCheck),
				CR_MEMBER(tempOrder),

				CR_MEMBER(lastPC),

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

CMobileCAI::CMobileCAI():
	CCommandAI(),
	goalPos(-1,-1,-1),
	lastBuggerGoalPos(-1,0,-1),
	lastUserGoal(0,0,0),
	lastIdleCheck(0),
	tempOrder(false),
	lastPC(-1),
	lastBuggerOffTime(-BUGGER_OFF_TTL),
	buggerOffPos(0,0,0),
	buggerOffRadius(0),
	commandPos1(ZeroVector),
	commandPos2(ZeroVector),
	cancelDistance(1024),
	lastCloseInTry(-1),
	slowGuard(false),
	moveDir(gs->randFloat() > 0.5)
{}


CMobileCAI::CMobileCAI(CUnit* owner):
	CCommandAI(owner),
	goalPos(-1,-1,-1),
	lastBuggerGoalPos(-1,0,-1),
	lastUserGoal(owner->pos),
	lastIdleCheck(0),
	tempOrder(false),
	lastPC(-1),
	lastBuggerOffTime(-BUGGER_OFF_TTL),
	buggerOffPos(0,0,0),
	buggerOffRadius(0),
	commandPos1(ZeroVector),
	commandPos2(ZeroVector),
	cancelDistance(1024),
	lastCloseInTry(-1),
	slowGuard(false),
	moveDir(gs->randFloat() > 0.5)
{
	CalculateCancelDistance();

	CommandDescription c;

	c.id=CMD_LOAD_ONTO;
	c.action="loadonto";
	c.type=CMDTYPE_ICON_UNIT;
	c.name="Load units";
	c.mouseicon=c.name;
	c.tooltip="Sets the unit to load itself onto a transport";
	c.hidden = true;
	possibleCommands.push_back(c);
	c.hidden = false;

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

/** helper function for CMobileCAI::GiveCommandReal */
template <typename T>
static T* GetAirMoveType(CUnit *owner)
{
	T* airMT;
	if (owner->usingScriptMoveType) {
		airMT = dynamic_cast<T*>(owner->prevMoveType);
	} else {
		airMT = dynamic_cast<T*>(owner->moveType);
	}

	return airMT;
}

void CMobileCAI::GiveCommandReal(const Command &c, bool fromSynced)
{
	if (!AllowedCommand(c, fromSynced))
		return;

	if (owner->unitDef->canfly && c.GetID() == CMD_AUTOREPAIRLEVEL) {
		if (c.params.empty()) {
			return;
		}

		AAirMoveType* airMT = GetAirMoveType<AAirMoveType>(owner);
		if (!airMT)
			return;

		switch ((int) c.params[0]) {
			case 0: { airMT->SetRepairBelowHealth(0.0f); break; }
			case 1: { airMT->SetRepairBelowHealth(0.3f); break; }
			case 2: { airMT->SetRepairBelowHealth(0.5f); break; }
			case 3: { airMT->SetRepairBelowHealth(0.8f); break; }
		}
		for (vector<CommandDescription>::iterator cdi = possibleCommands.begin();
				cdi != possibleCommands.end(); ++cdi) {
			if (cdi->id == CMD_AUTOREPAIRLEVEL) {
				char t[10];
				SNPRINTF(t, 10, "%d", (int) c.params[0]);
				cdi->params[0] = t;
				break;
			}
		}

		selectedUnits.PossibleCommandChange(owner);
		return;
	}

	if (owner->unitDef->canfly && c.GetID() == CMD_IDLEMODE) {
		if (c.params.empty())
			return;

		AAirMoveType* airMT = GetAirMoveType<AAirMoveType>(owner);

		if (airMT == NULL)
			return;

		// toggle between the "land" and "fly" idle-modes
		switch ((int) c.params[0]) {
			case 0: { airMT->autoLand = false; break; }
			case 1: { airMT->autoLand = true; break; }
		}

		if (!airMT->autoLand && !airMT->owner->beingBuilt)
			airMT->Takeoff();

		for (vector<CommandDescription>::iterator cdi = possibleCommands.begin();
				cdi != possibleCommands.end(); ++cdi) {
			if (cdi->id == CMD_IDLEMODE) {
				char t[10];
				SNPRINTF(t, 10, "%d", (int) c.params[0]);
				cdi->params[0] = t;
				break;
			}
		}
		selectedUnits.PossibleCommandChange(owner);
		return;
	}

	if (!(c.options & SHIFT_KEY) && nonQueingCommands.find(c.GetID()) == nonQueingCommands.end()) {
		tempOrder = false;
		StopSlowGuard();
	}

	CCommandAI::GiveAllowedCommand(c);
}


/// returns true if the unit has to land
bool CMobileCAI::RefuelIfNeeded()
{
	if (owner->moveType->GetReservedPad() != NULL) {
		// we already have a pad
		return false;
	}

	if (owner->currentFuel <= 0.0f) {
		// we're completely out of fuel
		owner->userAttackGround = false;
		owner->SetUserTarget(NULL);
		inCommand = false;

		CAirBaseHandler::LandingPad* lp =
			airBaseHandler->FindAirBase(owner, owner->unitDef->minAirBasePower);

		if (lp != NULL) {
			// found a pad
			owner->moveType->ReservePad(lp);
		} else {
			// no pads available, search for a landing
			// spot near any that are currently occupied
			const float3& landingPos = airBaseHandler->FindClosestAirBasePos(owner, owner->unitDef->minAirBasePower);

			if (landingPos != ZeroVector) {
				// NOTE: owner->userAttackGround is wrongly reset to
				// true in CUnit::AttackGround() via ExecuteAttack()
				// so don't call it
				SetGoal(landingPos, owner->pos);
			} else {
				StopMove();
			}
		}
		return true;
	} else if (owner->moveType->WantsRefuel() && true
		/* (commandQue.empty() || commandQue.front().id == CMD_PATROL || commandQue.front().id == CMD_FIGHT) */) {
		// current fuel level is below our bingo threshold
		// note: force the refuel attempt (irrespective of
		// what our currently active command is)

		CAirBaseHandler::LandingPad* lp =
			airBaseHandler->FindAirBase(owner, owner->unitDef->minAirBasePower);

		if (lp != NULL) {
			StopMove();
			owner->userAttackGround = false;
			owner->SetUserTarget(NULL);
			inCommand = false;
			owner->moveType->ReservePad(lp);
			return true;
		}
	}

	return false;
}

/// returns true if the unit has to land
bool CMobileCAI::LandRepairIfNeeded()
{
	if (owner->moveType->GetReservedPad() != NULL)
		return false;

	if (!owner->moveType->WantsRepair())
		return false;

	// we're damaged, just seek a pad for repairs
	CAirBaseHandler::LandingPad* lp =
		airBaseHandler->FindAirBase(owner, owner->unitDef->minAirBasePower);

	if (lp != NULL) {
		owner->moveType->ReservePad(lp);
		return true;
	}

	const float3& newGoal = airBaseHandler->FindClosestAirBasePos(owner, owner->unitDef->minAirBasePower);

	if (newGoal != ZeroVector) {
		SetGoal(newGoal, owner->pos);
		return true;
	}

	return false;
}

void CMobileCAI::SlowUpdate()
{
	if (gs->paused) // Commands issued may invoke SlowUpdate when paused
		return;
	bool wantToLand = false;
	if (dynamic_cast<AAirMoveType*>(owner->moveType)) {
		wantToLand = LandRepairIfNeeded();
		if (!wantToLand && owner->unitDef->maxFuel > 0) {
			wantToLand = RefuelIfNeeded();
		}
	}


	if (!commandQue.empty() && commandQue.front().timeOut < gs->frameNum) {
		StopMove();
		FinishCommand();
		return;
	}

	if (commandQue.empty()) {
		IdleCheck();

		//the attack order could terminate directly and thus cause a loop
		if (commandQue.empty() || commandQue.front().GetID() == CMD_ATTACK) {
			return;
		}
	}

	if (!slowGuard) {
		SlowUpdateMaxSpeed();
	}

	Execute();
}

/**
* @brief Executes the first command in the commandQue
*/
void CMobileCAI::Execute()
{
	Command& c = commandQue.front();
	switch (c.GetID()) {
		case CMD_SET_WANTED_MAX_SPEED: { ExecuteSetWantedMaxSpeed(c);	return; }
		case CMD_MOVE:                 { ExecuteMove(c);				return; }
		case CMD_PATROL:               { ExecutePatrol(c);				return; }
		case CMD_FIGHT:                { ExecuteFight(c);				return; }
		case CMD_GUARD:                { ExecuteGuard(c);				return; }
		case CMD_LOAD_ONTO:            { ExecuteLoadUnits(c);			return; }
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
			(commandQue.back().GetID() != CMD_SET_WANTED_MAX_SPEED)) {
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
	const float3 cmdPos(c.params[0], c.params[1], c.params[2]);

	if (cmdPos != goalPos) {
		SetGoal(cmdPos, owner->pos);
	}
	if ((owner->pos - goalPos).SqLength2D() < cancelDistance ||
			owner->moveType->progressState == AMoveType::Failed) {
		FinishCommand();
	}
	return;
}

void CMobileCAI::ExecuteLoadUnits(Command &c) {
	CUnit* unit = uh->GetUnit(c.params[0]);
	CTransportUnit* tran = dynamic_cast<CTransportUnit*>(unit);

	if (!tran) {
		FinishCommand();
		return;
	}

	if (!inCommand) {
		inCommand = true;
		Command newCommand(CMD_LOAD_UNITS, INTERNAL_ORDER | SHIFT_KEY);
		newCommand.params.push_back(owner->id);
		tran->commandAI->GiveCommandReal(newCommand);
	}
	if (owner->transporter) {
		if (!commandQue.empty())
			FinishCommand();
		return;
	}

	if (!unit) {
		return;
	}

	float3 pos = unit->pos;
	if ((pos - goalPos).SqLength2D() > cancelDistance) {
		SetGoal(pos, owner->pos);
	}
	if ((owner->pos - goalPos).SqLength2D() < cancelDistance) {
		StopMove();
	}

	return;
}

/**
* @brief Executes the Patrol command c
*/
void CMobileCAI::ExecutePatrol(Command &c)
{
	assert(owner->unitDef->canPatrol);
	if (c.params.size() < 3) {
		LOG_L(L_ERROR,
				"Received a Patrol command with less than 3 params on %s in MobileCAI",
				owner->unitDef->humanName.c_str());
		return;
	}
	Command temp(CMD_FIGHT, c.options | INTERNAL_ORDER);
	temp.params.push_back(c.params[0]);
	temp.params.push_back(c.params[1]);
	temp.params.push_back(c.params[2]);

	commandQue.push_back(c);
	commandQue.pop_front();
	commandQue.push_front(temp);

	Command tmpC(CMD_PATROL);
	eoh->CommandFinished(*owner, tmpC);
	ExecuteFight(temp);
}

/**
* @brief Executes the Fight command c
*/
void CMobileCAI::ExecuteFight(Command &c)
{
	assert((c.options & INTERNAL_ORDER) || owner->unitDef->canFight);
	if(c.params.size() == 1) {
		if(orderTarget && !owner->weapons.empty()
				&& !owner->weapons.front()->AttackUnit(orderTarget, false)) {
			CUnit* newTarget = helper->GetClosestValidTarget(
				owner->pos, owner->maxRange, owner->allyteam, this);
			if ((newTarget != NULL) && owner->weapons.front()->AttackUnit(newTarget, false)) {
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
	if (c.params.size() < 3) {
		LOG_L(L_ERROR,
				"Received a Fight command with less than 3 params on %s in MobileCAI",
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

	if (owner->unitDef->canAttack && owner->fireState >= FIRESTATE_FIREATWILL && !owner->weapons.empty()) {
		const float3 curPosOnLine = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
		const float searchRadius = owner->maxRange + 100 * owner->moveState * owner->moveState;
		CUnit* enemy = helper->GetClosestValidTarget(curPosOnLine, searchRadius, owner->allyteam, this);
		if (enemy != NULL) {
			Command c2(CMD_FIGHT, c.options|INTERNAL_ORDER);
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
		&& !enemy->neutral
		// on "Hold pos", a target can not be valid if there exists no line of fire to it.
		&& (owner->moveState > MOVESTATE_HOLDPOS || owner->weapons.empty() ||
				owner->weapons.front()->TryTargetRotate(const_cast<CUnit*>(enemy), false));
}

/**
* @brief Executes the guard command c
*/
void CMobileCAI::ExecuteGuard(Command &c)
{
	assert(owner->unitDef->canGuard);
	assert(!c.params.empty());

	const CUnit* guardee = uh->GetUnit(c.params[0]);

	if (guardee == NULL) { FinishCommand(); return; }
	if (UpdateTargetLostTimer(guardee->id) == 0) { FinishCommand(); return; }
	if (guardee->outOfMapTime > (GAME_SPEED * 5)) { FinishCommand(); return; }

	const bool pushAttackCommand =
		owner->unitDef->canAttack &&
		(guardee->lastAttack + 40 < gs->frameNum) &&
		IsValidTarget(guardee->lastAttacker);

	if (pushAttackCommand) {
		Command nc(CMD_ATTACK, c.options);
		nc.params.push_back(guardee->lastAttacker->id);
		commandQue.push_front(nc);

		StopSlowGuard();
		SlowUpdate();
	} else {
		const float3 dif = (guardee->pos - owner->pos).SafeNormalize();
		const float3 goal = guardee->pos - dif * (guardee->radius + owner->radius + 64.0f);
		const bool resetGoal =
			((goalPos - goal).SqLength2D() > 1600.0f) ||
			(goalPos - owner->pos).SqLength2D() < Square(owner->moveType->GetMaxSpeed() * GAME_SPEED + 1 + SQUARE_SIZE * 2);

		if (resetGoal) {
			SetGoal(goal, owner->pos);
		}

		if ((goal - owner->pos).SqLength2D() < 6400.0f) {
			StartSlowGuard(guardee->moveType->GetMaxSpeed());

			if ((goal - owner->pos).SqLength2D() < 1800.0f) {
				StopMove();
				NonMoving();
			}
		} else {
			StopSlowGuard();
		}
	}
}


void CMobileCAI::ExecuteStop(Command &c)
{
	StopMove();
	return CCommandAI::ExecuteStop(c);
}



void CMobileCAI::ExecuteAttack(Command &c)
{
	assert(owner->unitDef->canAttack);

	// limit how far away we fly based on our movestate
	if (tempOrder && orderTarget) {
		const float3& closestPos = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
		const float curTargetDist = LinePointDist(closestPos, commandPos2, orderTarget->pos);
		const float maxTargetDist = (500 * owner->moveState + owner->maxRange);

		if (owner->moveState < MOVESTATE_ROAM && curTargetDist > maxTargetDist) {
			StopMove();
			FinishCommand();
			return;
		}
	}

	// check if we are in direct command of attacker
	if (!inCommand) {
		// don't start counting until the owner->AttackGround() order is given
		owner->commandShotCount = -1;

		if (c.params.size() == 1) {
			CUnit* targetUnit = uh->GetUnit(c.params[0]);

			// check if we have valid target parameter and that we aren't attacking ourselves
			if (targetUnit == NULL) { StopMove(); FinishCommand(); return; }
			if (targetUnit == owner) { StopMove(); FinishCommand(); return; }
			if (targetUnit->GetTransporter() != NULL) { StopMove(); FinishCommand(); return; }

			const float3 fix = targetUnit->pos + owner->posErrorVector * 128;
			const float3 diff = (fix - owner->pos).Normalize();

			SetGoal(fix - diff * targetUnit->radius, owner->pos);
			SetOrderTarget(targetUnit);

			inCommand = true;
		}
		else if (c.params.size() >= 3) {
			// user gave force-fire attack command
			float3 pos(c.params[0], c.params[1], c.params[2]);
			SetGoal(pos, owner->pos);
			inCommand = true;
		}
	}
	else if ((c.params.size() == 3) && (owner->commandShotCount > 0) && (commandQue.size() > 1)) {
		// the trailing CMD_SET_WANTED_MAX_SPEED in a command pair does not count
		if ((commandQue.size() > 2) || (commandQue.back().GetID() != CMD_SET_WANTED_MAX_SPEED)) {
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
		bool b2 = false;
		bool b3 = false;
		bool b4 = false;
		float edgeFactor = 0.0f; // percent offset to target center
		const float3 diff = owner->pos - orderTarget->midPos;

		if (!owner->weapons.empty()) {
			if (!(c.options & ALT_KEY) && SkipParalyzeTarget(orderTarget)) {
				StopMove();
				FinishCommand();
				return;
			}

			CWeapon* w = owner->weapons.front();

			if (c.GetID() == CMD_MANUALFIRE) {
				assert(owner->unitDef->canManualFire);
				for (vector<CWeapon*>::iterator it = owner->weapons.begin(); it != owner->weapons.end(); ++it) {
					if ((*it)->weaponDef->manualfire) {
						w = *it; break;
					}
				}
			}

			// if we have at least one weapon then check if we
			// can hit target with our first (meanest) one
			b2 = w->TryTargetRotate(orderTarget, c.GetID() == CMD_MANUALFIRE);
			b3 = Square(w->range - (w->relWeaponPos).Length()) > (orderTarget->pos.SqDistance(owner->pos));
			b4 = w->TryTargetHeading(GetHeadingFromVector(-diff.x, -diff.z), orderTarget->pos, orderTarget != NULL, orderTarget);
			edgeFactor = fabs(w->targetBorder);
		}

		const float diffLength2d = diff.Length2D();

		// if w->AttackUnit() returned true then we are already
		// in range with our biggest weapon so stop moving
		// also make sure that we're not locked in close-in/in-range state loop
		// due to rotates invoked by in-range or out-of-range states
		if (b2) {
			if (!(tempOrder && owner->moveState == MOVESTATE_HOLDPOS)
				&& (diffLength2d * 1.4f > owner->maxRange
					- orderTarget->speed.SqLength()
							/ owner->unitDef->maxAcc)
				&& b4 && diff.dot(orderTarget->speed) < 0)
			{
				SetGoal(owner->pos + (orderTarget->speed * 80), owner->pos,
						SQUARE_SIZE, orderTarget->speed.Length() * 1.1f);
			} else {
				StopMove();

				if (gs->frameNum > lastCloseInTry + MAX_CLOSE_IN_RETRY_TICKS) {
					owner->moveType->KeepPointingTo(orderTarget->midPos,
							std::min((float) owner->losRadius * loshandler->losDiv,
								owner->maxRange * 0.9f), true);
				}
			}
			owner->AttackUnit(orderTarget, c.GetID() == CMD_MANUALFIRE);
		}

		// if we're on hold pos in a temporary order, then none of the close-in
		// code below should run, and the attack command is cancelled.
		else if (tempOrder && owner->moveState == MOVESTATE_HOLDPOS) {
			StopMove();
			FinishCommand();
			return;
		}

		// if ((our movetype has type HoverAirMoveType and length of 2D vector from us to target
		// less than 90% of our maximum range) OR squared length of 2D vector from us to target
		// less than 1024) then we are close enough
		else if(diffLength2d < (owner->maxRange * 0.9f)){
			if (dynamic_cast<CHoverAirMoveType*>(owner->moveType)
					|| (diff.SqLength2D() < 1024))
			{
				StopMove();
				owner->moveType->KeepPointingTo(orderTarget->midPos,
						std::min((float) owner->losRadius * loshandler->losDiv,
							owner->maxRange * 0.9f), true);
			}

			// if (((first weapon range minus first weapon length greater than distance to target)
			// and length of 2D vector from us to target less than 90% of our maximum range)
			// then we are close enough, but need to move sideways to get a shot.
			//assumption is flawed: The unit may be aiming or otherwise unable to shoot
			else if (owner->unitDef->strafeToAttack && b3 && diffLength2d < (owner->maxRange * 0.9f))
			{
				moveDir ^= (owner->moveType->progressState == AMoveType::Failed);
				const float sin = moveDir ? 3.0/5 : -3.0/5;
				const float cos = 4.0/5;
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
		else if ((orderTarget->pos + owner->posErrorVector * 128).SqDistance2D(goalPos)
				> Square(10 + orderTarget->pos.distance2D(owner->pos) * 0.2f)) {
			// if the target isn't in LOS, go to its approximate position
			// otherwise try to go precisely to the target
			// this should fix issues with low range weapons (mainly melee)
			float3 fix = orderTarget->pos +
					(orderTarget->losStatus[owner->allyteam] & LOS_INLOS ?
						float3(0.f,0.f,0.f) :
						owner->posErrorVector * 128);
			const float3 norm = float3(fix - owner->pos).Normalize();
			const float3 goal = fix - norm*(orderTarget->radius*edgeFactor*0.8f);
			SetGoal(goal, owner->pos);
			if (lastCloseInTry < gs->frameNum + MAX_CLOSE_IN_RETRY_TICKS)
				lastCloseInTry = gs->frameNum;
		}
	}

	// user is attacking ground
	else if (c.params.size() >= 3) {
		const float3 pos(c.params[0], c.params[1], c.params[2]);
		const float3 diff = owner->pos - pos;

		if (!owner->weapons.empty()) {
			// if we have at least one weapon then check if
			// we can hit position with our first (assumed
			// to be meanest) one
			CWeapon* w = owner->weapons.front();

			// XXX HACK - special weapon overrides any checks
			if (c.GetID() == CMD_MANUALFIRE) {
				assert(owner->unitDef->canManualFire);
				float rr = owner->maxRange * owner->maxRange;

				for (vector<CWeapon*>::iterator it = owner->weapons.begin(); it != owner->weapons.end(); ++it) {
					if ((*it)->weaponDef->manualfire) {
						rr = (*it)->range * (*it)->range; break;
					}
				}

				if (diff.SqLength() < rr) {
					StopMove();
					owner->AttackGround(pos, c.GetID() == CMD_MANUALFIRE);
					owner->moveType->KeepPointingTo(pos, owner->maxRange * 0.9f, true);
				}
			} else {
				const bool inAngle = w->TryTargetRotate(pos, c.GetID() == CMD_MANUALFIRE);
				const bool inRange = diff.SqLength2D() < Square(w->range - (w->relWeaponPos).Length2D());

				if (inAngle || inRange) {
					StopMove();
					owner->AttackGround(pos, c.GetID() == CMD_MANUALFIRE);
					owner->moveType->KeepPointingTo(pos, owner->maxRange * 0.9f, true);
				}
			}
		}

		else if (diff.SqLength2D() < 1024) {
			StopMove();
			owner->moveType->KeepPointingTo(pos, owner->maxRange * 0.9f, true);
		}

		// if we are more than 10 units distant from target position then keeping moving closer
		else if (pos.SqDistance2D(goalPos) > 100) {
			SetGoal(pos, owner->pos);
		}
	}
}




int CMobileCAI::GetDefaultCmd(const CUnit* pointed, const CFeature* feature)
{
	if (pointed) {
		if (!teamHandler->Ally(gu->myAllyTeam,pointed->allyteam)) {
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



void CMobileCAI::BuggerOff(const float3& pos, float radius)
{
	if (radius < 0.0f) {
		lastBuggerOffTime = gs->frameNum - BUGGER_OFF_TTL;
		return;
	}
	lastBuggerOffTime = gs->frameNum;
	buggerOffPos = pos;
	buggerOffRadius = radius + owner->radius;
}

void CMobileCAI::NonMoving()
{
	if (owner->usingScriptMoveType) {
		return;
	}
	if (lastBuggerOffTime > gs->frameNum - BUGGER_OFF_TTL) {
		float3 dif = owner->pos-buggerOffPos;
		dif.y = 0.0f;
		float length=dif.Length();
		if (!length) {
			length = 0.1f;
			dif = float3(0.1f, 0.0f, 0.0f);
		}
		if (length < buggerOffRadius) {
			float3 goalPos = buggerOffPos + dif * ((buggerOffRadius + 128) / length);
			bool randomize = (goalPos.x == lastBuggerGoalPos.x) && (goalPos.z == lastBuggerGoalPos.z);
			lastBuggerGoalPos.x = goalPos.x;
			lastBuggerGoalPos.z = goalPos.z;
			if (randomize) {
				lastBuggerGoalPos.y += 32.0f; // gradually increase the amplitude of the random factor
				goalPos.x += (2.0f * lastBuggerGoalPos.y) * gs->randFloat() - lastBuggerGoalPos.y;
				goalPos.z += (2.0f * lastBuggerGoalPos.y) * gs->randFloat() - lastBuggerGoalPos.y;
			}
			else
				lastBuggerGoalPos.y = 0.0f;

			Command c(CMD_MOVE);
			//c.options = INTERNAL_ORDER;
			c.params.push_back(goalPos.x);
			c.params.push_back(goalPos.y);
			c.params.push_back(goalPos.z);
			c.timeOut = gs->frameNum + 40;
			commandQue.push_front(c);
			unimportantMove = true;
		}
	}
}

void CMobileCAI::FinishCommand()
{
	if (!(commandQue.front().options & INTERNAL_ORDER)) {
		lastUserGoal = owner->pos;
	}
	StopSlowGuard();
	CCommandAI::FinishCommand();
}

void CMobileCAI::IdleCheck()
{
	if (owner->unitDef->canAttack && owner->fireState > FIRESTATE_HOLDFIRE
			&& !owner->weapons.empty() && owner->haveTarget) {
		if(!owner->userTarget) {
			owner->haveTarget = false;
		} else if(owner->pos.SqDistance2D(owner->userTarget->pos) <
				Square(owner->maxRange + 200*owner->moveState*owner->moveState)) {
			Command c(CMD_ATTACK);
			c.options=INTERNAL_ORDER;
			c.params.push_back(owner->userTarget->id);
			c.timeOut = gs->frameNum + 140;
			commandQue.push_front(c);
			tempOrder = true;
			commandPos1 = owner->pos;
			commandPos2 = owner->pos;
			return;
		}
	}
	if (owner->unitDef->canAttack && owner->fireState > FIRESTATE_HOLDFIRE
				&& !owner->weapons.empty() && !owner->haveTarget) {
		if(owner->lastAttacker && owner->lastAttack + 200 > gs->frameNum
				&& !(owner->unitDef->noChaseCategory & owner->lastAttacker->category)){
			float3 apos=owner->lastAttacker->pos;
			float dist=apos.SqDistance2D(owner->pos);
			if(dist<Square(owner->maxRange+200*owner->moveState*owner->moveState)){
				Command c(CMD_ATTACK);
				c.options=INTERNAL_ORDER;
				c.params.push_back(owner->lastAttacker->id);
				c.timeOut=gs->frameNum+140;
				commandQue.push_front(c);
				tempOrder = true;
				commandPos1 = owner->pos;
				commandPos2 = owner->pos;
				return;
			}
		}
	}
	if (owner->unitDef->canAttack && (gs->frameNum >= lastIdleCheck + 10)
			&& owner->fireState >= FIRESTATE_FIREATWILL && !owner->weapons.empty() && !owner->haveTarget)
	{
		const float searchRadius = owner->maxRange + 150 * owner->moveState * owner->moveState;
		CUnit* enemy = helper->GetClosestValidTarget(owner->pos, searchRadius, owner->allyteam, this);
		if (enemy != NULL) {
			Command c(CMD_ATTACK);
			c.options=INTERNAL_ORDER;
			c.params.push_back(enemy->id);
			c.timeOut=gs->frameNum+140;
			commandQue.push_front(c);
			tempOrder = true;
			commandPos1 = owner->pos;
			commandPos2 = owner->pos;
			return;
		}
	}
	if (owner->usingScriptMoveType) {
		return;
	}
	lastIdleCheck = gs->frameNum;
	if (((owner->pos - lastUserGoal).SqLength2D() > 10000.0f) &&
	    !owner->haveTarget && !dynamic_cast<CHoverAirMoveType*>(owner->moveType)) {
		//note that this is not internal order so that we dont keep generating
		//new orders if we cant get to that pos
		Command c(CMD_MOVE);
		c.params.push_back(lastUserGoal.x);
		c.params.push_back(lastUserGoal.y);
		c.params.push_back(lastUserGoal.z);
		commandQue.push_front(c);
		unimportantMove=true;
	} else {
		NonMoving();
	}
}

void CMobileCAI::StopSlowGuard() {
	if (!slowGuard)
		return;

	slowGuard = false;

	// restore our default maximum speed
	owner->moveType->SetMaxSpeed(owner->moveType->GetMaxSpeedDef());
}

void CMobileCAI::StartSlowGuard(float speed) {
	if (slowGuard)
		return;

	slowGuard = true;

	if (speed <= 0.0f) { return; }
	if (commandQue.empty()) { return; }
	if (owner->moveType->GetMaxSpeed() < speed) { return; }

	const Command& c = (commandQue.size() > 1)? commandQue[1]: Command(CMD_STOP);

	// when guarding, temporarily adopt the maximum
	// (forward) speed of the guardee unit as our own
	if ((c.GetID() != CMD_SET_WANTED_MAX_SPEED) || (c.params[0] > speed)) {
		owner->moveType->SetMaxSpeed(speed);
	}
}


void CMobileCAI::CalculateCancelDistance()
{
	// calculate a rough turn radius
	const float turnFrames = SPRING_CIRCLE_DIVS / std::max(owner->unitDef->turnRate, 1.0f);
	const float turnRadius = ((owner->unitDef->speed / GAME_SPEED) * turnFrames) / (PI + PI);
	const float tmp = turnRadius + (SQUARE_SIZE << 1);

	// clamp it a bit because the units don't have to turn at max speed
	cancelDistance = std::min(std::max(tmp * tmp, 1024.f), 2048.f);
}
