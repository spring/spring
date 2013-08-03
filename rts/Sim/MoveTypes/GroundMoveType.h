/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUNDMOVETYPE_H
#define GROUNDMOVETYPE_H

#include "MoveType.h"
#include "System/Sync/SyncedFloat3.h"

struct UnitDef;
struct MoveDef;
class CSolidObject;
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
	void StartMoving(float3 pos, float goalRadius, float speed) { StartMoving(pos, goalRadius); }
	void StopMoving(bool callScript = false, bool hardStop = false);

	void KeepPointingTo(float3 pos, float distance, bool aggressive);
	void KeepPointingTo(CUnit* unit, float distance, bool aggressive);

	void TestNewTerrainSquare();
	bool CanApplyImpulse(const float3&);
	void LeaveTransport();

	bool OnSlope(float minSlideTolerance);
	bool IsReversing() const { return reversing; }

private:
	float3 GetObstacleAvoidanceDir(const float3& desiredDir);
	float3 GetNewSpeedVector(const float hAcc, const float vAcc) const;

	#define SQUARE(x) ((x) * (x))
	bool StartSkidding(const float3& vel, const float3& dir) const { return (SQUARE(vel.dot(dir)) < (vel.SqLength() * 0.95f)); }
	bool StopSkidding(const float3& vel, const float3& dir) const { return (SQUARE(vel.dot(dir)) >= (vel.SqLength() * 0.95f)); }
	bool StartFlying(const float3& vel, const float3& dir) const { return (vel.dot(dir) > 0.2f); }
	bool StopFlying(const float3& vel, const float3& dir) const { return (vel.dot(dir) <= 0.2f); }
	#undef SQUARE

	float Distance2D(CSolidObject* object1, CSolidObject* object2, float marginal = 0.0f);

	void GetNewPath();
	void GetNextWayPoint();
	bool CanGetNextWayPoint();
	bool ReRequestPath(bool callScript, bool forceRequest);

	float BrakingDistance(float speed) const;
	float3 Here();

	void StartEngine(bool callScript);
	void StopEngine(bool callScript, bool hardStop = false);

	void Arrived(bool callScript);
	void Fail(bool callScript);

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
	void UpdateOwnerPos(const float3&, const float3&);
	bool OwnerMoved(const short, const float3&, const float3&);
	bool FollowPath();
	bool WantReverse(const float3&) const;

private:
	IPathController* pathController;

public:
	float turnRate;
	float accRate;
	float decRate;
	float myGravity;

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

	bool reversing;
	bool idling;
	bool canReverse;
	bool useMainHeading;

	float3 skidRotVector;  /// vector orthogonal to skidDir
	float skidRotSpeed;    /// rotational speed when skidding (radians / (GAME_SPEED frames))
	float skidRotAccel;    /// rotational acceleration when skidding (radians / (GAME_SPEED frames^2))

	float3 waypointDir;
	float3 flatFrontDir;
	float3 lastAvoidanceDir;
	float3 mainHeadingPos;

	unsigned int nextObstacleAvoidanceFrame;
	unsigned int lastPathRequestFrame;

	/// {in, de}creased every Update if idling is true/false and pathId != 0
	unsigned int numIdlingUpdates;
	/// {in, de}creased every SlowUpdate if idling is true/false and pathId != 0
	unsigned int numIdlingSlowUpdates;

	short wantedHeading;
};

#endif // GROUNDMOVETYPE_H

