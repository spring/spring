/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUNDMOVETYPE_H
#define GROUNDMOVETYPE_H

#include <array>

#include "MoveType.h"
#include "Sim/Path/IPathController.hpp"
#include "System/Sync/SyncedFloat3.h"

struct UnitDef;
struct MoveDef;
class CSolidObject;

class CGroundMoveType : public AMoveType
{
	CR_DECLARE_DERIVED(CGroundMoveType)

public:
	CGroundMoveType(CUnit* owner);
	~CGroundMoveType();

	struct MemberData {
		std::array<std::pair<unsigned int,  bool*>, 3>  bools;
		std::array<std::pair<unsigned int, short*>, 1> shorts;
		std::array<std::pair<unsigned int, float*>, 9> floats;
	};

	void PostLoad();

	bool Update() override;
	void SlowUpdate() override;

	void StartMovingRaw(const float3 moveGoalPos, float moveGoalRadius) override;
	void StartMoving(float3 pos, float moveGoalRadius) override;
	void StartMoving(float3 pos, float moveGoalRadius, float speed) override { StartMoving(pos, moveGoalRadius); }
	void StopMoving(bool callScript = false, bool hardStop = false, bool cancelRaw = false) override;
	bool IsMovingTowards(const float3& pos, float radius, bool checkProgress) const override {
		return (goalPos == pos * XZVector && goalRadius == radius && (!checkProgress || progressState == Active));
	}

	void KeepPointingTo(float3 pos, float distance, bool aggressive) override;
	void KeepPointingTo(CUnit* unit, float distance, bool aggressive) override;

	void TestNewTerrainSquare();
	bool CanApplyImpulse(const float3&) override;
	void LeaveTransport() override;

	void InitMemberPtrs(MemberData* memberData);
	bool SetMemberValue(unsigned int memberHash, void* memberValue) override;

	bool OnSlope(float minSlideTolerance);
	bool IsReversing() const override { return reversing; }
	bool IsPushResistant() const override { return pushResistant; }
	bool WantToStop() const { return (pathID == 0 && (!useRawMovement || atEndOfPath)); }

	void TriggerSkipWayPoint() {
		currWayPoint.y = -1.0f;
		// nextWayPoint.y = -1.0f;
	}
	void TriggerCallArrived() {
		atEndOfPath = true;
		atGoal = true;
	}


	float GetTurnRate() const { return turnRate; }
	float GetTurnSpeed() const { return turnSpeed; }
	float GetTurnAccel() const { return turnAccel; }

	float GetAccRate() const { return accRate; }
	float GetDecRate() const { return decRate; }
	float GetMyGravity() const { return myGravity; }
	float GetOwnerRadius() const { return ownerRadius; }

	float GetMaxReverseSpeed() const { return maxReverseSpeed; }
	float GetWantedSpeed() const { return wantedSpeed; }
	float GetCurrentSpeed() const { return currentSpeed; }
	float GetDeltaSpeed() const { return deltaSpeed; }

	float GetCurrWayPointDist() const { return currWayPointDist; }
	float GetPrevWayPointDist() const { return prevWayPointDist; }
	float GetGoalRadius(float s = 0.0f) const override { return (goalRadius + extraRadius * s); }

	unsigned int GetPathID() const { return pathID; }

	const SyncedFloat3& GetCurrWayPoint() const { return currWayPoint; }
	const SyncedFloat3& GetNextWayPoint() const { return nextWayPoint; }

	const float3& GetFlatFrontDir() const { return flatFrontDir; }
	const float3& GetGroundNormal(const float3&) const;
	float GetGroundHeight(const float3&) const;

private:
	float3 GetObstacleAvoidanceDir(const float3& desiredDir);
	float3 Here() const;

	#define SQUARE(x) ((x) * (x))
	bool StartSkidding(const float3& vel, const float3& dir) const { return ((SQUARE(vel.dot(dir)) + 0.01f) < (vel.SqLength() * sqSkidSpeedMult)); }
	bool StopSkidding(const float3& vel, const float3& dir) const { return ((SQUARE(vel.dot(dir)) + 0.01f) >= (vel.SqLength() * sqSkidSpeedMult)); }
	bool StartFlying(const float3& vel, const float3& dir) const { return (vel.dot(dir) > 0.2f); }
	bool StopFlying(const float3& vel, const float3& dir) const { return (vel.dot(dir) <= 0.2f); }
	#undef SQUARE

	float Distance2D(CSolidObject* object1, CSolidObject* object2, float marginal = 0.0f);

	unsigned int GetNewPath();

	void SetNextWayPoint();
	bool CanSetNextWayPoint();
	void ReRequestPath(bool forceRequest);

	void StartEngine(bool callScript);
	void StopEngine(bool callScript, bool hardStop = false);

	void Arrived(bool callScript);
	void Fail(bool callScript);

	void HandleObjectCollisions();
	bool HandleStaticObjectCollision(
		CUnit* collider,
		CSolidObject* collidee,
		const MoveDef* colliderMD,
		const float colliderRadius,
		const float collideeRadius,
		const float3& separationVector,
		bool canRequestPath,
		bool checkYardMap,
		bool checkTerrain
	);

	void HandleUnitCollisions(
		CUnit* collider,
		const float3& colliderParams,
		const UnitDef* colliderUD,
		const MoveDef* colliderMD
	);
	void HandleFeatureCollisions(
		CUnit* collider,
		const float3& colliderParams,
		const UnitDef* colliderUD,
		const MoveDef* colliderMD
	);

	void SetMainHeading();
	void ChangeSpeed(float, bool, bool = false);
	void ChangeHeading(short newHeading);

	void UpdateSkid();
	void UpdateControlledDrop();
	void CheckCollisionSkid();
	void CalcSkidRot();

	void AdjustPosToWaterLine();
	bool UpdateDirectControl();
	void UpdateOwnerAccelAndHeading();
	void UpdateOwnerPos(const float3&, const float3&);
	bool UpdateOwnerSpeed(float oldSpeedAbs, float newSpeedAbs, float newSpeedRaw);
	bool OwnerMoved(const short, const float3&, const float3&);
	bool FollowPath();
	bool WantReverse(const float3& wpDir, const float3& ffDir) const;

private:
	GMTDefaultPathController pathController;

	SyncedFloat3 currWayPoint;
	SyncedFloat3 nextWayPoint;

	float3 waypointDir;
	float3 flatFrontDir;
	float3 lastAvoidanceDir;
	float3 mainHeadingPos;
	float3 skidRotVector;                   /// vector orthogonal to skidDir

	float turnRate = 0.1f;                  /// maximum angular speed (angular units/frame)
	float turnSpeed = 0.0f;                 /// current angular speed (angular units/frame)
	float turnAccel = 0.0f;                 /// angular acceleration (angular units/frame^2)

	float accRate = 0.01f;
	float decRate = 0.01f;
	float myGravity = 0.0f;

	float maxReverseDist = 0.0f;
	float minReverseAngle = 0.0f;
	float maxReverseSpeed = 0.0f;
	float sqSkidSpeedMult = 0.95f;

	float wantedSpeed = 0.0f;
	float currentSpeed = 0.0f;
	float deltaSpeed = 0.0f;

	float currWayPointDist = 0.0f;
	float prevWayPointDist = 0.0f;

	float goalRadius = 0.0f;                /// original radius passed to StartMoving*
	float ownerRadius = 0.0f;               /// owner MoveDef footprint radius
	float extraRadius = 0.0f;               /// max(0, ownerRadius - goalRadius) if goal-pos is valid, 0 otherwise

	float skidRotSpeed = 0.0f;              /// rotational speed when skidding (radians / (GAME_SPEED frames))
	float skidRotAccel = 0.0f;              /// rotational acceleration when skidding (radians / (GAME_SPEED frames^2))

	unsigned int pathID = 0;
	unsigned int nextObstacleAvoidanceFrame = 0;

	unsigned int numIdlingUpdates = 0;      /// {in, de}creased every Update if idling is true/false and pathId != 0
	unsigned int numIdlingSlowUpdates = 0;  /// {in, de}creased every SlowUpdate if idling is true/false and pathId != 0

	short wantedHeading = 0;
	short minScriptChangeHeading = 0;       /// minimum required turn-angle before script->ChangeHeading is called

	bool atGoal = false;
	bool atEndOfPath = false;
	bool wantRepath = false;

	bool reversing = false;
	bool idling = false;
	bool pushResistant = false;
	bool canReverse = false;
	bool useMainHeading = false;            /// if true, turn toward mainHeadingPos until weapons[0] can TryTarget() it
	bool useRawMovement = false;            /// if true, move towards goal without invoking PFS (unrelated to MoveDef::allowRawMovement)
};

#endif // GROUNDMOVETYPE_H

