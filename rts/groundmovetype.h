#pragma once
#include "movetype.h"
#include "solidobject.h"

struct MoveData;

class CGroundMoveType :
	public CMoveType
{
public:
	CGroundMoveType(CUnit* owner);
	~CGroundMoveType(void);

	void Update();
	void SlowUpdate();

//	void SetGoal(float3 pos);
//	void SetWantedSpeed(float speed);
	
	void SetDeltaSpeed(void);
	bool TestNewPathGoal(const float3& newgoal);

	void StartMoving(float3 pos, float goalRadius);
	void StartMoving(float3 pos, float goalRadius, float speed);
	void StopMoving();

	void ImpulseAdded(void);

	//float baseSpeed;		//Not used
	//float maxSpeed;		//Moved to CMoveType, by Lars 04-08-23
	float baseTurnRate;
	float turnRate;
	float accRate;

	float wantedSpeed;
	float currentSpeed;
	float deltaSpeed;
	short int deltaHeading;
	
	float3 oldPos;
	float3 oldSlowUpdatePos;
	float3 flatFrontDir;

	unsigned int pathId;
	float3 goal;
	float goalRadius;

	float3 waypoint;
	int etaWaypoint;
	bool atWaypoint;
	bool waypointIsBlocked;
	bool haveFinalWaypoint;

	float requestedSpeed;
	short requestedTurnRate;

	float currentDistanceToWaypoint;

	float3 avoidanceVec;

	unsigned int restartDelay;
	float3 lastGetPathPos;

	unsigned int getPathFailures;		//how many times the pathfinder has said it cant find a path
	unsigned int pathFailures;
	unsigned int speedFailures;			//how many times the terrain has slowed us down to much
	unsigned int etaFailures;				//how many times we havent gotten to a waypoint in time
	unsigned int nonMovingFailures;	//how many times we have requested a path from the same place
	unsigned int obstacleFailures;
	unsigned int blockingFailures;

	int moveType;

	bool floatOnWater;

	int moveSquareX;
	int moveSquareY;
protected:
	int nextDeltaSpeedUpdate;
	int nextObstacleAvoidanceUpdate;

	float3 ObstacleAvoidance(float3 desiredDir);
	float Distance2D(CSolidObject *object1, CSolidObject *object2, float marginal = 0.0l);

	void GetNewPath();
	void GetNextWaypoint();

	float BreakingDistance(float speed);
	float3 Here();

	float MinDistanceToWaypoint();
	float MaxDistanceToWaypoint();

	void StartEngine();
	void StopEngine();
	
	void Arrived();
	void Fail();
	void CheckCollision(void);

	void ChangeHeading(short wantedHeading);

	void UpdateSkid(void);
	void CheckCollisionSkid(void);
	float GetFlyTime(float3 pos, float3 speed);
	void CalcSkidRot(void);

	bool skidding;
	bool flying;
	float skidRotSpeed;

	float3 skidRotVector;
	float skidRotSpeed2;
	float skidRotPos2;
	CSolidObject::PhysicalState oldPhysState;

	bool CheckColH(int x, int y1, int y2, float xmove, int squareTestX);
	bool CheckColV(int y, int x1, int x2, float zmove, int squareTestY);
};


