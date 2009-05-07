#ifndef __AIR_MOVE_TYPE_H__
#define __AIR_MOVE_TYPE_H__

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
	~CAirMoveType(void);
	virtual void Update();
	virtual void SlowUpdate();

	void UpdateManeuver();
	void UpdateFighterAttack();
	void UpdateAttack();
	void UpdateFlying(float wantedHeight,float engine);
	void UpdateLanded();
	void UpdateLanding();
	void UpdateAirPhysics(float rudder, float aileron, float elevator,
			float engine,const float3& engineVector);
	void SetState(AircraftState state);
	void UpdateTakeOff(float wantedHeight);
	void ImpulseAdded(void);
	float3 FindLandingPos(void);
	void CheckForCollision(void);
	void DependentDied(CObject* o);
	void SetMaxSpeed(float speed);

	void KeepPointingTo(float3 pos, float distance, bool aggressive);
	void StartMoving(float3 pos, float goalRadius);
	void StartMoving(float3 pos, float goalRadius, float speed);
	void StopMoving();

	void Takeoff();
	bool IsFighter();

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

	float3 oldSlowUpdatePos;

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
};

#endif // __AIR_MOVE_TYPE_H__
