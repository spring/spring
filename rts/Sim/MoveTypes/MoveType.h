/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MOVETYPE_H
#define MOVETYPE_H

#include "System/creg/creg_cond.h"
#include "System/Object.h"
#include "System/float3.h"
#include "System/SpringMath.h"
#include "Sim/Misc/GlobalConstants.h"

#include <algorithm>

class CUnit;

class AMoveType : public CObject
{
	CR_DECLARE(AMoveType)

public:
	AMoveType(CUnit* owner);
	virtual ~AMoveType() {}

	virtual void StartMovingRaw(const float3 moveGoalPos, float moveGoalRadius) {}
	virtual void StartMoving(float3 pos, float goalRadius) = 0;
	virtual void StartMoving(float3 pos, float goalRadius, float speed) = 0;
	virtual void KeepPointingTo(float3 pos, float distance, bool aggressive) = 0;
	virtual void KeepPointingTo(CUnit* unit, float distance, bool aggressive);
	virtual void StopMoving(bool callScript = false, bool hardStop = false, bool cancelRaw = false) = 0;
	virtual bool CanApplyImpulse(const float3&) { return false; }
	virtual void LeaveTransport() {}

	// generic setter for Lua-writable values
	virtual bool SetMemberValue(unsigned int memberHash, void* memberValue);

	virtual void SetGoal(const float3& pos, float distance = 0.0f) { goalPos = pos; }
	virtual bool IsMovingTowards(const float3& pos, float, bool) const { return (pos == goalPos && progressState == Active); }
	virtual bool IsAtGoalPos(const float3& pos, float radius) const { return (pos.SqDistance2D(goalPos) < (radius * radius)); }

	// NOTE:
	//   SetMaxSpeed is ONLY called by LuaSyncedMoveCtrl now
	//   other code (CommandAI) modifies a unit's speed only
	//   through Set*Wanted*MaxSpeed
	// NOTE:
	//   clamped because too much code in the derived
	//   MoveType classes expects maxSpeed to be != 0
	virtual void SetMaxSpeed(float speed) { maxSpeed = std::max(0.001f, speed); }
	virtual void SetWantedMaxSpeed(float speed) { maxWantedSpeed = speed; }
	virtual void SetManeuverLeash(float leashLength) { maneuverLeash = leashLength; }
	virtual void SetWaterline(float depth) { waterline = depth; }

	virtual bool Update() = 0;
	virtual void SlowUpdate();

	virtual bool IsSkidding() const { return false; }
	virtual bool IsFlying() const { return false; }
	virtual bool IsReversing() const { return false; }
	virtual bool IsPushResistant() const { return false; }

	bool UseHeading(      ) const { return (useHeading    ); }
	bool UseHeading(bool b)       { return (useHeading = b); }

	bool UseWantedSpeed(bool groupOrder) const { return useWantedSpeed[groupOrder]; }

	float GetMaxSpeed() const { return maxSpeed; }
	float GetMaxSpeedDef() const { return maxSpeedDef; }
	float GetMaxWantedSpeed() const { return maxWantedSpeed; }
	float GetManeuverLeash() const { return maneuverLeash; }
	float GetWaterline() const { return waterline; }

	virtual float GetGoalRadius(float s = 0.0f) const { return SQUARE_SIZE; }
	// The distance the unit will move before stopping,
	// starting from given speed and applying maximum
	// brake rate.
	virtual float BrakingDistance(float speed, float rate) const {
		const float time = speed / std::max(rate, 0.001f);
		const float dist = 0.5f * rate * time * time;
		return dist;
	}

	float CalcScriptMoveRate(float speed, float nsteps) const { return Clamp(math::floor((speed / maxSpeed) * nsteps), 0.0f, nsteps - 1.0f); }
	float CalcStaticTurnRadius() const;

public:
	CUnit* owner;

	float3 goalPos;
	float3 oldPos;                          // owner position at last Update()
	float3 oldSlowUpdatePos;                // owner position at last SlowUpdate()

	enum ProgressState {
		Done   = 0,
		Active = 1,
		Failed = 2
	};
	ProgressState progressState = Done;

protected:
	float maxSpeed;                         // current maximum speed owner is allowed to reach (changes with eg. guard orders)
	float maxSpeedDef;                      // default maximum speed owner can reach (as defined by its UnitDef, never changes)
	float maxWantedSpeed;                   // maximum speed (temporarily) set by a CommandAI

	float maneuverLeash;                    // maximum distance before target stops being chased
	float waterline;

	bool useHeading = true;
	bool useWantedSpeed[2] = {true, true};  // if false, SelUnitsAI will not (re)set wanted-speed for {[0] := individual, [1] := formation} orders
};

#endif // MOVETYPE_H
