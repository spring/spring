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
	CR_DECLARE_DERIVED(CSimpleParticleSystem)
	CR_DECLARE_SUB(Particle)

public:
	CSimpleParticleSystem();
	virtual ~CSimpleParticleSystem() { particles.clear(); }

	virtual void Draw(GL::RenderDataBufferTC* va) const override;
	virtual void Update() override;
	virtual void Init(const CUnit* owner, const float3& offset) override;

	int GetProjectilesCount() const override { return numParticles; }

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

protected:
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
		CR_DECLARE_STRUCT(Particle)

		float3 pos;
		float3 speed;

		float life;
		float decayrate;
		float size;
		float sizeGrowth;
		float sizeMod;
	};

protected:
	 std::vector<Particle> particles;
};

/**
 * Same behaviour as CSimpleParticleSystem but spawns the particles
 * as independant objects
 */
class CSphereParticleSpawner : public CSimpleParticleSystem
{
	CR_DECLARE_DERIVED(CSphereParticleSpawner)

public:
	CSphereParticleSpawner() {}

	void Draw(GL::RenderDataBufferTC* va) const override {}
	void Update() override {}
	void Init(const CUnit* owner, const float3& offset) override;

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);
};

#endif // SIMPLE_PARTICLE_SYSTEM_H
