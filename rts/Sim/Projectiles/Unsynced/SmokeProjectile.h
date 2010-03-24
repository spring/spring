/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMOKEPROJECTILE_H
#define SMOKEPROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CSmokeProjectile : public CProjectile
{
public:
	CR_DECLARE(CSmokeProjectile)

	void Update();
	void Draw();
	void Init(const float3& pos, CUnit *owner GML_PARG_H);
	CSmokeProjectile();
	CSmokeProjectile(const float3& pos,const float3& speed,float ttl,float startSize,float sizeExpansion, CUnit* owner,float color GML_PARG_H);
	virtual ~CSmokeProjectile();

	float color;
	float age;
	float ageSpeed;
	float size;
	float startSize;
	float sizeExpansion;
	int textureNum;
};

#endif /* SMOKEPROJECTILE_H */
