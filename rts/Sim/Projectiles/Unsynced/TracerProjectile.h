/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TRACER_PROJECTILE_H
#define TRACER_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CTracerProjectile : public CProjectile
{
public:
	CR_DECLARE(CTracerProjectile);

	CTracerProjectile();
	CTracerProjectile(const float3 pos, const float3 speed, const float range, CUnit* owner);
	virtual ~CTracerProjectile();

	void Draw();
	void Update();
	void Init(const float3& pos, CUnit *owner);

	float speedf;
	float length;
	float drawLength;
	float3 dir;
};

#endif /* TRACER_PROJECTILE_H */
