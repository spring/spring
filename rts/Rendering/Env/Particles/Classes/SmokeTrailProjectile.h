/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMOKE_TRAIL_PROJECTILE_H
#define SMOKE_TRAIL_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

struct AtlasedTexture;

class CSmokeTrailProjectile : public CProjectile
{
	CR_DECLARE_DERIVED(CSmokeTrailProjectile)
public:
	CSmokeTrailProjectile() { }
	CSmokeTrailProjectile(
		const CUnit* owner,
		const float3& pos1,
		const float3& pos2,
		const float3& dir1,
		const float3& dir2,
		bool firstSegment,
		bool lastSegment,
		float size,
		int time,
		float color,
		AtlasedTexture* texture
	);

	void Update() override;
	void Draw() override;

	int GetProjectilesCount() const override;

	void UpdateEndPos(const float3 pos, const float3 dir);

private:
	float3 pos1;
	float3 pos2;
	float orgSize;

	int creationTime;
	int lifeTime;
	float color;
	float3 dir1;
	float3 dir2;

	float3 dirpos1;
	float3 dirpos2;
	float3 midpos;
	float3 middir;
	bool drawSegmented;
	bool firstSegment;
	bool lastSegment;

private:
	AtlasedTexture* texture;
};

#endif /* SMOKE_TRAIL_PROJECTILE_H */
