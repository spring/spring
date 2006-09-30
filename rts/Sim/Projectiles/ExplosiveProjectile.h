// ExplosiveProjectile.h: interface for the CExplosiveProjectile class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __EXPLOSIVE_PROJECTILE_H__
#define __EXPLOSIVE_PROJECTILE_H__

#include "WeaponProjectile.h"
#include "Sim/Misc/DamageArray.h"

class CExplosiveProjectile : public CWeaponProjectile  
{
public:
	CExplosiveProjectile(const float3& pos,const float3& speed,CUnit* owner, WeaponDef *weaponDef, int ttl=100000,float areaOfEffect=8);
	virtual ~CExplosiveProjectile();
	virtual void Update();
	void Draw(void);
	void Collision(CUnit* unit);
	void Collision();
	int ShieldRepulse(CPlasmaRepulser* shield,float3 shieldPos, float shieldForce, float shieldMaxSpeed);

	int ttl;
	float areaOfEffect;
};

#endif // __EXPLOSIVE_PROJECTILE_H__
