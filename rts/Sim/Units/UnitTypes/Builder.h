/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __BUILDER_H__
#define __BUILDER_H__

#include <string>

#include "Sim/Units/Unit.h"
#include "System/float3.h"

struct UnitDef;
struct BuildInfo;
class CFeature;
class CSolidObject;

class CBuilder : public CUnit
{
private:
	void PreInit(const UnitDef* def, int team, int facing, const float3& position, bool build);

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
	void SlowUpdate();
	void DependentDied(CObject* o);
	virtual void DeleteDeathDependence(CObject* o, DependenceType dep);

	bool StartBuild(BuildInfo& buildInfo, CFeature*& feature, bool& waitstance);
	float CalculateBuildTerraformCost(BuildInfo& buildInfo);
	void StopBuild(bool callScript = true);
	void SetRepairTarget(CUnit* target);
	void SetReclaimTarget(CSolidObject* object);
	void StartRestore(float3 centerPos, float radius);
	void SetBuildStanceToward(float3 pos);

	void HelpTerraform(CBuilder* unit);
	void CreateNanoParticle(float3 goal, float radius, bool inverse);
	void SetResurrectTarget(CFeature* feature);
	void SetCaptureTarget(CUnit* unit);

public:
	bool range3D; ///< spheres instead of infinite cylinders for range tests

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
