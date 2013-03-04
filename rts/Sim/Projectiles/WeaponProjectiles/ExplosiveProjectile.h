/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EXPLOSIVE_PROJECTILE_H
#define _EXPLOSIVE_PROJECTILE_H

#include "WeaponProjectile.h"

class CExplosiveProjectile : public CWeaponProjectile
{
	CR_DECLARE(CExplosiveProjectile);
public:
	CExplosiveProjectile(const ProjectileParams& params);

	void Update();
	void Draw();

	int ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos,
			float shieldForce, float shieldMaxSpeed);

private:
	float areaOfEffect;
	float invttl;
	float curTime;
};

#endif // _EXPLOSIVE_PROJECTILE_H
