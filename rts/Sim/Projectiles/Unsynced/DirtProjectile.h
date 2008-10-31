// DirtProjectile.h: interface for the CDirtProjectile class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __DIRT_PROJECTILE_H__
#define __DIRT_PROJECTILE_H__

#include "Sim/Projectiles/Projectile.h"

struct AtlasedTexture;

class CDirtProjectile : public CProjectile
{
	CR_DECLARE(CDirtProjectile);
public:
	virtual void Draw();
	virtual void Update();
	CDirtProjectile();
	CDirtProjectile(const float3 pos,const float3 speed,const float ttl,const float size,const float expansion,float slowdown,CUnit* owner,const float3& color GML_PARG_H);
	virtual ~CDirtProjectile();

	float alpha;
	float alphaFalloff;
	float size;
	float sizeExpansion;
	float slowdown;
	float3 color;

	AtlasedTexture *texture;
};

#endif // __DIRT_PROJECTILE_H__
