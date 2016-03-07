/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SHIELD_PROJECTILE_H
#define SHIELD_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"
#include <System/creg/STL_List.h>
#include "System/float3.h"
#include "System/type2.h"
#include "System/Color.h"

class CUnit;
class CPlasmaRepulser;
struct WeaponDef;

struct AtlasedTexture;
class ShieldSegment;


class ShieldProjectile: public CProjectile
{
	CR_DECLARE_DERIVED(ShieldProjectile)
public:
	// creg only
	ShieldProjectile() { }
	ShieldProjectile(CPlasmaRepulser*);
	~ShieldProjectile();

	void Draw() override;
	void Update() override;
	virtual int GetProjectilesCount() const override;

public:
	void PreDelete();
	bool AllowDrawing();
	CPlasmaRepulser* GetShield() const { return shield; }
	const AtlasedTexture* GetShieldTexture() const { return shieldTexture; }
	float3 GetShieldDrawPos() const;

	void PostLoad();
private:
	CPlasmaRepulser* shield;
	const AtlasedTexture* shieldTexture;
	SColor color;

	unsigned int lastAllowDrawingUpdate;
	bool UsingPerlinNoise() const;

	void UpdateColor();

	std::vector<ShieldSegment> shieldSegments; //FIXME deque
};



class ShieldSegment
{
public:
	ShieldSegment(
		ShieldProjectile* shieldProjectile,
		const WeaponDef* shieldWeaponDef,
		const int xpart,
		const int ypart
	);
	~ShieldSegment();

	void Draw(CVertexArray* va, const float3& shieldPos, const SColor& color);

private:
	static const float3* GetSegmentVertices(const int xpart, const int ypart);
	static const float2* GetSegmentTexCoords(const AtlasedTexture* texture, const int xpart, const int ypart);

private:
	ShieldProjectile* shieldProjectile;
	const float3* vertices;
	const float2* texCoors;

	float segmentSize;
};

#endif

