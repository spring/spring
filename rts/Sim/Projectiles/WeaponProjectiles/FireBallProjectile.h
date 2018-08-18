/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FIRE_BALL_PROJECTILE_H
#define _FIRE_BALL_PROJECTILE_H

#include "WeaponProjectile.h"
#include <algorithm>
#include <deque>

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

	int GetProjectilesCount() const override {
		return (sparks.size() + std::min(size_t(10), sparks.size()));
	}

	void Collision() override;

	struct Spark {
		CR_DECLARE_STRUCT(Spark)
		float3 pos;
		float3 speed;
		float size;
		int ttl;
	};

private:
	std::deque<Spark> sparks;

	void EmitSpark();
};

#endif // _FIRE_BALL_PROJECTILE_H
