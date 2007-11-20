// Builder.h: interface for the CBuilder class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BUILDER_H__
#define __BUILDER_H__

#include <string>
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

using namespace std;
class CFeature;

class CBuilder : public CUnit
{
private:
	void UnitInit (const UnitDef* def, int team, const float3& position);
	static bool use2D;

public:
	static inline float f3Dist(const float3& a, const float3& b) {
		return use2D ? a.distance2D(b) : a.distance(b);
	}
	static inline float f3Len(const float3& a) {
		return use2D ? a.Length2D() : a.Length();
	}
	static inline float f3SqLen(const float3& a) {
		return use2D ? a.SqLength2D() : a.SqLength();
	}
	static inline void SetUse2D(bool value) { use2D = value; }
	static inline bool GetUse2D()    { return use2D; }
	
public:
	CR_DECLARE(CBuilder);

	CBuilder();
	virtual ~CBuilder();
	void PostLoad();

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

	void HelpTerraform(CBuilder* unit);
	void CreateNanoParticle(float3 goal, float radius, bool inverse);
	void SetResurrectTarget(CFeature* feature);
	void SetCaptureTarget(CUnit* unit);

public:
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
};

#endif // __BUILDER_H__
