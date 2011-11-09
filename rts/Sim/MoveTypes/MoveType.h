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

	virtual void StartMoving(float3 pos, float goalRadius) = 0;
	virtual void StartMoving(float3 pos, float goalRadius, float speed) = 0;
	virtual void KeepPointingTo(float3 pos, float distance, bool aggressive) = 0;
	virtual void KeepPointingTo(CUnit* unit, float distance, bool aggressive);
	virtual void StopMoving() = 0;
	virtual void ImpulseAdded(const float3&) {}
	virtual void ReservePad(CAirBaseHandler::LandingPad* lp);

	virtual void SetGoal(const float3& pos);
	virtual void SetMaxSpeed(float speed);
	virtual void SetWantedMaxSpeed(float speed);
	virtual void LeaveTransport() {}

	virtual bool Update() = 0;
	virtual void SlowUpdate();

	virtual bool IsSkidding() const { return false; }
	virtual bool IsFlying() const { return false; }
	virtual bool IsReversing() const { return false; }

	bool WantsRepair() const;
	bool WantsRefuel() const;

	CUnit* owner;

	float3 goalPos;
	float3 oldPos;             // owner position at last Update()
	float3 oldSlowUpdatePos;   // owner position at last SlowUpdate()

	float maxSpeed;
	float maxWantedSpeed;

	CAirBaseHandler::LandingPad* reservedPad;

	/// 0: moving toward, 1: landing at, 2: arrived
	int padStatus;
	float repairBelowHealth;

	/// TODO: probably should move the code in CUnit that reads this into the movement classes
	bool useHeading;

	enum ProgressState {
		Done   = 0,
		Active = 1,
		Failed = 2
	};
	ProgressState progressState;

	void DependentDied(CObject* o);
};

#endif // MOVETYPE_H
