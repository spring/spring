// RocketProjectile.h: interface for the CRocketProjectile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ROCKETPROJECTILE_H__DDA2ECE1_F7D3_11D5_AD55_A6010A7FC06C__INCLUDED_)
#define AFX_ROCKETPROJECTILE_H__DDA2ECE1_F7D3_11D5_AD55_A6010A7FC06C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WeaponProjectile.h"
#include "damagearray.h"

class CRocketProjectile : public CWeaponProjectile  
{
public:
	void Draw();
	void Collision(CUnit* unit);
	void Collision();
	virtual void Update();
	CRocketProjectile(const float3& pos,const float3& speed,CUnit* owner,DamageArray damages,float gravity,float wobble,float maxSpeed,const float3& targPos,float tracking, WeaponDef *weaponDef);
	virtual ~CRocketProjectile();

	float3 dir;
	float wobble;
	float myGravity;
	float maxSpeed;
	DamageArray damages;
	float curSpeed;
	float3 oldSmoke;
	float3 oldDir;
	float3 targetPos;
	int age;
	float3 newWobble;
	float3 oldWobble;
	float wobbleUse;
	float tracking;
};

#endif // !defined(AFX_ROCKETPROJECTILE_H__DDA2ECE1_F7D3_11D5_AD55_A6010A7FC06C__INCLUDED_)

