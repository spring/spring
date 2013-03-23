/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FIRE_PROJECTILE_H
#define FIRE_PROJECTILE_H

#include "Projectile.h"
#include <list>
#include "lib/gml/gmlcnf.h"


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
		CR_DECLARE_STRUCT(SubParticle);

		float3 pos;
		float3 posDif;
		float age;
		float maxSize;
		float rotSpeed;
		int smokeType;
	};

#if defined(USE_GML) && GML_ENABLE_SIM
	typedef gmlCircularQueue<CFireProjectile::SubParticle,16> part_list_type;
#else
	typedef std::list<SubParticle> part_list_type;
#endif

	part_list_type subParticles;
	part_list_type subParticles2;

private:
	virtual void FireImpl() {};
};

#endif // FIRE_PROJECTILE_H
