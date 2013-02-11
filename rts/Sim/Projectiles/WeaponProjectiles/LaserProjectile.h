/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LASER_PROJECTILE_H
#define LASER_PROJECTILE_H

#include "WeaponProjectile.h"
#include "Sim/Misc/DamageArray.h"

class CLaserProjectile : public CWeaponProjectile
{
	CR_DECLARE(CLaserProjectile);
public:
	CLaserProjectile(const ProjectileParams& params,
			float length, const float3& color, const float3& color2,
			float intensity);
	virtual ~CLaserProjectile();
	void Draw();
	void Update();
	void Collision(CUnit* unit);
	void Collision(CFeature* feature);
	void Collision();
	int ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos, float shieldForce, float shieldMaxSpeed);

private:
	float intensity;
	float3 color;
	float3 color2;
	float length;
	float curLength;
	float speedf;
	float intensityFalloff;
	float midtexx;
	/**
	 * Number of frames the laser had left to expand
	 * if it impacted before reaching full length.
	 */
	int stayTime;
};

#endif /* LASER_PROJECTILE_H */
