// Builder.h: interface for the CBuilder class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BUILDER_H__
#define __BUILDER_H__

#include <string>
#include "Unit.h"

using namespace std;

class CBuilder : public CUnit  
{
public:
	CBuilder(const float3 &pos,int team,UnitDef* unitDef);
	virtual ~CBuilder();

	void Update();
	void SlowUpdate(void);
	void DependentDied(CObject* o);

	bool StartBuild(const string& type,float3& pos);
	void StopBuild(bool callScript=true);
	void SetRepairTarget(CUnit* target);
	void SetReclaimTarget(CSolidObject* object);
	void StartRestore(float3 centerPos, float radius);
	void SetBuildStanceToward(float3 pos);

	float buildSpeed;
	float buildDistance;

	CUnit* curBuild;
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
};

#endif // __BUILDER_H__
