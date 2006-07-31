// Builder.h: interface for the CBuilder class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BUILDER_H__
#define __BUILDER_H__

#include <string>
#include "Sim/Units/Unit.h"

using namespace std;
class CFeature;

class CBuilder : public CUnit  
{
public:
	CR_DECLARE(CBuilder);

	CBuilder();
	virtual ~CBuilder();

	void Update();
	void SlowUpdate(void);
	void DependentDied(CObject* o);

	bool StartBuild(BuildInfo& buildInfo);
	void CalculateBuildTerraformCost(BuildInfo& buildInfo);
	void StopBuild(bool callScript=true);
	void SetRepairTarget(CUnit* target);
	void SetReclaimTarget(CSolidObject* object);
	void StartRestore(float3 centerPos, float radius);
	void SetBuildStanceToward(float3 pos);

	void UnitInit (UnitDef* def, int team, const float3& position);

	float buildSpeed;
	float buildDistance;

	CFeature* curResurrect;
	int lastResurrected;
	CUnit* curBuild;
	CUnit* curCapture;
	CSolidObject* curReclaim;
	CBuilder* helpTerraform;

	bool terraforming;
	float terraformLeft;
	float terraformHelp;
	enum {
		Terraform_Building,
		Terraform_Restore
	} terraformType;
	int tx1,tx2,tz1,tz2;
	float3 terraformCenter;
	float terraformRadius;

	string nextBuildType;
	float3 nextBuildPos;
	void HelpTerraform(CBuilder* unit);
	void CreateNanoParticle(float3 goal, float radius, bool inverse);
	void SetResurrectTarget(CFeature* feature);
	void SetCaptureTarget(CUnit* unit);
};

#endif // __BUILDER_H__
