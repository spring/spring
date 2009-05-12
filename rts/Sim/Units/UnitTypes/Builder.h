// Builder.h: interface for the CBuilder class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BUILDER_H__
#define __BUILDER_H__

#include <string>
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

class CFeature;

class CBuilder : public CUnit
{
private:
	void UnitInit (const UnitDef* def, int team, const float3& position);

public:
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
	CR_DECLARE(CBuilder);

	CBuilder();
	virtual ~CBuilder();
	void PostLoad();

	void Update();
	void SlowUpdate(void);
	void DependentDied(CObject* o);

	bool StartBuild(BuildInfo& buildInfo);
	float CalculateBuildTerraformCost(BuildInfo& buildInfo);
	void StopBuild(bool callScript=true);
	void SetRepairTarget(CUnit* target);
	void SetReclaimTarget(CSolidObject* object);
	void StartRestore(float3 centerPos, float radius);
	void SetBuildStanceToward(float3 pos);

	void HelpTerraform(CBuilder* unit);
	void CreateNanoParticle(float3 goal, float radius, bool inverse);
	void SetResurrectTarget(CFeature* feature);
	void SetCaptureTarget(CUnit* unit);

public:
	bool range3D; // spheres instead of infinite cylinders for range tests

	float buildDistance;
	float buildSpeed;
	float repairSpeed;
	float reclaimSpeed;
	float resurrectSpeed;
	float captureSpeed;
	float terraformSpeed;

	CFeature* curResurrect;
	int lastResurrected;
	CUnit* curBuild;
	CUnit* curCapture;
	CSolidObject* curReclaim;
	CBuilder* helpTerraform;

	bool terraforming;
	float terraformHelp;
	float myTerraformLeft;
	enum {
		Terraform_Building,
		Terraform_Restore
	} terraformType;
	int tx1,tx2,tz1,tz2;
	float3 terraformCenter;
	float terraformRadius;
};

#endif // __BUILDER_H__
