/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FIRE_BALL_PROJECTILE_H
#define _FIRE_BALL_PROJECTILE_H

#include "WeaponProjectile.h"

class CFireBallProjectile : public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CFireBallProjectile)
	CR_DECLARE_SUB(Spark)

public:
	// creg only
	CFireBallProjectile() { }
	CFireBallProjectile(const ProjectileParams& params);

	void Draw(GL::RenderDataBufferTC* va) const override;
	void Update() override;
	void Collision() override;

	int GetProjectilesCount() const override {
		return (numSparks + std::min(10u, numSparks));
	}

private:
	void EmitSpark();
	void TickSparks();

private:
	struct Spark {
		CR_DECLARE_STRUCT(Spark)
		float3 pos;
		float3 speed;
		float size = 0.0f;
		int ttl = 0;
	};

	// one spark emitted per (checkCol) Update, each lives N frames
	Spark sparks[12];

	unsigned int numSparks = 0;
};

#endif // _FIRE_BALL_PROJECTILE_H
