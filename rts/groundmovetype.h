#ifndef GROUNDMOVETYPE_H
#define GROUNDMOVETYPE_H
/*pragma once removed*/
#include "MoveType.h"
#include "SolidObject.h"

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
	float3 nextWaypoint;
	int etaWaypoint;			//by this time it really should have gotten there genereate new path otherwise
	int etaWaypoint2;			//by this time we get suspicious, check if goal is clogged if we are close
	bool atGoal;
	bool haveFinalWaypoint;
	float terrainSpeed;

	float requestedSpeed;
	short requestedTurnRate;

	float currentDistanceToWaypoint;

	float3 avoidanceVec;

	unsigned int restartDelay;
	float3 lastGetPathPos;

	unsigned int pathFailures;
	unsigned int etaFailures;				//how many times we havent gotten to a waypoint in time
	unsigned int nonMovingFailures;	//how many times we have requested a path from the same place

	int moveType;

	bool floatOnWater;

	int moveSquareX;
	int moveSquareY;
protected:
	int nextDeltaSpeedUpdate;
	int nextObstacleAvoidanceUpdate;

	int lastTrackUpdate;

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

	static std::vector<int2> lineTable[11][11];
public:
	static void CreateLineTable(void);
	void TestNewTerrainSquare(void);
	bool CheckGoalFeasability(void);
	virtual void LeaveTransport(void);
};



#endif /* GROUNDMOVETYPE_H */
