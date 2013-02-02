/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUNDMOVETYPE_H
#define GROUNDMOVETYPE_H

#include "MoveType.h"
#include "Sim/Objects/SolidObject.h"

struct UnitDef;
struct MoveDef;
class IPathController;

class CGroundMoveType : public AMoveType
{
	CR_DECLARE(CGroundMoveType);

public:
	CGroundMoveType(CUnit* owner);
	~CGroundMoveType();

	void PostLoad();

	bool Update();
	void SlowUpdate();

	void StartMoving(float3 pos, float goalRadius);
	void StartMoving(float3 pos, float goalRadius, float speed);
	void StopMoving();

	void KeepPointingTo(float3 pos, float distance, bool aggressive);
	void KeepPointingTo(CUnit* unit, float distance, bool aggressive);

	void TestNewTerrainSquare();
	bool CanApplyImpulse(const float3&);
	void LeaveTransport();

	void StartSkidding() { skidding = true; }
	void StartFlying() { skidding = true; flying = true; } // flying requires skidding

	bool OnSlope(float minSlideTolerance);
	bool IsSkidding() const { return skidding; }
	bool IsFlying() const { return flying; }
	bool IsReversing() const { return reversing; }

	static void CreateLineTable();
	static void DeleteLineTable();

private:
	float3 GetObstacleAvoidanceDir(const float3& desiredDir);
	float Distance2D(CSolidObject* object1, CSolidObject* object2, float marginal = 0.0f);

	void GetNewPath();
	void GetNextWayPoint();
	bool CanGetNextWayPoint();

	float BrakingDistance(float speed) const;
	float3 Here();

	void StartEngine();
	void StopEngine();

	void Arrived();
	void Fail();

	void HandleObjectCollisions();
	void HandleStaticObjectCollision(
		CUnit* collider,
		CSolidObject* collidee,
		const MoveDef* colliderMD,
		const float colliderRadius,
		const float collideeRadius,
		const float3& separationVector,
		bool canRequestPath,
		bool checkYardMap,
		bool checkTerrain);

	void HandleUnitCollisions(
		CUnit* collider,
		const float colliderSpeed,
		const float colliderRadius,
		const float3& sepDirMask,
		const UnitDef* colliderUD,
		const MoveDef* colliderMD);
	void HandleFeatureCollisions(
		CUnit* collider,
		const float colliderSpeed,
		const float colliderRadius,
		const float3& sepDirMask,
		const UnitDef* colliderUD,
		const MoveDef* colliderMD);

	void SetMainHeading();
	void ChangeSpeed(float, bool, bool = false);
	void ChangeHeading(short newHeading);

	void UpdateSkid();
	void UpdateControlledDrop();
	void CheckCollisionSkid();
	void CalcSkidRot();

	const float3& GetGroundNormal(const float3&) const;
	float GetGroundHeight(const float3&) const;
	void AdjustPosToWaterLine();
	bool UpdateDirectControl();
	void UpdateOwnerPos(bool);
	bool FollowPath();
	bool WantReverse(const float3&) const;

private:
	IPathController* pathController;

public:
	float turnRate;
	float accRate;
	float decRate;

	float maxReverseSpeed;
	float wantedSpeed;
	float currentSpeed;
	float deltaSpeed;

	unsigned int pathId;
	float goalRadius;

	SyncedFloat3 currWayPoint;
	SyncedFloat3 nextWayPoint;

private:
	bool atGoal;
	bool atEndOfPath;

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

