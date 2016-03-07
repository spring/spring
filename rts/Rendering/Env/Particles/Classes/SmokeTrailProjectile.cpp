/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SmokeTrailProjectile.h"

#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "System/myMath.h"

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
	texture(texture == NULL ? projectileDrawer->smoketrailtex : texture)
{
	checkCol = false;
	castShadow = true;

	UpdateEndPos(pos1, dir1);
	SetRadiusAndHeight(pos1.distance(pos2), 0.0f);

	if ((pos.y - CGround::GetApproximateHeight(pos.x, pos.z)) > 10) {
		useAirLos = true;
	}
}


void CSmokeTrailProjectile::UpdateEndPos(const float3 pos, const float3 dir)
{
	pos1 = pos;
	dir1 = dir;

	const float dist = pos1.distance(pos2);

	drawSegmented = false;
	SetPosition((pos1 + pos2) * 0.5f);
	SetRadiusAndHeight(dist, 0.0f);
	sortDistOffset = 10.f + dist * 0.5f; // so that missile's engine flame gets rendered above the trail

	if (dir1.dot(dir2) < 0.98f) {
		dirpos1 = pos1 - dir1 * dist * 0.33f;
		dirpos2 = pos2 + dir2 * dist * 0.33f;
		midpos = CalcBeizer(0.5f, pos1, dirpos1, dirpos2, pos2);
		middir = (dir1 + dir2).ANormalize();
		drawSegmented = true;
	}
}


void CSmokeTrailProjectile::Draw()
{
	inArray = true;
	const float age = gs->frameNum + globalRendering->timeOffset - creationTime;
	const float invLifeTime = (1.0f / lifeTime);
	va->EnlargeArrays(8, 0, VA_SIZE_TC);

	const float3 dif1  = (pos1 - camera->GetPos()).ANormalize();
	const float3 dif2  = (pos2 - camera->GetPos()).ANormalize();
	const float3 odir1 = (dif1.cross(dir1)).ANormalize();
	const float3 odir2 = (dif2.cross(dir2)).ANormalize();

	const SColor colBase(color, color, color, 1.f);

	float a1 = (1.f - age * invLifeTime) * (0.7f + std::fabs(dif1.dot(dir1)));
	if (lastSegment) {
		a1 = 0;
	}
	const float alpha1 = Clamp(a1, 0.f, 1.f);
	const SColor col = colBase * alpha1;

	float a2 = (1.f - (age + 8) * invLifeTime) * (0.7f + std::fabs(dif2.dot(dir2)));
	if (firstSegment) {
		a2 = 0;
	}
	const float alpha2 = Clamp(a2, 0.f, 1.f);
	const SColor col2 = colBase * alpha2;

	const float size =  1 + ( age      * invLifeTime) * orgSize;
	const float size2 = 1 + ((age + 8) * invLifeTime) * orgSize;

	if (drawSegmented) {
		const float t = (age + 4) * invLifeTime;
		const float3 dif3 = (midpos - camera->GetPos()).ANormalize();
		const float3 odir3 = (dif3.cross(middir)).ANormalize();
		const float size3 = (0.2f + t) * orgSize;

		const float a2 = (1.f - t) * (0.7f + std::fabs(dif3.dot(middir)));
		const float alpha = Clamp(a2, 0.f, 1.f);
		const SColor col3 = colBase * alpha;

		const float midtexx = mix(texture->xstart, texture->xend, 0.5f);

		va->AddVertexQTC(pos1   - (odir1 * size),  texture->xstart, texture->ystart, col);
		va->AddVertexQTC(pos1   + (odir1 * size),  texture->xstart, texture->yend,   col);
		va->AddVertexQTC(midpos + (odir3 * size3), midtexx,         texture->yend,   col3);
		va->AddVertexQTC(midpos - (odir3 * size3), midtexx,         texture->ystart, col3);

		va->AddVertexQTC(midpos - (odir3 * size3), midtexx,         texture->ystart, col3);
		va->AddVertexQTC(midpos + (odir3 * size3), midtexx,         texture->yend,   col3);
		va->AddVertexQTC(pos2   + (odir2 * size2), texture->xend,   texture->yend,   col2);
		va->AddVertexQTC(pos2   - (odir2 * size2), texture->xend,   texture->ystart, col2);
	} else {
		va->AddVertexQTC(pos1 - (odir1 * size),    texture->xstart, texture->ystart, col);
		va->AddVertexQTC(pos1 + (odir1 * size),    texture->xstart, texture->yend,   col);
		va->AddVertexQTC(pos2 + (odir2 * size2),   texture->xend,   texture->yend,   col2);
		va->AddVertexQTC(pos2 - (odir2 * size2),   texture->xend,   texture->ystart, col2);
	}
}

void CSmokeTrailProjectile::Update()
{
	deleteMe |= (gs->frameNum >= (creationTime + lifeTime));
}

int CSmokeTrailProjectile::GetProjectilesCount() const
{
	return 2;
}
