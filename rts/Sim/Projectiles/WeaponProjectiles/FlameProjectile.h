/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FLAME_PROJECTILE_H_
#define _FLAME_PROJECTILE_H_

#include "WeaponProjectile.h"

class CFlameProjectile : public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CFlameProjectile)
public:
	// creg only
	CFlameProjectile() { }

	CFlameProjectile(const ProjectileParams& params);

	void Update() override;
	void Draw(GL::RenderDataBufferTC* va) const override;
	void Collision() override;

	int GetProjectilesCount() const override { return 1; }

	int ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed) override;

private:
	float curTime;
	/// precentage of lifetime when the projectile is active and can collide
	float physLife;
	float invttl;

	float3 spread;
};

#endif // _FLAME_PROJECTILE_H_
