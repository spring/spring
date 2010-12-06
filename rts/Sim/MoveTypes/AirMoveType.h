/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AIR_MOVE_TYPE_H_
#define _AIR_MOVE_TYPE_H_

#include "AAirMoveType.h"
#include <vector>

/**
 * Air movement type definition
 */
class CAirMoveType : public AAirMoveType
{
	CR_DECLARE(CAirMoveType);
	CR_DECLARE_SUB(DrawLine);
	CR_DECLARE_SUB(RudderInfo);

public:

	CAirMoveType(CUnit* owner);
	~CAirMoveType();
	void Update();
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
	void ImpulseAdded();
	float3 FindLandingPos() const;
	void CheckForCollision();
	void DependentDied(CObject* o);
	void SetMaxSpeed(float speed);

	void KeepPointingTo(float3 pos, float distance, bool aggressive);
	void StartMoving(float3 pos, float goalRadius);
	void StartMoving(float3 pos, float goalRadius, float speed);
	void StopMoving();

	void Takeoff();
	bool IsFighter() const;

	int subState;

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
	float maxSpeed;
	float turnRadius;

	float maxAcc;
	float maxAileron;
	float maxElevator;
	float maxRudder;

	float inSupply;

	/// used while landing
	float mySide;
	float crashAileron;
	float crashElevator;
	float crashRudder;

	struct DrawLine {
		CR_DECLARE_STRUCT(DrawLine);
		float3 pos1, pos2;
		float3 color;
	};
	std::vector<DrawLine> lines;

	struct RudderInfo{
		CR_DECLARE_STRUCT(RudderInfo);
		float rotation;
	};

	RudderInfo rudder;
	RudderInfo elevator;
	RudderInfo aileronRight;
	RudderInfo aileronLeft;
	std::vector<RudderInfo*> rudders;

	float lastRudderUpdate;
	float lastRudderPos;
	float lastElevatorPos;
	float lastAileronPos;

	float inefficientAttackTime;
	/// used by fighters to turn away when closing in on ground targets
	float3 exitVector;

private:

	void HandleCollisions();
};

#endif // _AIR_MOVE_TYPE_H_
