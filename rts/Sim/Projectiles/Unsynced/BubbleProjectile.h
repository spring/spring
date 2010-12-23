/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BUBBLE_PROJECTILE_H
#define _BUBBLE_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

class CBubbleProjectile : public CProjectile
{
	CR_DECLARE(CBubbleProjectile);
public:
	CBubbleProjectile(float3 pos,float3 speed, float ttl, float startSize,
			float sizeExpansion, CUnit* owner, float alpha);
	virtual ~CBubbleProjectile();

	void Update();
	void Draw();

private:
	int ttl;
	float alpha;
	float size;
	float startSize;
	float sizeExpansion;
};

#endif // _BUBBLE_PROJECTILE_H
