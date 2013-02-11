/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FIRE_BALL_PROJECTILE_H
#define _FIRE_BALL_PROJECTILE_H

#include "WeaponProjectile.h"
#include <deque>
#include "lib/gml/gmlcnf.h"

#if defined(USE_GML) && GML_ENABLE_SIM
#define SPARK_QUEUE gmlCircularQueue<CFireBallProjectile::Spark,32>
#else
#define SPARK_QUEUE std::deque<Spark>
#endif

class CFireBallProjectile : public CWeaponProjectile
{
	CR_DECLARE(CFireBallProjectile);
	CR_DECLARE_SUB(Spark);
public:
	CFireBallProjectile(const ProjectileParams& params);
	~CFireBallProjectile();

	void Draw();
	void Update();

	void Collision();

	struct Spark {
		CR_DECLARE_STRUCT(Spark);
		float3 pos;
		float3 speed;
		float size;
		int ttl;
	};

private:
	SPARK_QUEUE sparks;

	void EmitSpark();
};

#endif // _FIRE_BALL_PROJECTILE_H
