// Cannon.h: interface for the CCannon class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CANNON_H__37A8B921_F94D_11D5_AD55_B82BBF40786C__INCLUDED_)
#define AFX_CANNON_H__37A8B921_F94D_11D5_AD55_B82BBF40786C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Weapon.h"

class CCannon : public CWeapon  
{
public:
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);
	void Update();
	CCannon(CUnit* owner);
	virtual ~CCannon();
	void Init(void);
	void Fire(void);

	float maxPredict;
	float minPredict;
	bool highTrajectory;
	bool selfExplode;
	void SlowUpdate(void);
};

#endif // !defined(AFX_CANNON_H__37A8B921_F94D_11D5_AD55_B82BBF40786C__INCLUDED_)
