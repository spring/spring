/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMOKE_PROJECTILE_H
#define SMOKE_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"
#include "System/float3.h"

class CUnit;

class CSmokeProjectile : public CProjectile
{
	CR_DECLARE(CSmokeProjectile)

public:
	CSmokeProjectile();
	CSmokeProjectile(
		CUnit* owner,
		const float3& pos,
		const float3& speed,
		float ttl,
		float startSize,
		float sizeExpansion,
		float color
	);

	void Update();
	void Draw();
	void Init(const CUnit* owner, const float3& offset);

private:
	float color;
	float age;
	float ageSpeed;
public:
	float size;
private:
	float startSize;
	float sizeExpansion;
	int textureNum;
};

#endif /* SMOKE_PROJECTILE_H */
