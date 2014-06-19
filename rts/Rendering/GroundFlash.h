/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUND_FLASH_H
#define GROUND_FLASH_H

#include "Sim/Projectiles/ExplosionGenerator.h"

struct AtlasedTexture;
struct GroundFXTexture;
class CColorMap;
class CVertexArray;

class CGroundFlash : public CExpGenSpawnable
{
public:
	CR_DECLARE(CGroundFlash);

	CGroundFlash(const float3& p);
	CGroundFlash();

	virtual ~CGroundFlash() {}
	virtual void Draw() {}
	/// @return false when it should be deleted
	virtual bool Update() { return false; }
	virtual void Init(const CUnit* owner, const float3& offset) {}

	float size;
	bool depthTest;
	bool depthMask;

	static CVertexArray* va;
};



class CStandardGroundFlash : public CGroundFlash
{
public:
	CR_DECLARE(CStandardGroundFlash);

	CStandardGroundFlash();
	CStandardGroundFlash(const float3& pos, float circleAlpha, float flashAlpha, float flashSize, float circleSpeed, float ttl, const float3& color = float3(1.0f, 1.0f, 0.7f));

	void Draw();
	bool Update();

	float3 side1;
	float3 side2;

	float flashSize;
	float circleSize;
	float circleGrowth;
	float circleAlpha;
	float flashAlpha;
	float flashAge;
	float flashAgeSpeed;
	float circleAlphaDec;

	unsigned char color[3];

	int ttl;
};

/**
 * A simple groundflash, using a texture and a colormap, used in
 * the custom explosion-generator (unlike CStandardGroundFlash)
 */
class CSimpleGroundFlash : public CGroundFlash
{
public:
	CR_DECLARE(CSimpleGroundFlash);

	CSimpleGroundFlash();

	void Init(const CUnit* owner, const float3& offset);
	void Draw();
	bool Update();

	float3 side1;
	float3 side2;

	float sizeGrowth;
	int ttl;
	float age, agerate;

	CColorMap* colorMap;
	GroundFXTexture* texture;
};

/**
 * A simple groundflash, only creating one quad with specified texture.
 * This one also sets alwaysVisible=true because it is used by the
 * seismic effect, so do not use it (or fix it) for things that should
 * be affected by LOS.
 */
class CSeismicGroundFlash : public CGroundFlash
{
public:
	CR_DECLARE(CSeismicGroundFlash);

	CSeismicGroundFlash(const float3& pos, int ttl, int fade, float size, float sizeGrowth, float alpha, const float3& col);

	void Draw();
	/// @return false when it should be deleted
	bool Update();

	float3 side1;
	float3 side2;

	AtlasedTexture* texture;

	float sizeGrowth;
	float alpha;

	int fade;
	int ttl;

	unsigned char color[4];
};

#endif /* GROUND_FLASH_H */
