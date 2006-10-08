#ifndef __SIMPLE_PARTICLE_SYSTEM_H__
#define __SIMPLE_PARTICLE_SYSTEM_H__

#include "Projectile.h"
#include "Rendering/Textures/TextureAtlas.h"

class CColorMap;

class CSimpleParticleSystem : public CProjectile
{
	CR_DECLARE(CSimpleParticleSystem);
public:
	virtual void Draw();
	virtual void Update();

	CSimpleParticleSystem(void);
	virtual ~CSimpleParticleSystem(void);
	virtual void Init(const float3& explosionPos, CUnit *owner);

	float3 emitVector;
	float3 emitMul;
	float3 gravity;
	float particleSpeed;
	float particleSpeedSpread;

	float emitRot;
	float emitRotSpread;

	AtlasedTexture *texture;
	CColorMap *colorMap;
	bool directional;

	float particleLife;
	float particleLifeSpread;
	float particleSize;
	float particleSizeSpread;
	float airdrag;

	int numParticles;

	struct Particle
	{
        float3 pos;
		float life;
		float3 speed;
		float decayrate;
		float size;
	};

protected:
	 Particle *particles;
};

#endif
