/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FLAME_PROJECTILE_H_
#define _FLAME_PROJECTILE_H_

#include "WeaponProjectile.h"

class CFlameProjectile : public CWeaponProjectile
{
	CR_DECLARE(CFlameProjectile);
public:
	CFlameProjectile(const ProjectileParams& params);

	void Update();
	void Draw();
	void Collision();

	int ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos,
			float shieldForce, float shieldMaxSpeed);

private:
	float curTime;
	/// precentage of lifetime when the projectile is active and can collide
	float physLife;
	float invttl;

	float3 spread;
};

#endif // _FLAME_PROJECTILE_H_
