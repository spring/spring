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


class ShieldSegmentCollection
{
	CR_DECLARE_STRUCT(ShieldSegmentCollection)
public:
	// creg only
	ShieldSegmentCollection() { }
	ShieldSegmentCollection(CPlasmaRepulser*);
	~ShieldSegmentCollection();

	void Update();

	bool AllowDrawing();
	CPlasmaRepulser* GetShield() const { return shield; }
	const AtlasedTexture* GetShieldTexture() const { return shieldTexture; }
	float3 GetShieldDrawPos() const;
	void UpdateColor();
	SColor GetColor() const { return color; }
	float GetSize() const { return size; }

	void PostLoad();
private:
	bool UsingPerlinNoise() const;
	CPlasmaRepulser* shield;
	const AtlasedTexture* shieldTexture;
	int lastAllowDrawingframe;
	bool allowDrawing;
	float size;
	SColor color;

	// NOTE: these are also registered in ProjectileHandler
	std::vector<ShieldSegmentProjectile*> shieldSegments; //FIXME deque
};



class ShieldSegmentProjectile: public CProjectile
{
	CR_DECLARE_DERIVED(ShieldSegmentProjectile)
public:
	ShieldSegmentProjectile() { }
	ShieldSegmentProjectile(
		ShieldSegmentCollection* collection,
		const WeaponDef* shieldWeaponDef,
		const float3& shieldSegmentPos,
		int xpart,
		int ypart
	);
	~ShieldSegmentProjectile();

	void Draw() override;
	void Update() override;
	void PreDelete();
	void Reload(
		ShieldSegmentCollection* collection,
		const int xpart,
		const int ypart
	);

	virtual int GetProjectilesCount() const override;

private:
	static const float3* GetSegmentVertices(const int xpart, const int ypart);
	static const float2* GetSegmentTexCoords(const AtlasedTexture* texture, const int xpart, const int ypart);

private:
	ShieldSegmentCollection* collection;
	const float3* vertices;
	const float2* texCoors;

};

#endif

