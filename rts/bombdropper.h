// Cannon.h: interface for the CCannon class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BombDropper_H__37A8B921_F94D_11D5_AD55_B82BBF40786C__INCLUDED_)
#define AFX_BombDropper_H__37A8B921_F94D_11D5_AD55_B82BBF40786C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Weapon.h"

class CBombDropper : public CWeapon  
{
public:
	void Update();
	CBombDropper(CUnit* owner);
	virtual ~CBombDropper();

	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);
	void Fire(void);
	void Init(void);
};

#endif // !defined(AFX_CANNON_H__37A8B921_F94D_11D5_AD55_B82BBF40786C__INCLUDED_)
