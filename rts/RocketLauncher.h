// RocketLauncher.h: interface for the CRocketLauncher class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ROCKETLAUNCHER_H__4898F724_F864_11D5_AD55_E2927DE7ED6F__INCLUDED_)
#define AFX_ROCKETLAUNCHER_H__4898F724_F864_11D5_AD55_E2927DE7ED6F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Weapon.h"

class CRocketLauncher : public CWeapon  
{
public:
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);
	void Update();
	CRocketLauncher(CUnit* owner);
	virtual ~CRocketLauncher();

	void Fire(void);
};

#endif // !defined(AFX_ROCKETLAUNCHER_H__4898F724_F864_11D5_AD55_E2927DE7ED6F__INCLUDED_)
