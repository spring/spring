/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WAKE_PROJECTILE_H
#define WAKE_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CWakeProjectile : public CProjectile
{
	CR_DECLARE(CWakeProjectile);
public:
	CWakeProjectile(const float3& pos, const float3 speed, float startSize,
			float sizeExpansion, CUnit* owner, float alpha, float alphaFalloff,
			float fadeupTime);
	virtual ~CWakeProjectile();

	void Update();
	void Draw();

private:
	float alpha;
	float alphaFalloff;
	float alphaAdd;
	int alphaAddTime;
	float size;
	float sizeExpansion;
	float rotation;
	float rotSpeed;
};

#endif /* WAKE_PROJECTILE_H */
