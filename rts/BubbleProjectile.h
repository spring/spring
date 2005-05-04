#pragma once
#include "projectile.h"

class CBubbleProjectile :
	public CProjectile
{
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
