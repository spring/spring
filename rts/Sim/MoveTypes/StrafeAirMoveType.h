/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STRAFE_AIR_MOVE_TYPE_H_
#define STRAFE_AIR_MOVE_TYPE_H_

#include "AAirMoveType.h"

struct float4;

/**
 * Air movement type definition
 */
class CStrafeAirMoveType: public AAirMoveType
{
	CR_DECLARE_DERIVED(CStrafeAirMoveType)
	CR_DECLARE_SUB(DrawLine)

public:
	enum {
		MANEUVER_FLY_STRAIGHT = 0,
		MANEUVER_IMMELMAN     = 1,
		MANEUVER_IMMELMAN_INV = 2,
	};

	CStrafeAirMoveType(CUnit* owner);

	bool Update() override;
	void SlowUpdate() override;

	bool SetMemberValue(unsigned int memberHash, void* memberValue) override;

	void UpdateManeuver();
	void UpdateAttack();
	bool UpdateFlying(float wantedHeight, float wantedThrottle);
	void UpdateLanding();
	void UpdateAirPhysics(const float4& controlInputs, const float3& thrustVector);
	void SetState(AircraftState state) override;
	void UpdateTakeOff();

	float3 FindLandingPos(float3 landPos);

	void SetMaxSpeed(float speed) override;
	float BrakingDistance(float speed, float rate) const override;

	void KeepPointingTo(float3 pos, float distance, bool aggressive) override {}
	void StartMoving(float3 pos, float goalRadius) override;
	void StartMoving(float3 pos, float goalRadius, float speed) override;
	void StopMoving(bool callScript = false, bool hardStop = false, bool cancelRaw = false) override;

	void Takeoff() override;

private:
	bool HandleCollisions(bool checkCollisions);

public:
	int maneuverBlockTime = 0;
	int maneuverState = MANEUVER_FLY_STRAIGHT;
	int maneuverSubState = 0;

	bool loopbackAttack = false;
	bool isFighter = false;

	float wingDrag = 0.07f;
	float wingAngle = 0.1f;
	float invDrag = 0.995f;
	/// actually the invDrag of crashDrag
	float crashDrag = 0.995f;

	float frontToSpeed = 0.04f;
	float speedToFront = 0.01f;
	float myGravity = 0.8f;

	float maxBank = 0.55f;
	float maxPitch = 0.35f;
	float turnRadius = 150.0f;

	float maxAileron = 0.04f;
	float maxElevator = 0.02f;
	float maxRudder = 0.01f;
	// fighters abort dive toward target if within this distance and climb back to normal altitude
	float attackSafetyDistance = 0.0f;

	/// used while landing
	float crashAileron = 0.0f;
	float crashElevator = 0.0f;
	float crashRudder = 0.0f;

	float lastRudderPos[2] = {0.0f, 0.0f};
	float lastElevatorPos[2] = {0.0f, 0.0f};
	float lastAileronPos[2] = {0.0f, 0.0f};
};

#endif // _AIR_MOVE_TYPE_H_
