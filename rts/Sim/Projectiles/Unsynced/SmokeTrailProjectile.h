/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SMOKE_TRAIL_PROJECTILE_H
#define SMOKE_TRAIL_PROJECTILE_H

#include "Sim/Projectiles/Projectile.h"

struct AtlasedTexture;

class CSmokeTrailProjectile : public CProjectile
{
	CR_DECLARE(CSmokeTrailProjectile);
public:
	CSmokeTrailProjectile(
		const CUnit* owner,
		const float3& pos1,
		const float3& pos2,
		const float3& dir1,
		const float3& dir2,
		bool firstSegment,
		bool lastSegment,
		float size = 1,
		int time = 80,
		float color = 0.7f,
		bool drawTrail = true,
		CProjectile* drawCallback = 0,
		AtlasedTexture* texture = 0
	);
	virtual ~CSmokeTrailProjectile();

	void Update();
	void Draw();

private:
	float3 pos1;
	float3 pos2;
	float orgSize;
public:
	int creationTime;
private:
	int lifeTime;
	float color;
	float3 dir1;
	float3 dir2;
	bool drawTrail;

	float3 dirpos1;
	float3 dirpos2;
	float3 midpos;
	float3 middir;
	bool drawSegmented;
	bool firstSegment;
	bool lastSegment;
public:
	CProjectile* drawCallbacker;
private:
	AtlasedTexture* texture;
};

#endif /* SMOKE_TRAIL_PROJECTILE_H */
