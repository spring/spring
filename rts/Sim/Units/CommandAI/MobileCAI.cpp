/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "MobileCAI.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Map/Ground.h"
#include "Sim/Misc/GlobalSynced.h"
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
#include "System/EventHandler.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include <assert.h>

#define AUTO_GENERATE_ATTACK_ORDERS 1

// MobileCAI is not always assigned to aircraft
static AAirMoveType* GetAirMoveType(const CUnit* owner) {
	assert(owner->unitDef->IsAirUnit());

	if (owner->UsingScriptMoveType())
		return static_cast<AAirMoveType*>(owner->prevMoveType);

	return static_cast<AAirMoveType*>(owner->moveType);
}

static float CalcTargetRadius(const CUnit* targetUnit, float minRadius, float radiusMult) {
	if (targetUnit->unitDef->IsImmobileUnit()) {
		const float fpSqRadius = (Square(targetUnit->xsize) + Square(targetUnit->zsize));
		const float fpRadius = (math::sqrt(fpSqRadius) * 0.5f) * SQUARE_SIZE;

		// 2 * SQUARE_SIZE is a buffer to make sure SetGoal stays on pathable ground
		return (std::max(minRadius * radiusMult, fpRadius + 2 * SQUARE_SIZE));
	}

	return (minRadius * radiusMult);
}



CR_BIND_DERIVED(CMobileCAI ,CCommandAI , )
CR_REG_METADATA(CMobileCAI, (
	CR_MEMBER(lastBuggerGoalPos),
	CR_MEMBER(lastUserGoal),

	CR_MEMBER(commandPos1),
	CR_MEMBER(commandPos2),
	CR_MEMBER(buggerOffPos),

	CR_MEMBER(buggerOffRadius),
	CR_MEMBER(repairBelowHealth),

	CR_MEMBER(tempOrder),
	CR_MEMBER(slowGuard),
	CR_MEMBER(moveDir),

	CR_MEMBER(cancelDistance),

	CR_MEMBER(lastCommandFrame),
	CR_MEMBER(lastCloseInTry),
	CR_MEMBER(lastBuggerOffTime),
	CR_MEMBER(numNonMovingCalls),
	CR_MEMBER(lastIdleCheck)
))

CMobileCAI::CMobileCAI():
	CCommandAI(),

	lastBuggerGoalPos(-XZVector),
	lastUserGoal(ZeroVector),

	buggerOffRadius(0.0f),
	repairBelowHealth(0.0f),

	tempOrder(false),
	slowGuard(false),
	moveDir(gsRNG.NextFloat() > 0.5f)
{}


CMobileCAI::CMobileCAI(CUnit* owner):
	CCommandAI(owner),

	lastBuggerGoalPos(-XZVector),
	lastUserGoal(owner->pos),

	buggerOffRadius(0.0f),
	repairBelowHealth(0.0f),

	tempOrder(false),
	slowGuard(false),
	moveDir(gsRNG.NextFloat() > 0.5f)
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
		possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
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
		possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
	}

	if (owner->unitDef->canPatrol) {
		SCommandDescription c;

		c.id   = CMD_PATROL;
		c.type = CMDTYPE_ICON_MAP;

		c.action    = "patrol";
		c.name      = "Patrol";
		c.tooltip   = c.name + ": Order the unit to patrol to one or more waypoints";
		c.mouseicon = c.name;
		possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
	}

	if (owner->unitDef->canFight) {
		SCommandDescription c;

		c.id   = CMD_FIGHT;
		c.type = CMDTYPE_ICON_FRONT;

		c.action    = "fight";
		c.name      = "Fight";
		c.tooltip   = c.name + ": Order the unit to take action while moving to a position";
		c.mouseicon = c.name;
		possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
	}

	if (owner->unitDef->canGuard) {
		SCommandDescription c;

		c.id   = CMD_GUARD;
		c.type = CMDTYPE_ICON_UNIT;

		c.action    = "guard";
		c.name      = "Guard";
		c.tooltip   = c.name + ": Order a unit to guard another unit and attack units attacking it";
		c.mouseicon = c.name;
		possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
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
			possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
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
			possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
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
			possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
		}
		{
			SCommandDescription c;

			c.id   = CMD_UNLOAD_UNITS;
			c.type = CMDTYPE_ICON_AREA;

			c.action    = "unloadunits";
			c.name      = "Unload units";
			c.tooltip   = c.name + ": Sets the transport to unload units in an area";
			c.mouseicon = c.name;
			possibleCommands.push_back(commandDescriptionCache.GetPtr(std::move(c)));
		}
	}

	UpdateNonQueueingCommands();
}

CMobileCAI::~CMobileCAI()
{
	SetTransportee(nullptr);
}


void CMobileCAI::GiveCommandReal(const Command& c, bool fromSynced)
{
	if (!AllowedCommand(c, fromSynced))
		return;

	if (owner->unitDef->IsAirUnit()) {
		AAirMoveType* airMT = GetAirMoveType(owner);

		if (c.GetID() == CMD_AUTOREPAIRLEVEL) {
			if (c.GetNumParams() == 0)
				return;

			switch ((int) c.GetParam(0)) {
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
			if (c.GetNumParams() == 0)
				return;

			// toggle between the "land" and "fly" idle-modes
			airMT->autoLand = (int(c.GetParam(0)) == 1);

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

	// directly issued queueing commands always cancel temporary (i.e. auto-generated attack) orders
	const bool directCmd = ((c.GetOpts() & SHIFT_KEY) == 0);
	const bool queingCmd = (nonQueingCommands.find(c.GetID()) == nonQueingCommands.end());

	if (directCmd && queingCmd) {
		tempOrder = false;

		SetTransportee(nullptr);
		StopSlowGuard();
	}

	CCommandAI::GiveAllowedCommand(c);
}


void CMobileCAI::SlowUpdate()
{
	if (gs->paused) // Commands issued may invoke SlowUpdate when paused
		return;

	if (!commandQue.empty() && commandQue.front().GetTimeOut() < gs->frameNum) {
		StopMoveAndFinishCommand();
		return;
	}

	if (commandQue.empty()) {
		MobileAutoGenerateTarget();

		// the attack order could terminate directly and thus cause a loop
		if (commandQue.empty() || (commandQue.front()).GetID() == CMD_ATTACK)
			return;
	}

	// when slow-guarding, regulate speed through {Start,Stop}SlowGuard
	Execute();
}

/**
* @brief Executes the first command in the commandQue
*/
void CMobileCAI::Execute()
{
	Command& c = commandQue.front();

	switch (c.GetID()) {
		case CMD_MOVE:                 { ExecuteMove(c);				return; }
		case CMD_PATROL:               { ExecutePatrol(c);				return; }
		case CMD_FIGHT:                { ExecuteFight(c);				return; }
		case CMD_GUARD:                { ExecuteGuard(c);				return; }
		case CMD_LOAD_ONTO:            { ExecuteLoadOnto(c);			return; }
	}

	if (owner->unitDef->IsTransportUnit()) {
		switch (c.GetID()) {
			case CMD_LOAD_UNITS:   { ExecuteLoadUnits(c);   return; }
			case CMD_UNLOAD_UNITS: { ExecuteUnloadUnits(c); return; }
			case CMD_UNLOAD_UNIT:  { ExecuteUnloadUnit(c);  return; }
		}
	}

	CCommandAI::SlowUpdate();
}

/**
* @brief executes the move command
*/
void CMobileCAI::ExecuteMove(Command& c)
{
	const AMoveType* moveType = owner->moveType;

	const float3& cmdPos = c.GetPos(0);
	const float3& ownPos = owner->pos;

	const float sqGoalDist = cmdPos.SqDistance2D(ownPos);

	// this check is important to process failed orders properly
	// NB: only works if the *non-extended* goal radius is passed
	if (!moveType->IsMovingTowards(cmdPos, moveType->GetGoalRadius(0.0f), false))
		SetGoal(cmdPos, ownPos);

	// compare against the moveType's own (possibly extended)
	// goal radius to determine if we can finish the command
	if (sqGoalDist < Square(moveType->GetGoalRadius(1.0f))) {
		if (!HasMoreMoveCommands())
			StopMove();

		FinishCommand();
		return;
	}

	if (moveType->progressState == AMoveType::Failed) {
		StopMoveAndFinishCommand();
		return;
	}

	// bypass cancel-distance check for internal (BUGGER_OFF) move commands
	// (not for internal CMD_FIGHT's created by ExecutePatrol; these should
	// get cancelled like regular user fight-commands)
	if (sqGoalDist >= cancelDistance || (c.IsInternalOrder() && !c.IsAttackCommand()) || !HasMoreMoveCommands())
		return;

	// fallback
	FinishCommand();
}

void CMobileCAI::ExecuteLoadOnto(Command& c) {
	CUnit* transport = unitHandler.GetUnit(c.GetParam(0));

	if (transport == nullptr) {
		StopMoveAndFinishCommand();
		return;
	}

	// prevent <owner> from chasing after full transports, etc
	if (!transport->unitDef->IsTransportUnit() || !transport->CanTransport(owner)) {
		StopMoveAndFinishCommand();
		return;
	}

	if (!inCommand) {
		inCommand = true;
		transport->commandAI->GiveCommandReal(Command(CMD_LOAD_UNITS, INTERNAL_ORDER | SHIFT_KEY, owner->id));
	}

	if (owner->GetTransporter() != nullptr) {
		assert(!commandQue.empty()); // <c> should still be in front
		StopMoveAndFinishCommand();
		return;
	}

	if ((owner->pos - transport->pos).SqLength2D() < cancelDistance) {
		StopMove();
	} else {
		SetGoal(transport->pos, owner->pos);
	}
}

/**
* @brief Executes the Patrol command c
*/
void CMobileCAI::ExecutePatrol(Command& c)
{
	assert(owner->unitDef->canPatrol);
	if (c.GetNumParams() < 3) {
		LOG_L(L_ERROR, "[MobileCAI::%s][f=%d][id=%d][#c.params=%d min=3]", __func__, gs->frameNum, owner->id, c.GetNumParams());
		return;
	}

	Command temp(CMD_FIGHT, c.GetOpts() | INTERNAL_ORDER, c.GetPos(0));

	commandQue.push_back(c);
	commandQue.pop_front();
	commandQue.push_front(temp);

	eoh->CommandFinished(*owner, Command(CMD_PATROL));
	ExecuteFight(temp);
}

/**
* @brief Executes the Fight command c
*/
void CMobileCAI::ExecuteFight(Command& c)
{
	assert(c.IsInternalOrder() || owner->unitDef->canFight);

	if (c.GetNumParams() == 1 && !owner->weapons.empty()) {
		CWeapon* w = owner->weapons.front();

		if ((orderTarget != nullptr) && !w->Attack(SWeaponTarget(orderTarget, false))) {
			CUnit* newTarget = CGameHelper::GetClosestValidTarget(owner->pos, owner->maxRange, owner->allyteam, this);

			if ((newTarget != nullptr) && w->Attack(SWeaponTarget(newTarget, false))) {
				c.SetParam(0, newTarget->id);

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
	if (c.GetNumParams() < 3) {
		LOG_L(L_ERROR, "[MobileCAI::%s][f=%d][id=%d][#c.params=%d min=3]", __func__, gs->frameNum, owner->id, c.GetNumParams());
		return;
	}
	if (c.GetNumParams() >= 6) {
		if (!inCommand)
			commandPos1 = c.GetPos(3);

	} else {
		// Some hackery to make sure the line (commandPos1,commandPos2) is NOT
		// rotated (only shortened) if we reach this because the previous return
		// fight command finished by the 'if((curPos-pos).SqLength2D()<(64*64))'
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
	if (c.GetNumParams() >= 6)
		cmdPos = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);

	if (owner->unitDef->canAttack && owner->fireState >= FIRESTATE_FIREATWILL && !owner->weapons.empty()) {
		const float3 curPosOnLine = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);

		const float leashRadius = 100.0f * owner->moveState * owner->moveState;
		const float searchRadius = owner->maxRange + leashRadius;

		CUnit* enemy = CGameHelper::GetClosestValidTarget(curPosOnLine, searchRadius, owner->allyteam, this);

		if (enemy != nullptr) {
			PushOrUpdateReturnFight();

			// make the attack-command inherit <c>'s options
			// NOTE: see AirCAI::ExecuteFight why we do not set INTERNAL_ORDER
			commandQue.push_front(Command(CMD_ATTACK, c.GetOpts(), enemy->id));

			inCommand = false;
			tempOrder = true;

			if (lastCommandFrame == gs->frameNum)
				return;

			lastCommandFrame = gs->frameNum;
			SlowUpdate();
			return;
		}
	}

	ExecuteMove(c);
}

bool CMobileCAI::IsValidTarget(const CUnit* enemy, CWeapon* weapon) const {
	if (enemy == nullptr)
		return false;

	if (enemy == owner)
		return false;

	if (owner->unitDef->noChaseCategory & enemy->category)
		return false;

	// don't _auto_ chase neutrals
	if (enemy->IsNeutral())
		return false;

	// test if given weapon belonging to owner can target
	// the enemy unit; indicates an auto-targeting context
	if (weapon != nullptr)
		return (weapon->TestTarget(enemy->pos, {enemy}) && (owner->moveState != MOVESTATE_HOLDPOS || weapon->TryTargetRotate(enemy, false, false)));

	// test if any of owner's weapons can target the enemy unit
	if (owner->weapons.empty())
		return false;

	for (CWeapon* w: owner->weapons) {
		if (w->TestTarget(enemy->pos, SWeaponTarget(enemy)) && (owner->moveState != MOVESTATE_HOLDPOS || w->TryTargetRotate(enemy, false, false)))
			return true;
	}

	return false;
}

/**
* @brief Executes the guard command c
*/
void CMobileCAI::ExecuteGuard(Command& c)
{
	assert(owner->unitDef->canGuard);
	assert(c.GetNumParams() != 0);

	const CUnit* guardee = unitHandler.GetUnit(c.GetParam(0));

	if (guardee == nullptr) {
		StopMoveAndFinishCommand();
		return;
	}
	if (UpdateTargetLostTimer(guardee->id) == 0) {
		StopMoveAndFinishCommand();
		return;
	}
	if (guardee->outOfMapTime > (GAME_SPEED * 5)) {
		StopMoveAndFinishCommand();
		return;
	}

	const bool pushAttackCommand =
		owner->unitDef->canAttack &&
		(guardee->lastAttackFrame + 40 < gs->frameNum) &&
		IsValidTarget(guardee->lastAttacker, nullptr);

	if (pushAttackCommand) {
		commandQue.push_front(Command(CMD_ATTACK, c.GetOpts(), guardee->lastAttacker->id));

		StopSlowGuard();
		SlowUpdate();
		return;
	}

	const float3 dif = (guardee->pos - owner->pos).SafeNormalize();
	const float3 goal = guardee->pos - dif * (guardee->radius + owner->radius + 64.0f);
	const bool resetGoal =
		((owner->moveType->goalPos - goal).SqLength2D() > 1600.0f) ||
		(owner->moveType->goalPos - owner->pos).SqLength2D() < Square(owner->moveType->GetMaxSpeed() * GAME_SPEED + 1 + SQUARE_SIZE * 2);

	if (resetGoal)
		SetGoal(goal, owner->pos);

	if ((goal - owner->pos).SqLength2D() < 6400.0f) {
		StartSlowGuard(guardee->moveType->GetMaxSpeed());

		if ((goal - owner->pos).SqLength2D() < 1800.0f) {
			StopMove();
			NonMoving();
		}

		return;
	}

	StopSlowGuard();
}


void CMobileCAI::ExecuteStop(Command& c)
{
	StopMove();
	return CCommandAI::ExecuteStop(c);
}




void CMobileCAI::ExecuteObjectAttack(Command& c)
{
	bool tryTargetRotate  = false;
	bool tryTargetHeading = false;

	float edgeFactor = 0.0f; // percent offset to target center

	const float3 targetMidPosVec = owner->midPos - orderTarget->midPos;
	const float3 targetErrPos = orderTarget->GetErrorPos(owner->allyteam, false);

	const float targetGoalDist = targetErrPos.SqDistance2D(owner->moveType->goalPos);
	const float targetPosDist = Square(10.0f + orderTarget->pos.distance2D(owner->pos) * 0.2f);
	const float minPointingDist = std::min(1.0f * owner->losRadius, owner->maxRange * 0.9f);

	// FIXME? targetMidPosMaxDist is 3D, but compared with a 2D value
	const float targetMidPosDist2D = targetMidPosVec.Length2D();
	// const float targetMidPosMaxDist = owner->maxRange - (Square(orderTarget->speed.w) / owner->unitDef->maxAcc);

	if (!owner->weapons.empty()) {
		if (!(c.GetOpts() & ALT_KEY) && SkipParalyzeTarget(orderTarget)) {
			StopMoveAndFinishCommand();
			return;
		}
	}

	// tell weapons about the ordered target-unit
	SWeaponTarget orderTgtInfo(orderTarget);
	orderTgtInfo.isUserTarget = (!c.IsInternalOrder());
	orderTgtInfo.isManualFire = (c.GetID() == CMD_MANUALFIRE);

	const short targetHeading = GetHeadingFromVector(-targetMidPosVec.x, -targetMidPosVec.z);

	assert(c.GetID() != CMD_MANUALFIRE || (!owner->weapons.empty() && owner->unitDef->canManualFire));

	for (CWeapon* w: owner->weapons) {
		if (c.GetID() == CMD_MANUALFIRE && !w->weaponDef->manualfire)
			continue;

		tryTargetRotate  = w->TryTargetRotate(orderTgtInfo.unit, orderTgtInfo.isUserTarget, orderTgtInfo.isManualFire);
		tryTargetHeading = w->TryTargetHeading(targetHeading, orderTgtInfo);

		edgeFactor = math::fabs(w->weaponDef->targetBorder);

		if (tryTargetRotate || tryTargetHeading)
			break;
	}

	// if w->AttackUnit() returned true then we are already
	// in range with our biggest (?) weapon, so stop moving
	// also make sure that we're not locked in close-in/in-range state
	// loop due to rotates invoked by in-range or out-of-range states
	if (tryTargetRotate) {
		const bool canChaseTarget = (owner->moveState != MOVESTATE_HOLDPOS);
		const bool targetBehind = (targetMidPosVec.dot(orderTarget->speed) < 0.0f);

		if (canChaseTarget && tryTargetHeading && targetBehind && !owner->unitDef->IsHoveringAirUnit()) {
			SetGoal(owner->pos + (orderTarget->speed * 80), owner->pos, SQUARE_SIZE, orderTarget->speed.w * 1.1f);
		} else {
			StopMove();

			if (gs->frameNum > (lastCloseInTry + MAX_CLOSE_IN_RETRY_TICKS))
				owner->moveType->KeepPointingTo(orderTarget->midPos, minPointingDist, true);
		}

		owner->AttackUnit(orderTgtInfo.unit, orderTgtInfo.isUserTarget, orderTgtInfo.isManualFire);
		return;
	}

	// if we are on hold-position in a *temporary* order, then none of the
	// close-in code below should run, and the attack command is cancelled
	// (units executing an *area* fight-command and set to hold-pos should
	// not chase any automatically assigned targets since that would allow
	// them to be baited)
	if (tempOrder && owner->moveState == MOVESTATE_HOLDPOS) {
		StopMoveAndFinishCommand();
		return;
	}

	// target is probably close enough
	if (targetMidPosDist2D < (owner->maxRange * 0.9f)) {
		if (owner->unitDef->IsHoveringAirUnit() || (targetMidPosVec.SqLength2D() < 1024)) {
			StopMove();
			owner->moveType->KeepPointingTo(orderTarget->midPos, minPointingDist, true);
			return;
		}

		// move sideways (i.e. strafe) to get a shot
		// note that the close-enough assumption is flawed:
		// unit may be aiming or otherwise unable to shoot
		if (owner->unitDef->strafeToAttack) {
			const int dirSign = Sign(int(moveDir ^= (owner->moveType->progressState == AMoveType::Failed)));

			constexpr float sin = 0.6f;
			constexpr float cos = 0.8f;

			float3 goalDiff;
			goalDiff.x = targetMidPosVec.dot(float3(cos          , 0.0f, -sin * dirSign));
			goalDiff.z = targetMidPosVec.dot(float3(sin * dirSign, 0.0f,  cos          ));
			goalDiff *= (targetMidPosDist2D < (owner->maxRange * 0.3f)) ? 1.0f / cos : cos;
			goalDiff += orderTarget->pos;

			SetGoal(goalDiff, owner->pos);
		}

		return;
	}

	// not a temporary order or not on hold-position; close in on target more
	assert(!tempOrder || owner->moveState != MOVESTATE_HOLDPOS);

	if (targetGoalDist > targetPosDist) {
		const float3 norm = (targetErrPos - owner->pos).Normalize();

		// if the target is outside LOS, this goes to its approximate position (errPos != pos)
		// otherwise it will move us to the exact target position which should fix issues with
		// low-range (mainly melee) weapons
		SetGoal(targetErrPos - norm * CalcTargetRadius(orderTarget, orderTarget->radius, edgeFactor * 0.8f), owner->pos);

		if (lastCloseInTry < (gs->frameNum + MAX_CLOSE_IN_RETRY_TICKS))
			lastCloseInTry = gs->frameNum;
	}
}

void CMobileCAI::ExecuteGroundAttack(Command& c)
{
	const float3 attackPos = c.GetPos(0);
	const float3 attackVec = attackPos - owner->pos;
	const short  attackHeading = GetHeadingFromVector(attackVec.x, attackVec.z);
	const SWeaponTarget attackTgtInfo(attackPos, !c.IsInternalOrder());

	if (c.GetID() == CMD_MANUALFIRE) {
		assert(owner->unitDef->canManualFire);

		if (owner->AttackGround(attackTgtInfo.groundPos, attackTgtInfo.isUserTarget, true)) {
			// StopMoveAndKeepPointing calls StopMove before KeepPointingTo
			// but we want to call it *after* KeepPointingTo to prevent 4131
			owner->moveType->KeepPointingTo(attackPos, owner->maxRange * 0.9f, true);
			StopMove();
		}

		return;
	}

	for (CWeapon* w: owner->weapons) {
		// NOTE:
		//   we call TryTargetHeading which is less restrictive than TryTarget
		//   (eg. the former succeeds even if the unit has not already aligned
		//   itself with <attackVec>)
		if (!w->TryTargetHeading(attackHeading, attackTgtInfo))
			continue;

		if (owner->AttackGround(attackTgtInfo.groundPos, attackTgtInfo.isUserTarget, false)) {
			StopMoveAndKeepPointing(attackTgtInfo.groundPos, owner->maxRange * 0.9f, true);
			return;
		}

		// for gunships, this pitches the nose down such that
		// TryTargetRotate (which also checks range for itself)
		// has a bigger chance of succeeding
		//
		// hence it must be called as soon as we get in range
		// and may not depend on what TryTargetRotate returns
		// (otherwise we might never get a firing solution)
		owner->moveType->KeepPointingTo(attackTgtInfo.groundPos, owner->maxRange * 0.9f, true);
	}

	if (attackVec.SqLength2D() >= Square(owner->maxRange * 0.9f))
		return;

	owner->AttackGround(attackTgtInfo.groundPos, attackTgtInfo.isUserTarget, false);
	StopMoveAndKeepPointing(attackPos, owner->maxRange * 0.9f, true);
}

void CMobileCAI::ExecuteAttack(Command& c)
{
	assert(owner->unitDef->canAttack);

	// limit how far away we fly based on our movestate
	if (tempOrder && orderTarget != nullptr) {
		const float3& closestPos = ClosestPointOnLine(commandPos1, commandPos2, owner->pos);

		const float curTargetDist = LinePointDist(closestPos, commandPos2, orderTarget->pos);
		const float maxTargetDist = (owner->moveType->GetManeuverLeash() * owner->moveState + owner->maxRange);

		if (owner->moveState < MOVESTATE_ROAM && curTargetDist > maxTargetDist) {
			StopMoveAndFinishCommand();
			return;
		}
	}

	if (!inCommand) {
		switch (c.GetNumParams()) {
			case 0: {
			} break;

			case 1: {
				CUnit* targetUnit = unitHandler.GetUnit(c.GetParam(0));

				// check if we have valid target parameter and that we aren't attacking ourselves
				if (targetUnit == nullptr) {
					StopMoveAndFinishCommand();
					return;
				}
				if (targetUnit == owner) {
					StopMoveAndFinishCommand();
					return;
				}
				if (targetUnit->GetTransporter() != nullptr && !modInfo.targetableTransportedUnits) {
					StopMoveAndFinishCommand();
					return;
				}

				const float3 tgtErrPos = targetUnit->GetErrorPos(owner->allyteam, false);
				const float3 tgtPosDir = (tgtErrPos - owner->pos).Normalize();

				// FIXME: don't call SetGoal() if target is already in range of some weapon?
				SetGoal(tgtErrPos - tgtPosDir * CalcTargetRadius(targetUnit, targetUnit->radius, 1.0f), owner->pos);
				SetOrderTarget(targetUnit);
				owner->AttackUnit(targetUnit, !c.IsInternalOrder(), c.GetID() == CMD_MANUALFIRE);

				inCommand = true;
			} break;

			case 2: {
			} break;

			default: {
				// user gave force-fire attack command
				SetGoal(c.GetPos(0), owner->pos);

				inCommand = true;
			} break;
		}
	}

	// if our target is dead or we lost it then stop attacking
	// NOTE: unit should actually just continue to target area!
	if (targetDied || (c.GetNumParams() == 1 && UpdateTargetLostTimer(int(c.GetParam(0))) == 0)) {
		// cancel keeppointingto
		StopMoveAndFinishCommand();
		return;
	}


	// user clicked on enemy unit; either stop and attack or turn/move toward it
	if (orderTarget != nullptr) {
		ExecuteObjectAttack(c);
		return;
	}

	// user wants to attack the ground
	if (c.GetNumParams() >= 3) {
		ExecuteGroundAttack(c);
		return;
	}
}




int CMobileCAI::GetDefaultCmd(const CUnit* pointed, const CFeature*)
{
	if (pointed == nullptr)
		return CMD_MOVE;

	if (!teamHandler.Ally(gu->myAllyTeam, pointed->allyteam)) {
		if (owner->unitDef->canAttack)
			return CMD_ATTACK;
		if (owner->CanTransport(pointed))
			return CMD_LOAD_UNITS;

		return CMD_MOVE;
	}

	if (owner->CanTransport(pointed))
		return CMD_LOAD_UNITS;
	if (pointed->CanTransport(owner))
		return CMD_LOAD_ONTO;
	if (owner->unitDef->canGuard)
		return CMD_GUARD;

	return CMD_MOVE;
}

void CMobileCAI::SetGoal(const float3& pos, const float3& /*curPos*/, float goalRadius)
{
	// check if owner already has a move order to this position with the same radius
	if (owner->moveType->IsMovingTowards(pos, goalRadius, true))
		return;

	// give new move order
	owner->moveType->StartMoving(pos, goalRadius);
}

void CMobileCAI::SetGoal(const float3& pos, const float3& /*curPos*/, float goalRadius, float speed)
{
	// check if owner already has a move order to this position with the same radius
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
		// AttachUnit call
		lastBuggerOffTime = gs->frameNum - BUGGER_OFF_TTL;
		return;
	}

	lastBuggerOffTime = gs->frameNum;
	// numNonMovingCalls = 0;

	buggerOffPos = pos;
	buggerOffRadius = radius + owner->radius;
}

void CMobileCAI::NonMoving()
{
	// wait one SlowUpdate for more commands to enter the queue
	// (so the bugger-off dir can be chosen more intelligently)
	if (!commandQue.empty() && (++numNonMovingCalls) <= 1)
		return;

	if (owner->UsingScriptMoveType())
		return;

	if (lastBuggerOffTime <= (gs->frameNum - BUGGER_OFF_TTL))
		return;

	if (((owner->pos - buggerOffPos) * XZVector).SqLength() >= Square(buggerOffRadius * 1.5f))
		return;

	float3 buggerVec;
	float3 buggerPos = -OnesVector;

	if (HasMoreMoveCommands()) {
		size_t i = 0;
		size_t j = 0;

		for (i =     0; (i < commandQue.size() && !commandQue[i].IsMoveCommand()); i++) {}
		for (j = i + 1; (j < commandQue.size() && !commandQue[j].IsMoveCommand()); j++) {}

		if (i < commandQue.size() && j < commandQue.size()) {
			buggerVec = commandQue[j].GetPos(0) - commandQue[i].GetPos(0);
			buggerPos = buggerOffPos + buggerVec.Normalize() * buggerOffRadius * 1.25f;
		}

		// check if buggerPos is (still) reachable; aircraft
		// (or all units) might want to ask the GBOM instead
		if (owner->moveDef != nullptr && !owner->moveDef->TestMoveSquare(nullptr, buggerPos, buggerVec))
			buggerPos = -OnesVector;
	}

	if (buggerPos.x == -1.0f) {
		// pick a random perimeter point and hope for the best
		for (int i = 0; i < 16 && !buggerPos.IsInMap(); i++) {
			buggerVec = gsRNG.NextVector2D();
			buggerPos = buggerOffPos + buggerVec.Normalize() * buggerOffRadius * 1.5f;
		}
	}

	Command c(CMD_MOVE, buggerPos);
	c.SetOpts(INTERNAL_ORDER);
	c.SetTimeOut(gs->frameNum + BUGGER_OFF_TTL);
	commandQue.push_front(c);

	numNonMovingCalls = 0;
}

void CMobileCAI::FinishCommand()
{
	SetTransportee(nullptr);

	if (!commandQue[0].IsInternalOrder())
		lastUserGoal = owner->pos;

	tempOrder = false;
	StopSlowGuard();
	CCommandAI::FinishCommand();

	if (owner->unitDef->IsTransportUnit()) {
		CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);

		if (am == nullptr || !commandQue.empty())
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
		NonMoving();
		return false;
	}
	if ((owner->pos - lastUserGoal).SqLength2D() <= (MAX_USERGOAL_TOLERANCE_DIST * MAX_USERGOAL_TOLERANCE_DIST)) {
		NonMoving();
		return false;
	}
	if (owner->unitDef->IsHoveringAirUnit()) {
		NonMoving();
		return false;
	}

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

	float  extraRadius = 200.0f * owner->moveState * owner->moveState;
	float searchRadius = owner->maxRange + extraRadius;

	int newAttackTargetId = -1;

	// pass bogus target-id and weapon-number s.t. script knows context and can set radius
	if (!eventHandler.AllowWeaponTarget(owner->id, -1, -1, 0, &searchRadius))
		return false;

	const SWeaponTarget& curTarget = owner->curTarget;

	const CUnit* tgt = nullptr;
	const CWeapon* wpn = owner->weapons[0];

	if (curTarget.type == Target_Unit) {
		//FIXME when is this the case (unit has target, but CAI doesn't !?)
		const float tgtDistSq = owner->pos.SqDistance2D((tgt = curTarget.unit)->pos);
		const float maxDistSq = Square(searchRadius);

		if (tgtDistSq < maxDistSq)
			if (eventHandler.AllowWeaponTarget(owner->id, tgt->id, wpn->weaponNum, wpn->weaponDef->id, nullptr))
				newAttackTargetId = tgt->id;
	} else {
		if ((tgt = owner->lastAttacker) != nullptr) {
			if (owner->pos.SqDistance2D(tgt->pos) < Square(searchRadius)) {
				const bool allowAttackerChase = !(owner->unitDef->noChaseCategory & tgt->category);
				const bool  keepAttackingLast = (gs->frameNum < (owner->lastAttackFrame + GAME_SPEED * 7));
				const bool allowAttackingLast = (allowAttackerChase && keepAttackingLast && eventHandler.AllowWeaponTarget(owner->id, tgt->id, wpn->weaponNum, wpn->weaponDef->id, nullptr));

				if (allowAttackingLast)
					newAttackTargetId = tgt->id;
			}
		}

		if (newAttackTargetId < 0 && owner->fireState >= FIRESTATE_FIREATWILL && (gs->frameNum >= lastIdleCheck + 10)) {
			// try getting target from weapons
			for (CWeapon* w: owner->weapons) {
				const SWeaponTarget& wTgt = w->GetCurrentTarget();

				// no current target, and nothing to auto-target
				if (!w->HaveTarget() && !w->AutoTarget())
					continue;
				// maybe a current target, but invalid type or category etc
				if (wTgt.type != Target_Unit || !IsValidTarget(wTgt.unit, w))
					continue;
				if (!eventHandler.AllowWeaponTarget(owner->id, wTgt.unit->id, w->weaponNum, w->weaponDef->id, nullptr))
					continue;

				newAttackTargetId = wTgt.unit->id;
				break;
			}

			// get target from wherever
			if (newAttackTargetId < 0) {
				tgt = CGameHelper::GetClosestValidTarget(owner->pos, searchRadius, owner->allyteam, this);

				if (tgt != nullptr && eventHandler.AllowWeaponTarget(owner->id, tgt->id, wpn->weaponNum, wpn->weaponDef->id, nullptr))
					newAttackTargetId = tgt->id;
			}
		}
	}

	if (newAttackTargetId < 0)
		return false;

	Command c(CMD_ATTACK, INTERNAL_ORDER, newAttackTargetId);
	c.SetTimeOut(gs->frameNum + GAME_SPEED * 5);
	commandQue.push_front(c);

	commandPos1 = owner->pos;
	commandPos2 = owner->pos;
	return true;
}

bool CMobileCAI::CanWeaponAutoTarget(const CWeapon* weapon) const {
	// check if the weapon actually targets the unit's order-target
	return (!tempOrder || weapon->GetCurrentTarget() != owner->curTarget);
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

	if (speed <= 0.0f)
		return;
	if (commandQue.empty())
		return;
	if (owner->moveType->GetMaxSpeed() < speed)
		return;

	// when guarding, temporarily adopt the maximum
	// (forward) speed of the guardee unit as our own
	// WANTED maximum
	owner->moveType->SetWantedMaxSpeed(speed);
}



void CMobileCAI::CalculateCancelDistance()
{
	// clamp it a bit because the units don't have to turn at max speed
	cancelDistance = Clamp(Square(owner->moveType->CalcStaticTurnRadius() + (SQUARE_SIZE << 1)), 1024.0f, 2048.0f);
}


/******************************************************************************/
/******************************************************************************/


void CMobileCAI::SetTransportee(CUnit* unit) {
	assert(unit == nullptr || owner->unitDef->IsTransportUnit());

	if (!owner->unitDef->IsTransportUnit())
		return;

	if (orderTarget != nullptr && orderTarget->loadingTransportId == owner->id)
		orderTarget->loadingTransportId = -1;

	SetOrderTarget(unit);

	if (unit == nullptr)
		return;

	CUnit* transport = (unit->loadingTransportId == -1) ? nullptr : unitHandler.GetUnitUnsafe(unit->loadingTransportId);

	// if no loading transport, then assign ourselves
	if (transport == nullptr) {
		unit->loadingTransportId = owner->id;
		return;
	}

	if (transport == owner)
		return;
	// let the closest transport be loadingTransportId, in case of multiple fighting transports
	if (transport->pos.SqDistance(unit->pos) <= owner->pos.SqDistance(unit->pos))
		return;

	unit->loadingTransportId = owner->id;
}


void CMobileCAI::ExecuteLoadUnits(Command& c)
{
	switch (c.GetNumParams()) {
		case 1: {
			// load single unit
			CUnit* unit = unitHandler.GetUnit(c.GetParam(0));

			if (unit == nullptr) {
				StopMoveAndFinishCommand();
				return;
			}

			if (c.IsInternalOrder()) {
				if (unit->commandAI->commandQue.empty()) {
					if (!LoadStillValid(unit)) {
						StopMoveAndFinishCommand();
						return;
					}
				} else {
					const Command& currentUnitCommand = unit->commandAI->commandQue[0];

					if ((currentUnitCommand.GetID() == CMD_LOAD_ONTO) && (currentUnitCommand.GetNumParams() == 1) && (int(currentUnitCommand.GetParam(0)) == owner->id)) {
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
				if (!owner->script->IsBusy())
					StopMoveAndFinishCommand();

				return;
			}

			if (!owner->CanTransport(unit) || !UpdateTargetLostTimer(int(c.GetParam(0)))) {
				StopMoveAndFinishCommand();
				return;
			}

			{
				SetTransportee(unit);

				CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);

				const float sqUnitDist = unit->pos.SqDistance2D(owner->pos);
				const float loadRadius = owner->unitDef->loadingRadius;

				const bool inLoadingRadius = (sqUnitDist <= Square(loadRadius));
				// subtract 1 square to account for PFS/GMT inaccuracy
				const bool outOfRange = (owner->moveType->goalPos.SqDistance2D(unit->pos) > Square(loadRadius - SQUARE_SIZE));
				const bool moveCloser = (!inLoadingRadius && (!owner->IsMoving() || (am != nullptr && am->aircraftState != AAirMoveType::AIRCRAFT_FLYING)));

				if (outOfRange || moveCloser)
					SetGoal(unit->pos, owner->pos, std::min(64.0f, loadRadius));

				if (inLoadingRadius) {
					float3 wantedPos = unit->pos;

					if (am != nullptr) {
						// handle air transports differently
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
						const bool isInRange = (owner->pos.SqDistance(wantedPos) < Square(AIRTRANSPORT_DOCKING_RADIUS));
						const bool isAligned = (std::abs(owner->heading - unit->heading) < AIRTRANSPORT_DOCKING_ANGLE);
						const bool isUpright = (owner->updir.dot(UpVector) > 0.995f);

						if (!eventHandler.AllowUnitTransportLoad(owner, unit, wantedPos, isInRange && isAligned && isUpright))
							return;

						am->SetAllowLanding(false);
						am->SetWantedAltitude(0.0f);

						owner->script->BeginTransport(unit);
						SetTransportee(nullptr);
						owner->AttachUnit(unit, owner->script->QueryTransport(unit));

						StopMoveAndFinishCommand();
					} else {
						if (!eventHandler.AllowUnitTransportLoad(owner, unit, wantedPos, true))
							return;

						inCommand = true;

						StopMove();
						owner->script->TransportPickup(unit);
					}

					return;
				}

				if (owner->moveType->progressState != AMoveType::Failed || sqUnitDist >= Square(200.0f))
					return;

				// if we're pretty close already but CGroundMoveType fails because it considers
				// the goal clogged (with the future passenger...), just try to move to the
				// point halfway between the transport and the passenger.
				SetGoal((unit->pos + owner->pos) * 0.5f, owner->pos);
			}
		} break;

		case 4: {
			// area-load, avoid infinite loops
			if (lastCommandFrame == gs->frameNum)
				return;

			lastCommandFrame = gs->frameNum;

			const float3 pos = c.GetPos(0);
			const float radius = c.GetParam(3);

			CUnit* unit = FindUnitToTransport(pos, radius);

			if (unit != nullptr && owner->CanTransport(unit)) {
				commandQue.push_front(Command(CMD_LOAD_UNITS, c.GetOpts() | INTERNAL_ORDER, unit->id));
				inCommand = false;

				SlowUpdate();
				return;
			}

			StopMoveAndFinishCommand();
			return;
		} break;
	}
}


void CMobileCAI::ExecuteUnloadUnits(Command& c)
{
	if (lastCommandFrame == gs->frameNum)
		return;

	lastCommandFrame = gs->frameNum;

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
		if (!owner->script->IsBusy())
			StopMoveAndFinishCommand();

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
	if (!CCommandAI::AllowedCommand(c, fromSynced))
		return false;

	if (!owner->unitDef->IsTransportUnit())
		return true;

	switch (c.GetID()) {
		case CMD_UNLOAD_UNIT:
		case CMD_UNLOAD_UNITS: {
			const auto& transportees = owner->transportedUnits;

			// allow unloading empty transports for easier setup of transport bridges
			if (transportees.empty())
				return true;

			if (c.GetNumParams() == 5) {
				if (fromSynced) {
					// point transported buildings (...) in their wanted direction after unloading
					for (const CUnit::TransportedUnit& tu: transportees) {
						tu.unit->buildFacing = std::abs(int(c.GetParam(4))) % NUM_FACINGS;
					}
				}
			}

			if (c.GetNumParams() >= 4) {
				// find unload positions for transportees (WHY can this run in unsynced context?)
				for (const CUnit::TransportedUnit& tu: transportees) {
					const CUnit* u = tu.unit;

					const float radius = (c.GetID() == CMD_UNLOAD_UNITS)? c.GetParam(3): 0.0f;
					const float spread = u->radius * owner->unitDef->unloadSpread;

					float3 foundPos;

					if (FindEmptySpot(u, c.GetPos(0), radius, spread, foundPos, fromSynced))
						return true;

					 // FIXME: support arbitrary unloading order for other unload types also
					if (owner->unitDef->transportUnloadMethod != UNLOAD_LAND)
						return false;
				}

				// no empty spot found for any transported unit
				return false;
			}

			break;
		}
	}

	return true;
}


bool CMobileCAI::FindEmptySpot(const CUnit* unloadee, const float3& center, float radius, float spread, float3& found, bool fromSynced)
{
	const UnitDef* unitDef = owner->unitDef;

	const float sqSpreadDiv = (spread * spread) / 100.0f;
	const float maxAttempts = Clamp(sqSpreadDiv, 100.0f, 1000.0f);

	// radius is the size of the command unloading-zone (e.g. dragged by player);
	// spread is the *minimum* distance between any pair of unloaded units which
	// also has to respect radius
	spread = Clamp(spread, 1.0f * SQUARE_SIZE, radius);

	// more attempts for larger unloading zones
	for (int a = 0; a < maxAttempts; ++a) {
		float3 delta;
		float3 pos = -OnesVector;

		const int bmax = std::max(10, a / 10);

		for (int b = 0; (b < bmax && !pos.IsInBounds()); ++b) {
			// uniform polar-coordinate sampling
			// FIXME:
			//   using a deterministic technique might be better, since it would allow an
			//   unload command to be tested for validity from unsynced (with predictable
			//   results)
			const float len = (fromSynced? gsRNG.NextFloat(): guRNG.NextFloat());
			const float ang = (fromSynced? gsRNG.NextFloat(): guRNG.NextFloat());
			const float  sr = math::sqrt(len);

			delta.x = sr * math::cos(ang * math::TWOPI) * radius;
			delta.z = sr * math::sin(ang * math::TWOPI) * radius;
			pos = center + delta;
		}

		if (!pos.IsInBounds())
			continue;

		// check slope and depth constraints; returns loading height in pos.y on success
		if (!owner->CanLoadUnloadAtPos(pos, unloadee, &pos.y))
			continue;

		// adjust to mid-position
		pos.y -= unloadee->radius;

		// passing spread as query-radius ensures units are spaced at least
		// this far apart, since <units> will be non-empty if pos is closer
		// to a previous unloadee than <spread> elmos
		QuadFieldQuery qfQuery;
		quadField.GetSolidsExact(qfQuery, pos, spread, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS);

		if (unitDef->IsAirUnit() && (qfQuery.solids->size() > 1 || (qfQuery.solids->size() == 1 && (*qfQuery.solids)[0] != owner)))
			continue;
		// ground-transports want the sampled position to be entirely free
		if (!unitDef->IsAirUnit() && !qfQuery.solids->empty())
			continue;

		found = pos;
		return true;
	}

	return false;
}


CUnit* CMobileCAI::FindUnitToTransport(float3 center, float radius)
{
	CUnit* bestUnit = nullptr;
	float bestDist = std::numeric_limits<float>::max();

	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, center, radius);

	for (CUnit* unit: *qfQuery.units) {
		const float dist = unit->pos.SqDistance2D(owner->pos);

		if (unit->loadingTransportId != -1 && unit->loadingTransportId != owner->id) {
			const CUnit* trans = unitHandler.GetUnitUnsafe(unit->loadingTransportId);

			// don't refuse to load a unit if an enemy transport is trying to at the same time
			if ((trans != nullptr) && teamHandler.AlliedTeams(owner->team, trans->team))
				continue;
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
	if (commandQue.size() < 2)
		return false;

	const Command& cmd = commandQue[1];

	// we are called from ExecuteLoadUnits only in the case that
	// that commandQue[0].id == CMD_LOAD_UNITS, so if the second
	// command is NOT an area- but a single-unit-loading command
	// (which has one parameter) then the first one will be valid
	// (ELU keeps pushing CMD_LOAD_UNITS as long as there are any
	// units to pick up)
	//
	if (cmd.GetID() != CMD_LOAD_UNITS || cmd.GetNumParams() != 4)
		return true;

	const float3& cmdPos = cmd.GetPos(0);

	// check slope and depth constraints
	if (!owner->CanLoadUnloadAtPos(cmdPos, unit))
		return false;

	return (unit->pos.SqDistance2D(cmdPos) <= Square(cmd.GetParam(3) * 2.0f));
}


bool CMobileCAI::SpotIsClear(float3 pos, CUnit* unloadee)
{
	// check slope and depth constraints
	if (!owner->CanLoadUnloadAtPos(pos, unloadee))
		return false;

	const float numSquares = math::ceil(unloadee->radius / SQUARE_SIZE);
	const float queryRadius = std::max(1.0f, numSquares) * SQUARE_SIZE;

	return (quadField.NoSolidsExact(pos, queryRadius, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS));
}


bool CMobileCAI::SpotIsClearIgnoreSelf(float3 pos, CUnit* unloadee)
{
	// check slope and depth constraints
	if (!owner->CanLoadUnloadAtPos(pos, unloadee))
		return false;

	const float numSquares = math::ceil(unloadee->radius / SQUARE_SIZE);
	const float queryRadius = std::max(1.0f, numSquares) * SQUARE_SIZE;

	QuadFieldQuery qfQuery;
	quadField.GetSolidsExact(qfQuery, pos, queryRadius, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS);

	for (auto objectsIt = qfQuery.solids->begin(); objectsIt != qfQuery.solids->end(); ++objectsIt) {
		// check if the queried objects are actually in the transport
		const auto pred = [&objectsIt](const CUnit::TransportedUnit& tu) { return (*objectsIt == tu.unit); };
		const auto iter = std::find_if(owner->transportedUnits.cbegin(), owner->transportedUnits.cend(), pred);

		if ((iter == owner->transportedUnits.cend()) && (*objectsIt != owner))
			return false;
	}

	return true;
}


bool CMobileCAI::FindEmptyDropSpots(float3 startpos, float3 endpos, std::vector<float3>& dropSpots)
{
	if (dynamic_cast<CHoverAirMoveType*>(owner->moveType) == nullptr)
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

		if (startpos.SqDistance(nextPos) > maxDist)
			break;

		nextPos.y = CGround::GetHeightAboveWater(nextPos.x, nextPos.z);

		// check landing spot is ok for landing on
		if (!SpotIsClear(nextPos, ti->unit))
			continue;

		dropSpots.push_back(nextPos);
		nextPos += gap;
		++ti;
	}

	return (!dropSpots.empty());
}


void CMobileCAI::UnloadUnits_Land(Command& c)
{
	const auto& transportees = owner->transportedUnits;
	const CUnit* transportee = nullptr;

	float3 unloadPos;

	for (const CUnit::TransportedUnit& tu: transportees) {
		const float3 pos = c.GetPos(0);

		const float radius = c.GetParam(3);
		const float spread = (tu.unit)->radius * owner->unitDef->unloadSpread;

		if (FindEmptySpot(tu.unit, pos, radius, spread, unloadPos)) {
			transportee = tu.unit;
			break;
		}
	}

	if (transportee != nullptr) {
		Command c2(CMD_UNLOAD_UNIT, c.GetOpts() | INTERNAL_ORDER, unloadPos);
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

	const bool canUnload = FindEmptyDropSpots(startingDropPos, startingDropPos + approachVector * std::max(16.0f, c.GetParam(3)), dropSpots);

	StopMoveAndFinishCommand();

	if (canUnload) {
		auto ti = transportees.begin();
		auto di = dropSpots.rbegin();

		for (; ti != transportees.end() && di != dropSpots.rend(); ++ti, ++di) {
			commandQue.push_front(Command(CMD_UNLOAD_UNIT, c.GetOpts() | INTERNAL_ORDER, *di));
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

	const float radius = c.GetParam(3);
	const float dist = std::max(64.0f, owner->unitDef->loadingRadius - radius);

	if (pos.SqDistance2D(owner->pos) > dist) {
		SetGoal(pos, owner->pos, dist);
		return;
	}

	const auto& transportees = owner->transportedUnits;
	const CUnit* transportee = transportees[0].unit;

	if (FindEmptySpot(transportee, pos, radius, transportee->radius * owner->unitDef->unloadSpread, found)) {
		commandQue.push_front(Command(CMD_UNLOAD_UNIT, c.GetOpts() | INTERNAL_ORDER, found));
		SlowUpdate();
		return;
	}

	StopMoveAndFinishCommand();
}


void CMobileCAI::UnloadLand(Command& c)
{
	// default unload
	CUnit* transportee = nullptr;
	CHoverAirMoveType* am = nullptr;

	float3 wantedPos = c.GetPos(0);

	const auto& transportees = owner->transportedUnits;

	SetGoal(wantedPos, owner->pos);

	if (c.GetNumParams() < 4) {
		// unload the first transportee
		transportee = transportees[0].unit;
	} else {
		const int unitID = c.GetParam(3);

		// unload a specific transportee
		for (const CUnit::TransportedUnit& tu: transportees) {
			CUnit* carried = tu.unit;

			if (unitID == carried->id) {
				transportee = carried;
				break;
			}
		}
		if (transportee == nullptr) {
			StopMoveAndFinishCommand();
			return;
		}
	}

	if (wantedPos.SqDistance2D(owner->pos) >= Square(owner->unitDef->loadingRadius * 0.9f))
		return;

	wantedPos.y = owner->GetTransporteeWantedHeight(wantedPos, transportee);

	if ((am = dynamic_cast<CHoverAirMoveType*>(owner->moveType)) == nullptr) {
		if (!eventHandler.AllowUnitTransportUnload(owner, transportee, wantedPos, true))
			return;

		inCommand = true;

		StopMove();
		owner->script->TransportDrop(transportee, wantedPos);
		return;
	}

	{
		// handle air transports differently
		SetGoal(wantedPos, owner->pos);

		am->SetWantedAltitude(wantedPos.y - CGround::GetHeightAboveWater(wantedPos.x, wantedPos.z));
		am->ForceHeading(owner->GetTransporteeWantedHeading(transportee));

		am->maxDrift = 1.0f;

		// FIXME: kill the hardcoded constants, use the command's radius
		// NOTE: 2D distance-check would mean units get dropped from air
		const bool isInRange = (owner->pos.SqDistance(wantedPos) < Square(AIRTRANSPORT_DOCKING_RADIUS));
		const bool isAligned = (std::abs(owner->heading - am->GetForcedHeading()) < AIRTRANSPORT_DOCKING_ANGLE);
		const bool isUpright = (owner->updir.dot(UpVector) > 0.99f);

		if (!eventHandler.AllowUnitTransportUnload(owner, transportee, wantedPos, isInRange && isAligned && isUpright))
			return;

		wantedPos.y -= transportee->radius;

		if (!SpotIsClearIgnoreSelf(wantedPos, transportee)) {
			// chosen spot is no longer clear to land, choose a new one
			// if a new spot cannot be found, don't unload at all
			float3 newWantedPos;

			if (FindEmptySpot(transportee, wantedPos, std::max(16.0f * SQUARE_SIZE, transportee->radius * 4.0f), transportee->radius, newWantedPos)) {
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
}


void CMobileCAI::UnloadDrop(Command& c)
{
	float3 pos = c.GetPos(0);

	// head towards goal
	// note that HoverAirMoveType must be modified to allow
	// non-stop movement through goals for this to work well
	if (owner->moveType->goalPos.SqDistance2D(pos) > Square(20.0f))
		SetGoal(pos, owner->pos);

	CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);
	CUnit* transportee = owner->transportedUnits[0].unit;

	if (am != nullptr) {
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
	// land, then release all units at once
	CUnit* transportee = nullptr;

	float3 wantedPos = c.GetPos(0);

	const auto& transportees = owner->transportedUnits;

	SetGoal(wantedPos, owner->pos);

	if (c.GetNumParams() < 4) {
		transportee = transportees[0].unit;
	} else {
		const int unitID = c.GetParam(3);

		for (const CUnit::TransportedUnit& tu: transportees) {
			CUnit* carried = tu.unit;

			if (unitID == carried->id) {
				transportee = carried;
				break;
			}
		}
		if (transportee == nullptr) {
			FinishCommand();
			return;
		}
	}

	if (wantedPos.SqDistance2D(owner->pos) < Square(owner->unitDef->loadingRadius * 0.9f)) {
		CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);
		wantedPos.y = owner->GetTransporteeWantedHeight(wantedPos, transportee);

		if (am != nullptr) {
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
