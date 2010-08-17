/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SIMPLE_PARTICLE_SYSTEM_H
#define SIMPLE_PARTICLE_SYSTEM_H

#include "Sim/Projectiles/Projectile.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "System/float3.h"

class CUnit;
class CColorMap;

class CSimpleParticleSystem : public CProjectile
{
	CR_DECLARE(CSimpleParticleSystem);
	CR_DECLARE_SUB(Particle);

public:
	CSimpleParticleSystem();
	virtual ~CSimpleParticleSystem();

	virtual void Draw();
	virtual void Update();
	virtual void Init(const float3& explosionPos, CUnit* owner);

	float3 emitVector;
	float3 emitMul;
	float3 gravity;
	float particleSpeed;
	float particleSpeedSpread;

	float emitRot;
	float emitRotSpread;

	AtlasedTexture* texture;
	CColorMap* colorMap;
	bool directional;

	float particleLife;
	float particleLifeSpread;
	float particleSize;
	float particleSizeSpread;
	float airdrag;
	float sizeGrowth;
	float sizeMod;

	int numParticles;

	struct Particle
	{
		CR_DECLARE_STRUCT(Particle);

		float3 pos;
		float life;
		float3 speed;
		float decayrate;
		float size;
		float sizeGrowth;
		float sizeMod;
	};

protected:
	 Particle* particles;
};

/**
 * Same behaviour as CSimpleParticleSystem but spawns the particles
 * as independant objects
 */
class CSphereParticleSpawner : public CSimpleParticleSystem
{
	CR_DECLARE(CSphereParticleSpawner);

public:
	CSphereParticleSpawner();
	~CSphereParticleSpawner();

	void Draw() {};
	void Update() {};
	virtual void Init(const float3& explosionPos, CUnit* owner);

};

#endif // SIMPLE_PARTICLE_SYSTEM_H
