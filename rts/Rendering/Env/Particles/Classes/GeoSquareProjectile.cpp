/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Game/Camera.h"
#include "GeoSquareProjectile.h"
#include "Rendering/Env/Particles/ProjectileDrawer.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/TextureAtlas.h"


CR_BIND_DERIVED(CGeoSquareProjectile, CProjectile, )

CR_REG_METADATA(CGeoSquareProjectile,(
	CR_MEMBER(p1),
	CR_MEMBER(p2),
	CR_MEMBER(v1),
	CR_MEMBER(v2),
	CR_MEMBER(xsize),
	CR_MEMBER(ysize),
	CR_MEMBER(r),
	CR_MEMBER(g),
	CR_MEMBER(b),
	CR_MEMBER(a)
))


CGeoSquareProjectile::CGeoSquareProjectile(const float3& p1, const float3& p2, const float3& v1, const float3& v2, float xsize, float ysize)
	: CProjectile((p1 + p2) * 0.5f, ZeroVector, nullptr, false, false, false),
	p1(p1),
	p2(p2),
	v1(v1),
	v2(v2),
	xsize(xsize),
	ysize(ysize),
	r(0.5f),
	g(1.0f),
	b(0.5f),
	a(0.5f)
{
	checkCol = false;
	alwaysVisible = true;

	SetRadiusAndHeight(p1.distance(p2) * 0.55f, 0.0f);
}


void CGeoSquareProjectile::Draw(GL::RenderDataBufferTC* va) const
{
	unsigned char col[4];
	col[0] = (unsigned char) (r * a * 255);
	col[1] = (unsigned char) (g * a * 255);
	col[2] = (unsigned char) (b * a * 255);
	col[3] = (unsigned char) (    a * 255);

	float3 dif  = (p1 - camera->GetPos()).ANormalize();
	float3 dif2 = (p2 - camera->GetPos()).ANormalize();

	const float3 xdir = (dif.cross(v1)).ANormalize();
	const float3 ydir = (dif2.cross(v2)).ANormalize();


	const float u   = (projectileDrawer->geosquaretex->xstart + projectileDrawer->geosquaretex->xend) * 0.5f;
	const float v0s = projectileDrawer->geosquaretex->ystart;
	const float v0e = projectileDrawer->geosquaretex->yend;

	if (ysize != 0) {
		va->SafeAppend({p1 - xdir * xsize, u, v0e, col});
		va->SafeAppend({p1 + xdir * xsize, u, v0s, col});
		va->SafeAppend({p2 + ydir * ysize, u, v0s, col});

		va->SafeAppend({p2 + ydir * ysize, u, v0s, col});
		va->SafeAppend({p2 - ydir * ysize, u, v0e, col});
		va->SafeAppend({p1 - xdir * xsize, u, v0e, col});
	} else {
		va->SafeAppend({p1 - xdir * xsize, u, v0e,                      col});
		va->SafeAppend({p1 + xdir * xsize, u, v0s,                      col});
		va->SafeAppend({p2,                u, v0s + (v0e - v0s) * 0.5f, col});

		va->SafeAppend({p2,                u, v0s + (v0e - v0s) * 0.5f, col});
		va->SafeAppend({p2,                u, v0s + (v0e - v0s) * 1.5f, col});
		va->SafeAppend({p1 - xdir * xsize, u, v0e,                      col});
	}
}

