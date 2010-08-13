/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "ShieldPartProjectile.h"
#include "Rendering/ProjectileDrawer.hpp"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ProjectileHandler.h"

CR_BIND_DERIVED(CShieldPartProjectile, CProjectile, (ZeroVector, 0, 0, 0, ZeroVector, 0, NULL, NULL));

CR_REG_METADATA(CShieldPartProjectile,(
	CR_MEMBER(centerPos),
	CR_MEMBER(vectors),
	CR_MEMBER(texCoords),
	CR_MEMBER(sphereSize),
	CR_MEMBER(baseAlpha),
	CR_MEMBER(color),
	CR_MEMBER(usePerlin),
	CR_RESERVED(16)
	));

CShieldPartProjectile::CShieldPartProjectile(
	const float3& centerPos, int xpart, int ypart, float sphereSize,
	float3 color, float alpha, AtlasedTexture* texture, CUnit* owner)
:	CProjectile(centerPos, ZeroVector, owner, false, false, false),
	centerPos(centerPos),
	sphereSize(sphereSize),
	baseAlpha(alpha),
	color(color)
{
	checkCol = false;

	if (texture) {
		AtlasedTexture* tex = texture;

		for (int y = 0; y < 5; ++y) {
			float yp = (y + ypart) / 16.0f * PI - PI / 2;

			for (int x = 0; x < 5; ++x) {
				float xp = (x + xpart) / 32.0f * 2 * PI;

				vectors[y * 5 + x] = float3(sin(xp) * cos(yp), sin(yp), cos(xp) * cos(yp));
				texCoords[y * 5 + x].x = (vectors[y * 5 + x].x * (2 - fabs(vectors[y * 5 + x].y))) * ((tex->xend - tex->xstart) * 0.25f) + ((tex->xstart + tex->xend) * 0.5f);
				texCoords[y * 5 + x].y = (vectors[y * 5 + x].z * (2 - fabs(vectors[y * 5 + x].y))) * ((tex->yend - tex->ystart) * 0.25f) + ((tex->ystart + tex->yend) * 0.5f);
			}
		}
	}
	pos = centerPos + vectors[12] * sphereSize;

	alwaysVisible = false;
	useAirLos = true;
	drawRadius = sphereSize * 0.4f;
	usePerlin = false;

	if (texture == projectileDrawer->perlintex) {
		usePerlin = true;
		ph->numPerlinProjectiles++;
	}
}

CShieldPartProjectile::~CShieldPartProjectile()
{
	if (ph && usePerlin) {
		ph->numPerlinProjectiles--;
	}
}

void CShieldPartProjectile::Update()
{
	pos = centerPos + vectors[12] * sphereSize;
}

void CShieldPartProjectile::Draw()
{
	if (baseAlpha <= 0.0f) {
		return;
	}

	inArray = true;
	unsigned char col[4];

	float alpha = baseAlpha * 255;
	col[0] = (unsigned char) (color.x * alpha);
	col[1] = (unsigned char) (color.y * alpha);
	col[2] = (unsigned char) (color.z * alpha);
	col[3] = (unsigned char) (alpha);

	va->EnlargeArrays(4 * 4 * 4, 0, VA_SIZE_TC);
	for (int y = 0; y < 4; ++y) { //! CAUTION: loop count must match EnlargeArrays above
		for (int x = 0; x < 4; ++x) {
			va->AddVertexQTC(centerPos + vectors[(y    ) * 5 + x    ] * sphereSize, texCoords[(y    ) * 5 + x    ].x, texCoords[(y    ) * 5 + x    ].y, col);
			va->AddVertexQTC(centerPos + vectors[(y    ) * 5 + x + 1] * sphereSize, texCoords[(y    ) * 5 + x + 1].x, texCoords[(y    ) * 5 + x + 1].y, col);
			va->AddVertexQTC(centerPos + vectors[(y + 1) * 5 + x + 1] * sphereSize, texCoords[(y + 1) * 5 + x + 1].x, texCoords[(y + 1) * 5 + x + 1].y, col);
			va->AddVertexQTC(centerPos + vectors[(y + 1) * 5 + x    ] * sphereSize, texCoords[(y + 1) * 5 + x    ].x, texCoords[(y + 1) * 5 + x    ].y, col);
		}
	}
}
