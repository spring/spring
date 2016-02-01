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
class ShieldSegmentProjectile;


class ShieldProjectile: public CProjectile
{
	CR_DECLARE(ShieldProjectile)
public:
	// creg only
	ShieldProjectile() { }
	ShieldProjectile(CPlasmaRepulser*);

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

	unsigned int lastAllowDrawingUpdate;
	bool allowDrawing;

	// NOTE: these are also registered in ProjectileHandler
	std::list<ShieldSegmentProjectile*> shieldSegments; //FIXME deque
};



class ShieldSegmentProjectile: public CProjectile
{
public:
	ShieldSegmentProjectile(
		ShieldProjectile* shieldProjectile,
		const WeaponDef* shieldWeaponDef,
		const float3& shieldSegmentPos,
		const int xpart,
		const int ypart
	);
	~ShieldSegmentProjectile();

	void Draw() override;
	void Update() override;
	void PreDelete();

	virtual int GetProjectilesCount() const override;

private:
	bool UsingPerlinNoise() const;
	static const float3* GetSegmentVertices(const int xpart, const int ypart);
	static const float2* GetSegmentTexCoords(const AtlasedTexture* texture, const int xpart, const int ypart);

private:
	ShieldProjectile* shieldProjectile;
	const float3* vertices;
	const float2* texCoors;

	SColor segmentColor;
	float3 segmentPos;
	float segmentSize;
};

#endif

