/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TRACER_PROJECTILE_H
#define TRACER_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CTracerProjectile : public CProjectile
{
public:
	CR_DECLARE(CTracerProjectile);

	CTracerProjectile();
	CTracerProjectile(
		CUnit* owner,
		const float3& pos,
		const float3& speed,
		const float range
	);

	void Draw();
	void Update();
	void Init(const CUnit* owner, const float3& offset);

private:
	float speedf;
	float length;
	float drawLength;
};

#endif /* TRACER_PROJECTILE_H */
