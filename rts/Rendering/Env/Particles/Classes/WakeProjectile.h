/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WAKE_PROJECTILE_H
#define WAKE_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CWakeProjectile : public CProjectile
{
	CR_DECLARE_DERIVED(CWakeProjectile)
public:
	CWakeProjectile() { }
	CWakeProjectile(
		CUnit* owner,
		const float3& pos,
		const float3& speed,
		float startSize,
		float sizeExpansion,
		float alpha,
		float alphaFalloff,
		float fadeupTime
	);

	void Update() override;
	void Draw(GL::RenderDataBufferTC* va) const override;

	int GetProjectilesCount() const override { return 1; }

private:
	float alpha;
	float alphaFalloff;
	float alphaAdd;

	int alphaAddTime;

	float size;
	float sizeExpansion;
	float rotation;
	float rotSpeed;
};

#endif /* WAKE_PROJECTILE_H */
