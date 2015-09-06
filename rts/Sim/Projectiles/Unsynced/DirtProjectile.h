/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _DIRT_PROJECTILE_H
#define _DIRT_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

struct AtlasedTexture;

class CDirtProjectile : public CProjectile
{
	CR_DECLARE(CDirtProjectile)
public:
	CDirtProjectile();
	CDirtProjectile(
		CUnit* owner,
		const float3& pos,
		const float3& speed,
		const float ttl,
		const float size,
		const float expansion,
		float slowdown,
		const float3& color
	);
	virtual ~CDirtProjectile();

	virtual void Draw() override;
	virtual void Update() override;

	virtual int GetProjectilesCount() const override;

private:
	float alpha;
	float alphaFalloff;
	float size;
	float sizeExpansion;
	float slowdown;
	float3 color;

	AtlasedTexture* texture;
};

#endif // _DIRT_PROJECTILE_H
