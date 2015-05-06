/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SHIELD_PROJECTILE_H
#define SHIELD_PROJECTILE_H

#include <list>
#include "Sim/Projectiles/Projectile.h"
#include "System/float3.h"
#include "System/type2.h"

class CUnit;
class CPlasmaRepulser;
struct WeaponDef;

struct AtlasedTexture;
class ShieldSegmentProjectile;


class ShieldProjectile: public CProjectile {
	CR_DECLARE(ShieldProjectile)
public:
	ShieldProjectile(CPlasmaRepulser*);
	~ShieldProjectile();

	void Update();
	bool AllowDrawing();

	void PreDelete() {
		deleteMe = true;
		shield = NULL;
	}

	inline CPlasmaRepulser* GetShield() const { return shield; }
	const AtlasedTexture* GetShieldTexture() const { return shieldTexture; }

private:
	CPlasmaRepulser* shield;
	const AtlasedTexture* shieldTexture;

	unsigned int lastAllowDrawingUpdate;
	bool allowDrawing;

	// NOTE: these are also registered in ProjectileHandler
	std::list<ShieldSegmentProjectile*> shieldSegments;
};



class ShieldSegmentProjectile: public CProjectile {
	CR_DECLARE(ShieldSegmentProjectile)
public:
	ShieldSegmentProjectile(
		ShieldProjectile* shieldProjectile,
		const WeaponDef* shieldWeaponDef,
		const float3& shieldSegmentPos,
		const int xpart,
		const int ypart
	);
	~ShieldSegmentProjectile();

	void Draw();
	void Update();
	void PreDelete() {
		deleteMe = true;
		shieldProjectile = NULL;
	}

private:
	static const float3* GetSegmentVertices(const int xpart, const int ypart);
	static const float2* GetSegmentTexCoords(const AtlasedTexture* texture, const int xpart, const int ypart);

private:
	ShieldProjectile* shieldProjectile;

	float3 segmentPos;
	float3 segmentColor;

	const float3* vertices;
	const float2* texCoors;

	float segmentSize;
	float segmentAlpha;
	bool usePerlinTex;
};

#endif

