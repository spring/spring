/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Game/Camera.h"
#include "GeoSquareProjectile.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"

CR_BIND_DERIVED(CGeoSquareProjectile, CProjectile, )

CR_REG_METADATA(CGeoSquareProjectile,(
	CR_MEMBER(p1),
	CR_MEMBER(p2),
	CR_MEMBER(v1),
	CR_MEMBER(v2),
	CR_MEMBER(w1),
	CR_MEMBER(w2),
	CR_MEMBER(r),
	CR_MEMBER(g),
	CR_MEMBER(b),
	CR_MEMBER(a)
))


CGeoSquareProjectile::CGeoSquareProjectile(const float3& p1, const float3& p2, const float3& v1, const float3& v2, float w1, float w2)
	: CProjectile((p1 + p2) * 0.5f, ZeroVector, NULL, false, false, false),
	p1(p1),
	p2(p2),
	v1(v1),
	v2(v2),
	w1(w1),
	w2(w2),
	r(0.5f),
	g(1.0f),
	b(0.5f),
	a(0.5f)
{
	checkCol = false;
	alwaysVisible = true;

	SetRadiusAndHeight(p1.distance(p2) * 0.55f, 0.0f);
}

CGeoSquareProjectile::~CGeoSquareProjectile()
{
}

void CGeoSquareProjectile::Draw()
{
	inArray = true;
	unsigned char col[4];
	col[0] = (unsigned char) (r * a * 255);
	col[1] = (unsigned char) (g * a * 255);
	col[2] = (unsigned char) (b * a * 255);
	col[3] = (unsigned char) (    a * 255);

	float3 dif(p1 - camera->GetPos());
	dif.ANormalize();
	float3 dir1(dif.cross(v1));
	dir1.ANormalize();
	float3 dif2(p2 - camera->GetPos());
	dif2.ANormalize();
	float3 dir2(dif2.cross(v2));
	dir2.ANormalize();


	const float u = (projectileDrawer->geosquaretex->xstart + projectileDrawer->geosquaretex->xend) / 2;
	const float v0 = projectileDrawer->geosquaretex->ystart;
	const float v1 = projectileDrawer->geosquaretex->yend;

	if (w2 != 0) {
		va->AddVertexTC(p1 - dir1 * w1, u, v1, col);
		va->AddVertexTC(p1 + dir1 * w1, u, v0, col);
		va->AddVertexTC(p2 + dir2 * w2, u, v0, col);
		va->AddVertexTC(p2 - dir2 * w2, u, v1, col);
	} else {
		va->AddVertexTC(p1 - dir1 * w1, u, v1,                    col);
		va->AddVertexTC(p1 + dir1 * w1, u, v0,                    col);
		va->AddVertexTC(p2,             u, v0 + (v1 - v0) * 0.5f, col);
		va->AddVertexTC(p2,             u, v0 + (v1 - v0) * 1.5f, col);
	}
}

void CGeoSquareProjectile::Update()
{
}

int CGeoSquareProjectile::GetProjectilesCount() const
{
	return 1;
}
