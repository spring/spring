/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "BitmapMuzzleFlame.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ProjectileHandler.h"

CR_BIND_DERIVED(CBitmapMuzzleFlame, CProjectile, );

CR_REG_METADATA(CBitmapMuzzleFlame,
(
	CR_MEMBER(invttl),
	CR_MEMBER(life),
	CR_MEMBER(createTime),
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(sideTexture),
		CR_MEMBER(frontTexture),
		CR_MEMBER(dir),
		CR_MEMBER(colorMap),
		CR_MEMBER(size),
		CR_MEMBER(length),
		CR_MEMBER(sizeGrowth),
		CR_MEMBER(ttl),
		CR_MEMBER(frontOffset),
	CR_MEMBER_ENDFLAG(CM_Config)
));

CBitmapMuzzleFlame::CBitmapMuzzleFlame()
	: CProjectile()
	, sideTexture(NULL)
	, frontTexture(NULL)
	, dir(ZeroVector)
	, colorMap(NULL)
	, size(0.0f)
	, length(0.0f)
	, sizeGrowth(0.0f)
	, frontOffset(0.0f)
	, ttl(0)
	, invttl(0.0f)
	, life(0.0f)
	, createTime(0)
{
	// set fields from super-classes
	useAirLos = true;
	checkCol  = false;
	deleteMe  = false;
}

CBitmapMuzzleFlame::~CBitmapMuzzleFlame()
{
}

void CBitmapMuzzleFlame::Draw()
{
	inArray = true;
	life = (gs->frameNum - createTime + globalRendering->timeOffset) * invttl;

	unsigned char col[4];
	colorMap->GetColor(col, life);

	float invlife = 1 - life;
	float igrowth = sizeGrowth * (1 - invlife * invlife);
	float isize = size + size * igrowth;
	float ilength = length + length * igrowth;

	float3 sidedir = dir.cross(float3(0, 1, 0)).SafeANormalize();
	float3 updir = dir.cross(sidedir).SafeANormalize();

	va->AddVertexTC(pos + updir * isize,                 sideTexture->xstart, sideTexture->ystart, col);
	va->AddVertexTC(pos + updir * isize + dir * ilength, sideTexture->xend,   sideTexture->ystart, col);
	va->AddVertexTC(pos - updir * isize + dir * ilength, sideTexture->xend,   sideTexture->yend,   col);
	va->AddVertexTC(pos - updir * isize,                 sideTexture->xstart, sideTexture->yend,   col);

	va->AddVertexTC(pos + sidedir * isize,                 sideTexture->xstart, sideTexture->ystart, col);
	va->AddVertexTC(pos + sidedir * isize + dir * ilength, sideTexture->xend,   sideTexture->ystart, col);
	va->AddVertexTC(pos - sidedir * isize + dir * ilength, sideTexture->xend,   sideTexture->yend,   col);
	va->AddVertexTC(pos - sidedir * isize,                 sideTexture->xstart, sideTexture->yend,   col);

	float3 frontpos = pos + dir * frontOffset * ilength;

	va->AddVertexTC(frontpos - sidedir * isize + updir * isize, frontTexture->xstart, frontTexture->ystart, col);
	va->AddVertexTC(frontpos + sidedir * isize + updir * isize, frontTexture->xend,   frontTexture->ystart, col);
	va->AddVertexTC(frontpos + sidedir * isize - updir * isize, frontTexture->xend,   frontTexture->yend,   col);
	va->AddVertexTC(frontpos - sidedir * isize - updir * isize, frontTexture->xstart, frontTexture->yend,   col);

}

void CBitmapMuzzleFlame::Update()
{
	ttl--;
	if (ttl == 0) {
		deleteMe = true;
	}
}

void CBitmapMuzzleFlame::Init(const float3& pos, CUnit* owner)
{
	CProjectile::Init(pos, owner);
	life = 0.0f;
	createTime = gs->frameNum;
	invttl = 1.0f / (float) ttl;
}
