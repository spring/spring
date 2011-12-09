/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUNDMOVETYPE_H
#define GROUNDMOVETYPE_H

#include "MoveType.h"
#include "Sim/Objects/SolidObject.h"

struct UnitDef;
struct MoveData;
class CMoveMath;

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

	void ImpulseAdded(const float3&);

	void KeepPointingTo(float3 pos, float distance, bool aggressive);
	void KeepPointingTo(CUnit* unit, float distance, bool aggressive);

	bool OnSlope(float minSlideTolerance);

	void TestNewTerrainSquare();
	void LeaveTransport();

	void StartSkidding() { skidding = true; }
	void StartFlying() { skidding = true; flying = true; } // flying requires skidding

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

	unsigned int pathId;
	float goalRadius;

	SyncedFloat3 currWayPoint;
	SyncedFloat3 nextWayPoint;

protected:
	float3 ObstacleAvoidance(const float3& desiredDir);
	float Distance2D(CSolidObject* object1, CSolidObject* object2, float marginal = 0.0f);

	void GetNewPath();
	void GetNextWayPoint();

	float BreakingDistance(float speed) const;
	float3 Here();

	void StartEngine();
	void StopEngine();

	void Arrived();
	void Fail();
	void HandleObjectCollisions();
	void HandleUnitCollisions(
		CUnit* collider,
		const float3& colliderCurPos,
		const float3& colliderOldPos,
		const float colliderSpeed,
		const float colliderRadius,
		const float3& sepDirMask,
		const UnitDef* colliderUD,
		const MoveData* colliderMD,
		const CMoveMath* colliderMM);
	void HandleFeatureCollisions(
		CUnit* collider,
		const float3& colliderCurPos,
		const float3& colliderOldPos,
		const float colliderSpeed,
		const float colliderRadius,
		const float3& sepDirMask,
		const UnitDef* colliderUD,
		const MoveData* colliderMD,
		const CMoveMath* colliderMM);

	void SetMainHeading();
	void ChangeHeading(short newHeading);

	void UpdateSkid();
	void UpdateControlledDrop();
	void CheckCollisionSkid();
	void CalcSkidRot();

	float GetGroundHeight(const float3&) const;
	void AdjustPosToWaterLine();
	bool UpdateDirectControl();
	void UpdateOwnerPos(bool);
	bool FollowPath();
	bool WantReverse(const float3&) const;


	bool atGoal;
	bool haveFinalWaypoint;
	float currWayPointDist;
	float prevWayPointDist;

	bool skidding;
	bool flying;
	bool reversing;
	bool idling;
	bool canReverse;
	bool useMainHeading;

	float3 skidRotVector;  /// vector orthogonal to skidDir
	float skidRotSpeed;    /// rotational speed when skidding (radians / (GAME_SPEED frames))
	float skidRotAccel;    /// rotational acceleration when skidding (radians / (GAME_SPEED frames^2))

	CSolidObject::PhysicalState oldPhysState;

	float3 waypointDir;
	float3 flatFrontDir;
	float3 lastAvoidanceDir;
	float3 mainHeadingPos;

	// number of grid-cells along each dimension; should be an odd number
	static const int LINETABLE_SIZE = 11;
	static std::vector<int2> lineTable[LINETABLE_SIZE][LINETABLE_SIZE];

	unsigned int nextObstacleAvoidanceUpdate;
	unsigned int pathRequestDelay;

	/// {in, de}creased every Update if idling is true/false and pathId != 0
	unsigned int numIdlingUpdates;
	/// {in, de}creased every SlowUpdate if idling is true/false and pathId != 0
	unsigned int numIdlingSlowUpdates;

	int moveSquareX;
	int moveSquareY;

	short wantedHeading;
};

#endif // GROUNDMOVETYPE_H
