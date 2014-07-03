/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ExploSpikeProjectile.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"

CR_BIND_DERIVED(CExploSpikeProjectile, CProjectile, );

CR_REG_METADATA(CExploSpikeProjectile,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(length),
		CR_MEMBER(width),
		CR_MEMBER(alpha),
		CR_MEMBER(alphaDecay),
		CR_MEMBER(lengthGrowth),
		CR_MEMBER(color),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_RESERVED(8)
));

CExploSpikeProjectile::CExploSpikeProjectile()
	: CProjectile()
	, length(0.0f)
	, width(0.0f)
	, alpha(0.0f)
	, alphaDecay(0.0f)
	, lengthGrowth(0.0f)
	, color(1.0f, 0.8f, 0.5f)
{
}

CExploSpikeProjectile::CExploSpikeProjectile(
	CUnit* owner,
	const float3& pos,
	const float3& spd,
	float length,
	float width,
	float alpha,
	float alphaDecay
):
	CProjectile(pos, spd, owner, false, false, false),
	length(length),
	width(width),
	alpha(alpha),
	alphaDecay(alphaDecay),
	color(1.0f, 0.8f, 0.5f)
{
	lengthGrowth = speed.w * (0.5f + gu->RandFloat() * 0.4f);

	checkCol  = false;
	useAirLos = true;

	SetRadiusAndHeight(length + lengthGrowth * alpha / alphaDecay, 0.0f);
}

void CExploSpikeProjectile::Update()
{
	pos += speed;
	length += lengthGrowth;
	alpha -= alphaDecay;

	if (alpha <= 0) {
		alpha = 0;
		deleteMe = true;
	}
}

void CExploSpikeProjectile::Draw()
{
	inArray = true;

	const float3 dif = (pos - camera->GetPos()).ANormalize();
	const float3 dir2 = (dif.cross(dir)).ANormalize();

	unsigned char col[4];
	const float a = std::max(0.0f, alpha-alphaDecay * globalRendering->timeOffset) * 255;
	col[0] = (unsigned char)(a * color.x);
	col[1] = (unsigned char)(a * color.y);
	col[2] = (unsigned char)(a * color.z);
	col[3] = 1;

	const float3 l = (dir * length) + (lengthGrowth * globalRendering->timeOffset);
	const float3 w = dir2 * width;

	#define let projectileDrawer->laserendtex
	va->AddVertexTC(drawPos + l + w, let->xend,   let->yend,   col);
	va->AddVertexTC(drawPos + l - w, let->xend,   let->ystart, col);
	va->AddVertexTC(drawPos - l - w, let->xstart, let->ystart, col);
	va->AddVertexTC(drawPos - l + w, let->xstart, let->yend,   col);
	#undef let
}

void CExploSpikeProjectile::Init(const CUnit* owner, const float3& offset)
{
	CProjectile::Init(owner, offset);

	lengthGrowth = dir.Length() * (0.5f + gu->RandFloat() * 0.4f);
	dir /= lengthGrowth;

	checkCol = false;
	useAirLos = true;

	SetRadiusAndHeight(length + lengthGrowth * alpha / alphaDecay, 0.0f);
}
