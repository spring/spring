/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LASER_PROJECTILE_H
#define LASER_PROJECTILE_H

#include "WeaponProjectile.h"

class CLaserProjectile : public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CLaserProjectile)
public:
	CLaserProjectile(const ProjectileParams& params);

	void Draw() override;
	void Update() override;
	void Collision(CUnit* unit) override;
	void Collision(CFeature* feature) override;
	void Collision() override;
	int ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed) override;

	virtual int GetProjectilesCount() const override;

private:
	void UpdateIntensity();
	void UpdateLength();
	void UpdatePos(const float4& oldSpeed);
	void CollisionCommon(const float3& oldPos);

private:
	CLaserProjectile() { }

	float speedf;

	float maxLength;
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
