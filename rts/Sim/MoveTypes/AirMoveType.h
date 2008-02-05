// AirCAI.h: Air movement type definition
///////////////////////////////////////////////////////////////////////////

#ifndef __AIR_MOVE_TYPE_H__
#define __AIR_MOVE_TYPE_H__

#include "AAirMoveType.h"

class CAirMoveType :
	public AAirMoveType
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

	int subState;

	int maneuver;
	int maneuverSubState;

	bool loopbackAttack;
	bool isFighter;

	float wingDrag;
	float wingAngle;
	float invDrag;
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


	float mySide;		//used while landing
	float crashAileron;
	float crashElevator;
	float crashRudder;

	float3 oldSlowUpdatePos;

	struct DrawLine {
		CR_DECLARE_STRUCT(DrawLine);
		float3 pos1,pos2;
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
	vector<RudderInfo*> rudders;

	float lastRudderUpdate;
	float lastRudderPos;
	float lastElevatorPos;
	float lastAileronPos;
	
	float inefficientAttackTime;
	float3 exitVector;	//used by fighters to turn away when closing in on ground targets
};

#endif // __AIR_MOVE_TYPE_H__
