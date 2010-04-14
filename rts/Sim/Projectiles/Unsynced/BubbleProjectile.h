/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __BUBBLE_PROJECTILE_H__
#define __BUBBLE_PROJECTILE_H__

#include "Sim/Projectiles/Projectile.h"

class CBubbleProjectile :
	public CProjectile
{
	CR_DECLARE(CBubbleProjectile);
public:
	CBubbleProjectile(float3 pos,float3 speed,float ttl,float startSize,float sizeExpansion, CUnit* owner,float alpha);
	virtual ~CBubbleProjectile();
	void Update();
	void Draw();

	int ttl;
	float alpha;
	float size;
	float startSize;
	float sizeExpansion;
};

#endif // __BUBBLE_PROJECTILE_H__
