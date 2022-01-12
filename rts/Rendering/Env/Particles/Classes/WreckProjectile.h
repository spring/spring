/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WRECK_PROJECTILE_H
#define WRECK_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CWreckProjectile : public CProjectile
{
	CR_DECLARE_DERIVED(CWreckProjectile)
public:
	CWreckProjectile() { }
	CWreckProjectile(CUnit* owner, float3 pos, float3 speed, float temperature);

	void Update() override;
	void Draw(GL::RenderDataBufferTC* va) const override;
	void DrawOnMinimap(GL::RenderDataBufferC* va) override;

	int GetProjectilesCount() const override { return 1; }
};

#endif /* WRECK_PROJECTILE_H */
