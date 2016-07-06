/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EMG_PROJECTILE_H
#define _EMG_PROJECTILE_H

#include "WeaponProjectile.h"

class CEmgProjectile : public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CEmgProjectile)
public:
	CEmgProjectile() { }
	CEmgProjectile(const ProjectileParams& params);

	void Update() override;
	void Draw() override;

	virtual int GetProjectilesCount() const override;

	int ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed) override;

private:
	float intensity;
	float3 color;
};

#endif // _EMG_PROJECTILE_H
