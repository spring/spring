/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BUILDER_CAI_H_
#define _BUILDER_CAI_H_

#include "MobileCAI.h"
#include "Sim/Units/BuildInfo.h"
#include "System/Misc/BitwiseEnum.h"
#include "System/UnorderedSet.hpp"

#include <vector>

class CUnit;
class CBuilder;
class CFeature;
class CSolidObject;
class CWorldObject;
struct Command;
struct UnitDef;


class CBuilderCAI : public CMobileCAI
{
public:
	CR_DECLARE(CBuilderCAI)
	CBuilderCAI(CUnit* owner);
	CBuilderCAI();
	~CBuilderCAI();

	static void InitStatic();
	void PostLoad();

	int GetDefaultCmd(const CUnit* unit, const CFeature* feature);
	void SlowUpdate();

	void FinishCommand();
	void GiveCommandReal(const Command& c, bool fromSynced = true);
	void BuggerOff(const float3& pos, float radius);
	bool TargetInterceptable(const CUnit* unit, float uspeed);

	void ExecuteBuildCmd(Command& c);
	void ExecutePatrol(Command& c);
	void ExecuteFight(Command& c);
	void ExecuteGuard(Command& c);
	void ExecuteStop(Command& c);
	virtual void ExecuteRepair(Command& c);
	virtual void ExecuteCapture(Command& c);
	virtual void ExecuteReclaim(Command& c);
	virtual void ExecuteResurrect(Command& c);
	virtual void ExecuteRestore(Command& c);

	bool ReclaimObject(CSolidObject* o);
	bool ResurrectObject(CFeature* feature);

	/**
	 * Checks if a unit is being reclaimed by a friendly con.
	 */
	static bool IsUnitBeingReclaimed(const CUnit* unit, const CUnit* friendUnit = nullptr);
	static bool IsFeatureBeingReclaimed(int featureId, const CUnit* friendUnit = nullptr);
	static bool IsFeatureBeingResurrected(int featureId, const CUnit* friendUnit = nullptr);

	bool IsInBuildRange(const CWorldObject* obj) const;
	bool IsInBuildRange(const float3& pos, const float radius) const;

public:
	spring::unordered_set<int> buildOptions;

	static spring::unordered_set<int> reclaimers;
	static spring::unordered_set<int> featureReclaimers;
	static spring::unordered_set<int> resurrecters;

	static std::vector<int> removees;

private:
	enum ReclaimOptions {
		REC_NORESCHECK = 1<<0,
		REC_UNITS      = 1<<1,
		REC_NONREZ     = 1<<2,
		REC_ENEMY      = 1<<3,
		REC_ENEMYONLY  = 1<<4,
		REC_SPECIAL    = 1<<5
	};
	typedef Bitwise::BitwiseEnum<ReclaimOptions> ReclaimOption;

private:
	/**
	 * @param pos position where to reclaim
	 * @param radius radius to search for objects to reclaim
	 * @param cmdopts command options
	 * @param recoptions reclaim optioons
	 */
	bool FindReclaimTargetAndReclaim(const float3& pos, float radius, unsigned char cmdopt, ReclaimOption recoptions);
	/**
	 * @param freshOnly reclaims only corpses that have rez progress or all the metal left
	 */
	bool FindResurrectableFeatureAndResurrect(const float3& pos, float radius, unsigned char options, bool freshOnly);

	/**
	 * @param builtOnly skips units that are under construction
	 */
	bool FindRepairTargetAndRepair(const float3& pos, float radius, unsigned char options, bool attackEnemy, bool builtOnly);
	/**
	 * @param pos         position where to search for units to capture
	 * @param radius      radius in which are searched units to capture
	 * @param options     command options
	 * @param healthyOnly only capture units with capture progress or 100% health remaining
	 */
	bool FindCaptureTargetAndCapture(const float3& pos, float radius, unsigned char options, bool healthyOnly);

	int FindReclaimTarget(const float3& pos, float radius, unsigned char cmdopt, ReclaimOption recoptions, float bestStartDist = 1.0e30f) const;

	float GetBuildRange(const float targetRadius) const;
	bool MoveInBuildRange(const CWorldObject* obj, const bool checkMoveTypeForFailed = false);
	bool MoveInBuildRange(const float3& pos, float radius, const bool checkMoveTypeForFailed = false);

	bool IsBuildPosBlocked(const BuildInfo& bi, const CUnit** nanoFrame) const;
	bool IsBuildPosBlocked(const BuildInfo& bi) const {
		const CUnit* u = nullptr;
		return IsBuildPosBlocked(build, &u);
	}

	void CancelRestrictedUnit();
	bool OutOfImmobileRange(const Command& cmd) const;
	/// add a command to reclaim a feature that is blocking our build-site
	void ReclaimFeature(CFeature* f);

	/// fix for patrolling cons repairing/resurrecting stuff that's being reclaimed
	static void AddUnitToReclaimers(CUnit*);
	static void RemoveUnitFromReclaimers(CUnit*);

	/// fix for cons wandering away from their target circle
	static void AddUnitToFeatureReclaimers(CUnit*);
	static void RemoveUnitFromFeatureReclaimers(CUnit*);

	/// fix for patrolling cons reclaiming stuff that is being resurrected
	static void AddUnitToResurrecters(CUnit*);
	static void RemoveUnitFromResurrecters(CUnit*);

	inline float f3Dist(const float3& a, const float3& b) const {
		return range3D ? a.distance(b) : a.distance2D(b);
	}
	inline float f3SqDist(const float3& a, const float3& b) const {
		return range3D ? a.SqDistance(b) : a.SqDistance2D(b);
	}
	//inline float f3Len(const float3& a) const {
	//	return range3D ? a.Length() : a.Length2D();
	//}
	//inline float f3SqLen(const float3& a) const {
	//	return range3D ? a.SqLength() : a.SqLength2D();
	//}

	float GetBuildOptionRadius(const UnitDef* unitdef, int cmdId);

private:
	CBuilder* ownerBuilder;

	bool building;
	BuildInfo build;

	int cachedRadiusId;
	float cachedRadius;

	int buildRetries;
	int randomCounter; ///< used to balance intervals of time intensive ai optimizations

	int lastPC1; ///< helps avoid infinite loops
	int lastPC2;
	int lastPC3;

	bool range3D;
};

#endif // _BUILDER_CAI_H_
