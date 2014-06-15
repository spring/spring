/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOVETYPE_H
#define MOVETYPE_H

#include "System/creg/creg_cond.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "System/Object.h"
#include "System/float3.h"

class CUnit;

class AMoveType : public CObject
{
	CR_DECLARE(AMoveType);

public:
	AMoveType(CUnit* owner);
	virtual ~AMoveType() {}

	virtual void StartMovingRaw(const float3 moveGoalPos, float moveGoalRadius) {}
	virtual void StartMoving(float3 pos, float goalRadius) = 0;
	virtual void StartMoving(float3 pos, float goalRadius, float speed) = 0;
	virtual void KeepPointingTo(float3 pos, float distance, bool aggressive) = 0;
	virtual void KeepPointingTo(CUnit* unit, float distance, bool aggressive);
	virtual void StopMoving(bool callScript = false, bool hardStop = false) = 0;
	virtual bool CanApplyImpulse(const float3&) { return false; }
	virtual void LeaveTransport() {}

	// generic setter for Lua-writable values
	virtual bool SetMemberValue(unsigned int memberHash, void* memberValue);

	virtual void SetGoal(const float3& pos, float distance = 0.0f) { goalPos = pos; }

	// NOTE:
	//     SetMaxSpeed is ONLY called by LuaSyncedMoveCtrl now
	//     other code (CommandAI) modifies a unit's speed only
	//     through SetMaxWantedSpeed, via SET_WANTED_MAX_SPEED
	//     commands
	// NOTE:
	//     clamped because too much code in the derived
	//     MoveType classes expects maxSpeed to be != 0
	virtual void SetMaxSpeed(float speed) { maxSpeed = std::max(0.001f, speed); }
	virtual void SetWantedMaxSpeed(float speed) { maxWantedSpeed = speed; }

	virtual bool Update() = 0;
	virtual void SlowUpdate();

	virtual bool IsSkidding() const { return false; }
	virtual bool IsFlying() const { return false; }
	virtual bool IsReversing() const { return false; }

	virtual void ReservePad(CAirBaseHandler::LandingPad* lp) { /* AAirMoveType only */ }
	virtual void UnreservePad(CAirBaseHandler::LandingPad* lp) { /* AAirMoveType only */ }
	virtual CAirBaseHandler::LandingPad* GetReservedPad() { return NULL; }

	bool WantsRepair() const;
	bool WantsRefuel() const;

	void SetRepairBelowHealth(float rbHealth) { repairBelowHealth = rbHealth; }

	float GetMaxSpeed() const { return maxSpeed; }
	float GetMaxSpeedDef() const { return maxSpeedDef; }
	float GetMaxWantedSpeed() const { return maxWantedSpeed; }
	float GetRepairBelowHealth() const { return repairBelowHealth; }

public:
	CUnit* owner;

	float3 goalPos;
	float3 oldPos;             // owner position at last Update()
	float3 oldSlowUpdatePos;   // owner position at last SlowUpdate()

	/// TODO: probably should move the code in CUnit that reads this into the movement classes
	bool useHeading;

	enum ProgressState {
		Done   = 0,
		Active = 1,
		Failed = 2
	};
	ProgressState progressState;

protected:
	float maxSpeed;            // current maximum speed owner is allowed to reach (changes with eg. guard orders)
	float maxSpeedDef;         // default maximum speed owner can reach (as defined by its UnitDef, never changes)
	float maxWantedSpeed;      // maximum speed (temporarily) set by a CMD_SET_WANTED_MAX_SPEED modifier command

	float repairBelowHealth;
};

#endif // MOVETYPE_H
