/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FIRE_PROJECTILE_H
#define FIRE_PROJECTILE_H

#include "Projectile.h"
#include <list>

#if defined(USE_GML) && GML_ENABLE_SIM
#define SUBPARTICLE_LIST gmlCircularQueue<CFireProjectile::SubParticle,16>
#else
#define SUBPARTICLE_LIST std::list<SubParticle>
#endif

class CFireProjectile : public CProjectile
{
	CR_DECLARE(CFireProjectile);
	CR_DECLARE_SUB(SubParticle);
public:
	CFireProjectile(const float3& pos,const float3& speed,CUnit* owner,int emitTtl,float emitRadius,int particleTtl,float particleSize);
	~CFireProjectile();

	void Draw();
	void Update();
	void StopFire();

	int ttl;
	float3 emitPos;
	float emitRadius;
		
	int particleTime;
	float particleSize;
	float ageSpeed;

	struct SubParticle {
		CR_DECLARE(SubParticle);
		float3 pos;
		float3 posDif;
		float age;
		float maxSize;
		float rotSpeed;
		int smokeType;
	};

	SUBPARTICLE_LIST subParticles;
	SUBPARTICLE_LIST subParticles2;

private:
	virtual void FireImpl() {};
};

#endif // FIRE_PROJECTILE_H
