/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "BubbleProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ProjectileHandler.h"

CR_BIND_DERIVED(CBubbleProjectile, CProjectile, (NULL, ZeroVector, ZeroVector, 0.0f, 0.0f, 0.0f, 0.0f))

CR_REG_METADATA(CBubbleProjectile, (
	CR_MEMBER(ttl),
	CR_MEMBER(alpha),
	CR_MEMBER(size),
	CR_MEMBER(startSize),
	CR_MEMBER(sizeExpansion)
))


CBubbleProjectile::CBubbleProjectile(
	CUnit* owner,
	float3 pos,
	float3 speed,
	float ttl,
	float startSize,
	float sizeExpansion,
	float alpha
):
	CProjectile(pos, speed, owner, false, false, false),
	ttl((int) ttl),
	alpha(alpha),
	size(startSize * 0.4f),
	startSize(startSize),
	sizeExpansion(sizeExpansion)
{
	checkCol = false;
}

CBubbleProjectile::~CBubbleProjectile()
{

}


void CBubbleProjectile::Update()
{
	pos += speed;
	--ttl;
	size += sizeExpansion;
	if (size < startSize) {
		size += (startSize - size) * 0.2f;
	}
	drawRadius=size;

	if (pos.y > (-size * 0.7f)) {
		pos.y = -size * 0.7f;
		alpha -= 0.03f;
	}
	if (ttl < 0) {
		alpha -= 0.03f;
	}
	if (alpha < 0) {
		alpha = 0;
		deleteMe = true;
	}
}

void CBubbleProjectile::Draw()
{
	inArray = true;
	unsigned char col[4];
	col[0] = (unsigned char)(255 * alpha);
	col[1] = (unsigned char)(255 * alpha);
	col[2] = (unsigned char)(255 * alpha);
	col[3] = (unsigned char)(255 * alpha);

	const float interSize = size + sizeExpansion * globalRendering->timeOffset;

	#define bt projectileDrawer->bubbletex
	va->AddVertexTC(drawPos - camera->GetRight() * interSize - camera->GetUp() * interSize, bt->xstart, bt->ystart, col);
	va->AddVertexTC(drawPos + camera->GetRight() * interSize - camera->GetUp() * interSize, bt->xend,   bt->ystart, col);
	va->AddVertexTC(drawPos + camera->GetRight() * interSize + camera->GetUp() * interSize, bt->xend,   bt->yend,   col);
	va->AddVertexTC(drawPos - camera->GetRight() * interSize + camera->GetUp() * interSize, bt->xstart, bt->yend,   col);
	#undef bt
}

int CBubbleProjectile::GetProjectilesCount() const
{
	return 1;
}
