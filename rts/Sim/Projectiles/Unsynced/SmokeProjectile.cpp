/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SmokeProjectile.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Misc/Wind.h"

CR_BIND_DERIVED(CSmokeProjectile, CProjectile, );

CR_REG_METADATA(CSmokeProjectile,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(color),
		CR_MEMBER(size),
		CR_MEMBER(startSize),
		CR_MEMBER(sizeExpansion),
		CR_MEMBER(ageSpeed),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_MEMBER(age),
	CR_MEMBER(textureNum)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSmokeProjectile::CSmokeProjectile():
	CProjectile(),

	color(0.5f),
	age(0.0f),
	ageSpeed(1.0f),
	size(0.0f),
	startSize(0.0f),
	sizeExpansion(0.0f),
	textureNum(0)
{
	checkCol = false;
}

CSmokeProjectile::CSmokeProjectile(
	CUnit* owner,
	const float3& pos,
	const float3& speed,
	float ttl,
	float startSize,
	float sizeExpansion,
	float color
):
	CProjectile(pos, speed, owner, false, false, false),

	color(color),
	age(0),
	size(0),
	startSize(startSize),
	sizeExpansion(sizeExpansion)
{
	ageSpeed = 1.0f / ttl;
	checkCol = false;
	castShadow = true;
	textureNum = (int) (gu->RandInt() % projectileDrawer->smoketex.size());

	if ((pos.y - CGround::GetApproximateHeight(pos.x, pos.z, false)) > 10) {
		useAirLos = true;
	}

	if (!owner) {
		alwaysVisible = true;
	}
}



void CSmokeProjectile::Init(const CUnit* owner, const float3& offset)
{
	textureNum = (int) (gu->RandInt() % projectileDrawer->smoketex.size());

	if (offset.y - CGround::GetApproximateHeight(offset.x, offset.z, false) > 10.0f) {
		useAirLos = true;
	}

	if (!owner) {
		alwaysVisible = true;
	}

	CProjectile::Init(owner, offset);
}

void CSmokeProjectile::Update()
{
	pos += speed;
	pos += wind.GetCurrentWind() * age * 0.05f;
	age += ageSpeed;
	size += sizeExpansion;
	if (size < startSize) {
		size+=(startSize-size) * 0.2f;
	}
	drawRadius = size;
	if (age > 1) {
		age = 1;
		deleteMe = true;
	}
}

void CSmokeProjectile::Draw()
{
	inArray = true;
	unsigned char col[4];
	unsigned char alpha = (unsigned char) ((1 - age) * 255);
	col[0] = (unsigned char) (color * alpha);
	col[1] = (unsigned char) (color * alpha);
	col[2] = (unsigned char) (color * alpha);
	col[3] = (unsigned char) alpha/*-alphaFalloff*globalRendering->timeOffset*/;
	//int frame=textureNum;
	//float xmod=0.125f+(float(int(frame%6)))/16;
	//float ymod=(int(frame/6))/16.0f;

	const float interSize = size + (sizeExpansion * globalRendering->timeOffset);
	const float3 pos1 ((camera->right - camera->up) * interSize);
	const float3 pos2 ((camera->right + camera->up) * interSize);

	#define st projectileDrawer->smoketex[textureNum]
	va->AddVertexTC(drawPos - pos2, st->xstart, st->ystart, col);
	va->AddVertexTC(drawPos + pos1, st->xend,   st->ystart, col);
	va->AddVertexTC(drawPos + pos2, st->xend,   st->yend,   col);
	va->AddVertexTC(drawPos - pos1, st->xstart, st->yend,   col);
	#undef st
}
