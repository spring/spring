// SmokeTrailProjectile.h: interface for the CSmokeTrailProjectile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SMOKETRAILPROJECTILE_H__940A4391_5E16_429E_B3E6_285CEAD102C2__INCLUDED_)
#define AFX_SMOKETRAILPROJECTILE_H__940A4391_5E16_429E_B3E6_285CEAD102C2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Projectile.h"

class CSmokeTrailProjectile : public CProjectile  
{
public:
	void Update();
	void Draw();
	CSmokeTrailProjectile(const float3& pos1,const float3& pos2,const float3& dir1,const float3& dir2, CUnit* owner,bool firstSegment,bool lastSegment,float size=1,float time=80,float color=0.7,bool drawTrail=true,CProjectile* drawCallback=0);
	virtual ~CSmokeTrailProjectile();

	float3 pos1;
	float3 pos2;
	float orgSize;
	int creationTime;
	int lifeTime;
	float color;
	float3 dir1;
	float3 dir2;
	bool drawTrail;

	float3 dirpos1;
	float3 dirpos2;
	float3 midpos;
	float3 middir;
	bool drawSegmented;
	bool firstSegment,lastSegment;
	CProjectile* drawCallbacker;
};

#endif // !defined(AFX_SMOKETRAILPROJECTILE_H__940A4391_5E16_429E_B3E6_285CEAD102C2__INCLUDED_)

