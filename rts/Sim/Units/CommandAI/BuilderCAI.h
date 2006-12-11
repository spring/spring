#ifndef __BUILDER_CAI_H__
#define __BUILDER_CAI_H__

#include <map>
#include "MobileCAI.h"
#include "../UnitDef.h"

class CBuilderCAI :
	public CMobileCAI
{
public:
	CBuilderCAI(CUnit* owner);
	~CBuilderCAI(void);
	int GetDefaultCmd(CUnit* unit,CFeature* feature);
	void SlowUpdate();

	void DrawCommands(void);
	void GiveCommand(const Command& c);
	void DrawQuedBuildingSquares(void);

	bool FindReclaimableFeatureAndReclaim(float3 pos, float radius,unsigned char options, bool recAny);
	bool FindResurrectableFeatureAndResurrect(float3 pos, float radius,unsigned char options);
	void FinishCommand(void);
	bool FindRepairTargetAndRepair(float3 pos, float radius,unsigned char options,bool attackEnemy);
	bool FindCaptureTargetAndCapture(float3 pos, float radius,unsigned char options);

	void ExecutePatrol(Command &c);
	void ExecuteFight(Command &c);
	void ExecuteGuard(Command &c);
	void ExecuteStop(Command &c);
	virtual void ExecuteRepair(Command &c);
	virtual void ExecuteCapture(Command &c);
	virtual void ExecuteReclaim(Command &c);
	virtual void ExecuteResurrect(Command &c);
	virtual void ExecuteRestore(Command &c);

	map<int,string> buildOptions;
	bool building;
	BuildInfo build;

	float cachedRadius;
	int cachedRadiusId;

	int buildRetries;

	int lastPC1; //helps avoid infinite loops
	int lastPC2;
};

#endif // __BUILDER_CAI_H__
