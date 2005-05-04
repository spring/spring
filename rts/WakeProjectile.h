#pragma once
#include "projectile.h"

class CWakeProjectile :
	public CProjectile
{
public:
	CWakeProjectile(float3 pos,float3 speed,float startSize,float sizeExpansion, CUnit* owner,float alpha,float alphaFalloff,float fadeupTime);
	virtual ~CWakeProjectile();
	void Update();
	void Draw();

	float alpha;
	float alphaFalloff;
	float alphaAdd;
	int alphaAddTime;
	float size;
	float sizeExpansion;
	float rotation;
	float rotSpeed;
};
