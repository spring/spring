/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STRAFE_AIR_MOVE_TYPE_H_
#define STRAFE_AIR_MOVE_TYPE_H_

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
	enum {
		MANEUVER_FLY_STRAIGHT = 0,
		MANEUVER_IMMELMAN     = 1,
		MANEUVER_IMMELMAN_INV = 2,
	};

	CStrafeAirMoveType(CUnit* owner);

	bool Update();
	void SlowUpdate();

	bool SetMemberValue(unsigned int memberHash, void* memberValue);

	void UpdateManeuver();
	void UpdateAttack();
	bool UpdateFlying(float wantedHeight, float engine);
	void UpdateLanded();
	void UpdateLanding();
	void UpdateAirPhysics(
		float rudder,
		float aileron,
		float elevator,
		float engine,
		const float3& engineVector
	);
	void SetState(AircraftState state);
	void UpdateTakeOff(float wantedHeight);

	float3 FindLandingPos(float brakeRate) const;

	void SetMaxSpeed(float speed);

	void KeepPointingTo(float3 pos, float distance, bool aggressive) {}
	void StartMoving(float3 pos, float goalRadius);
	void StartMoving(float3 pos, float goalRadius, float speed);
	void StopMoving(bool callScript = false, bool hardStop = false);

	void Takeoff();

	int maneuverState;
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

private:
	bool HandleCollisions(bool checkCollisions);
};

#endif // _AIR_MOVE_TYPE_H_
