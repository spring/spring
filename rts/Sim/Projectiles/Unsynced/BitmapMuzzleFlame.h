/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BITMAP_MUZZLE_FLAME_H
#define BITMAP_MUZZLE_FLAME_H

#include "Sim/Projectiles/Projectile.h"

class CColorMap;
struct AtlasedTexture;

class CBitmapMuzzleFlame : public CProjectile
{
	CR_DECLARE(CBitmapMuzzleFlame);

public:
	CBitmapMuzzleFlame();

	void Draw();
	void Update();

	void Init(const CUnit* owner, const float3& offset);

private:
	AtlasedTexture* sideTexture;
	AtlasedTexture* frontTexture;

	CColorMap* colorMap;

	float size;
	float length;
	float sizeGrowth;
	float frontOffset;
	int ttl;

	float invttl;
	float life;
	int createTime;
};

#endif // BITMAP_MUZZLE_FLAME_H
