/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __GENERIC_PARTICLE_PROJECTILE_H__
#define __GENERIC_PARTICLE_PROJECTILE_H__

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

#endif // __GENERIC_PARTICLE_PROJECTILE_H__
