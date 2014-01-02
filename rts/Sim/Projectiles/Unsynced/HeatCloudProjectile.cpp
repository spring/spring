/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Game/Camera.h"
#include "HeatCloudProjectile.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"

CR_BIND_DERIVED(CHeatCloudProjectile, CProjectile, );

CR_REG_METADATA(CHeatCloudProjectile,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(heat),
		CR_MEMBER(maxheat),
		CR_MEMBER(heatFalloff),
		CR_MEMBER(size),
		CR_MEMBER(sizeGrowth),
		CR_MEMBER(sizemod),
		CR_MEMBER(sizemodmod),
		CR_MEMBER(texture),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_RESERVED(8)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHeatCloudProjectile::CHeatCloudProjectile()
	: CProjectile()

	, heat(0.0f)
	, maxheat(0.0f)
	, heatFalloff(0.0f)
	, size(0.0f)
	, sizeGrowth(0.0f)
	, sizemod(0.0f)
	, sizemodmod(0.0f)
{
	checkCol = false;
	useAirLos = true;
	texture = projectileDrawer->heatcloudtex;
}

CHeatCloudProjectile::CHeatCloudProjectile(
	CUnit* owner,
	const float3& pos,
	const float3& speed,
	const float temperature,
	const float size
)
	: CProjectile(pos, speed, owner, false, false, false)

	, heat(temperature)
	, maxheat(temperature)
	, heatFalloff(1.0f)
	, size(0.0f)
	, sizemod(0.0f)
	, sizemodmod(0.0f)
{
	sizeGrowth = size / temperature;
	checkCol = false;
	useAirLos = true;
	texture = projectileDrawer->heatcloudtex;

	SetRadiusAndHeight(size + sizeGrowth * heat / heatFalloff, 0.0f);
}

CHeatCloudProjectile::~CHeatCloudProjectile()
{
}

void CHeatCloudProjectile::Update()
{
	pos += speed;
	heat -= heatFalloff;
	if (heat <= 0) {
		deleteMe = true;
		heat = 0;
	}
	size += sizeGrowth;
	sizemod *= sizemodmod;
}

void CHeatCloudProjectile::Draw()
{
	inArray = true;
	unsigned char col[4];
	float dheat = heat-globalRendering->timeOffset;
	if (dheat < 0) {
		dheat = 0;
	}
	float alpha = (dheat / maxheat) * 255.0f;
	col[0] = (unsigned char) alpha;
	col[1] = (unsigned char) alpha;
	col[2] = (unsigned char) alpha;
	col[3] = 1;//(dheat/maxheat)*255.0f;

	const float drawsize = (size + sizeGrowth * globalRendering->timeOffset) * (1.0f - sizemod);

	va->AddVertexTC(drawPos - camera->right * drawsize - camera->up * drawsize, texture->xstart, texture->ystart, col);
	va->AddVertexTC(drawPos + camera->right * drawsize - camera->up * drawsize, texture->xend,   texture->ystart, col);
	va->AddVertexTC(drawPos + camera->right * drawsize + camera->up * drawsize, texture->xend,   texture->yend,   col);
	va->AddVertexTC(drawPos - camera->right * drawsize + camera->up * drawsize, texture->xstart, texture->yend,   col);
}
