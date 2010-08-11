/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GENERIC_PARTICLE_PROJECTILE_H
#define GENERIC_PARTICLE_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"
#include "Rendering/Textures/TextureAtlas.h"

class CColorMap;

class CGenericParticleProjectile : public CProjectile
{
	CR_DECLARE(CGenericParticleProjectile);

public:
	CGenericParticleProjectile(const float3& pos, const float3& speed, CUnit* owner);
	~CGenericParticleProjectile();

	virtual void Draw();
	virtual void Update();

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
