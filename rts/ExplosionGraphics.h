#ifndef __EXPLOSION_GRAPHICS_H__
#define __EXPLOSION_GRAPHICS_H__

#include "DamageArray.h"

class CUnit;

class CExplosionGraphics
{
public:
	CExplosionGraphics(void);
	virtual ~CExplosionGraphics(void);

	virtual void Explosion(const float3 &pos, const DamageArray& damages, float radius, CUnit *owner,float gfxMod)=0;
};

class CStdExplosionGraphics : public CExplosionGraphics
{
public:
	CStdExplosionGraphics(void);
	virtual ~CStdExplosionGraphics(void);

	void Explosion(const float3 &pos, const DamageArray& damages, float radius, CUnit *owner,float gfxMod);
};

#endif // __EXPLOSION_GRAPHICS_H__
