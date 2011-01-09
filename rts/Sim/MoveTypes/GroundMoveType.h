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
	~CGroundMoveType();

	void PostLoad();

	void Update();
	void SlowUpdate();

	void SetDeltaSpeed(bool);

	void StartMoving(float3 pos, float goalRadius);
	void StartMoving(float3 pos, float goalRadius, float speed);
	void StopMoving();

	void SetMaxSpeed(float speed);

	void ImpulseAdded();

	void KeepPointingTo(float3 pos, float distance, bool aggressive);
	void KeepPointingTo(CUnit* unit, float distance, bool aggressive);

	bool OnSlope();

	float turnRate;
	float accRate;
	float decRate;

	float maxReverseSpeed;
	float wantedSpeed;
	float currentSpeed;
	float deltaSpeed;
	short int deltaHeading;

	float3 flatFrontDir;
	float3 waypointDir;

	unsigned int pathId;
	float goalRadius;

	SyncedFloat3 waypoint;
	SyncedFloat3 nextWaypoint;

	bool atGoal;
	bool haveFinalWaypoint;

	float requestedSpeed;

	float currentDistanceToWaypoint;

	unsigned int restartDelay;
	float3 lastGetPathPos;

	/// how many times we havent gotten to a waypoint in time
	unsigned int etaFailures;
	/// how many times we have requested a path from the same place
	unsigned int nonMovingFailures;

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

	float BreakingDistance(float speed) const;
	float3 Here();

	void StartEngine();
	void StopEngine();

	void Arrived();
	void Fail();
	void HandleObjectCollisions();

	void ChangeHeading(short wantedHeading);

	void UpdateSkid();
	void UpdateControlledDrop();
	void CheckCollisionSkid();
	void CalcSkidRot();

	float GetGroundHeight(const float3&) const;
	void AdjustPosToWaterLine();
	bool UpdateDirectControl();
	void UpdateOwnerPos(bool);
	bool WantReverse(const float3&) const;

	bool skidding;
	bool flying;
	bool reversing;
	bool idling;
	bool canReverse;

	float skidRotSpeed;
	float dropSpeed;
	float dropHeight;

	float3 skidRotVector;
	float skidRotSpeed2;
	float skidRotPos2;
	CSolidObject::PhysicalState oldPhysState;

	// number of grid-cells along each dimension; should be an odd number
	static const int LINETABLE_SIZE = 11;
	static std::vector<int2> lineTable[LINETABLE_SIZE][LINETABLE_SIZE];

	float3 mainHeadingPos;
	bool useMainHeading;
	void SetMainHeading();

public:
	static void CreateLineTable();
	static void DeleteLineTable();

	void TestNewTerrainSquare();
	void LeaveTransport();

	void StartSkidding();
	void StartFlying();

	bool IsSkidding() const { return skidding; }
	bool IsFlying() const { return flying; }
	bool IsReversing() const { return reversing; }
};



#endif // GROUNDMOVETYPE_H
