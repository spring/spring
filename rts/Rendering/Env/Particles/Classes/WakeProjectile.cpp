/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "WakeProjectile.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"

CR_BIND_DERIVED(CWakeProjectile, CProjectile, )

CR_REG_METADATA(CWakeProjectile,(
	CR_MEMBER(alpha),
	CR_MEMBER(alphaFalloff),
	CR_MEMBER(alphaAdd),
	CR_MEMBER(alphaAddTime),
	CR_MEMBER(size),
	CR_MEMBER(sizeExpansion),
	CR_MEMBER(rotation),
	CR_MEMBER(rotSpeed)
))


CWakeProjectile::CWakeProjectile(
	CUnit* owner,
	const float3& pos,
	const float3& speed,
	float startSize,
	float sizeExpansion,
	float alpha,
	float alphaFalloff,
	float fadeupTime
)
: CProjectile(pos, speed, owner, false, false, false)
, alpha(0.0f)
, alphaFalloff(alphaFalloff)
, alphaAdd(alpha / fadeupTime)
, alphaAddTime((int)fadeupTime)
, size(startSize)
, sizeExpansion(sizeExpansion)
{
	this->pos.y = 0.0f;
	this->speed.y = 0.0f;
	rotation = gu->RandFloat() * PI*2;
	rotSpeed = (gu->RandFloat() - 0.5f) * PI*2*0.01f;
	checkCol = false;
	if (water->BlockWakeProjectiles()) {
		this->alpha = 0;
		alphaAddTime = 0;
		size = 0;
	}
}

void CWakeProjectile::Update()
{
	pos += speed;
	rotation += rotSpeed;
	alpha -= alphaFalloff;
	size += sizeExpansion;
	drawRadius = size;

	if (alphaAddTime != 0) {
		alpha += alphaAdd;
		--alphaAddTime;
	} else if (alpha < 0) {
		alpha = 0;
		deleteMe = true;
	}
}

void CWakeProjectile::Draw()
{
	inArray = true;
	unsigned char col[4];
	col[0] = (unsigned char) (255 * alpha);
	col[1] = (unsigned char) (255 * alpha);
	col[2] = (unsigned char) (255 * alpha);
	col[3] = (unsigned char) (255 * alpha)/*-alphaFalloff*globalRendering->timeOffset*/;

	float interSize = size + sizeExpansion * globalRendering->timeOffset;
	float interRot = rotation + rotSpeed * globalRendering->timeOffset;

	const float3 dir1 = float3(std::cos(interRot), 0, std::sin(interRot)) * interSize;
	const float3 dir2 = dir1.cross(UpVector);

	#define wt projectileDrawer->waketex
	va->AddVertexTC(drawPos + dir1 + dir2, wt->xstart, wt->ystart, col);
	va->AddVertexTC(drawPos + dir1 - dir2, wt->xstart, wt->yend,   col);
	va->AddVertexTC(drawPos - dir1 - dir2, wt->xend,   wt->yend,   col);
	va->AddVertexTC(drawPos - dir1 + dir2, wt->xend,   wt->ystart, col);
	#undef wt
}

int CWakeProjectile::GetProjectilesCount() const
{
	return 1;
}
