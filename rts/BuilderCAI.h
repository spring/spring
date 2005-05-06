#ifndef __BUILDER_CAI_H__
#define __BUILDER_CAI_H__

#include "archdef.h"

#include <map>
#include "MobileCAI.h"

class CBuilderCAI :
	public CMobileCAI
{
public:
	CBuilderCAI(CUnit* owner);
	~CBuilderCAI(void);
	int GetDefaultCmd(CUnit* unit,CFeature* feature);
	void SlowUpdate();

	void DrawCommands(void);
	void GiveCommand(Command& c);
	void DrawQuedBuildingSquares(void);

	bool FindReclaimableFeatureAndReclaim(float3 pos, float radius,unsigned char options);
	void FinishCommand(void);
	bool FindRepairTargetAndRepair(float3 pos, float radius,unsigned char options,bool attackEnemy);

	bool commandFromPatrol;
	map<int,string> buildOptions;
	bool building;
	float3 buildPos;

	float cachedRadius;
	int cachedRadiusId;

	int buildRetries;
};

#endif // __BUILDER_CAI_H__
