/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EMG_PROJECTILE_H
#define _EMG_PROJECTILE_H

#include "WeaponProjectile.h"

class CEmgProjectile : public CWeaponProjectile
{
	CR_DECLARE(CEmgProjectile);
public:
	CEmgProjectile(const ProjectileParams& params);

	void Update();
	void Draw();

	int ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos,
			float shieldForce, float shieldMaxSpeed);

private:
	float intensity;
	float3 color;
};

#endif // _EMG_PROJECTILE_H
