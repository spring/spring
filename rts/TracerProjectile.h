#ifndef TRACERPROJECTILE_H
#define TRACERPROJECTILE_H
// TracerProjectile.h: interface for the CTracerProjectile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TRACERPROJECTILE_H__7BB823C2_7EB6_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_TRACERPROJECTILE_H__7BB823C2_7EB6_11D4_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

#include "Projectile.h"

class CTracerProjectile : public CProjectile  
{
public:
	void Draw();
	void Update();
	CTracerProjectile(const float3 pos,const float3 speed,const float range,CUnit* owner);
	virtual ~CTracerProjectile();
	float speedf;
	float length;
	float drawLength;
	float3 dir;

};

#endif // !defined(AFX_TRACERPROJECTILE_H__7BB823C2_7EB6_11D4_AD55_0080ADA84DE3__INCLUDED_)


#endif /* TRACERPROJECTILE_H */
