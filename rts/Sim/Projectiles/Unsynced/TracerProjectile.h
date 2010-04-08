/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TRACERPROJECTILE_H
#define TRACERPROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CTracerProjectile : public CProjectile
{
public:
	CR_DECLARE(CTracerProjectile);

	void Draw();
	void Update();
	void Init(const float3& pos, CUnit *owner);
	CTracerProjectile();
	CTracerProjectile(const float3 pos,const float3 speed,const float range,CUnit* owner);
	virtual ~CTracerProjectile();
	float speedf;
	float length;
	float drawLength;
	float3 dir;

};

#endif /* TRACERPROJECTILE_H */
