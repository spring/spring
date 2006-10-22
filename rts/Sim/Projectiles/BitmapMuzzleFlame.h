#ifndef BITMAPMUZZLEFLAME_H
#define BITMAPMUZZLEFLAME_H

#include "Projectile.h"

class CColorMap;
struct AtlasedTexture;
class CBitmapMuzzleFlame : public CProjectile
{
	CR_DECLARE(CBitmapMuzzleFlame);
public:
	CBitmapMuzzleFlame(void);
	~CBitmapMuzzleFlame(void);
	void Draw(void);
	void Update(void);
	virtual void Init(const float3 &pos, CUnit *owner);

	AtlasedTexture *sideTexture;
	AtlasedTexture *frontTexture;

	float3 dir;
	CColorMap *colorMap;

	float size;
	float length;
	float sizeGrowth;
	float frontOffset;
	int ttl;

	float invttl;
	float life;
	int createTime;
};

#endif
