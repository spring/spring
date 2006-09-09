#include "StdAfx.h"
#include "GeoSquareProjectile.h"
#include "Rendering/GL/VertexArray.h"
#include "Game/Camera.h"
#include "mmgr.h"
#include "ProjectileHandler.h"

CGeoSquareProjectile::CGeoSquareProjectile(const float3& p1,const float3& p2,const float3& v1,const float3& v2,float w1,float w2)
: CProjectile((p1+p2)*0.5,ZeroVector,0),
	p1(p1),
	p2(p2),
	v1(v1),
	v2(v2),
	w1(w1),
	w2(w2),
	r(0.5),
	g(1.0),
	b(0.5),
	a(0.5)
{
	checkCol=false;
	alwaysVisible=true;
	SetRadius(p1.distance(p2)*0.55);
}

CGeoSquareProjectile::~CGeoSquareProjectile(void)
{
}

void CGeoSquareProjectile::Draw(void)
{
	inArray=true;
	unsigned char col[4];
	col[0]=(unsigned char) (r*a*255);
	col[1]=(unsigned char) (g*a*255);
	col[2]=(unsigned char) (b*a*255);
	col[3]=(unsigned char) (a*255);

	float3 dif(p1-camera->pos);
	dif.Normalize();
	float3 dir1(dif.cross(v1));
	dir1.Normalize();
	float3 dif2(p2-camera->pos);
	dif2.Normalize();
	float3 dir2(dif2.cross(v2));
	dir2.Normalize();

	/* FIXME this shouldn't use circularthingytex,
	it could look fugly if mods override it (through resources.tdf) */

	float u = (ph->circularthingytex.xstart + ph->circularthingytex.xend) / 2;
	float v0 = ph->circularthingytex.ystart;
	float v1 = ph->circularthingytex.yend;

	if(w2!=0){
		va->AddVertexTC(p1-dir1*w1,u,v1,col);
		va->AddVertexTC(p1+dir1*w1,u,v0,col);
		va->AddVertexTC(p2+dir2*w2,u,v0,col);
		va->AddVertexTC(p2-dir2*w2,u,v1,col);
	} else {
		va->AddVertexTC(p1-dir1*w1,u,v1,col);
		va->AddVertexTC(p1+dir1*w1,u,v0,col);
		va->AddVertexTC(p2,u,v0+(v1-v0)*0.5,col);
		va->AddVertexTC(p2,u,v0+(v1-v0)*1.5,col);
	}
}

void CGeoSquareProjectile::Update(void)
{
}
