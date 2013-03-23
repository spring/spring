/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LASER_PROJECTILE_H
#define LASER_PROJECTILE_H

#include "WeaponProjectile.h"
#include "Sim/Misc/DamageArray.h"

class CLaserProjectile : public CWeaponProjectile
{
	CR_DECLARE(CLaserProjectile);
public:
	CLaserProjectile(const ProjectileParams& params);

	void Draw();
	void Update();
	void Collision(CUnit* unit);
	void Collision(CFeature* feature);
	void Collision();
	int ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos, float shieldForce, float shieldMaxSpeed);

private:
	float speedf;

	float length;
	float curLength;
	float intensity;
	float intensityFalloff;
	float midtexx;

	/**
	 * Number of frames the laser had left to expand
	 * if it impacted before reaching full length.
	 */
	int stayTime;

	float3 color;
	float3 color2;
};

#endif /* LASER_PROJECTILE_H */
