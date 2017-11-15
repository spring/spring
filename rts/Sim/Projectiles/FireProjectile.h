/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FIRE_PROJECTILE_H
#define FIRE_PROJECTILE_H

#include "Projectile.h"
#include <deque>


class CFireProjectile : public CProjectile
{
	CR_DECLARE_DERIVED(CFireProjectile)
	CR_DECLARE_SUB(SubParticle)
public:
	CFireProjectile(
		const float3& pos,
		const float3& spd,
		CUnit* owner,
		int emitTtl,
		int particleTtl,
		float emitRadius,
		float particleSize
	);

	void Draw(GL::RenderDataBufferTC* va) const override;
	void Update() override;
	void StopFire() { ttl = 0; }

	int GetProjectilesCount() const override { return (subParticles2.size() + subParticles.size() * 2); }

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

	std::deque<SubParticle> subParticles;
	std::deque<SubParticle> subParticles2;

private:
	CFireProjectile() { }

};

#endif // FIRE_PROJECTILE_H
