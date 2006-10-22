#include "StdAfx.h"
#include "BitmapMuzzleFlame.h"
#include "Rendering/GL/VertexArray.h"
#include "ProjectileHandler.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Textures/ColorMap.h"

CR_BIND_DERIVED(CBitmapMuzzleFlame, CProjectile);

CR_REG_METADATA(CBitmapMuzzleFlame, 
(
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

CBitmapMuzzleFlame::CBitmapMuzzleFlame(void)
{
	deleteMe=false;
	checkCol=false;
	useAirLos=true;
}

CBitmapMuzzleFlame::~CBitmapMuzzleFlame(void)
{
}

void CBitmapMuzzleFlame::Draw(void)
{
	inArray=true;

	life = (gs->frameNum-createTime+gu->timeOffset)*invttl;

	unsigned char col[4];
	colorMap->GetColor(col, life);

	float invlife = 1-life;

	float igrowth = sizeGrowth*(1-invlife*invlife);
	float isize = size + size*igrowth;
	float ilength = length + length*igrowth;

	float3 sidedir = dir.cross(float3(0,1,0));
	float3 updir = dir.cross(sidedir);

	va->AddVertexTC(pos+updir*isize,sideTexture->xstart,sideTexture->ystart,col);
	va->AddVertexTC(pos+updir*isize+dir*ilength,sideTexture->xend,sideTexture->ystart,col);
	va->AddVertexTC(pos-updir*isize+dir*ilength,sideTexture->xend,sideTexture->yend,col);
	va->AddVertexTC(pos-updir*isize,sideTexture->xstart,sideTexture->yend,col);

	va->AddVertexTC(pos+sidedir*isize,sideTexture->xstart,sideTexture->ystart,col);
	va->AddVertexTC(pos+sidedir*isize+dir*ilength,sideTexture->xend,sideTexture->ystart,col);
	va->AddVertexTC(pos-sidedir*isize+dir*ilength,sideTexture->xend,sideTexture->yend,col);
	va->AddVertexTC(pos-sidedir*isize,sideTexture->xstart,sideTexture->yend,col);

	float3 frontpos = pos + dir*frontOffset*ilength;

	va->AddVertexTC(frontpos-sidedir*isize+updir*isize,frontTexture->xstart,frontTexture->ystart,col);
	va->AddVertexTC(frontpos+sidedir*isize+updir*isize,frontTexture->xend,frontTexture->ystart,col);
	va->AddVertexTC(frontpos+sidedir*isize-updir*isize,frontTexture->xend,frontTexture->yend,col);
	va->AddVertexTC(frontpos-sidedir*isize-updir*isize,frontTexture->xstart,frontTexture->yend,col);

}

void CBitmapMuzzleFlame::Update(void)
{
	ttl--;
	if(!ttl)
		deleteMe = true;
}

void CBitmapMuzzleFlame::Init(const float3 &pos, CUnit *owner)
{
	CProjectile::Init(pos, owner);
	life = 0;
	createTime = gs->frameNum;
	invttl = 1.0f/(float)ttl;
}
