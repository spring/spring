/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SmokeTrailProjectile.h"

#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/SpringMath.h"

CR_BIND_DERIVED(CSmokeTrailProjectile, CProjectile, )

CR_REG_METADATA(CSmokeTrailProjectile,(
	CR_MEMBER(pos1),
	CR_MEMBER(pos2),
	CR_MEMBER(orgSize),
	CR_MEMBER(creationTime),
	CR_MEMBER(lifeTime),
	CR_MEMBER(color),
	CR_MEMBER(dir1),
	CR_MEMBER(dir2),
	CR_MEMBER(dirpos1),
	CR_MEMBER(dirpos2),
	CR_MEMBER(midpos),
	CR_MEMBER(middir),
	CR_MEMBER(drawSegmented),
	CR_MEMBER(firstSegment),
	CR_MEMBER(lastSegment),
	CR_MEMBER(texture)
))


CSmokeTrailProjectile::CSmokeTrailProjectile(
	const CUnit* owner,
	AtlasedTexture* texture,
	const float3& pos1,
	const float3& pos2,
	const float3& dir1,
	const float3& dir2,
	int time,
	float size,
	float color,
	bool firstSegment,
	bool lastSegment
):
	CProjectile((pos1 + pos2) * 0.5f, ZeroVector, owner, false, false, false),

	pos1(pos1),
	pos2(pos2),
	orgSize(size),
	creationTime(gs->frameNum),
	lifeTime(time),
	color(color),
	dir1(dir1),
	dir2(dir2),
	drawSegmented(false),
	firstSegment(firstSegment),
	lastSegment(lastSegment),
	texture((texture == nullptr)? projectileDrawer->smoketrailtex : texture)
{
	checkCol = false;
	castShadow = true;

	UpdateEndPos(pos1, dir1);
	SetRadiusAndHeight(pos1.distance(pos2), 0.0f);

	useAirLos |= ((pos.y - CGround::GetApproximateHeight(pos.x, pos.z)) > 10.0f);
}


void CSmokeTrailProjectile::UpdateEndPos(const float3 pos, const float3 dir)
{
	pos1 = pos;
	dir1 = dir;

	const float dist = pos1.distance(pos2);

	SetPosition((pos1 + pos2) * 0.5f);
	SetRadiusAndHeight(dist, 0.0f);
	sortDistOffset = 10.0f + dist * 0.5f; // so that missile's engine flame gets rendered above the trail

	if ((drawSegmented = (dir1.dot(dir2) < 0.98f))) {
		dirpos1 = pos1 - dir1 * dist * 0.33f;
		dirpos2 = pos2 + dir2 * dist * 0.33f;
		midpos = CalcBeizer(0.5f, pos1, dirpos1, dirpos2, pos2);
		middir = (dir1 + dir2).ANormalize();
	}
}


void CSmokeTrailProjectile::Draw(GL::RenderDataBufferTC* va) const
{
	const float age = gs->frameNum + globalRendering->timeOffset - creationTime;
	const float invLifeTime = (1.0f / lifeTime);

	const float3 dif1  = (pos1 - camera->GetPos()).ANormalize();
	const float3 dif2  = (pos2 - camera->GetPos()).ANormalize();
	const float3 odir1 = (dif1.cross(dir1)).ANormalize();
	const float3 odir2 = (dif2.cross(dir2)).ANormalize();

	const float t1 = (age    ) * invLifeTime;
	const float t3 = (age + 4) * invLifeTime;
	const float t2 = (age + 8) * invLifeTime;

	const float lerp1 = ((1.0f - t1) * (0.7f + std::fabs(dif1.dot(dir1)))) * (1 -  lastSegment);
	const float lerp2 = ((1.0f - t2) * (0.7f + std::fabs(dif2.dot(dir2)))) * (1 - firstSegment);

	const float size1 = 1.0f + t1 * orgSize;
	const float size2 = 1.0f + t2 * orgSize;

	const SColor colBase = {color, color, color, 1.0f};
	const SColor col1    = colBase * Clamp(lerp1, 0.0f, 1.0f);
	const SColor col2    = colBase * Clamp(lerp2, 0.0f, 1.0f);

	if (drawSegmented) {
		const float3  dif3 = (midpos - camera->GetPos()).ANormalize();
		const float3 odir3 = (dif3.cross(middir)).ANormalize();

		const float lerp3 = (1.0f - t3) * (0.7f + std::fabs(dif3.dot(middir)));
		const float size3 = (0.2f + t3) * orgSize;
		const float midtexx = mix(texture->xstart, texture->xend, 0.5f);

		const SColor col3 = colBase * Clamp(lerp3, 0.0f, 1.0f);

		{
			va->SafeAppend({pos1   - (odir1 * size1), texture->xstart, texture->ystart, col1});
			va->SafeAppend({pos1   + (odir1 * size1), texture->xstart, texture->yend,   col1});
			va->SafeAppend({midpos + (odir3 * size3), midtexx,         texture->yend,   col3});

			va->SafeAppend({midpos + (odir3 * size3), midtexx,         texture->yend,   col3});
			va->SafeAppend({midpos - (odir3 * size3), midtexx,         texture->ystart, col3});
			va->SafeAppend({pos1   - (odir1 * size1), texture->xstart, texture->ystart, col1});
		}
		{
			va->SafeAppend({midpos - (odir3 * size3), midtexx,         texture->ystart, col3});
			va->SafeAppend({midpos + (odir3 * size3), midtexx,         texture->yend,   col3});
			va->SafeAppend({pos2   + (odir2 * size2), texture->xend,   texture->yend,   col2});

			va->SafeAppend({pos2   + (odir2 * size2), texture->xend,   texture->yend,   col2});
			va->SafeAppend({pos2   - (odir2 * size2), texture->xend,   texture->ystart, col2});
			va->SafeAppend({midpos - (odir3 * size3), midtexx,         texture->ystart, col3});
		}
	} else {
		va->SafeAppend({pos1 - (odir1 * size1), texture->xstart, texture->ystart, col1});
		va->SafeAppend({pos1 + (odir1 * size1), texture->xstart, texture->yend,   col1});
		va->SafeAppend({pos2 + (odir2 * size2), texture->xend,   texture->yend,   col2});

		va->SafeAppend({pos2 + (odir2 * size2), texture->xend,   texture->yend,   col2});
		va->SafeAppend({pos2 - (odir2 * size2), texture->xend,   texture->ystart, col2});
		va->SafeAppend({pos1 - (odir1 * size1), texture->xstart, texture->ystart, col1});
	}
}

void CSmokeTrailProjectile::Update()
{
	deleteMe |= (gs->frameNum >= (creationTime + lifeTime));
}

