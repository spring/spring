/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUND_FLASH_H
#define GROUND_FLASH_H

#include "Sim/Projectiles/ExpGenSpawnable.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "System/Color.h"

struct AtlasedTexture;
struct GroundFlashInfo;
class CColorMap;
class CVertexArray;

class CGroundFlash : public CExpGenSpawnable
{
public:
	CR_DECLARE(CGroundFlash)

	CGroundFlash(const float3& _pos);
	CGroundFlash();

	virtual ~CGroundFlash() {}
	virtual void Draw() {}
	/// @return false when it should be deleted
	virtual bool Update() { return false; }
	virtual void Init(const CUnit* owner, const float3& offset) {}

	float3 CalcNormal(const float3 midPos, const float3 camDir, const float quadSize) const;

protected:
	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

public:
	float size;
	bool depthTest;
	bool depthMask;

	static CVertexArray* va;
};



class CStandardGroundFlash : public CGroundFlash
{
public:
	CR_DECLARE(CStandardGroundFlash)

	CStandardGroundFlash();
	CStandardGroundFlash(const float3& pos, const GroundFlashInfo& info);
	CStandardGroundFlash(
		const float3& pos,
		float _circleAlpha,
		float _flashAlpha,
		float _flashSize,
		float _circleGrowth,
		float _ttl,
		const float3& _color = float3(1.0f, 1.0f, 0.7f)
	);

	void InitCommon(const float3& _pos, const float3& _color);

	void Draw();
	bool Update();

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

private:
	float3 side1;
	float3 side2;

	int ttl;

	float flashSize;
	float circleSize;
	float circleGrowth;
	float circleAlpha;
	float flashAlpha;
	float flashAge;
	float flashAgeSpeed;
	float circleAlphaDec;


	SColor color;
};

/**
 * A simple groundflash, using a texture and a colormap, used in
 * the custom explosion-generator (unlike CStandardGroundFlash)
 */
class CSimpleGroundFlash : public CGroundFlash
{
public:
	CR_DECLARE_DERIVED(CSimpleGroundFlash)

	CSimpleGroundFlash();

	void Init(const CUnit* owner, const float3& offset) override;
	void Draw() override;
	bool Update() override;

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

private:
	float3 side1;
	float3 side2;

	float sizeGrowth;
	int ttl;
	float age, agerate;

	CColorMap* colorMap;
	AtlasedTexture* texture;
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
	CR_DECLARE(CSeismicGroundFlash)

	CSeismicGroundFlash(
		const float3& _pos,
		int _ttl,
		int _fade,
		float _size,
		float _sizeGrowth,
		float _alpha,
		const float3& _color
	);

	void Draw();
	/// @return false when it should be deleted
	bool Update();

private:
	float3 side1;
	float3 side2;

	AtlasedTexture* texture;

	float sizeGrowth;
	float alpha;

	int fade;
	int ttl;

	SColor color;
};

#endif /* GROUND_FLASH_H */
