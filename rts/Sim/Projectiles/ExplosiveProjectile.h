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

#endif // __EXPLOSIVE_PROJECTILE_H__
