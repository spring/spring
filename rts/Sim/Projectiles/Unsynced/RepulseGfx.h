/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef REPULSE_GFX_H
#define REPULSE_GFX_H

#include "Sim/Projectiles/Projectile.h"

class CUnit;

class CRepulseGfx : public CProjectile
{
	CR_DECLARE(CRepulseGfx);
public:
	CRepulseGfx(CUnit* owner, CProjectile* repulsed, float maxDist, const float3& color);
	~CRepulseGfx();

	void Draw();
	void Update();

	void DependentDied(CObject* o);

	CProjectile* repulsed;
	float sqMaxDist;
	int age;
	float3 color;

	float difs[25];
};

#endif // REPULSE_GFX_H
