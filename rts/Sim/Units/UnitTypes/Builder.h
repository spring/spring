/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BUILDER_H
#define _BUILDER_H

#include <string>

#include "Sim/Misc/NanoPieceCache.h"
#include "Sim/Units/Unit.h"
#include "System/float3.h"

struct UnitDef;
struct BuildInfo;
class CFeature;
class CSolidObject;

class CBuilder : public CUnit
{
private:
	void PreInit(const UnitLoadParams& params);

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
	CR_DECLARE(CBuilder)

	CBuilder();
	virtual ~CBuilder();

	void Update();
	void SlowUpdate();
	void DependentDied(CObject* o);

	bool StartBuild(BuildInfo& buildInfo, CFeature*& feature, bool& waitStance);
	float CalculateBuildTerraformCost(BuildInfo& buildInfo);
	void StopBuild(bool callScript = true);
	void SetRepairTarget(CUnit* target);
	void SetReclaimTarget(CSolidObject* object);
	void StartRestore(float3 centerPos, float radius);
	bool ScriptStartBuilding(float3 pos, bool silent);

	void HelpTerraform(CBuilder* unit);
	void CreateNanoParticle(const float3& goal, float radius, bool inverse, bool highPriority = false);
	void SetResurrectTarget(CFeature* feature);
	void SetCaptureTarget(CUnit* unit);

	bool CanAssistUnit(const CUnit* u, const UnitDef* def = NULL) const;
	bool CanRepairUnit(const CUnit* u) const;

	const NanoPieceCache& GetNanoPieceCache() const { return nanoPieceCache; }
	      NanoPieceCache& GetNanoPieceCache()       { return nanoPieceCache; }

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
	bool reclaimingUnit;
	CBuilder* helpTerraform;

	bool terraforming;
	float terraformHelp;
	float myTerraformLeft;
	enum TerraformType {
		Terraform_Building,
		Terraform_Restore
	} terraformType;
	int tx1,tx2,tz1,tz2;
	float3 terraformCenter;
	float terraformRadius;

private:
	NanoPieceCache nanoPieceCache;
};

#endif // _BUILDER_H
