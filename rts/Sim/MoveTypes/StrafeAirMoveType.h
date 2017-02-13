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
	bool UpdateFlying(float wantedHeight, float engine);
	void UpdateLanding();
	void UpdateAirPhysics(
		float rudder,
		float aileron,
		float elevator,
		float engine,
		const float3& engineVector
	);
	void SetState(AircraftState state) override;
	void UpdateTakeOff();

	float3 FindLandingPos();

	void SetMaxSpeed(float speed) override;
	float BrakingDistance(float speed, float rate) const override;

	void KeepPointingTo(float3 pos, float distance, bool aggressive) override {}
	void StartMoving(float3 pos, float goalRadius) override;
	void StartMoving(float3 pos, float goalRadius, float speed) override;
	void StopMoving(bool callScript = false, bool hardStop = false) override;

	void Takeoff() override;

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
	// fighters abort dive toward target if within this distance and climb back to normal altitude
	float attackSafetyDistance;

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
