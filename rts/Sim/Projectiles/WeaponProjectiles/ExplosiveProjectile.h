/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EXPLOSIVE_PROJECTILE_H
#define _EXPLOSIVE_PROJECTILE_H

#include "WeaponProjectile.h"

class CExplosiveProjectile : public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CExplosiveProjectile)
public:
	CExplosiveProjectile(const ProjectileParams& params);

	void Update() override;
	void Draw() override;

	virtual int GetProjectilesCount() const override;

	int ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed) override;

private:
	CExplosiveProjectile() { }

	float invttl;
	float curTime;
};

#endif // _EXPLOSIVE_PROJECTILE_H
