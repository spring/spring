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

	bool Update();
	void SlowUpdate();

	void SetDeltaSpeed(float, bool, bool = false);

	void StartMoving(float3 pos, float goalRadius);
	void StartMoving(float3 pos, float goalRadius, float speed);
	void StopMoving();

	void SetMaxSpeed(float speed);

	void ImpulseAdded();

	void KeepPointingTo(float3 pos, float distance, bool aggressive);
	void KeepPointingTo(CUnit* unit, float distance, bool aggressive);

	bool OnSlope();

	void TestNewTerrainSquare();
	void LeaveTransport();

	void StartSkidding();
	void StartFlying();

	bool IsSkidding() const { return skidding; }
	bool IsFlying() const { return flying; }
	bool IsReversing() const { return reversing; }

	static void CreateLineTable();
	static void DeleteLineTable();


	float turnRate;
	float accRate;
	float decRate;

	float maxReverseSpeed;
	float wantedSpeed;
	float currentSpeed;
	float requestedSpeed;
	float deltaSpeed;
	short int deltaHeading;

	unsigned int pathId;
	float goalRadius;

	SyncedFloat3 waypoint;
	SyncedFloat3 nextWaypoint;

protected:
	float3 ObstacleAvoidance(const float3& desiredDir);
	float Distance2D(CSolidObject* object1, CSolidObject* object2, float marginal = 0.0f);

	void GetNewPath();
	void GetNextWaypoint();

	float BreakingDistance(float speed) const;
	float3 Here();

	void StartEngine();
	void StopEngine();

	void Arrived();
	void Fail();
	void HandleObjectCollisions();

	void SetMainHeading();
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


	bool atGoal;
	bool haveFinalWaypoint;
	float currentDistanceToWaypoint;

	bool skidding;
	bool flying;
	bool reversing;
	bool idling;
	bool canReverse;
	bool useMainHeading;

	float3 skidRotVector;
	float skidRotSpeed;
	float skidRotSpeed2;
	float skidRotPos2;

	CSolidObject::PhysicalState oldPhysState;

	float3 waypointDir;
	float3 flatFrontDir;
	float3 mainHeadingPos;

	// number of grid-cells along each dimension; should be an odd number
	static const int LINETABLE_SIZE = 11;
	static std::vector<int2> lineTable[LINETABLE_SIZE][LINETABLE_SIZE];

	unsigned int nextDeltaSpeedUpdate;
	unsigned int nextObstacleAvoidanceUpdate;

	unsigned int pathRequestDelay;

	/// {in, de}creased every Update if idling is true/false and pathId != 0
	unsigned int numIdlingUpdates;
	/// {in, de}creased every SlowUpdate if idling is true/false and pathId != 0
	unsigned int numIdlingSlowUpdates;

	int moveSquareX;
	int moveSquareY;
};

#endif // GROUNDMOVETYPE_H
