#pragma once
#include "unit.h"

class CPropeller;

class CAircraft :
	public CUnit
{
public:
	CAircraft(float3 &pos,int team);
	virtual ~CAircraft(void);

	enum AircraftState{
		AIRCRAFT_LANDED,
		AIRCRAFT_FLYING,
		AIRCRAFT_LANDING,
		AIRCRAFT_CRASHING,
	} aircraftState;
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

	float3 basePos;
	float3 baseDir;
	bool bingoFuel;

	float mySide;		//used while landing
	float crashAileron;
	float crashElevator;
	float crashRudder;

	static float transMatrix[16];

	float3 oldpos;
	float3 oldGoalPos;

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

	void Update(void);
	void SlowUpdate(void);
	void Draw(void);
	void UpdateAirPhysics(float rudder, float aileron, float elevator,float engine);
	void UpdateFlying(float wantedHeight,float engine);
	bool TestBingoFuel(float supplyDif);
	void UpdateLanded(void);
	void UpdateLanding(void);
	void SetState(AircraftState state);

	void UpdateAttack(void);
	void Init(void);
	void UpdateFighterAttack(void);
	void UpdateManeuver(void);
	void KillUnit(bool SelfDestruct);
};
