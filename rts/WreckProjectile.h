// WreckProjectile.h: interface for the CWreckProjectile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WRECKPROJECTILE_H__1A3049E1_6CCF_11D5_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_WRECKPROJECTILE_H__1A3049E1_6CCF_11D5_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Projectile.h"

class CWreckProjectile : public CProjectile  
{
public:
	void Update();
	CWreckProjectile(float3 pos,float3 speed,float temperature,CUnit* owner);
	virtual ~CWreckProjectile();

	void Draw(void);
};

#endif // !defined(AFX_WRECKPROJECTILE_H__1A3049E1_6CCF_11D5_AD55_0080ADA84DE3__INCLUDED_)

