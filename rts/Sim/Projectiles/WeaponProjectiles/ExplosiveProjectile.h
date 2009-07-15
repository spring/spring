// ExplosiveProjectile.h: interface for the CExplosiveProjectile class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __EXPLOSIVE_PROJECTILE_H__
#define __EXPLOSIVE_PROJECTILE_H__

#include "WeaponProjectile.h"
#include "Sim/Misc/DamageArray.h"

class CExplosiveProjectile : public CWeaponProjectile
{
	CR_DECLARE(CExplosiveProjectile);
public:
	CExplosiveProjectile(const float3& pos, const float3& speed, CUnit* owner,
		const WeaponDef *weaponDef, int ttl = 100000, float areaOfEffect = 8,
		float gravity = 0 GML_PARG_H);
	virtual ~CExplosiveProjectile();
	virtual void Update();
	void Draw(void);
	void Collision(CUnit* unit);
	void Collision();
	int ShieldRepulse(CPlasmaRepulser* shield,float3 shieldPos, float shieldForce, float shieldMaxSpeed);

	float areaOfEffect;
	float invttl;
	float curTime;
};

#endif // __EXPLOSIVE_PROJECTILE_H__
