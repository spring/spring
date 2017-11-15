/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef REPULSE_GFX_H
#define REPULSE_GFX_H

#include "Sim/Projectiles/Projectile.h"

class CUnit;

class CRepulseGfx : public CProjectile
{
	CR_DECLARE_DERIVED(CRepulseGfx)
public:
	CRepulseGfx() { }
	CRepulseGfx(
		CUnit* owner,
		CProjectile* repulsee,
		float maxOwnerDist,
		const float4& gfxColor
	);

	void Draw(GL::RenderDataBufferTC* va) const override;
	void Update() override;

	int GetProjectilesCount() const override { return 20; }

	void DependentDied(CObject* o) override;

private:
	CProjectile* repulsed;

	int age;

	float sqMaxOwnerDist;
	float vertexDists[25];

	float4 color;
};

#endif // REPULSE_GFX_H
