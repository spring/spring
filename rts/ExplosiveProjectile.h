// ExplosiveProjectile.h: interface for the CExplosiveProjectile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EXPLOSIVEPROJECTILE_H__37A8B922_F94D_11D5_AD55_B82BBF40786C__INCLUDED_)
#define AFX_EXPLOSIVEPROJECTILE_H__37A8B922_F94D_11D5_AD55_B82BBF40786C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WeaponProjectile.h"
#include "damagearray.h"

class CExplosiveProjectile : public CWeaponProjectile  
{
public:
	void Collision(CUnit* unit);
	void Collision();
	virtual void Update();
	CExplosiveProjectile(const float3& pos,const float3& speed,CUnit* owner,const DamageArray& damages, WeaponDef *weaponDef, int ttl=100000,float areaOfEffect=8);
	virtual ~CExplosiveProjectile();

	DamageArray damages;
	int ttl;
	float areaOfEffect;
	void Draw(void);
};

#endif // !defined(AFX_EXPLOSIVEPROJECTILE_H__37A8B922_F94D_11D5_AD55_B82BBF40786C__INCLUDED_)
