/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SmokeTrailProjectile.h"

#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/Wind.h"
#include "System/myMath.h"

CR_BIND_DERIVED(CSmokeTrailProjectile, CProjectile, (NULL, ZeroVector, ZeroVector, ZeroVector, ZeroVector, false, false, 0.0f, 0, 0.0f, false, NULL, NULL))

CR_REG_METADATA(CSmokeTrailProjectile,(
	CR_MEMBER(pos1),
	CR_MEMBER(pos2),
	CR_MEMBER(orgSize),
	CR_MEMBER(creationTime),
	CR_MEMBER(lifeTime),
	CR_MEMBER(color),
	CR_MEMBER(dir1),
	CR_MEMBER(dir2),
	CR_MEMBER(drawTrail),
	CR_MEMBER(dirpos1),
	CR_MEMBER(dirpos2),
	CR_MEMBER(midpos),
	CR_MEMBER(middir),
	CR_MEMBER(drawSegmented),
	CR_MEMBER(firstSegment),
	CR_MEMBER(lastSegment),
	CR_MEMBER(drawCallbacker),
	CR_MEMBER(texture)
))

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

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
	bool drawTrail,
	CProjectile* drawCallback,
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
	drawTrail(drawTrail),
	drawSegmented(false),
	firstSegment(firstSegment),
	lastSegment(lastSegment),
	drawCallbacker(drawCallback),
	texture(texture == NULL ? projectileDrawer->smoketrailtex : texture)
{
	checkCol = false;
	castShadow = true;

	if (!drawTrail) {
		const float dist = pos1.distance(pos2);
		dirpos1 = pos1 - dir1 * dist * 0.33f;
		dirpos2 = pos2 + dir2 * dist * 0.33f;
	} else if (dir1.dot(dir2) < 0.98f) {
		const float dist = pos1.distance(pos2);
		dirpos1 = pos1 - dir1 * dist * 0.33f;
		dirpos2 = pos2 + dir2 * dist * 0.33f;
//		float3 mp = (pos1 + pos2) / 2;
		midpos = CalcBeizer(0.5f, pos1, dirpos1, dirpos2, pos2);
		middir = (dir1 + dir2).ANormalize();
		drawSegmented = true;
	}
	SetRadiusAndHeight(pos1.distance(pos2), 0.0f);

	if ((pos.y - CGround::GetApproximateHeight(pos.x, pos.z)) > 10) {
		useAirLos = true;
	}
}

CSmokeTrailProjectile::~CSmokeTrailProjectile()
{

}

void CSmokeTrailProjectile::Draw()
{
	inArray = true;
	const float age = gs->frameNum + globalRendering->timeOffset - creationTime;
	const float invLifeTime = (1.0f / lifeTime);
	va->EnlargeArrays(8 * 4, 0, VA_SIZE_TC);

	if (drawTrail) {
		const float3 dif1  = (pos1 - camera->GetPos()).ANormalize();
		const float3 dif2  = (pos2 - camera->GetPos()).ANormalize();
		const float3 odir1 = (dif1.cross(dir1)).ANormalize();
		const float3 odir2 = (dif2.cross(dir2)).ANormalize();

		unsigned char col[4];
		float a1 = (1.f - age * invLifeTime) * 255.f;
		if (lastSegment) {
			a1 = 0;
		}
		a1 *= 0.7f + math::fabs(dif1.dot(dir1));
		float alpha = Clamp(a1, 0.f, 255.f);
		col[0] = (unsigned char) (color * alpha);
		col[1] = (unsigned char) (color * alpha);
		col[2] = (unsigned char) (color * alpha);
		col[3] = (unsigned char) alpha;

		unsigned char col2[4];
		float a2 = (1.f - (age + 8) * invLifeTime) * 255.f;
		if (firstSegment) {
			a2 = 0;
		}
		a2 *= 0.7f + math::fabs(dif2.dot(dir2));
		alpha = Clamp(a2, 0.f, 255.f);
		col2[0] = (unsigned char) (color * alpha);
		col2[1] = (unsigned char) (color * alpha);
		col2[2] = (unsigned char) (color * alpha);
		col2[3] = (unsigned char) alpha;

		float size =  1 + ( age      * invLifeTime) * orgSize;
		float size2 = 1 + ((age + 8) * invLifeTime) * orgSize;

		if (drawSegmented) {
			const float t = (age + 4) * invLifeTime;
			const float3 dif3 = (midpos - camera->GetPos()).ANormalize();
			const float3 odir3 = (dif3.cross(middir)).ANormalize();
			float size3 = (0.2f + t) * orgSize;

			float a2 = (1.f - t) * 255.f;
			a2 *= 0.7f + math::fabs(dif3.dot(middir));
			alpha = Clamp(a2, 0.f, 255.f);

			unsigned char col3[4];
			col3[0] = (unsigned char) (color * alpha);
			col3[1] = (unsigned char) (color * alpha);
			col3[2] = (unsigned char) (color * alpha);
			col3[3] = (unsigned char) alpha;

			const float midtexx = texture->xstart + ((texture->xend - texture->xstart) * 0.5f);

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
	} else {
		// draw as particles
		unsigned char col[4];

		for (int a = 0; a < 8; ++a) {
			const float t = (age + a) * invLifeTime;

			const float a1 = (1.f - t) * 255.f;
			const float alpha = Clamp(a1, 0.f, 255.f);
			const float size = (0.2f + t) * orgSize * 1.2f;

			col[0] = (unsigned char) (color * alpha);
			col[1] = (unsigned char) (color * alpha);
			col[2] = (unsigned char) (color * alpha);
			col[3] = (unsigned char) alpha;

			#define st projectileDrawer->smoketex[0]
			va->AddVertexQTC(pos1 + ( camera->GetUp() + camera->GetRight()) * size, st->xstart, st->ystart, col);
			va->AddVertexQTC(pos1 + ( camera->GetUp() - camera->GetRight()) * size, st->xend,   st->ystart, col);
			va->AddVertexQTC(pos1 + (-camera->GetUp() - camera->GetRight()) * size, st->xend,   st->ystart, col);
			va->AddVertexQTC(pos1 + (-camera->GetUp() + camera->GetRight()) * size, st->xstart, st->ystart, col);
			#undef st
		}
	}

	if (drawCallbacker != NULL) {
		drawCallbacker->DrawCallback();
	}
}

void CSmokeTrailProjectile::Update()
{
	deleteMe |= (gs->frameNum >= (creationTime + lifeTime));
}

int CSmokeTrailProjectile::GetProjectilesCount() const
{
	return (drawTrail) ? 2 : 8;
}
