/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EXPLOSIVE_PROJECTILE_H
#define _EXPLOSIVE_PROJECTILE_H

#include "WeaponProjectile.h"

class CExplosiveProjectile : public CWeaponProjectile
{
	CR_DECLARE(CExplosiveProjectile);
public:
	CExplosiveProjectile(const float3& pos, const float3& speed,
		CUnit* owner, const WeaponDef* weaponDef,
		int ttl = 100000, float areaOfEffect = 8.0f,
		float gravity = 0.0f);

	void Update();
	void Draw();
	void Collision(CUnit* unit);
	void Collision();

	int ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos,
			float shieldForce, float shieldMaxSpeed);

private:
	float areaOfEffect;
	float invttl;
	float curTime;
};

#endif // _EXPLOSIVE_PROJECTILE_H
