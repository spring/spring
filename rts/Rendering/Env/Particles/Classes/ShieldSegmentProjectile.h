/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SHIELD_PROJECTILE_H
#define SHIELD_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"
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
	ShieldSegmentCollection() {}
	ShieldSegmentCollection(CPlasmaRepulser* shield) { Init(shield); }
	ShieldSegmentCollection(ShieldSegmentCollection&& ssc) { *this = std::move(ssc); }
	~ShieldSegmentCollection() { Kill(); }

	ShieldSegmentCollection& operator=(ShieldSegmentCollection&& ssc) noexcept;

	void Init(CPlasmaRepulser*);
	void Kill();
	void Update();
	void UpdateColor();

	bool AllowDrawing();

	const CPlasmaRepulser* GetShield() const { return shield; }
	const AtlasedTexture* GetShieldTexture() const { return shieldTexture; }

	float3 GetShieldDrawPos() const;

	SColor GetColor() const { return color; }
	float GetSize() const { return size; }

	void PostLoad();

public:
	static constexpr unsigned int NUM_SEGMENTS_X = 6;
	static constexpr unsigned int NUM_SEGMENTS_Y = 4;

private:
	bool UsingPerlinNoise() const;

	const CPlasmaRepulser* shield = nullptr;
	const AtlasedTexture* shieldTexture = nullptr;

	int lastAllowDrawFrame = -1;
	bool allowDrawing = false;
	float size = 0.0f;

	SColor color;

	// NOTE: these are also registered in ProjectileHandler
	std::vector<ShieldSegmentProjectile*> shieldSegments;
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

	void Draw(GL::RenderDataBufferTC* va) const override;
	void Update() override;
	void PreDelete() {
		collection = nullptr;
		deleteMe = true;
	}
	void Reload(
		ShieldSegmentCollection* collection,
		const int xpart,
		const int ypart
	);

	int GetProjectilesCount() const override { return ((NUM_VERTICES_Y - 1) * (NUM_VERTICES_X - 1)); }

public:
	static constexpr unsigned int NUM_VERTICES_X = 5;
	static constexpr unsigned int NUM_VERTICES_Y = 3;

private:
	static const float3* GetSegmentVertices(const int xpart, const int ypart);
	static const float2* GetSegmentTexCoords(const AtlasedTexture* texture, const int xpart, const int ypart);

private:
	ShieldSegmentCollection* collection;
	const float3* vertices;
	const float2* texCoors;

};

#endif

