/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FIRE_PROJECTILE_H
#define FIRE_PROJECTILE_H

#include "Projectile.h"
#include <list>


class CFireProjectile : public CProjectile
{
	CR_DECLARE_DERIVED(CFireProjectile)
	CR_DECLARE_SUB(SubParticle)
public:
	CFireProjectile() { }
	CFireProjectile(
		const float3& pos,
		const float3& spd,
		CUnit* owner,
		int emitTtl,
		int particleTtl,
		float emitRadius,
		float particleSize
	);

	void Draw() override;
	void Update() override;
	void StopFire();

	virtual int GetProjectilesCount() const override;

public:
	int ttl;
	float3 emitPos;
	float emitRadius;

	int particleTime;
	float particleSize;
	float ageSpeed;

	struct SubParticle {
		CR_DECLARE_STRUCT(SubParticle)

		float3 pos;
		float3 posDif;
		float age;
		float maxSize;
		float rotSpeed;
		int smokeType;
	};

	typedef std::list<SubParticle> part_list_type; //FIXME

	part_list_type subParticles;
	part_list_type subParticles2;
};

#endif // FIRE_PROJECTILE_H
