/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "BubbleProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.hpp"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "System/GlobalUnsynced.h"

CR_BIND_DERIVED(CBubbleProjectile, CProjectile, (float3(0,0,0),float3(0,0,0),0,0,0,NULL,0));

CR_REG_METADATA(CBubbleProjectile, (
	CR_MEMBER(ttl),
	CR_MEMBER(alpha),
	CR_MEMBER(size),
	CR_MEMBER(startSize),
	CR_MEMBER(sizeExpansion),
	CR_RESERVED(8)
	));


CBubbleProjectile::CBubbleProjectile(float3 pos,float3 speed, float ttl, float startSize, float sizeExpansion, CUnit* owner, float alpha):
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
	pos+=speed;
	--ttl;
	size+=sizeExpansion;
	if (size < startSize)
		size += (startSize - size) * 0.2f;
	drawRadius=size;

	if (pos.y>-size * 0.7f) {
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
	va->AddVertexTC(drawPos - camera->right * interSize - camera->up * interSize, bt->xstart, bt->ystart, col);
	va->AddVertexTC(drawPos + camera->right * interSize - camera->up * interSize, bt->xend,   bt->ystart, col);
	va->AddVertexTC(drawPos + camera->right * interSize + camera->up * interSize, bt->xend,   bt->yend,   col);
	va->AddVertexTC(drawPos - camera->right * interSize + camera->up * interSize, bt->xstart, bt->yend,   col);
	#undef bt
}
