// DirtProjectile.h: interface for the CDirtProjectile class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __DIRT_PROJECTILE_H__
#define __DIRT_PROJECTILE_H__

#include "Projectile.h"

class CDirtProjectile : public CProjectile  
{
public:
public:
	virtual void Draw();
	virtual void Update();
	CDirtProjectile(const float3 pos,const float3 speed,const float ttl,const float size,const float expansion,float slowdown,CUnit* owner,const float3& color);
	virtual ~CDirtProjectile();

	float alpha;
	float alphaFalloff;
	float size;
	float sizeExpansion;
	float slowdown;
	float3 color;
};

#endif // __DIRT_PROJECTILE_H__
