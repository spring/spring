/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CLASSICGROUNDMOVETYPE_H
#define CLASSICGROUNDMOVETYPE_H

#include "MoveType.h"
#include "Sim/Objects/SolidObject.h"

struct MoveData;

class CClassicGroundMoveType : public AMoveType
{
public:
	CClassicGroundMoveType(CUnit* owner);
	~CClassicGroundMoveType();

	bool Update();
	void SlowUpdate();

	void StartMoving(float3 pos, float goalRadius);
	void StartMoving(float3 pos, float goalRadius, float speed);
	void StopMoving(bool callScript = false, bool hardStop = false);

	void KeepPointingTo(float3 pos, float distance, bool aggressive);
	void KeepPointingTo(CUnit* unit, float distance, bool aggressive);

	bool CanApplyImpulse(const float3&);
	void SetMaxSpeed(float speed);
	void LeaveTransport();

	static void CreateLineTable();
	static void DeleteLineTable();

private:
	float BreakingDistance(float speed);
	float3 Here();

	void StartEngine();
	void StopEngine();

	void Arrived();
	void Fail();
	void CheckCollision();

	void ChangeSpeed();
	void ChangeHeading(short wantedHeading);

	void UpdateSkid();
	void UpdateControlledDrop();
	void CheckCollisionSkid();
	float GetFlyTime(float3 pos, float3 speed);
	void CalcSkidRot();

	void AdjustPosToWaterLine();
	bool UpdateDirectControl();
	void UpdateOwnerPos();

	bool OnSlope();
	void TestNewTerrainSquare();
	bool CheckGoalFeasability();
	void SetMainHeading();

	bool CheckColH(int x, int y1, int y2, float xmove, int squareTestX);
	bool CheckColV(int y, int x1, int x2, float zmove, int squareTestY);

	float3 ObstacleAvoidance(float3 desiredDir);
	float Distance2D(CSolidObject* object1, CSolidObject* object2, float marginal = 0.0f);

	void GetNewPath();
	void GetNextWayPoint();

private:
	SyncedFloat3 waypoint;
	SyncedFloat3 nextWaypoint;

	unsigned int pathId;
	float goalRadius;

	bool atGoal;
	bool haveFinalWaypoint;

	float turnRate;
	float accRate;
	float decRate;

	float skidRotSpeed;
	//float dropSpeed;
	//float dropHeight;

	float3 skidRotVector;
	float skidRotSpeed2;
	float skidRotPos2;

	float3 flatFrontDir;
	float3 mainHeadingPos;
	float3 lastGetPathPos;

	bool useMainHeading;
	short int deltaHeading;

	float wantedSpeed;
	float currentSpeed;
	float requestedSpeed;
	float deltaSpeed;
	float terrainSpeed;
	float currentDistanceToWaypoint;

	int etaWaypoint; /// by this time it really should have gotten there genereate new path otherwise
	int etaWaypoint2; /// by this time we get suspicious, check if goal is clogged if we are close

	int nextDeltaSpeedUpdate;
	int nextObstacleAvoidanceUpdate;
	unsigned int restartDelay;

	unsigned int pathFailures;
	unsigned int etaFailures; /// how many times we havent gotten to a waypoint in time
	unsigned int nonMovingFailures; /// how many times we have requested a path from the same place

	int moveSquareX;
	int moveSquareY;

	// number of grid-cells along each dimension; should be an odd number
	static const int LINETABLE_SIZE = 11;
	static std::vector<int2> lineTable[LINETABLE_SIZE][LINETABLE_SIZE];
};

#endif

