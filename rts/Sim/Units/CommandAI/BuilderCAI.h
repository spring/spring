#ifndef __BUILDER_CAI_H__
#define __BUILDER_CAI_H__

#include <map>
#include "MobileCAI.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Objects/SolidObject.h"

class CUnitSet;

class CBuilderCAI :
	public CMobileCAI
{
public:
	CR_DECLARE(CBuilderCAI);
	CBuilderCAI(CUnit* owner);
	CBuilderCAI();
	~CBuilderCAI(void);
	void PostLoad();
	int GetDefaultCmd(CUnit* unit,CFeature* feature);
	void SlowUpdate();

	void DrawCommands(void);
	void GiveCommandReal(const Command& c, bool fromSynced = true);
	void DrawQuedBuildingSquares(void);

	bool FindReclaimTargetAndReclaim(const float3& pos, float radius,
	                                      unsigned char options,
	                                      bool noResCheck,  // no resources check
	                                      bool recAnyTeam,  // allows self-reclamation
	                                      bool recUnits,    // reclaims units and features
	                                      bool recNonRez,   // reclaims non resurrectable only
	                                      bool recEnemy);   // reclaims enemy units only
	bool FindResurrectableFeatureAndResurrect(const float3& pos, float radius,
	                                          unsigned char options,
											  bool freshOnly); // reclaims only corpses that have rez progress or all the metal left
	void FinishCommand(void);
	bool FindRepairTargetAndRepair(const float3& pos, float radius,
	                               unsigned char options, 
								   bool attackEnemy, 
								   bool builtOnly); // skips units that are under construction
	bool FindCaptureTargetAndCapture(const float3& pos, float radius,
	                                 unsigned char options,
									 bool healthyOnly); // only capture units with capture progress or 100% health remaining

	void ExecutePatrol(Command &c);
	void ExecuteFight(Command &c);
	void ExecuteGuard(Command &c);
	void ExecuteStop(Command &c);
	virtual void ExecuteRepair(Command &c);
	virtual void ExecuteCapture(Command &c);
	virtual void ExecuteReclaim(Command &c);
	virtual void ExecuteResurrect(Command &c);
	virtual void ExecuteRestore(Command &c);

	bool ReclaimObject(CSolidObject* o);
	bool ResurrectObject(CFeature *feature);

	std::map<int, std::string> buildOptions;
	bool building;
	BuildInfo build;

	int cachedRadiusId;
	float cachedRadius;
	float GetUnitDefRadius(const UnitDef* unitdef, int cmdId);

	int buildRetries;

	int lastPC1; //helps avoid infinite loops
	int lastPC2;
	int lastPC3;

	bool range3D;

	inline float f3Dist(const float3& a, const float3& b) const {
		return range3D ? a.distance(b) : a.distance2D(b);
	}
	inline float f3SqDist(const float3& a, const float3& b) const {
		return range3D ? a.SqDistance(b) : a.SqDistance2D(b);
	}
	inline float f3Len(const float3& a) const {
		return range3D ? a.Length() : a.Length2D();
	}
	inline float f3SqLen(const float3& a) const {
		return range3D ? a.SqLength() : a.SqLength2D();
	}

public:
	static CUnitSet reclaimers;
	static CUnitSet featureReclaimers;
	static CUnitSet resurrecters;

private:

	void CancelRestrictedUnit(const std::string& buildOption);
	bool ObjInBuildRange(const CWorldObject* obj) const;
	bool OutOfImmobileRange(const Command& cmd) const;

	// fix for patrolling cons repairing/resurrecting stuff that's being reclaimed
	static void AddUnitToReclaimers(CUnit*);
	static void RemoveUnitFromReclaimers(CUnit*);

	// fix for cons wandering away from their target circle
	static void AddUnitToFeatureReclaimers(CUnit*);
	static void RemoveUnitFromFeatureReclaimers(CUnit*);

	// fix for patrolling cons reclaiming stuff that is being resurrected
	static void AddUnitToResurrecters(CUnit*);
	static void RemoveUnitFromResurrecters(CUnit*);
public:
	static bool IsUnitBeingReclaimed(CUnit *unit, CUnit *friendUnit=NULL);
	static bool IsFeatureBeingReclaimed(int featureId, CUnit *friendUnit=NULL);
	static bool IsFeatureBeingResurrected(int featureId, CUnit *friendUnit=NULL);

private:
};

#endif // __BUILDER_CAI_H__
