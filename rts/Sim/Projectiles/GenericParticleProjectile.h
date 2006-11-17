#ifndef __GENERIC_PARTICLE_PROJECTILE_H__
#define __GENERIC_PARTICLE_PROJECTILE_H__

#include "Projectile.h"
#include "Rendering/Textures/TextureAtlas.h"

class CColorMap;
class CGenericParticleProjectile : public CProjectile
{
public:
	float3 gravity;

	AtlasedTexture *texture;
	CColorMap *colorMap;
	bool directional;

	float life;
	float decayrate;
	float size;

	float airdrag;
	float sizeGrowth;
	float sizeMod;

	CGenericParticleProjectile(const float3& pos,const float3& speed,CUnit* owner);
	~CGenericParticleProjectile(void);

	virtual void Update();
	virtual void Draw();
};

#endif
