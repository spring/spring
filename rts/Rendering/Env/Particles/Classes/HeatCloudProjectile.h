/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef HEAT_CLOUD_PROJECTILE_H
#define HEAT_CLOUD_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

struct AtlasedTexture;

class CHeatCloudProjectile : public CProjectile
{
	CR_DECLARE_DERIVED(CHeatCloudProjectile)
public:
	CHeatCloudProjectile();
	/// projectile starts at size 0 and ends at size \<size\>
	CHeatCloudProjectile(
		CUnit* owner,
		const float3& pos,
		const float3& speed,
		const float temperature,
		const float size
	);

	void Draw(GL::RenderDataBufferTC* va) const override;
	void Update() override;

	int GetProjectilesCount() const override { return 1; }

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

private:
	float heat;
	float maxheat;
	float heatFalloff;
public:
	float size;
private:
	float sizeGrowth;
	float sizemod;
	float sizemodmod;

	AtlasedTexture* texture;
};

#endif /* HEAT_CLOUD_PROJECTILE_H */
