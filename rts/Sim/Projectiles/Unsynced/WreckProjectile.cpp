/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "SmokeProjectile.h"
#include "WreckProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/Colors.h"
#include "Rendering/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ProjectileHandler.h"

CR_BIND_DERIVED(CWreckProjectile, CProjectile, (ZeroVector, ZeroVector, 0.0f, NULL));

CR_REG_METADATA(CWreckProjectile,
	CR_RESERVED(8)
);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWreckProjectile::CWreckProjectile(float3 pos, float3 speed, float temperature, CUnit* owner)
	: CProjectile(pos,speed,owner, false, false, false)
{
	checkCol = false;
	drawRadius = 2.0f;
}

CWreckProjectile::~CWreckProjectile()
{

}

void CWreckProjectile::Update()
{
	speed.y += mygravity;
	speed.x *= 0.994f;
	speed.z *= 0.994f;

	if (speed.y > 0) {
		speed.y *= 0.998f;
	}

	pos += speed;

	if (!(gs->frameNum & (projectileHandler->particleSaturation < 0.5f? 1: 3))) {
		CSmokeProjectile* hp = new CSmokeProjectile(pos, ZeroVector, 50, 4, 0.3f, owner(), 0.5f);
		hp->size += 0.1f;
	}
	if (pos.y + 0.3f < ground->GetApproximateHeight(pos.x, pos.z)) {
		deleteMe = true;
	}
}

void CWreckProjectile::Draw()
{
	inArray = true;
	unsigned char col[4];
	col[0] = (unsigned char) (0.15f * 200);
	col[1] = (unsigned char) (0.1f  * 200);
	col[2] = (unsigned char) (0.05f * 200);
	col[3] = 200;

	#define wt projectileDrawer->wrecktex
	va->AddVertexTC(drawPos - camera->right * drawRadius - camera->up * drawRadius, wt->xstart, wt->ystart, col);
	va->AddVertexTC(drawPos + camera->right * drawRadius - camera->up * drawRadius, wt->xend,   wt->ystart, col);
	va->AddVertexTC(drawPos + camera->right * drawRadius + camera->up * drawRadius, wt->xend,   wt->yend,   col);
	va->AddVertexTC(drawPos - camera->right * drawRadius + camera->up * drawRadius, wt->xstart, wt->yend,   col);
	#undef wt
}

void CWreckProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	points.AddVertexQC(pos, color4::redA);
}
