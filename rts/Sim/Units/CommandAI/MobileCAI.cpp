/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "MobileCAI.h"
#include "TransportCAI.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Map/Ground.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/AAirMoveType.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/Util.h"
#include <assert.h>

#define AUTO_GENERATE_ATTACK_ORDERS 1
#define BUGGER_OFF_TTL 200
#define MAX_CLOSE_IN_RETRY_TICKS 30
#define MAX_USERGOAL_TOLERANCE_DIST 100.0f

// MobileCAI is not always assigned to aircraft
static AAirMoveType* GetAirMoveType(const CUnit* owner) {
	assert(owner->unitDef->IsAirUnit());

	if (owner->UsingScriptMoveType()) {
		return static_cast<AAirMoveType*>(owner->prevMoveType);
	}

	return static_cast<AAirMoveType*>(owner->moveType);
}



CR_BIND_DERIVED(CMobileCAI ,CCommandAI , )
CR_REG_METADATA(CMobileCAI, (
	CR_MEMBER(goalPos),
	CR_MEMBER(goalRadius),
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
))

CMobileCAI::CMobileCAI():
	CCommandAI(),
	goalPos(-1,-1,-1),
	goalRadius(0.0f),
	lastBuggerGoalPos(-1,0,-1),
	lastUserGoal(ZeroVector),
	lastIdleCheck(0),
	tempOrder(false),
	lastPC(-1),
	lastBuggerOffTime(-BUGGER_OFF_TTL),
	buggerOffPos(ZeroVector),
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
	goalRadius(0.0f),
	lastBuggerGoalPos(-1,0,-1),
	lastUserGoal(owner->pos),
	lastIdleCheck(0),
	tempOrder(false),
	lastPC(-1),
	lastBuggerOffTime(-BUGGER_OFF_TTL),
	buggerOffPos(ZeroVector),
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

	if (owner->unitDef->canPatrol) {
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

	if (owner->unitDef->canGuard) {
		c.id=CMD_GUARD;
		c.action="guard";
		c.type=CMDTYPE_ICON_UNIT;
		c.name="Guard";
		c.mouseicon=c.name;
		c.tooltip="Guard: Order a unit to guard another unit and attack units attacking it";
		possibleCommands.push_back(c);
	}

	if (owner->unitDef->canfly) {
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



void CMobileCAI::GiveCommandReal(const Command& c, bool fromSynced)
{
	if (!AllowedCommand(c, fromSynced))
		return;

	if (owner->unitDef->IsAirUnit()) {
		AAirMoveType* airMT = GetAirMoveType(owner);

		if (c.GetID() == CMD_AUTOREPAIRLEVEL) {
			if (c.params.empty())
				return;

			switch ((int) c.params[0]) {
				case 0: { airMT->SetRepairBelowHealth(0.0f); break; }
				case 1: { airMT->SetRepairBelowHealth(0.3f); break; }
				case 2: { airMT->SetRepairBelowHealth(0.5f); break; }
				case 3: { airMT->SetRepairBelowHealth(0.8f); break; }
			}

			for (unsigned int n = 0; n < possibleCommands.size(); n++) {
				if (possibleCommands[n].id != CMD_AUTOREPAIRLEVEL)
					continue;

				possibleCommands[n].params[0] = IntToString(int(c.params[0]), "%d");
				break;
			}

			selectedUnitsHandler.PossibleCommandChange(owner);
			return;
		}

		if (c.GetID() == CMD_IDLEMODE) {
			if (c.params.empty())
				return;

			// toggle between the "land" and "fly" idle-modes
			switch ((int) c.params[0]) {
				case 0: { airMT->autoLand = false; break; }
				case 1: { airMT->autoLand = true; break; }
			}

			if (!airMT->owner->beingBuilt) {
				if (!airMT->autoLand)
					airMT->Takeoff();
				else
					airMT->Land();
			}

			for (unsigned int n = 0; n < possibleCommands.size(); n++) {
				if (possibleCommands[n].id != CMD_IDLEMODE)
					continue;

				possibleCommands[n].params[0] = IntToString(int(c.params[0]), "%d");
				break;
			}

			selectedUnitsHandler.PossibleCommandChange(owner);
			return;
		}
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
	if (owner->unitDef->maxFuel <= 0.0f)
		return false;

	if (owner->moveType->GetReservedPad() != NULL) {
		// we already have a pad
		return false;
	}

	if (owner->currentFuel <= 0.0f) {
		// we're completely out of fuel
		owner->AttackUnit(NULL, false, false);
		inCommand = false;

		CAirBaseHandler::LandingPad* lp =
			airBaseHandler->FindAirBase(owner, owner->unitDef->minAirBasePower, true);

		if (lp != NULL) {
			// found a pad
			owner->moveType->ReservePad(lp);
		} else {
			// no pads available, search for a landing
			// spot near any that are currently occupied
			const float3& landingPos = airBaseHandler->FindClosestAirBasePos(owner, owner->unitDef->minAirBasePower);

			if (landingPos != ZeroVector) {
				SetGoal(landingPos, owner->pos);
			} else {
				StopMove();
			}
		}

		return true;
	} else if (owner->moveType->WantsRefuel()) {
		// current fuel level is below our bingo threshold
		// note: force the refuel attempt (irrespective of
		// what our currently active command is)

		CAirBaseHandler::LandingPad* lp =
			airBaseHandler->FindAirBase(owner, owner->unitDef->minAirBasePower, true);

		if (lp != NULL) {
			StopMove();
			owner->AttackUnit(NULL, false, false);
			owner->moveType->ReservePad(lp);
			inCommand = false;
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
		airBaseHandler->FindAirBase(owner, owner->unitDef->minAirBasePower, true);

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

	if (!owner->UsingScriptMoveType() && owner->unitDef->IsAirUnit()) {
		LandRepairIfNeeded() || RefuelIfNeeded();
	}


	if (!commandQue.empty() && commandQue.front().timeOut < gs->frameNum) {
		StopMove();
		FinishCommand();
		return;
	}

	if (commandQue.empty()) {
		MobileAutoGenerateTarget();

		// the attack order could terminate directly and thus cause a loop
		if (commandQue.empty() || (commandQue.front()).GetID() == CMD_ATTACK) {
			return;
		}
	}

	if (!slowGuard) {
		// when slow-guarding, regulate speed through {Start,Stop}SlowGuard
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
	const float3 cmdPos = c.GetPos(0);

	if (cmdPos != goalPos) {
		SetGoal(cmdPos, owner->pos);
	}

	if ((owner->pos - goalPos).SqLength2D() < cancelDistance || owner->moveType->progressState == AMoveType::Failed) {
		FinishCommand();
	}
	return;
}

void CMobileCAI::ExecuteLoadUnits(Command &c) {
	CUnit* unit = unitHandler->GetUnit(c.params[0]);
	CTransportUnit* tran = dynamic_cast<CTransportUnit*>(unit);

	if (!tran) {
		FinishCommand();
		return;
	}

	if (!inCommand) {
		inCommand = true;
		Command newCommand(CMD_LOAD_UNITS, INTERNAL_ORDER | SHIFT_KEY, owner->id);
		tran->commandAI->GiveCommandReal(newCommand);
	}
	if (owner->GetTransporter() != NULL) {
		if (!commandQue.empty())
			FinishCommand();
		return;
	}

	if (unit == NULL)
		return;

	if ((unit->pos - goalPos).SqLength2D() > cancelDistance) {
		SetGoal(unit->pos, owner->pos);
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
		LOG_L(L_ERROR, "[MCAI::%s][f=%d][id=%d] CMD_FIGHT #params < 3", __FUNCTION__, gs->frameNum, owner->id);
		return;
	}
	Command temp(CMD_FIGHT, c.options | INTERNAL_ORDER, c.GetPos(0));

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
void CMobileCAI::ExecuteFight(Command& c)
{
	assert((c.options & INTERNAL_ORDER) || owner->unitDef->canFight);

	if (c.params.size() == 1 && !owner->weapons.empty()) {
		CWeapon* w = owner->weapons.front();

		if ((orderTarget != NULL) && !w->Attack(SWeaponTarget(orderTarget, false))) {
			CUnit* newTarget = CGameHelper::GetClosestValidTarget(owner->pos, owner->maxRange, owner->allyteam, this);

			if ((newTarget != NULL) && w->Attack(SWeaponTarget(newTarget, false))) {
				c.params[0] = newTarget->id;

				inCommand = false;
			}
		}

		ExecuteAttack(c);
		return;
	}

	if (tempOrder) {
		inCommand = true;
		tempOrder = false;
	}
	if (c.params.size() < 3) {
		LOG_L(L_ERROR, "[MCAI::%s][f=%d][id=%d] CMD_FIGHT #params < 3", __FUNCTION__, gs->frameNum, owner->id);
		return;
	}
	if (c.params.size() >= 6) {
		if (!inCommand) {
			commandPos1 = c.GetPos(3);
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

	float3 pos = c.GetPos(0);

	if (!inCommand) {
		inCommand = true;
		commandPos2 = pos;
		lastUserGoal = commandPos2;
	}
	if (c.params.size() >= 6) {
		pos = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
	}
	if (pos != goalPos) {
		SetGoal(pos, owner->pos);
	}

	if (owner->unitDef->canAttack && owner->fireState >= FIRESTATE_FIREATWILL && !owner->weapons.empty()) {
		const float3 curPosOnLine = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
		const float searchRadius = owner->maxRange + 100 * owner->moveState * owner->moveState;
		CUnit* enemy = CGameHelper::GetClosestValidTarget(curPosOnLine, searchRadius, owner->allyteam, this);

		if (enemy != NULL) {
			PushOrUpdateReturnFight();

			// make the attack-command inherit <c>'s options
			// NOTE: see AirCAI::ExecuteFight why we do not set INTERNAL_ORDER
			Command c2(CMD_ATTACK, c.options, enemy->id);
			commandQue.push_front(c2);

			inCommand = false;
			tempOrder = true;

			// avoid infinite loops (?)
			if (lastPC != gs->frameNum) {
				lastPC = gs->frameNum;
				SlowUpdate();
			}
			return;
		}
	}

	if ((owner->pos - goalPos).SqLength2D() < (64 * 64)
			|| (owner->moveType->progressState == AMoveType::Failed)){
		FinishCommand();
	}
}

bool CMobileCAI::IsValidTarget(const CUnit* enemy) const {
	if (!enemy)
		return false;

	if (enemy == owner)
		return false;

	if (owner->unitDef->noChaseCategory & enemy->category)
		return false;

	// don't _auto_ chase neutrals
	if (enemy->IsNeutral())
		return false;

	if (owner->weapons.empty())
		return false;

	// on "Hold pos", a target can not be valid if there exists no line of fire to it.
	// FIXME: even if not on HOLDPOS there are situations where having LOF is not enough
	if (owner->moveState == MOVESTATE_HOLDPOS && !owner->weapons.front()->TryTargetRotate(enemy, false))
		return false;

	// test if any weapon can target the enemy unit
	for (const CWeapon* w: owner->weapons) {
		if (w->TestTarget(enemy->pos, SWeaponTarget(enemy))) {
			return true;
		}
	}

	return false;
}

/**
* @brief Executes the guard command c
*/
void CMobileCAI::ExecuteGuard(Command &c)
{
	assert(owner->unitDef->canGuard);
	assert(!c.params.empty());

	const CUnit* guardee = unitHandler->GetUnit(c.params[0]);

	if (guardee == NULL) { FinishCommand(); return; }
	if (UpdateTargetLostTimer(guardee->id) == 0) { FinishCommand(); return; }
	if (guardee->outOfMapTime > (GAME_SPEED * 5)) { FinishCommand(); return; }

	const bool pushAttackCommand =
		owner->unitDef->canAttack &&
		(guardee->lastAttackFrame + 40 < gs->frameNum) &&
		IsValidTarget(guardee->lastAttacker);

	if (pushAttackCommand) {
		Command nc(CMD_ATTACK, c.options, guardee->lastAttacker->id);
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
		const float maxTargetDist = (owner->moveType->GetManeuverLeash() * owner->moveState + owner->maxRange);

		if (owner->moveState < MOVESTATE_ROAM && curTargetDist > maxTargetDist) {
			StopMove();
			FinishCommand();
			return;
		}
	}

	if (!inCommand) {
		if (c.params.size() == 1) {
			CUnit* targetUnit = unitHandler->GetUnit(c.params[0]);

			// check if we have valid target parameter and that we aren't attacking ourselves
			if (targetUnit == NULL) { StopMove(); FinishCommand(); return; }
			if (targetUnit == owner) { StopMove(); FinishCommand(); return; }
			if (targetUnit->GetTransporter() != NULL && !modInfo.targetableTransportedUnits) {
				StopMove(); FinishCommand(); return;
			}

			const float3 tgtErrPos = targetUnit->pos + owner->posErrorVector * 128;
			const float3 tgtPosDir = (tgtErrPos - owner->pos).Normalize();

			// FIXME: don't call SetGoal() if target is already in range of some weapon?
			SetGoal(tgtErrPos - tgtPosDir * targetUnit->radius, owner->pos, owner->maxRange * 0.9f);
			SetOrderTarget(targetUnit);
			owner->AttackUnit(targetUnit, (c.options & INTERNAL_ORDER) == 0, c.GetID() == CMD_MANUALFIRE);

			inCommand = true;
		}
		else if (c.params.size() >= 3) {
			// user gave force-fire attack command
			SetGoal(c.GetPos(0), owner->pos, owner->maxRange * 0.9f);

			inCommand = true;
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
	if (orderTarget != NULL) {
		bool tryTargetRotate  = false;
		bool tryTargetHeading = false;

		float edgeFactor = 0.0f; // percent offset to target center
		const float3 targetMidPosVec = owner->midPos - orderTarget->midPos;

		const float targetGoalDist = (orderTarget->pos + owner->posErrorVector * 128.0f).SqDistance2D(goalPos);
		const float targetPosDist = Square(10.0f + orderTarget->pos.distance2D(owner->pos) * 0.2f);
		const float minPointingDist = std::min(1.0f * owner->losRadius * losHandler->losDiv, owner->maxRange * 0.9f);

		// FIXME? targetMidPosMaxDist is 3D, but compared with a 2D value
		const float targetMidPosDist2D = targetMidPosVec.Length2D();
		// const float targetMidPosMaxDist = owner->maxRange - (Square(orderTarget->speed.w) / owner->unitDef->maxAcc);

		if (!owner->weapons.empty()) {
			if (!(c.options & ALT_KEY) && SkipParalyzeTarget(orderTarget)) {
				StopMove();
				FinishCommand();
				return;
			}
		}

		const SWeaponTarget trg(orderTarget, (c.options & INTERNAL_ORDER) == 0);

		for (CWeapon* w: owner->weapons) {
			if (c.GetID() == CMD_MANUALFIRE) {
				assert(owner->unitDef->canManualFire);

				if (!w->weaponDef->manualfire) {
					continue;
				}
			}

			tryTargetRotate  = w->TryTargetRotate(orderTarget, (c.options & INTERNAL_ORDER) == 0);
			tryTargetHeading = w->TryTargetHeading(GetHeadingFromVector(-targetMidPosVec.x, -targetMidPosVec.z), trg);

			if (tryTargetRotate || tryTargetHeading)
				break;

			edgeFactor = math::fabs(w->weaponDef->targetBorder);
		}

		// if w->AttackUnit() returned true then we are already
		// in range with our biggest (?) weapon, so stop moving
		// also make sure that we're not locked in close-in/in-range state
		// loop due to rotates invoked by in-range or out-of-range states
		if (tryTargetRotate) {
			const bool canChaseTarget = (!tempOrder || owner->moveState != MOVESTATE_HOLDPOS);
			const bool targetBehind = (targetMidPosVec.dot(orderTarget->speed) < 0.0f);

			if (canChaseTarget && tryTargetHeading && targetBehind) {
				SetGoal(owner->pos + (orderTarget->speed * 80), owner->pos, owner->maxRange * 0.9f, orderTarget->speed.w * 1.1f);
			} else {
				StopMove();

				if (gs->frameNum > lastCloseInTry + MAX_CLOSE_IN_RETRY_TICKS) {
					owner->moveType->KeepPointingTo(orderTarget->midPos, minPointingDist, true);
				}
			}

			owner->AttackUnit(orderTarget, (c.options & INTERNAL_ORDER) == 0, c.GetID() == CMD_MANUALFIRE);
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
		else if (targetMidPosDist2D < (owner->maxRange * 0.9f)) {
			if (owner->unitDef->IsHoveringAirUnit() || (targetMidPosVec.SqLength2D() < 1024)) {
				StopMove();
				owner->moveType->KeepPointingTo(orderTarget->midPos, minPointingDist, true);
			}

			// if (((first weapon range minus first weapon length greater than distance to target)
			// and length of 2D vector from us to target less than 90% of our maximum range)
			// then we are close enough, but need to move sideways to get a shot.
			//assumption is flawed: The unit may be aiming or otherwise unable to shoot
			else if (owner->unitDef->strafeToAttack && targetMidPosDist2D < (owner->maxRange * 0.9f)) {
				moveDir ^= (owner->moveType->progressState == AMoveType::Failed);

				const float sin = moveDir ? 3.0/5 : -3.0/5;
				const float cos = 4.0 / 5;

				float3 goalDiff;
				goalDiff.x = targetMidPosVec.dot(float3(cos, 0, -sin));
				goalDiff.z = targetMidPosVec.dot(float3(sin, 0,  cos));
				goalDiff *= (targetMidPosDist2D < (owner->maxRange * 0.3f)) ? 1/cos : cos;
				goalDiff += orderTarget->pos;
				SetGoal(goalDiff, owner->pos, owner->maxRange * 0.9f);
			}
		}

		// if 2D distance of (target position plus attacker error vector times 128)
		// to goal position greater than
		// (10 plus 20% of 2D distance between attacker and target) then we need to close
		// in on target more
		else if (targetGoalDist > targetPosDist) {
			// if the target isn't in LOS, go to its approximate position
			// otherwise try to go precisely to the target
			// this should fix issues with low range weapons (mainly melee)
			const float3 errPos = ((orderTarget->losStatus[owner->allyteam] & LOS_INLOS)? ZeroVector: owner->posErrorVector * 128.0f);
			const float3 tgtPos = orderTarget->pos + errPos;

			const float3 norm = (tgtPos - owner->pos).Normalize();
			const float3 goal = tgtPos - norm * (orderTarget->radius * edgeFactor * 0.8f);

			SetGoal(goal, owner->pos, owner->maxRange * 0.9f);

			if (lastCloseInTry < gs->frameNum + MAX_CLOSE_IN_RETRY_TICKS)
				lastCloseInTry = gs->frameNum;
		}
	}

	// user wants to attack the ground
	else if (c.params.size() >= 3) {
		const float3 attackPos = c.GetPos(0);
		const float3 attackVec = attackPos - owner->pos;
		const short  attackHead = GetHeadingFromVector(attackVec.x, attackVec.z);
		const SWeaponTarget trg(attackPos, (c.options & INTERNAL_ORDER) == 0);

		if (c.GetID() == CMD_MANUALFIRE) {
			assert(owner->unitDef->canManualFire);

			if (owner->AttackGround(trg.groundPos, trg.isUserTarget, true)) {
				// StopMoveAndKeepPointing calls StopMove before KeepPointingTo
				// but we want to call it *after* KeepPointingTo to prevent 4131
				owner->moveType->KeepPointingTo(attackPos, owner->maxRange * 0.9f, true);
				StopMove();
				return;
			}
		} else {
			for (CWeapon* w: owner->weapons) {
				// NOTE:
				//   we call TryTargetHeading which is less restrictive than TryTarget
				//   (eg. the former succeeds even if the unit has not already aligned
				//   itself with <attackVec>)
				if (!w->TryTargetHeading(attackHead, trg))
					continue;

				if (owner->AttackGround(trg.groundPos, trg.isUserTarget, false)) {
					StopMoveAndKeepPointing(trg.groundPos, owner->maxRange * 0.9f, true);
					return;
				}

				// for gunships, this pitches the nose down such that
				// TryTargetRotate (which also checks range for itself)
				// has a bigger chance of succeeding
				//
				// hence it must be called as soon as we get in range
				// and may not depend on what TryTargetRotate returns
				// (otherwise we might never get a firing solution)
				owner->moveType->KeepPointingTo(trg.groundPos, owner->maxRange * 0.9f, true);
			}

			if (attackVec.SqLength2D() < Square(goalRadius)) {
				owner->AttackGround(trg.groundPos, trg.isUserTarget, false);
				StopMoveAndKeepPointing(attackPos, owner->maxRange * 0.9f, true);
			}
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
			const CTransportCAI* tran = dynamic_cast<CTransportCAI*>(pointed->commandAI);

			if (tran != NULL && tran->CanTransport(owner)) {
				return CMD_LOAD_ONTO;
			} else if (owner->unitDef->canGuard) {
				return CMD_GUARD;
			}
		}
	}
	return CMD_MOVE;
}

void CMobileCAI::SetGoal(const float3& pos, const float3& /*curPos*/, float goalRadius)
{
	// already have equal move order?
	if (owner->moveType->progressState == AMoveType::Active && goalPos == pos && this->goalRadius == goalRadius)
		return;

	// give new move order
	owner->moveType->StartMoving(goalPos = pos, this->goalRadius = goalRadius);
}

void CMobileCAI::SetGoal(const float3& pos, const float3& /*curPos*/, float goalRadius, float speed)
{
	// already have equal move order?
	if (owner->moveType->progressState == AMoveType::Active && goalPos == pos && this->goalRadius == goalRadius)
		return;

	// give new move order
	owner->moveType->StartMoving(goalPos = pos, this->goalRadius = goalRadius, speed);
}

bool CMobileCAI::SetFrontMoveCommandPos(const float3& pos)
{
	if (commandQue.empty())
		return false;
	if ((commandQue.front()).GetID() != CMD_MOVE)
		return false;

	(commandQue.front()).SetPos(0, pos);
	return true;
}

void CMobileCAI::StopMove()
{
	owner->moveType->StopMoving();
	goalPos = owner->pos;
	goalRadius = 0.f;
}

void CMobileCAI::StopMoveAndKeepPointing(const float3& p, const float r, bool b)
{
	StopMove();
	owner->moveType->KeepPointingTo(p, std::max(r, 10.f), b);
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
	if (owner->UsingScriptMoveType())
		return;

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

			Command c(CMD_MOVE, goalPos);
			//c.options = INTERNAL_ORDER;
			c.timeOut = gs->frameNum + 40;
			commandQue.push_front(c);
			unimportantMove = true;
		}
	}
}

void CMobileCAI::FinishCommand()
{
	if (!((commandQue.front()).options & INTERNAL_ORDER)) {
		lastUserGoal = owner->pos;
	}
	StopSlowGuard();
	CCommandAI::FinishCommand();
}

bool CMobileCAI::MobileAutoGenerateTarget()
{
	//FIXME merge with CWeapon::AutoTarget()
	assert(commandQue.empty());

	#if (AUTO_GENERATE_ATTACK_ORDERS == 1)
	if (GenerateAttackCmd())
		return (tempOrder = true);
	#endif

	if (owner->UsingScriptMoveType())
		return false;

	lastIdleCheck = gs->frameNum;

	if (owner->HaveTarget()) {
		NonMoving(); return false;
	}
	if ((owner->pos - lastUserGoal).SqLength2D() <= (MAX_USERGOAL_TOLERANCE_DIST * MAX_USERGOAL_TOLERANCE_DIST)) {
		NonMoving(); return false;
	}
	if (owner->unitDef->IsHoveringAirUnit()) {
		NonMoving(); return false;
	}

	unimportantMove = true;
	return false;
}


bool CMobileCAI::GenerateAttackCmd()
{
	if (!owner->unitDef->canAttack)
		return false;

	if (owner->weapons.empty())
		return false;

	if (owner->fireState == FIRESTATE_HOLDFIRE)
		return false;

	const float extraRange = 200.0f * owner->moveState * owner->moveState;
	const float maxRangeSq = Square(owner->maxRange + extraRange);
	int newAttackTargetId = -1;

	if (owner->curTarget.type == Target_Unit) {
		//FIXME when is this the case (unit has target, but CAI doesn't !?)
		if (owner->pos.SqDistance2D(owner->curTarget.unit->pos) < maxRangeSq) {
			newAttackTargetId = owner->curTarget.unit->id;
		}
	} else {
		if (owner->lastAttacker != nullptr) {
			const bool freshAttack = (gs->frameNum < (owner->lastAttackFrame + GAME_SPEED * 7));
			const bool canChaseAttacker = !(owner->unitDef->noChaseCategory & owner->lastAttacker->category);
			if (freshAttack && canChaseAttacker) {
				const float r = owner->pos.SqDistance2D(owner->lastAttacker->pos);

				if (r < maxRangeSq) {
					newAttackTargetId = owner->lastAttacker->id;
				}
			}
		}

		if (newAttackTargetId < 0 &&
		    owner->fireState >= FIRESTATE_FIREATWILL && (gs->frameNum >= lastIdleCheck + 10)
		) {
			const float searchRadius = owner->maxRange + 150.0f * owner->moveState * owner->moveState;
			const CUnit* enemy = CGameHelper::GetClosestValidTarget(owner->pos, searchRadius, owner->allyteam, this);

			if (enemy != NULL) {
				newAttackTargetId = enemy->id;
			}
		}
	}

	if (newAttackTargetId >= 0) {
		Command c(CMD_ATTACK, INTERNAL_ORDER, newAttackTargetId);
		c.timeOut = gs->frameNum + GAME_SPEED * 5;
		commandQue.push_front(c);

		commandPos1 = owner->pos;
		commandPos2 = owner->pos;
		return true;
	}

	return false;
}


void CMobileCAI::StopSlowGuard() {
	if (!slowGuard)
		return;

	slowGuard = false;

	// restore maxWantedSpeed to our current maxSpeed
	// (StartSlowGuard modifies maxWantedSpeed, so we
	// do not know its old value here)
	owner->moveType->SetWantedMaxSpeed(owner->moveType->GetMaxSpeed());
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
	// WANTED maximum
	if (c.GetID() == CMD_SET_WANTED_MAX_SPEED) {
		owner->moveType->SetWantedMaxSpeed(speed);
	}
}



void CMobileCAI::CalculateCancelDistance()
{
	const float tmp = owner->moveType->CalcStaticTurnRadius() + (SQUARE_SIZE << 1);

	// clamp it a bit because the units don't have to turn at max speed
	cancelDistance = Clamp(tmp * tmp, 1024.0f, 2048.0f);
}

