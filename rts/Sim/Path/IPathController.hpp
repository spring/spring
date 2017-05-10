/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#ifndef PATH_CONTROLLER_H
#define PATH_CONTROLLER_H

#include "System/float3.h"

class CUnit;
class CFeature;
struct MoveDef;

// provides control inputs for a ground-unit following a path
//
// inputs are not required to obey a unit's "physical" limits
// (but doing so is a good idea because they are NOT clamped
// before being applied)
//
// TODO: for crowds, track units with the same move order
class IPathController {
protected:
	IPathController(CUnit* owner_): owner(owner_) {}
	virtual ~IPathController() {}

public:
	// if a unit has a path to follow (and is not stunned,
	// being built, etc) this gets called every sim-frame
	virtual float GetDeltaSpeed(
		unsigned int pathID,
		float targetSpeed,  // max. speed <owner> is ALLOWED to be moving at
		float currentSpeed, // speed <owner> is currently moving at
		float maxAccRate,
		float maxDecRate,
		bool wantReverse,
		bool isReversing
	) const = 0;

	// if a unit has a path to follow (and is not stunned,
	// being built, etc) this gets called every sim-frame
	#if 1
	virtual short GetDeltaHeading(
		unsigned int pathID,
		short newHeading,
		short oldHeading,
		float maxTurnRate
	) const = 0;
	#endif

	virtual short GetDeltaHeading(
		unsigned int pathID,
		short newHeading,
		short oldHeading,
		float maxTurnSpeed,
		float maxTurnAccel,
		float turnBrakeDist,
		float* curTurnSpeed
	) const = 0;

	virtual bool AllowSetTempGoalPosition(unsigned int pathID, const float3& pos) const = 0;
	virtual void SetTempGoalPosition(unsigned int pathID, const float3& pos) = 0;
	virtual void SetRealGoalPosition(unsigned int pathID, const float3& pos) = 0;

	virtual bool IgnoreTerrain(const MoveDef& md, const float3& pos) const = 0;
	// if these return true, the respective collisions are NOT handled
	virtual bool IgnoreCollision(const CUnit* collider, const CUnit* collidee) const = 0;
	virtual bool IgnoreCollision(const CUnit* collider, const CFeature* collidee) const = 0;

protected:
	CUnit* owner;
};


class GMTDefaultPathController: public IPathController {
public:
	GMTDefaultPathController(CUnit* owner_): IPathController(owner_) {}

	float GetDeltaSpeed(
		unsigned int pathID,
		float targetSpeed,
		float currentSpeed,
		float maxAccRate,
		float maxDecRate,
		bool wantReverse,
		bool isReversing
	) const;

	#if 1
	short GetDeltaHeading(
		unsigned int pathID,
		short newHeading,
		short oldHeading,
		float maxTurnRate
	) const;
	#endif

	short GetDeltaHeading(
		unsigned int pathID,
		short newHeading,
		short oldHeading,
		float maxTurnSpeed,
		float maxTurnAccel,
		float turnBrakeDist,
		float* curTurnSpeed
	) const;

	bool AllowSetTempGoalPosition(unsigned int pathID, const float3& pos) const { return true; }
	void SetTempGoalPosition(unsigned int pathID, const float3& pos) { realGoalPos = pos; }
	void SetRealGoalPosition(unsigned int pathID, const float3& pos) { tempGoalPos = pos; }

	bool IgnoreTerrain(const MoveDef& md, const float3& pos) const;
	bool IgnoreCollision(const CUnit* collider, const CUnit* collidee) const { return false; }
	bool IgnoreCollision(const CUnit* collider, const CFeature* collidee) const { return false; }

private:
	float3 realGoalPos; // where <owner> ultimately wants to go (its final waypoint)
	float3 tempGoalPos; // where <owner> currently wants to go (its next waypoint)
};

#endif

