// AirCAI.h: Air movement type definition
///////////////////////////////////////////////////////////////////////////

#ifndef __AIR_MOVE_TYPE_H__
#define __AIR_MOVE_TYPE_H__

#include "MoveType.h"
#include "Sim/Misc/AirBaseHandler.h"

class CPropeller;

class CAirMoveType :
	public CMoveType
{
public:
	enum AircraftState{
		AIRCRAFT_LANDED,
		AIRCRAFT_FLYING,
		AIRCRAFT_LANDING,
		AIRCRAFT_CRASHING,
		AIRCRAFT_TAKEOFF,
	} aircraftState;

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
	void UpdateAirPhysics(float rudder, float aileron, float elevator,float engine,const float3& engineVector);
	void SetState(AircraftState state);
	void UpdateTakeOff(float wantedHeight);
	void ImpulseAdded(void);
	float3 FindLandingPos(void);
	void CheckForCollision(void);
	void DependentDied(CObject* o);

	int subState;

	int maneuver;
	int maneuverSubState;

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
	float wantedHeight;

	float maxAcc;
	float maxAileron;
	float maxElevator;
	float maxRudder;

	float3 goalPos;

	float inSupply;

	float3 reservedLandingPos;

	float mySide;		//used while landing
	float crashAileron;
	float crashElevator;
	float crashRudder;

//	static float transMatrix[16];

	float3 oldpos;
	float3 oldGoalPos;
	float3 oldSlowUpdatePos;

	struct DrawLine {
		float3 pos1,pos2;
		float3 color;
	};
	std::vector<DrawLine> lines;
	std::vector<CPropeller*> myPropellers;

	struct RudderInfo{
		//CUnit3DLoader::UnitModel* model;
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
	float3 exitVector;							//used by fighters to turn away when closing in on ground targets

	CUnit* lastColWarning;		//unit found to be dangerously close to our path
	int lastColWarningType;		//1=generally forward of us,2=directly in path

	float repairBelowHealth;
	CAirBaseHandler::LandingPad* reservedPad;
	int padStatus;						//0 flying toward,1 landing at,2 landed
};

#endif // __AIR_MOVE_TYPE_H__
