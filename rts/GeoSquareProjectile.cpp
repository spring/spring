#include "stdafx.h"
#include ".\geosquareprojectile.h"
#include "projectilehandler.h"
#include "geosquareprojectile.h"
#include "mygl.h"
#include "vertexarray.h"
#include "camera.h"
//#include "mmgr.h"

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
	col[0]=r*a*255;
	col[1]=g*a*255;
	col[2]=b*a*255;
	col[3]=a*255;

	float3 dif(p1-camera->pos);
	dif.Normalize();
	float3 dir1(dif.cross(v1));
	dir1.Normalize();
	float3 dif2(p2-camera->pos);
	dif2.Normalize();
	float3 dir2(dif2.cross(v2));
	dir2.Normalize();

	if(w2!=0){
		va->AddVertexTC(p1-dir1*w1,1.0/16,1.0/8,col);
		va->AddVertexTC(p1+dir1*w1,1.0/16,0.0/8,col);
		va->AddVertexTC(p2+dir2*w2,1.0/16,0.0/8,col);
		va->AddVertexTC(p2-dir2*w2,1.0/16,1.0/8,col);
	} else {
		va->AddVertexTC(p1-dir1*w1,1.0/16,1.0/8,col);
		va->AddVertexTC(p1+dir1*w1,1.0/16,0.0/8,col);
		va->AddVertexTC(p2+dir2*w2,1.0/16,0.5/8,col);
		va->AddVertexTC(p2-dir2*w2,1.0/16,1.5/8,col);
	}
}

void CGeoSquareProjectile::Update(void)
{
}
