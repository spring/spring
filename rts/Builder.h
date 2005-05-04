// Builder.h: interface for the CBuilder class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BUILDER_H__FC524CB8_F2F2_4CCA_A05B_A8F37AB87874__INCLUDED_)
#define AFX_BUILDER_H__FC524CB8_F2F2_4CCA_A05B_A8F37AB87874__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
#include "unit.h"

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

#endif // !defined(AFX_BUILDER_H__FC524CB8_F2F2_4CCA_A05B_A8F37AB87874__INCLUDED_)
