// DirtProjectile.h: interface for the CDirtProjectile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DirtPROJECTILE_H__23CBBD23_8FBE_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_DirtPROJECTILE_H__23CBBD23_8FBE_11D4_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Projectile.h"

class CDirtProjectile : public CProjectile  
{
public:
public:
	virtual void Draw();
	virtual void Update();
	CDirtProjectile(const float3 pos,const float3 speed,const float ttl,const float size,const float expansion,float slowdown,CUnit* owner,const float3& color);
	virtual ~CDirtProjectile();

	float alpha;
	float alphaFalloff;
	float size;
	float sizeExpansion;
	float slowdown;
	float3 color;
};

#endif // !defined(AFX_DirtPROJECTILE_H__23CBBD23_8FBE_11D4_AD55_0080ADA84DE3__INCLUDED_)
