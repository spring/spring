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

void CBitmapMuzzleFlame::Draw()
{
	inArray = true;
	life = (gs->frameNum - createTime + globalRendering->timeOffset) * invttl;

	unsigned char col[4];
	colorMap->GetColor(col, life);

	const float igrowth = sizeGrowth * (1.0f - (1.0f - life) * (1.0f - life));
	const float isize = size * (igrowth + 1.0f);
	const float ilength = length * (igrowth + 1.0f);

	const float3 udir = (std::fabs(dir.dot(UpVector)) >= 0.99f)? FwdVector: UpVector;
	const float3 xdir = (dir.cross(udir)).SafeANormalize();
	const float3 ydir = (dir.cross(xdir)).SafeANormalize();

	va->AddVertexTC(pos + ydir * isize,                 sideTexture->xstart, sideTexture->ystart, col);
	va->AddVertexTC(pos + ydir * isize + dir * ilength, sideTexture->xend,   sideTexture->ystart, col);
	va->AddVertexTC(pos - ydir * isize + dir * ilength, sideTexture->xend,   sideTexture->yend,   col);
	va->AddVertexTC(pos - ydir * isize,                 sideTexture->xstart, sideTexture->yend,   col);

	va->AddVertexTC(pos + xdir * isize,                 sideTexture->xstart, sideTexture->ystart, col);
	va->AddVertexTC(pos + xdir * isize + dir * ilength, sideTexture->xend,   sideTexture->ystart, col);
	va->AddVertexTC(pos - xdir * isize + dir * ilength, sideTexture->xend,   sideTexture->yend,   col);
	va->AddVertexTC(pos - xdir * isize,                 sideTexture->xstart, sideTexture->yend,   col);

	float3 frontpos = pos + dir * frontOffset * ilength;

	va->AddVertexTC(frontpos - xdir * isize + ydir * isize, frontTexture->xstart, frontTexture->ystart, col);
	va->AddVertexTC(frontpos + xdir * isize + ydir * isize, frontTexture->xend,   frontTexture->ystart, col);
	va->AddVertexTC(frontpos + xdir * isize - ydir * isize, frontTexture->xend,   frontTexture->yend,   col);
	va->AddVertexTC(frontpos - xdir * isize - ydir * isize, frontTexture->xstart, frontTexture->yend,   col);

}

void CBitmapMuzzleFlame::Update()
{
	deleteMe |= ((ttl--) == 0);
}

void CBitmapMuzzleFlame::Init(const CUnit* owner, const float3& offset)
{
	CProjectile::Init(owner, offset);

	life = 0.0f;
	invttl = 1.0f / ttl;

	createTime = gs->frameNum;
}
