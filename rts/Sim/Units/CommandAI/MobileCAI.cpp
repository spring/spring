/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "MobileCAI.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Map/Ground.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/AAirMoveType.h"
#include "Sim/MoveTypes/HoverAirMoveType.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Scripts/UnitScript.h"
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

#define AIRTRANSPORT_DOCKING_RADIUS 16
#define AIRTRANSPORT_DOCKING_ANGLE 50
#define UNLOAD_LAND 0
#define UNLOAD_DROP 1
#define UNLOAD_LANDFLOOD 2

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
	CR_MEMBER(lastBuggerGoalPos),
	CR_MEMBER(lastUserGoal),

	CR_MEMBER(lastIdleCheck),
	CR_MEMBER(tempOrder),

	CR_MEMBER(lastPC),

	CR_MEMBER(lastBuggerOffTime),
	CR_MEMBER(buggerOffPos),
	CR_MEMBER(buggerOffRadius),

	CR_MEMBER(repairBelowHealth),

	CR_MEMBER(commandPos1),
	CR_MEMBER(commandPos2),

	CR_MEMBER(lastCloseInTry),

	CR_MEMBER(cancelDistance),
	CR_MEMBER(slowGuard),
	CR_MEMBER(moveDir)
))

CMobileCAI::CMobileCAI():
	CCommandAI(),
	lastBuggerGoalPos(-1,0,-1),
	lastUserGoal(ZeroVector),
	lastIdleCheck(0),
	tempOrder(false),
	lastPC(-1),
	lastBuggerOffTime(-BUGGER_OFF_TTL),
	buggerOffPos(ZeroVector),
	buggerOffRadius(0),
	repairBelowHealth(0.0f),
	commandPos1(ZeroVector),
	commandPos2(ZeroVector),
	cancelDistance(1024),
	lastCloseInTry(-1),
	slowGuard(false),
	moveDir(gs->randFloat() > 0.5)
{}


CMobileCAI::CMobileCAI(CUnit* owner):
	CCommandAI(owner),
	lastBuggerGoalPos(-1,0,-1),
	lastUserGoal(owner->pos),
	lastIdleCheck(0),
	tempOrder(false),
	lastPC(-1),
	lastBuggerOffTime(-BUGGER_OFF_TTL),
	buggerOffPos(ZeroVector),
	buggerOffRadius(0),
	repairBelowHealth(0.0f),
	commandPos1(ZeroVector),
	commandPos2(ZeroVector),
	cancelDistance(1024),
	lastCloseInTry(-1),
	slowGuard(false),
	moveDir(gs->randFloat() > 0.5f)
{
	CalculateCancelDistance();

	{
		SCommandDescription c;

		c.id   = CMD_LOAD_ONTO;
		c.type = CMDTYPE_ICON_UNIT;

		c.action    = "loadonto";
		c.name      = "Load units";
		c.tooltip   = c.name + ": Sets the unit to load itself onto a transport";
		c.mouseicon = c.name;

		c.hidden = true;
		possibleCommands.push_back(commandDescriptionCache->GetPtr(c));
	}

	if (owner->unitDef->canmove) {
		SCommandDescription c;

		c.id   = CMD_MOVE;
		c.type = CMDTYPE_ICON_FRONT;

		c.action    = "move";
		c.name      = "Move";
		c.tooltip   = c.name + ": Order the unit to move to a position";
		c.mouseicon = c.name;

		c.params.push_back("1000000"); // max distance
		possibleCommands.push_back(commandDescriptionCache->GetPtr(c));
	}

	if (owner->unitDef->canPatrol) {
		SCommandDescription c;

		c.id   = CMD_PATROL;
		c.type = CMDTYPE_ICON_MAP;

		c.action    = "patrol";
		c.name      = "Patrol";
		c.tooltip   = c.name + ": Order the unit to patrol to one or more waypoints";
		c.mouseicon = c.name;
		possibleCommands.push_back(commandDescriptionCache->GetPtr(c));
	}

	if (owner->unitDef->canFight) {
		SCommandDescription c;

		c.id   = CMD_FIGHT;
		c.type = CMDTYPE_ICON_FRONT;

		c.action    = "fight";
		c.name      = "Fight";
		c.tooltip   = c.name + ": Order the unit to take action while moving to a position";
		c.mouseicon = c.name;
		possibleCommands.push_back(commandDescriptionCache->GetPtr(c));
	}

	if (owner->unitDef->canGuard) {
		SCommandDescription c;

		c.id   = CMD_GUARD;
		c.type = CMDTYPE_ICON_UNIT;

		c.action    = "guard";
		c.name      = "Guard";
		c.tooltip   = c.name + ": Order a unit to guard another unit and attack units attacking it";
		c.mouseicon = c.name;
		possibleCommands.push_back(commandDescriptionCache->GetPtr(c));
	}

	if (owner->unitDef->canfly) {
		{
			SCommandDescription c;

			c.id   = CMD_AUTOREPAIRLEVEL;
			c.type = CMDTYPE_ICON_MODE;

			c.action    = "autorepairlevel";
			c.name      = "Repair level";
			c.tooltip   = c.name + ": Sets at which health level an aircraft will try to find a repair pad";
			c.mouseicon = c.name;

			c.queueing = false;

			c.params.push_back("1");
			c.params.push_back("LandAt 0");
			c.params.push_back("LandAt 30");
			c.params.push_back("LandAt 50");
			c.params.push_back("LandAt 80");
			possibleCommands.push_back(commandDescriptionCache->GetPtr(c));
		}
		{
			SCommandDescription c;

			c.id   = CMD_IDLEMODE;
			c.type = CMDTYPE_ICON_MODE;

			c.action    = "idlemode";
			c.name      = "Land mode";
			c.tooltip   = c.name + ": Sets what aircraft will do on idle";
			c.mouseicon = c.name;

			c.queueing = false;

			c.params.push_back("1");
			c.params.push_back(" Fly ");
			c.params.push_back("Land");
			possibleCommands.push_back(commandDescriptionCache->GetPtr(c));
		}
	}

	if (owner->unitDef->IsTransportUnit()) {
		{
			SCommandDescription c;

			c.id   = CMD_LOAD_UNITS;
			c.type = CMDTYPE_ICON_UNIT_OR_AREA;

			c.action    = "loadunits";
			c.name      = "Load units";
			c.tooltip   = c.name + ": Sets the transport to load a unit or units within an area";
			c.mouseicon = c.name;
			possibleCommands.push_back(commandDescriptionCache->GetPtr(c));
		}
		{
			SCommandDescription c;

			c.id   = CMD_UNLOAD_UNITS;
			c.type = CMDTYPE_ICON_AREA;

			c.action    = "unloadunits";
			c.name      = "Unload units";
			c.tooltip   = c.name + ": Sets the transport to unload units in an area";
			c.mouseicon = c.name;
			possibleCommands.push_back(commandDescriptionCache->GetPtr(c));
		}
	}

	UpdateNonQueueingCommands();
}

CMobileCAI::~CMobileCAI()
{
	// if uh == NULL then all pointers to units should be considered dangling
	if (unitHandler != nullptr) {
		SetTransportee(nullptr);
	}
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
				case 0: { repairBelowHealth = 0.0f; break; }
				case 1: { repairBelowHealth = 0.3f; break; }
				case 2: { repairBelowHealth = 0.5f; break; }
				case 3: { repairBelowHealth = 0.8f; break; }
			}

			for (unsigned int n = 0; n < possibleCommands.size(); n++) {
				if (possibleCommands[n]->id != CMD_AUTOREPAIRLEVEL)
					continue;

				UpdateCommandDescription(n, c);
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
				if (possibleCommands[n]->id != CMD_IDLEMODE)
					continue;

				UpdateCommandDescription(n, c);
				break;
			}

			selectedUnitsHandler.PossibleCommandChange(owner);
			return;
		}
	}

	if (!(c.options & SHIFT_KEY) && nonQueingCommands.find(c.GetID()) == nonQueingCommands.end()) {
		tempOrder = false;
		SetTransportee(NULL);
		StopSlowGuard();
	}

	CCommandAI::GiveAllowedCommand(c);
}


void CMobileCAI::SlowUpdate()
{
	if (gs->paused) // Commands issued may invoke SlowUpdate when paused
		return;


	if (!commandQue.empty() && commandQue.front().timeOut < gs->frameNum) {
		StopMoveAndFinishCommand();
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
		case CMD_LOAD_ONTO:            { ExecuteLoadOnto(c);			return; }
	}
	if (owner->unitDef->IsTransportUnit()) {
		switch (c.GetID()) {
			case CMD_LOAD_UNITS:   { ExecuteLoadUnits(c); return; }
			case CMD_UNLOAD_UNITS: { ExecuteUnloadUnits(c); return; }
			case CMD_UNLOAD_UNIT:  { ExecuteUnloadUnit(c);  return; }
		}
	}
	CCommandAI::SlowUpdate();
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

	const float sqDist = owner->pos.SqDistance2D(cmdPos);
	if (sqDist < Square(SQUARE_SIZE)) {
		if (!HasMoreMoveCommands())
			StopMove();

		FinishCommand();
		return;
	}

	// This check is important to process failed orders properly
	if (!owner->moveType->IsMovingTowards(cmdPos, SQUARE_SIZE, false))
		SetGoal(cmdPos, owner->pos);

	if (owner->moveType->progressState == AMoveType::Failed) {
		StopMoveAndFinishCommand();
		return;
	}

	if (sqDist < cancelDistance && HasMoreMoveCommands())
		FinishCommand();
}

void CMobileCAI::ExecuteLoadOnto(Command &c) {
	CUnit* unit = unitHandler->GetUnit(c.params[0]);

	if (unit == nullptr || !unit->unitDef->IsTransportUnit()) {
		StopMoveAndFinishCommand();
		return;
	}

	if (!inCommand) {
		inCommand = true;
		Command newCommand(CMD_LOAD_UNITS, INTERNAL_ORDER | SHIFT_KEY, owner->id);
		unit->commandAI->GiveCommandReal(newCommand);
	}
	if (owner->GetTransporter() != NULL) {
		if (!commandQue.empty())
			StopMoveAndFinishCommand();

		return;
	}

	if (unit == NULL)
		return;

	if ((owner->pos - unit->pos).SqLength2D() < cancelDistance) {
		StopMove();
	} else {
		SetGoal(unit->pos, owner->pos);
	}
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

	float3 cmdPos = c.GetPos(0);

	if (!inCommand) {
		inCommand = true;
		commandPos2 = cmdPos;
		lastUserGoal = commandPos2;
	}
	if (c.params.size() >= 6) {
		cmdPos = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);
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

	ExecuteMove(c);
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

	// test if any weapon can target the enemy unit
	for (CWeapon* w: owner->weapons) {
		if (w->TestTarget(enemy->pos, SWeaponTarget(enemy)) &&
			(owner->moveState != MOVESTATE_HOLDPOS || w->TryTargetRotate(enemy, false, false))) {
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

	if (guardee == NULL) { StopMoveAndFinishCommand(); return; }
	if (UpdateTargetLostTimer(guardee->id) == 0) { StopMoveAndFinishCommand(); return; }
	if (guardee->outOfMapTime > (GAME_SPEED * 5)) { StopMoveAndFinishCommand(); return; }

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
			((owner->moveType->goalPos - goal).SqLength2D() > 1600.0f) ||
			(owner->moveType->goalPos - owner->pos).SqLength2D() < Square(owner->moveType->GetMaxSpeed() * GAME_SPEED + 1 + SQUARE_SIZE * 2);

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
			StopMoveAndFinishCommand();
			return;
		}
	}
	if (!inCommand) {
		if (c.params.size() == 1) {
			CUnit* targetUnit = unitHandler->GetUnit(c.params[0]);

			// check if we have valid target parameter and that we aren't attacking ourselves
			if (targetUnit == NULL) { StopMoveAndFinishCommand(); return; }
			if (targetUnit == owner) { StopMoveAndFinishCommand(); return; }
			if (targetUnit->GetTransporter() != NULL && !modInfo.targetableTransportedUnits) {
				StopMoveAndFinishCommand(); return;
			}

			const float3 tgtErrPos = targetUnit->GetErrorPos(owner->allyteam, false);
			const float3 tgtPosDir = (tgtErrPos - owner->pos).Normalize();
			float radius = targetUnit->radius;
			if (targetUnit->unitDef->IsImmobileUnit()){
				//FIXME change into a function
				const float fpSqRadius = (targetUnit->unitDef->xsize * targetUnit->unitDef->xsize + targetUnit->unitDef->zsize * targetUnit->unitDef->zsize);
				const float fpRadius = (math::sqrt(fpSqRadius) * 0.5f) * SQUARE_SIZE;
				// 2 * SQUARE_SIZE is a buffer that work to make sure it's pathable ground
				radius = std::max(radius, fpRadius + 2 * SQUARE_SIZE);
			}
			// FIXME: don't call SetGoal() if target is already in range of some weapon?
			SetGoal(tgtErrPos - tgtPosDir * radius, owner->pos);
			SetOrderTarget(targetUnit);
			owner->AttackUnit(targetUnit, (c.options & INTERNAL_ORDER) == 0, c.GetID() == CMD_MANUALFIRE);

			inCommand = true;
		}
		else if (c.params.size() >= 3) {
			// user gave force-fire attack command
			SetGoal(c.GetPos(0), owner->pos);

			inCommand = true;
		}
	}

	// if our target is dead or we lost it then stop attacking
	// NOTE: unit should actually just continue to target area!
	if (targetDied || (c.params.size() == 1 && UpdateTargetLostTimer(int(c.params[0])) == 0)) {
		// cancel keeppointingto
		StopMoveAndFinishCommand();
		return;
	}


	// user clicked on enemy unit (note that we handle aircrafts slightly differently)
	if (orderTarget != NULL) {
		bool tryTargetRotate  = false;
		bool tryTargetHeading = false;

		float edgeFactor = 0.0f; // percent offset to target center
		const float3 targetMidPosVec = owner->midPos - orderTarget->midPos;

		const float3 errPos = orderTarget->GetErrorPos(owner->allyteam, false);

		const float targetGoalDist = errPos.SqDistance2D(owner->moveType->goalPos);
		const float targetPosDist = Square(10.0f + orderTarget->pos.distance2D(owner->pos) * 0.2f);
		const float minPointingDist = std::min(1.0f * owner->losRadius, owner->maxRange * 0.9f);

		// FIXME? targetMidPosMaxDist is 3D, but compared with a 2D value
		const float targetMidPosDist2D = targetMidPosVec.Length2D();
		// const float targetMidPosMaxDist = owner->maxRange - (Square(orderTarget->speed.w) / owner->unitDef->maxAcc);

		if (!owner->weapons.empty()) {
			if (!(c.options & ALT_KEY) && SkipParalyzeTarget(orderTarget)) {
				StopMoveAndFinishCommand();
				return;
			}
		}

		SWeaponTarget trg(orderTarget, (c.options & INTERNAL_ORDER) == 0);
		trg.isManualFire = (c.GetID() == CMD_MANUALFIRE);
		const short targetHead = GetHeadingFromVector(-targetMidPosVec.x, -targetMidPosVec.z);

		for (CWeapon* w: owner->weapons) {
			if (c.GetID() == CMD_MANUALFIRE) {
				assert(owner->unitDef->canManualFire);

				if (!w->weaponDef->manualfire) {
					continue;
				}
			}

			tryTargetRotate  = w->TryTargetRotate(trg.unit, trg.isUserTarget, trg.isManualFire);
			tryTargetHeading = w->TryTargetHeading(targetHead, trg);

			edgeFactor = math::fabs(w->weaponDef->targetBorder);
			if (tryTargetRotate || tryTargetHeading)
				break;
		}

		// if w->AttackUnit() returned true then we are already
		// in range with our biggest (?) weapon, so stop moving
		// also make sure that we're not locked in close-in/in-range state
		// loop due to rotates invoked by in-range or out-of-range states
		if (tryTargetRotate) {
			const bool canChaseTarget = (!tempOrder || owner->moveState != MOVESTATE_HOLDPOS);
			const bool targetBehind = (targetMidPosVec.dot(orderTarget->speed) < 0.0f);
			if (canChaseTarget && tryTargetHeading && targetBehind && !owner->unitDef->IsHoveringAirUnit()) {
				SetGoal(owner->pos + (orderTarget->speed * 80), owner->pos, SQUARE_SIZE, orderTarget->speed.w * 1.1f);
			} else {
				StopMove();

				if (gs->frameNum > lastCloseInTry + MAX_CLOSE_IN_RETRY_TICKS) {
					owner->moveType->KeepPointingTo(orderTarget->midPos, minPointingDist, true);
				}
			}

			owner->AttackUnit(trg.unit, trg.isUserTarget, trg.isManualFire);
		}

		// if we're on hold pos in a temporary order, then none of the close-in
		// code below should run, and the attack command is cancelled.
		else if (tempOrder && owner->moveState == MOVESTATE_HOLDPOS) {
			StopMoveAndFinishCommand();
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
			else if (owner->unitDef->strafeToAttack) {
				moveDir ^= (owner->moveType->progressState == AMoveType::Failed);

				const float sin = moveDir ? 3.0/5 : -3.0/5;
				const float cos = 4.0 / 5;

				float3 goalDiff;
				goalDiff.x = targetMidPosVec.dot(float3(cos, 0, -sin));
				goalDiff.z = targetMidPosVec.dot(float3(sin, 0,  cos));
				goalDiff *= (targetMidPosDist2D < (owner->maxRange * 0.3f)) ? 1/cos : cos;
				goalDiff += orderTarget->pos;
				SetGoal(goalDiff, owner->pos);
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

			const float3 norm = (errPos - owner->pos).Normalize();
			float radius = (orderTarget->radius * edgeFactor * 0.8f);
			if (orderTarget->unitDef->IsImmobileUnit()){
				//FIXME change into a function
				const float fpSqRadius = (orderTarget->unitDef->xsize * orderTarget->unitDef->xsize + orderTarget->unitDef->zsize * orderTarget->unitDef->zsize);
				const float fpRadius = (math::sqrt(fpSqRadius) * 0.5f) * SQUARE_SIZE;
				// 2 * SQUARE_SIZE is a buffer that work to make sure it's pathable ground
				radius = std::max(radius, fpRadius + 2 * SQUARE_SIZE);
			}
			const float3 goal = errPos - norm * radius;
			SetGoal(goal, owner->pos);

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

			if (attackVec.SqLength2D() < Square(owner->maxRange * 0.9f)) {
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
			} else if (owner->CanTransport(pointed)) {
				return CMD_LOAD_UNITS;
			}
		} else {
			if (owner->CanTransport(pointed)) {
				return CMD_LOAD_UNITS;
			} else if (pointed->CanTransport(owner)) {
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
	if (owner->moveType->IsMovingTowards(pos, goalRadius, true))
		return;

	// give new move order
	owner->moveType->StartMoving(pos, goalRadius);
}

void CMobileCAI::SetGoal(const float3& pos, const float3& /*curPos*/, float goalRadius, float speed)
{
	// already have equal move order?
	if (owner->moveType->IsMovingTowards(pos, goalRadius, true))
		return;

	// give new move order
	owner->moveType->StartMoving(pos, goalRadius, speed);
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
}

void CMobileCAI::StopMoveAndFinishCommand()
{
	StopMove();
	FinishCommand();
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
	SetTransportee(NULL);
	if (!((commandQue.front()).options & INTERNAL_ORDER)) {
		lastUserGoal = owner->pos;
	}
	tempOrder = false;
	StopSlowGuard();
	CCommandAI::FinishCommand();

	if (owner->unitDef->IsTransportUnit()) {
		CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);

		if (am == NULL || !commandQue.empty())
			return;

		am->SetWantedAltitude(0.0f);
		am->wantToStop = true;
	}
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
			//Try getting target from weapons
			for (CWeapon* w: owner->weapons) {
				if (w->HaveTarget() || w->AutoTarget()) {
					const auto t = w->GetCurrentTarget();
					if (t.type == Target_Unit && IsValidTarget(t.unit)) {
						newAttackTargetId = t.unit->id;
						break;
					}
				}
			}

			//Get target from wherever
			if (newAttackTargetId < 0) {
				const float searchRadius = owner->maxRange + 150.0f * owner->moveState * owner->moveState;
				const CUnit* enemy = CGameHelper::GetClosestValidTarget(owner->pos, searchRadius, owner->allyteam, this);

				if (enemy != NULL) {
					newAttackTargetId = enemy->id;
				}
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

bool CMobileCAI::CanWeaponAutoTarget(const CWeapon* weapon) const {
	return (!tempOrder) || //Check if the weapon actually targets the unit's order target
		weapon->GetCurrentTarget() != owner->curTarget;
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


/******************************************************************************/
/******************************************************************************/


void CMobileCAI::SetTransportee(CUnit* unit) {
	assert(unit == nullptr || owner->unitDef->IsTransportUnit());

	if (!owner->unitDef->IsTransportUnit()) {
		return;
	}

	if (orderTarget != nullptr && orderTarget->loadingTransportId == owner->id) {
		orderTarget->loadingTransportId = -1;
	}
	SetOrderTarget(unit);
	if (unit != nullptr) {
		CUnit* transport = (unit->loadingTransportId == -1) ? NULL : unitHandler->GetUnitUnsafe(unit->loadingTransportId);
		// let the closest transport be loadingTransportId, in case of multiple fighting transports
		if ((transport == NULL) || ((transport != owner) && (transport->pos.SqDistance(unit->pos) > owner->pos.SqDistance(unit->pos)))) {
			unit->loadingTransportId = owner->id;
		}
	}
}


void CMobileCAI::ExecuteLoadUnits(Command& c)
{
	if (c.params.size() == 1) {
		// load single unit
		CUnit* unit = unitHandler->GetUnit(c.params[0]);

		if (unit == NULL) {
			StopMoveAndFinishCommand();
			return;
		}

		if (c.options & INTERNAL_ORDER) {
			if (unit->commandAI->commandQue.empty()) {
				if (!LoadStillValid(unit)) {
					StopMoveAndFinishCommand();
					return;
				}
			} else {
				Command& currentUnitCommand = unit->commandAI->commandQue[0];

				if ((currentUnitCommand.GetID() == CMD_LOAD_ONTO) && (currentUnitCommand.params.size() == 1) && (int(currentUnitCommand.params[0]) == owner->id)) {
					if ((unit->moveType->progressState == AMoveType::Failed) && (owner->moveType->progressState == AMoveType::Failed)) {
						unit->commandAI->FinishCommand();
						StopMoveAndFinishCommand();
						return;
					}
				} else if (!LoadStillValid(unit)) {
					StopMoveAndFinishCommand();
					return;
				}
			}
		}

		if (inCommand) {
			if (!owner->script->IsBusy()) {
				StopMoveAndFinishCommand();
			}
			return;
		}
		if (owner->CanTransport(unit) && UpdateTargetLostTimer(int(c.params[0]))) {
			SetTransportee(unit);

			const float sqDist = unit->pos.SqDistance2D(owner->pos);
			const bool inLoadingRadius = (sqDist <= Square(owner->unitDef->loadingRadius));

			CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);

			// subtract 1 square to account for PFS/GMT inaccuracy
			const bool outOfRange = (owner->moveType->goalPos.SqDistance2D(unit->pos) > Square(owner->unitDef->loadingRadius - SQUARE_SIZE));
			const bool moveCloser = (!inLoadingRadius && (!owner->IsMoving() || (am != NULL && am->aircraftState != AAirMoveType::AIRCRAFT_FLYING)));

			if (outOfRange || moveCloser) {
				SetGoal(unit->pos, owner->pos, std::min(64.0f, owner->unitDef->loadingRadius));
			}

			if (inLoadingRadius) {
				if (am != NULL) {
					// handle air transports differently
					float3 wantedPos = unit->pos;
					wantedPos.y = owner->GetTransporteeWantedHeight(wantedPos, unit);

					// calls am->StartMoving() which sets forceHeading to false (and also
					// changes aircraftState, possibly in mid-pickup) --> must check that
					// wantedPos == goalPos using some epsilon tolerance
					// we do not want the forceHeading change at point of pickup because
					// am->UpdateHeading() will suddenly notice a large deltaHeading and
					// break the DOCKING_ANGLE constraint so call am->ForceHeading() next
					SetGoal(wantedPos, owner->pos, 1.0f);

					am->ForceHeading(owner->GetTransporteeWantedHeading(unit));
					am->SetWantedAltitude(wantedPos.y - CGround::GetHeightAboveWater(wantedPos.x, wantedPos.z));
					am->maxDrift = 1.0f;

					// FIXME: kill the hardcoded constants, use the command's radius
					const bool b1 = (owner->pos.SqDistance(wantedPos) < Square(AIRTRANSPORT_DOCKING_RADIUS));
					const bool b2 = (std::abs(owner->heading - unit->heading) < AIRTRANSPORT_DOCKING_ANGLE);
					const bool b3 = (owner->updir.dot(UpVector) > 0.995f);

					if (b1 && b2 && b3) {
						am->SetAllowLanding(false);
						am->SetWantedAltitude(0.0f);

						owner->script->BeginTransport(unit);
						SetTransportee(NULL);
						owner->AttachUnit(unit, owner->script->QueryTransport(unit));

						StopMoveAndFinishCommand();
						return;
					}
				} else {
					inCommand = true;

					StopMove();
					owner->script->TransportPickup(unit);
				}
			} else if (owner->moveType->progressState == AMoveType::Failed && sqDist < (200 * 200)) {
				// if we're pretty close already but CGroundMoveType fails because it considers
				// the goal clogged (with the future passenger...), just try to move to the
				// point halfway between the transport and the passenger.
				SetGoal((unit->pos + owner->pos) * 0.5f, owner->pos);
			}
		} else {
			StopMoveAndFinishCommand();
		}
	} else if (c.params.size() == 4) { // area-load
		if (lastPC == gs->frameNum) { // avoid infinite loops
			return;
		}

		lastPC = gs->frameNum;

		const float3 pos = c.GetPos(0);
		const float radius = c.params[3];

		CUnit* unit = FindUnitToTransport(pos, radius);

		if (unit && owner->CanTransport(unit)) {
			Command c2(CMD_LOAD_UNITS, c.options|INTERNAL_ORDER, unit->id);
			commandQue.push_front(c2);
			inCommand = false;

			SlowUpdate();
			return;
		} else {
			StopMoveAndFinishCommand();
			return;
		}
	}
}


void CMobileCAI::ExecuteUnloadUnits(Command& c)
{
	if (lastPC == gs->frameNum) {
		// avoid infinite loops
		return;
	}

	lastPC = gs->frameNum;

	if (owner->transportedUnits.empty()) {
		StopMoveAndFinishCommand();
		return;
	}

	switch (owner->unitDef->transportUnloadMethod) {
		case UNLOAD_LAND: {
			UnloadUnits_Land(c);
		} break;
		case UNLOAD_DROP: {
			if (owner->unitDef->canfly) {
				UnloadUnits_Drop(c);
			} else {
				UnloadUnits_Land(c);
			}
		} break;
		case UNLOAD_LANDFLOOD: {
			UnloadUnits_LandFlood(c);
		} break;
		default: {
			UnloadUnits_Land(c);
		} break;
	}
}


void CMobileCAI::ExecuteUnloadUnit(Command& c)
{
	if (inCommand) {
		if (!owner->script->IsBusy()) {
			StopMoveAndFinishCommand();
		}
		return;
	}
	if (owner->transportedUnits.empty()) {
		StopMoveAndFinishCommand();
		return;
	}
	// new methods
	switch (owner->unitDef->transportUnloadMethod) {
		case UNLOAD_LAND: UnloadLand(c); break;
		case UNLOAD_DROP: {
			if (owner->unitDef->canfly) {
				UnloadDrop(c);
			} else {
				UnloadLand(c);
			}
		} break;
		case UNLOAD_LANDFLOOD: UnloadLandFlood(c); break;

		default: UnloadLand(c); break;
	}
}


bool CMobileCAI::AllowedCommand(const Command& c, bool fromSynced)
{
	if (!CCommandAI::AllowedCommand(c, fromSynced)) {
		return false;
	}

	if (!owner->unitDef->IsTransportUnit()) {
		return true;
	}

	switch (c.GetID()) {
		case CMD_UNLOAD_UNIT:
		case CMD_UNLOAD_UNITS: {
			const auto& transportees = owner->transportedUnits;

			// allow unloading empty transports for easier setup of transport bridges
			if (transportees.empty())
				return true;

			if (c.GetParamsCount() == 5) {
				if (fromSynced) {
					// point transported buildings (...) in their wanted direction after unloading
					for (auto& tu: transportees) {
						tu.unit->buildFacing = std::abs(int(c.GetParam(4))) % NUM_FACINGS;
					}
				}
			}

			if (c.GetParamsCount() >= 4) {
				// find unload positions for transportees (WHY can this run in unsynced context?)
				for (const CUnit::TransportedUnit& tu: transportees) {
					const CUnit* u = tu.unit;

					const float radius = (c.GetID() == CMD_UNLOAD_UNITS)? c.GetParam(3): 0.0f;
					const float spread = u->radius * owner->unitDef->unloadSpread;

					float3 foundPos;

					if (FindEmptySpot(c.GetPos(0), radius, spread, foundPos, u, fromSynced)) {
						return true;
					}
					 // FIXME: support arbitrary unloading order for other unload types also
					if (owner->unitDef->transportUnloadMethod != UNLOAD_LAND) {
						return false;
					}
				}

				// no empty spot found for any transported unit
				return false;
			}

			break;
		}
	}

	return true;
}


bool CMobileCAI::FindEmptySpot(const float3& center, float radius, float spread, float3& found, const CUnit* unitToUnload, bool fromSynced)
{
	const MoveDef* moveDef = unitToUnload->moveDef;

	const bool isAirTrans = (dynamic_cast<AAirMoveType*>(owner->moveType) != NULL);
	const float amax = std::max(100.0f, std::min(1000.0f, (radius * radius / 100.0f)));

	spread = std::max(1.0f, math::ceil(spread / SQUARE_SIZE)) * SQUARE_SIZE;

	// more attempts for large unloading zone
	for (int a = 0; a < amax; ++a) {
		float3 delta;
		float3 pos;

		const float bmax = std::max(10, a / 10);

		for (int b = 0; b < bmax; ++b) {
			// FIXME: using a deterministic technique might be better, since it would allow an unload command to be tested for validity from unsynced (with predictable results)
			const float ang = 2.0f * PI * (fromSynced ? gs->randFloat() : gu->RandFloat());
			const float len = math::sqrt(fromSynced ? gs->randFloat() : gu->RandFloat());

			delta.x = len * math::sin(ang);
			delta.z = len * math::cos(ang);
			pos = center + delta * radius;
			if (pos.IsInBounds())
				break;
		}

		if (!pos.IsInBounds())
			continue;

		// returns loading height in pos.y
		if (!owner->CanLoadUnloadAtPos(pos, unitToUnload, &pos.y))
			continue;

		// adjust to middle pos
		pos.y -= unitToUnload->radius;

		// don't unload unit on too-steep slopes
		if (moveDef != NULL && CGround::GetSlope(pos.x, pos.z) > moveDef->maxSlope)
			continue;

		const std::vector<CSolidObject*>& units = quadField->GetSolidsExact(pos, spread, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS);

		if (isAirTrans && (units.size() > 1 || (units.size() == 1 && units[0] != owner)))
			continue;
		if (!isAirTrans && !units.empty())
			continue;

		found = pos;
		return true;
	}

	return false;
}


CUnit* CMobileCAI::FindUnitToTransport(float3 center, float radius)
{
	CUnit* bestUnit = NULL;
	float bestDist = std::numeric_limits<float>::max();

	const std::vector<CUnit*>& units = quadField->GetUnitsExact(center, radius);

	for (std::vector<CUnit*>::const_iterator ui = units.begin(); ui != units.end(); ++ui) {
		CUnit* unit = (*ui);
		float dist = unit->pos.SqDistance2D(owner->pos);

		if (unit->loadingTransportId != -1 && unit->loadingTransportId != owner->id) {
			CUnit* trans = unitHandler->GetUnitUnsafe(unit->loadingTransportId);
			if ((trans != NULL) && teamHandler->AlliedTeams(owner->team, trans->team)) {
				continue; // don't refuse to load comm only because the enemy is trying to nap it at the same time
			}
		}
		if (dist >= bestDist)
			continue;
		if (!owner->CanTransport(unit))
			continue;

		if (unit->losStatus[owner->allyteam] & (LOS_INRADAR | LOS_INLOS)) {
			bestDist = dist;
			bestUnit = unit;
		}
	}

	return bestUnit;
}


bool CMobileCAI::LoadStillValid(CUnit* unit)
{
	if (commandQue.size() < 2) {
		return false;
	}

	const Command& cmd = commandQue[1];

	// we are called from ExecuteLoadUnits only in the case that
	// that commandQue[0].id == CMD_LOAD_UNITS, so if the second
	// command is NOT an area- but a single-unit-loading command
	// (which has one parameter) then the first one will be valid
	// (ELU keeps pushing CMD_LOAD_UNITS as long as there are any
	// units to pick up)
	//
	if (cmd.GetID() != CMD_LOAD_UNITS || cmd.GetParamsCount() != 4) {
		return true;
	}

	const float3& cmdPos = cmd.GetPos(0);

	if (!owner->CanLoadUnloadAtPos(cmdPos, unit)) {
		return false;
	}

	return (unit->pos.SqDistance2D(cmdPos) <= Square(cmd.GetParam(3) * 2.0f));
}


bool CMobileCAI::SpotIsClear(float3 pos, CUnit* unitToUnload)
{
	if (!owner->CanLoadUnloadAtPos(pos, unitToUnload))
		return false;
	if (unitToUnload->moveDef != NULL && CGround::GetSlope(pos.x, pos.z) > unitToUnload->moveDef->maxSlope)
		return false;

	const float radius = std::max(1.0f, math::ceil(unitToUnload->radius / SQUARE_SIZE)) * SQUARE_SIZE;

	if (!quadField->NoSolidsExact(pos, radius, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
		return false;

	return true;
}


bool CMobileCAI::SpotIsClearIgnoreSelf(float3 pos, CUnit* unitToUnload)
{
	if (!owner->CanLoadUnloadAtPos(pos, unitToUnload))
		return false;
	if (unitToUnload->moveDef != NULL && CGround::GetSlope(pos.x, pos.z) > unitToUnload->moveDef->maxSlope)
		return false;

	const float radius = std::max(1.0f, math::ceil(unitToUnload->radius / SQUARE_SIZE)) * SQUARE_SIZE;

	const std::vector<CSolidObject*>& units = quadField->GetSolidsExact(pos, radius, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS);

	for (auto objectsIt = units.begin(); objectsIt != units.end(); ++objectsIt) {
		// check if the units are in the transport
		bool found = false;

		for (auto& tu: owner->transportedUnits) {
			if ((found |= (*objectsIt == tu.unit)))
				break;
		}
		if (!found && (*objectsIt != owner)) {
			return false;
		}
	}

	return true;
}


bool CMobileCAI::FindEmptyDropSpots(float3 startpos, float3 endpos, std::vector<float3>& dropSpots)
{
	if (dynamic_cast<CHoverAirMoveType*>(owner->moveType) == NULL)
		return false;

	const float maxDist = startpos.SqDistance(endpos);
	const float3 spreadVector = (endpos - startpos).Normalize() * owner->unitDef->unloadSpread;

	const auto& transportees = owner->transportedUnits;
	auto ti = transportees.begin();

	float3 nextPos = startpos - spreadVector * ti->unit->radius;

	// remaining spots
	while (ti != transportees.end()) {
		const float3 gap = spreadVector * ti->unit->radius;
		nextPos += gap;

		if (startpos.SqDistance(nextPos) > maxDist) {
			break;
		}

		nextPos.y = CGround::GetHeightAboveWater(nextPos.x, nextPos.z);
		// check landing spot is ok for landing on
		if (!SpotIsClear(nextPos, ti->unit))
			continue;

		dropSpots.push_back(nextPos);
		nextPos += gap;
		++ti;
	}

	return dropSpots.size() > 0;
}


void CMobileCAI::UnloadUnits_Land(Command& c)
{
	const auto& transportees = owner->transportedUnits;
	const CUnit* transportee = nullptr;

	float3 unloadPos;

	for (const CUnit::TransportedUnit& tu: transportees) {
		const float3 pos = c.GetPos(0);
		const float radius = c.params[3];
		const float spread = (tu.unit)->radius * owner->unitDef->unloadSpread;

		if (FindEmptySpot(pos, radius, spread, unloadPos, tu.unit)) {
			transportee = tu.unit; break;
		}
	}

	if (transportee != NULL) {
		Command c2(CMD_UNLOAD_UNIT, c.options | INTERNAL_ORDER, unloadPos);
		c2.PushParam(transportee->id);
		commandQue.push_front(c2);
		SlowUpdate();
		return;
	}
	StopMoveAndFinishCommand();
}


void CMobileCAI::UnloadUnits_Drop(Command& c)
{
	const auto& transportees = owner->transportedUnits;

	const float3 startingDropPos = c.GetPos(0);
	const float3 approachVector = (startingDropPos - owner->pos).Normalize();

	std::vector<float3> dropSpots;

	const bool canUnload = FindEmptyDropSpots(startingDropPos, startingDropPos + approachVector * std::max(16.0f, c.params[3]), dropSpots);

	StopMoveAndFinishCommand();

	if (canUnload) {
		auto ti = transportees.begin();
		auto di = dropSpots.rbegin();
		for (;ti != transportees.end() && di != dropSpots.rend(); ++ti, ++di) {
			Command c2(CMD_UNLOAD_UNIT, c.options | INTERNAL_ORDER, *di);
			commandQue.push_front(c2);
		}
		SlowUpdate();
		return;
	}

	StopMoveAndFinishCommand();
}


void CMobileCAI::UnloadUnits_LandFlood(Command& c)
{
	float3 pos = c.GetPos(0);
	float3 found;
	const float radius = c.params[3];
	const float dist = std::max(64.0f, owner->unitDef->loadingRadius - radius);


	if (pos.SqDistance2D(owner->pos) > dist) {
		SetGoal(pos, owner->pos, dist);
		return;
	}

	const auto& transportees = owner->transportedUnits;
	const CUnit* transportee = transportees[0].unit;

	const float spread = transportee->radius * owner->unitDef->unloadSpread;
	const bool canUnload = FindEmptySpot(pos, radius, spread, found, transportee);

	if (canUnload) {
		Command c2(CMD_UNLOAD_UNIT, c.options | INTERNAL_ORDER, found);
		commandQue.push_front(c2);
		SlowUpdate();
		return;
	}

	StopMoveAndFinishCommand();
}


void CMobileCAI::UnloadLand(Command& c)
{
	// default unload
	CUnit* transportee = NULL;

	float3 wantedPos = c.GetPos(0);

	const auto& transportees = owner->transportedUnits;

	SetGoal(wantedPos, owner->pos);

	if (c.params.size() < 4) {
		// unload the first transportee
		transportee = transportees[0].unit;
	} else {
		const int unitID = c.params[3];

		// unload a specific transportee
		for (auto& tu: transportees) {
			CUnit* carried = tu.unit;

			if (unitID == carried->id) {
				transportee = carried;
				break;
			}
		}
		if (transportee == NULL) {
			StopMoveAndFinishCommand();
			return;
		}
	}

	if (wantedPos.SqDistance2D(owner->pos) < Square(owner->unitDef->loadingRadius * 0.9f)) {
		CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);
		wantedPos.y = owner->GetTransporteeWantedHeight(wantedPos, transportee);

		if (am != NULL) {
			// handle air transports differently
			SetGoal(wantedPos, owner->pos);

			am->SetWantedAltitude(wantedPos.y - CGround::GetHeightAboveWater(wantedPos.x, wantedPos.z));
			am->ForceHeading(owner->GetTransporteeWantedHeading(transportee));

			am->maxDrift = 1.0f;

			// FIXME: kill the hardcoded constants, use the command's radius
			// NOTE: 2D distance-check would mean units get dropped from air
			const bool b1 = (owner->pos.SqDistance(wantedPos) < Square(AIRTRANSPORT_DOCKING_RADIUS));
			const bool b2 = (std::abs(owner->heading - am->GetForcedHeading()) < AIRTRANSPORT_DOCKING_ANGLE);
			const bool b3 = (owner->updir.dot(UpVector) > 0.99f);

			if (b1 && b2 && b3) {
				wantedPos.y -= transportee->radius;

				if (!SpotIsClearIgnoreSelf(wantedPos, transportee)) {
					// chosen spot is no longer clear to land, choose a new one
					// if a new spot cannot be found, don't unload at all
					float3 newWantedPos;

					if (FindEmptySpot(wantedPos, std::max(16.0f * SQUARE_SIZE, transportee->radius * 4.0f), transportee->radius, newWantedPos, transportee)) {
						c.SetPos(0, newWantedPos);
						SetGoal(newWantedPos + UpVector * transportee->model->height, owner->pos);
						return;
					}
				} else {
					owner->DetachUnit(transportee);

					if (transportees.empty()) {
						am->SetAllowLanding(true);
						owner->script->EndTransport();
					}
				}

				// move the transport away slightly
				SetGoal(owner->pos + owner->frontdir * 20.0f, owner->pos);
				FinishCommand();
			}
		} else {
			inCommand = true;
			StopMove();
			owner->script->TransportDrop(transportee, wantedPos);
		}
	}
}


void CMobileCAI::UnloadDrop(Command& c)
{
	float3 pos = c.GetPos(0);

	// head towards goal
	// note that HoverAirMoveType must be modified to allow
	// non-stop movement through goals for this to work well
	if (owner->moveType->goalPos.SqDistance2D(pos) > Square(20.0f)) {
		SetGoal(pos, owner->pos);
	}

	CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);
	CUnit* transportee = owner->transportedUnits[0].unit;

	if (am != NULL) {
		pos.y = CGround::GetHeightAboveWater(pos.x, pos.z);
		am->maxDrift = 1.0f;

		// if near target or passed it accidentally, drop unit
		if (owner->pos.SqDistance2D(pos) < Square(40.0f) || (((pos - owner->pos).Normalize()).SqDistance(owner->frontdir) > 0.25 && owner->pos.SqDistance2D(pos)< (205*205))) {
			am->SetAllowLanding(false);

			owner->script->EndTransport();
			owner->DetachUnitFromAir(transportee, pos);

			FinishCommand();
		}
	} else {
		inCommand = true;
		StopMove();
		owner->script->TransportDrop(transportee, pos);
	}
}


void CMobileCAI::UnloadLandFlood(Command& c)
{
	//land, then release all units at once
	CUnit* transportee = NULL;

	float3 wantedPos = c.GetPos(0);

	const auto& transportees = owner->transportedUnits;

	SetGoal(wantedPos, owner->pos);

	if (c.params.size() < 4) {
		transportee = transportees[0].unit;
	} else {
		const int unitID = c.params[3];

		for (auto& tu: transportees) {
			CUnit* carried = tu.unit;

			if (unitID == carried->id) {
				transportee = carried;
				break;
			}
		}
		if (transportee == NULL) {
			FinishCommand();
			return;
		}
	}

	if (wantedPos.SqDistance2D(owner->pos) < Square(owner->unitDef->loadingRadius * 0.9f)) {
		CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);
		wantedPos.y = owner->GetTransporteeWantedHeight(wantedPos, transportee);

		if (am != NULL) {
			// lower to ground
			SetGoal(wantedPos, owner->pos);

			am->SetWantedAltitude(wantedPos.y - CGround::GetHeightAboveWater(wantedPos.x, wantedPos.z));
			am->ForceHeading(owner->GetTransporteeWantedHeading(transportee));
			am->SetAllowLanding(true);
			am->maxDrift = 1.0f;

			// once at ground
			if (owner->pos.y - CGround::GetHeightAboveWater(wantedPos.x, wantedPos.z) < SQUARE_SIZE) {
				// nail it to the ground before it tries jumping up, only to land again...
				am->SetState(am->AIRCRAFT_LANDED);
				// call this so that other animations such as opening doors may be started
				owner->script->TransportDrop(transportees[0].unit, wantedPos);
				owner->DetachUnitFromAir(transportee, wantedPos);

				FinishCommand();

				if (transportees.empty()) {
					am->SetAllowLanding(true);
					owner->script->EndTransport();
					am->UpdateLanded();
				}
			}
		} else {
			// land transports
			inCommand = true;

			StopMove();
			owner->script->TransportDrop(transportee, wantedPos);
			owner->DetachUnitFromAir(transportee, wantedPos);

			if (transportees.empty()) {
				owner->script->EndTransport();
			}
		}
	}
}
