/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FIRE_BALL_PROJECTILE_H
#define _FIRE_BALL_PROJECTILE_H

#include "WeaponProjectile.h"
#include <deque>

class CFireBallProjectile : public CWeaponProjectile
{
	CR_DECLARE_DERIVED(CFireBallProjectile)
	CR_DECLARE_SUB(Spark)
public:
	CFireBallProjectile() { }
	CFireBallProjectile(const ProjectileParams& params);

	void Draw() override;
	void Update() override;

	virtual int GetProjectilesCount() const override;

	void Collision() override;

	struct Spark {
		CR_DECLARE_STRUCT(Spark)
		float3 pos;
		float3 speed;
		float size;
		int ttl;
	};

	typedef std::deque<Spark> spark_list_type;

private:
	spark_list_type sparks;

	void EmitSpark();
};

#endif // _FIRE_BALL_PROJECTILE_H
