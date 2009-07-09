#include "StdAfx.h"
#include "mmgr.h"

#include "Game/Camera.h"
#include "GeoSquareProjectile.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "GlobalUnsynced.h"

CR_BIND_DERIVED(CGeoSquareProjectile, CProjectile, (float3(0,0,0),float3(0,0,0),float3(0,0,0),float3(0,0,0),0,0));

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
	CR_MEMBER(a),
	CR_RESERVED(8)
	));

CGeoSquareProjectile::CGeoSquareProjectile(const float3& p1,const float3& p2,const float3& v1,const float3& v2,float w1,float w2 GML_PARG_C)
: CProjectile((p1+p2)*0.5f,ZeroVector,0, false, false, false GML_PARG_P),
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
	SetRadius(p1.distance(p2) * 0.55f);
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
	dif.ANormalize();
	float3 dir1(dif.cross(v1));
	dir1.ANormalize();
	float3 dif2(p2-camera->pos);
	dif2.ANormalize();
	float3 dir2(dif2.cross(v2));
	dir2.ANormalize();


	float u = (ph->geosquaretex.xstart + ph->geosquaretex.xend) / 2;
	float v0 = ph->geosquaretex.ystart;
	float v1 = ph->geosquaretex.yend;

	if(w2!=0){
		va->AddVertexTC(p1-dir1*w1,u,v1,col);
		va->AddVertexTC(p1+dir1*w1,u,v0,col);
		va->AddVertexTC(p2+dir2*w2,u,v0,col);
		va->AddVertexTC(p2-dir2*w2,u,v1,col);
	} else {
		va->AddVertexTC(p1-dir1*w1,u,v1,col);
		va->AddVertexTC(p1+dir1*w1,u,v0,col);
		va->AddVertexTC(p2,u,v0+(v1-v0)*0.5f,col);
		va->AddVertexTC(p2,u,v0+(v1-v0)*1.5f,col);
	}
}

void CGeoSquareProjectile::Update(void)
{
}
