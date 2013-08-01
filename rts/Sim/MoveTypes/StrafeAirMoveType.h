/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AIR_MOVE_TYPE_H_
#define _AIR_MOVE_TYPE_H_

#include "AAirMoveType.h"
#include <vector>

/**
 * Air movement type definition
 */
class CStrafeAirMoveType: public AAirMoveType
{
	CR_DECLARE(CStrafeAirMoveType);
	CR_DECLARE_SUB(DrawLine);

public:
	CStrafeAirMoveType(CUnit* owner);

	bool Update();
	void SlowUpdate();

	void UpdateManeuver();
	void UpdateFighterAttack();
	void UpdateAttack();
	void UpdateFlying(float wantedHeight, float engine);
	void UpdateLanded();
	void UpdateLanding();
	void UpdateAirPhysics(float rudder, float aileron, float elevator,
			float engine, const float3& engineVector);
	void SetState(AircraftState state);
	void UpdateTakeOff(float wantedHeight);

	float3 FindLandingPos() const;

	void SetMaxSpeed(float speed);

	void KeepPointingTo(float3 pos, float distance, bool aggressive) {}
	void StartMoving(float3 pos, float goalRadius);
	void StartMoving(float3 pos, float goalRadius, float speed);
	void StopMoving(bool callScript = false, bool hardStop = false);

	void Takeoff();
	bool IsFighter() const { return isFighter; }

	int maneuver;
	int maneuverSubState;

	bool loopbackAttack;
	bool isFighter;

	float wingDrag;
	float wingAngle;
	float invDrag;
	/// actually the invDrag of crashDrag
	float crashDrag;
	float frontToSpeed;
	float speedToFront;
	float myGravity;

	float maxBank;
	float maxPitch;
	float turnRadius;

	float maxAcc;
	float maxAileron;
	float maxElevator;
	float maxRudder;

	/// used while landing
	float crashAileron;
	float crashElevator;
	float crashRudder;

	float lastRudderPos;
	float lastElevatorPos;
	float lastAileronPos;

	float inefficientAttackTime;
	/// used by fighters to turn away when closing in on ground targets
	float3 exitVector;

private:
	bool HandleCollisions();
};

#endif // _AIR_MOVE_TYPE_H_
