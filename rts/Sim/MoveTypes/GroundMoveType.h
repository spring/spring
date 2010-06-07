/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUNDMOVETYPE_H
#define GROUNDMOVETYPE_H

#include "MoveType.h"
#include "Sim/Objects/SolidObject.h"

struct MoveData;

class CGroundMoveType : public AMoveType
{
	CR_DECLARE(CGroundMoveType);

public:
	CGroundMoveType(CUnit* owner);
	~CGroundMoveType(void);

	void PostLoad();

	void Update();
	void SlowUpdate();

	void SetDeltaSpeed(bool);

	void StartMoving(float3 pos, float goalRadius);
	void StartMoving(float3 pos, float goalRadius, float speed);
	void StopMoving();

	virtual void SetMaxSpeed(float speed);

	void ImpulseAdded(void);

	void KeepPointingTo(float3 pos, float distance, bool aggressive);
	void KeepPointingTo(CUnit* unit, float distance, bool aggressive);

	bool OnSlope(void);

	float baseTurnRate;
	float turnRate;
	float accRate;
	float decRate;

	float maxReverseSpeed;
	float wantedSpeed;
	float currentSpeed;
	float deltaSpeed;
	short int deltaHeading;

	float3 flatFrontDir;

	unsigned int pathId;
	float goalRadius;

	SyncedFloat3 waypoint;
	SyncedFloat3 nextWaypoint;
	/// by this time it really should have gotten there genereate new path otherwise
	int etaWaypoint;
	/// by this time we get suspicious, check if goal is clogged if we are close
	int etaWaypoint2;
	bool atGoal;
	bool haveFinalWaypoint;
	float terrainSpeed;

	float requestedSpeed;
	short requestedTurnRate;

	float currentDistanceToWaypoint;

	unsigned int restartDelay;
	float3 lastGetPathPos;

	unsigned int pathFailures;
	/// how many times we havent gotten to a waypoint in time
	unsigned int etaFailures;
	/// how many times we have requested a path from the same place
	unsigned int nonMovingFailures;

	bool floatOnWater;

	int moveSquareX;
	int moveSquareY;
protected:
	int nextDeltaSpeedUpdate;
	int nextObstacleAvoidanceUpdate;

	int lastTrackUpdate;

	float3 ObstacleAvoidance(float3 desiredDir);
	float Distance2D(CSolidObject *object1, CSolidObject *object2, float marginal = 0.0f);

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
	void UpdateControlledDrop(void);
	void CheckCollisionSkid(void);
	float GetFlyTime(float3 pos, float3 speed);
	void CalcSkidRot(void);

	void AdjustPosToWaterLine();
	bool UpdateDirectControl();
	void UpdateOwnerPos(bool);
	bool WantReverse(const float3&) const;

	unsigned int lastHeatRequestFrame;
	unsigned int RequestPath(float3 startPos, float3 goalPos, float goalRadius = 8);
	void UpdateHeatMap();

	bool skidding;
	bool flying;
	bool reversing;
	float skidRotSpeed;
	float dropSpeed;
	float dropHeight;

	float3 skidRotVector;
	float skidRotSpeed2;
	float skidRotPos2;
	CSolidObject::PhysicalState oldPhysState;

	bool CheckColH(int x, int y1, int y2, float xmove, int squareTestX);
	bool CheckColV(int y, int x1, int x2, float zmove, int squareTestY);

	static std::vector<int2> (*lineTable)[11];

	float3 mainHeadingPos;
	bool useMainHeading;
	void SetMainHeading();

public:
	static void CreateLineTable(void);
	static void DeleteLineTable(void);
	void TestNewTerrainSquare(void);
	bool CheckGoalFeasability(void);
	virtual void LeaveTransport(void);

	void StartSkidding(void);
	void StartFlying(void);

	bool IsSkidding() const { return skidding; }
	bool IsFlying() const { return flying; }
	bool IsReversing() const { return reversing; }
};



#endif // GROUNDMOVETYPE_H
