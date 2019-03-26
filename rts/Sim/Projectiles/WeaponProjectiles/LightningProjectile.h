/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LIGHTNING_PROJECTILE_H
#define LIGHTNING_PROJECTILE_H

#include "WeaponProjectile.h"

class CWeapon;

class CLightningProjectile : public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CLightningProjectile)
public:
	// creg only
	CLightningProjectile() { }

	CLightningProjectile(const ProjectileParams& params);

	void Update() override;
	void Draw(GL::RenderDataBufferTC* va) const override;
	void DrawOnMinimap(GL::RenderDataBufferC* va) override;

	int GetProjectilesCount() const override { return (NUM_DISPLACEMENTS * 2); }

private:
	static constexpr unsigned int NUM_DISPLACEMENTS = 10;

	float3 color;

	float displacements[NUM_DISPLACEMENTS];
	float displacements2[NUM_DISPLACEMENTS];
};

#endif /* LIGHTNING_PROJECTILE_H */
