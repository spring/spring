/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GENERIC_PARTICLE_PROJECTILE_H
#define GENERIC_PARTICLE_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"
#include "Rendering/Textures/TextureAtlas.h"

class CColorMap;

class CGenericParticleProjectile : public CProjectile
{
	CR_DECLARE_DERIVED(CGenericParticleProjectile)

public:
	CGenericParticleProjectile() { }
	CGenericParticleProjectile(
		const CUnit* owner,
		const float3& pos,
		const float3& speed
	);

	void Draw(GL::RenderDataBufferTC* va) const override;
	void Update() override;

	int GetProjectilesCount() const override { return 1; }

public:
	float3 gravity;

	AtlasedTexture* texture;
	CColorMap* colorMap;
	bool directional;

	float life;
	float decayrate;
	float size;

	float airdrag;
	float sizeGrowth;
	float sizeMod;
};

#endif // GENERIC_PARTICLE_PROJECTILE_H
