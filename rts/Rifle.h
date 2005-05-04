// Rifle.h: interface for the CRifle class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RIFLE_H__3D0EEF61_FBE1_11D5_AD55_B771F8FC7D53__INCLUDED_)
#define AFX_RIFLE_H__3D0EEF61_FBE1_11D5_AD55_B771F8FC7D53__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Weapon.h"

class CRifle : public CWeapon  
{
public:
	bool TryTarget(const float3 &pos,bool userTarget,CUnit* unit);
	void Update();
	CRifle(CUnit* owner);
	virtual ~CRifle();

	void Fire(void);
};

#endif // !defined(AFX_RIFLE_H__3D0EEF61_FBE1_11D5_AD55_B771F8FC7D53__INCLUDED_)

