/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _DIRT_PROJECTILE_H
#define _DIRT_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

struct AtlasedTexture;

class CDirtProjectile : public CProjectile
{
	CR_DECLARE_DERIVED(CDirtProjectile)
public:
	CDirtProjectile();
	CDirtProjectile(
		CUnit* owner,
		const float3& pos,
		const float3& speed,
		float ttl,
		float size,
		float expansion,
		float slowdown,
		const float3& color
	);
	virtual ~CDirtProjectile();

	virtual void Draw() override;
	virtual void Update() override;

	virtual int GetProjectilesCount() const override;

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

private:
	float alpha;
	float alphaFalloff;
	float size;
	float sizeExpansion;
	float slowdown;
	float3 color;

	AtlasedTexture* texture;
};

#endif // _DIRT_PROJECTILE_H
