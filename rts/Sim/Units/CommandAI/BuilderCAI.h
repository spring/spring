#ifndef __BUILDER_CAI_H__
#define __BUILDER_CAI_H__

#include <map>
#include "MobileCAI.h"
#include "../UnitDef.h"
#include "../../Objects/SolidObject.h"

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
	void GiveCommandReal(const Command& c);
	void DrawQuedBuildingSquares(void);

	bool FindReclaimableFeatureAndReclaim(const float3& pos, float radius,
	                                      unsigned char options,
	                                      bool noResCheck,  // no resources check
	                                      bool recAnyTeam); // allows self-reclamation
	bool FindResurrectableFeatureAndResurrect(const float3& pos, float radius,
	                                          unsigned char options);
	void FinishCommand(void);
	bool FindRepairTargetAndRepair(const float3& pos, float radius,
	                               unsigned char options, bool attackEnemy);
	bool FindCaptureTargetAndCapture(const float3& pos, float radius,
	                                 unsigned char options);

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

	std::map<int, std::string> buildOptions;
	bool building;
	BuildInfo build;

	int cachedRadiusId;
	float cachedRadius;
	float GetUnitDefRadius(const UnitDef* unitdef, int cmdId);

	int buildRetries;

	int lastPC1; //helps avoid infinite loops
	int lastPC2;

	static CUnitSet reclaimers;
private:

	void CancelRestrictedUnit(const std::string& buildOption);
	bool ObjInBuildRange(const CWorldObject* obj) const;

	// fix for patrolling cons repairing stuff that's being reclaimed
	static void AddUnitToReclaimers(CUnit*);
	static void RemoveUnitFromReclaimers(CUnit*);
	static bool IsUnitBeingReclaimedByFriend(CUnit*);
};

#endif // __BUILDER_CAI_H__
