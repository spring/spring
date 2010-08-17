/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "Game/Camera.h"
#include "GenericParticleProjectile.h"
#include "GlobalUnsynced.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/ColorMap.h"
#include "Sim/Projectiles/ProjectileHandler.h"

CR_BIND_DERIVED(CGenericParticleProjectile, CProjectile, (ZeroVector, ZeroVector, NULL));

CR_REG_METADATA(CGenericParticleProjectile,(
	CR_MEMBER(gravity),
	CR_MEMBER(texture),
	CR_MEMBER(colorMap),
	CR_MEMBER(directional),
	CR_MEMBER(life),
	CR_MEMBER(decayrate),
	CR_MEMBER(size),
	CR_MEMBER(airdrag),
	CR_MEMBER(sizeGrowth),
	CR_MEMBER(sizeMod),
	CR_RESERVED(8)
	));

CGenericParticleProjectile::CGenericParticleProjectile(const float3& pos, const float3& speed, CUnit* owner)
	: CProjectile(pos, speed, owner, false, false, false)
	, gravity(ZeroVector)
	, texture(NULL)
	, colorMap(NULL)
	, directional(false)
	, life(0.0f)
	, decayrate(0.0f)
	, size(0.0f)
	, airdrag(0.0f)
	, sizeGrowth(0.0f)
	, sizeMod(0.0f)
{
	// set fields from super-classes
	useAirLos = true;
	checkCol  = false;
	deleteMe  = false;
}

CGenericParticleProjectile::~CGenericParticleProjectile()
{
}

void CGenericParticleProjectile::Update()
{
	pos += speed;
	life += decayrate;
	speed += gravity;
	speed *= airdrag;
	size = size * sizeMod + sizeGrowth;

	if (life > 1.0f) {
		deleteMe = true;
	}
}

void CGenericParticleProjectile::Draw()
{
	inArray = true;

	if (directional) {
		float3 dif(pos-camera->pos);
		dif.ANormalize();
		float3 dir1(dif.cross(speed));
		dir1.ANormalize();
		float3 dir2(dif.cross(dir1));

		unsigned char color[4];

		colorMap->GetColor(color, life);

		va->AddVertexTC(drawPos - dir1 * size - dir2 * size, texture->xstart, texture->ystart, color);
		va->AddVertexTC(drawPos - dir1 * size + dir2 * size, texture->xend,   texture->ystart, color);
		va->AddVertexTC(drawPos + dir1 * size + dir2 * size, texture->xend,   texture->yend,   color);
		va->AddVertexTC(drawPos + dir1 * size - dir2 * size, texture->xstart, texture->yend,   color);
	} else {
		unsigned char color[4];

		colorMap->GetColor(color, life);

		va->AddVertexTC(drawPos - camera->right * size - camera->up * size, texture->xstart, texture->ystart, color);
		va->AddVertexTC(drawPos + camera->right * size - camera->up * size, texture->xend,   texture->ystart, color);
		va->AddVertexTC(drawPos + camera->right * size + camera->up * size, texture->xend,   texture->yend,   color);
		va->AddVertexTC(drawPos - camera->right * size + camera->up * size, texture->xstart, texture->yend,   color);
	}
}
