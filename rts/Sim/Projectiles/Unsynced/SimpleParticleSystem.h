#ifndef __SIMPLE_PARTICLE_SYSTEM_H__
#define __SIMPLE_PARTICLE_SYSTEM_H__

#include "Sim/Projectiles/Projectile.h"
#include "Rendering/Textures/TextureAtlas.h"

class CColorMap;

class CSimpleParticleSystem : public CProjectile
{
	CR_DECLARE(CSimpleParticleSystem);
	CR_DECLARE_SUB(Particle);
public:
	virtual void Draw();
	virtual void Update();

	CSimpleParticleSystem(void);
	virtual ~CSimpleParticleSystem(void);
	virtual void Init(const float3& explosionPos, CUnit* owner GML_PARG_H);

	float3 emitVector;
	float3 emitMul;
	float3 gravity3;
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
	 Particle *particles;
};

//same behaviour as CSimpleParticleSystem but spawn the particles as independant objects
class CSphereParticleSpawner : public CSimpleParticleSystem
{
	CR_DECLARE(CSphereParticleSpawner);

public:
	CSphereParticleSpawner();
	~CSphereParticleSpawner();

	void Draw(){};
	void Update(){};

	virtual void Init(const float3& explosionPos, CUnit *owner GML_PARG_H);

};

#endif
