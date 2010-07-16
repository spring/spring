/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <algorithm>
#include "mmgr.h"

#include "SpherePartProjectile.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ProjectileDrawer.hpp"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "System/GlobalUnsynced.h"

using std::min;

CR_BIND_DERIVED(CSpherePartProjectile, CProjectile, (float3(0, 0, 0), 0, 0, 0.0, 0.0, 0, NULL, float3(0, 0, 0)));

CR_REG_METADATA(CSpherePartProjectile, (
	CR_MEMBER(centerPos),
	CR_MEMBER(vectors),
	CR_MEMBER(color),
	CR_MEMBER(sphereSize),
	CR_MEMBER(expansionSpeed),
	CR_MEMBER(xbase),
	CR_MEMBER(ybase),
	CR_MEMBER(baseAlpha),
	CR_MEMBER(age),
	CR_MEMBER(ttl),
	CR_MEMBER(texx),
	CR_MEMBER(texy),
	CR_RESERVED(16)
	));

CSpherePartProjectile::CSpherePartProjectile(const float3& centerPos, int xpart, int ypart, float expansionSpeed, float alpha, int ttl, CUnit* owner, const float3& color):
	CProjectile(centerPos, ZeroVector, owner, false, false, false),
	centerPos(centerPos),
	color(color),
	sphereSize(expansionSpeed),
	expansionSpeed(expansionSpeed),
	xbase(xpart),
	ybase(ypart),
	baseAlpha(alpha),
	age(0),
	ttl(ttl)
{
	deleteMe=false;
	checkCol=false;

	for(int y=0;y<5;++y){
		float yp=(y+ypart)/16.0f*PI-PI/2;
		for(int x=0;x<5;++x){
			float xp=(x+xpart)/32.0f*2*PI;
			vectors[y*5+x]=float3(sin(xp)*cos(yp),sin(yp),cos(xp)*cos(yp));
		}
	}
	pos=centerPos+vectors[12]*sphereSize;

	drawRadius=60;
	alwaysVisible=true;
	texx = projectileDrawer->sphereparttex->xstart + (projectileDrawer->sphereparttex->xend - projectileDrawer->sphereparttex->xstart) * 0.5f;
	texy = projectileDrawer->sphereparttex->ystart + (projectileDrawer->sphereparttex->yend - projectileDrawer->sphereparttex->ystart) * 0.5f;
}

CSpherePartProjectile::~CSpherePartProjectile(void)
{
}

void CSpherePartProjectile::Update(void)
{
	age++;
	if(age>=ttl)
		deleteMe=true;
	sphereSize+=expansionSpeed;
	pos=centerPos+vectors[12]*sphereSize;
}

void CSpherePartProjectile::Draw(void)
{
	unsigned char col[4];
	va->EnlargeArrays(4*4*4,0,VA_SIZE_TC);

	const float interSize = sphereSize + expansionSpeed * globalRendering->timeOffset;

	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			float alpha =
				baseAlpha *
				(1.0f - min(1.0f, float(age + globalRendering->timeOffset) / (float) ttl)) *
				(1.0f - fabs(y + ybase - 8.0f) / 8.0f * 1.0f);

			col[0]=(unsigned char) (color.x*255.0f*alpha);
			col[1]=(unsigned char) (color.y*255.0f*alpha);
			col[2]=(unsigned char) (color.z*255.0f*alpha);
			col[3]=((unsigned char) (40*alpha))+1;
			va->AddVertexQTC(centerPos+vectors[y*5+x]*interSize,texx,texy,col);
			va->AddVertexQTC(centerPos+vectors[y*5+x+1]*interSize,texx,texy,col);
			alpha=baseAlpha*(1.0f-min(float(1.0f),float(age+globalRendering->timeOffset)/(float)ttl))*(1-fabs(y+1+ybase-8.0f)/8.0f*1.0f);

			col[0]=(unsigned char) (color.x*255.0f*alpha);
			col[1]=(unsigned char) (color.y*255.0f*alpha);
			col[2]=(unsigned char) (color.z*255.0f*alpha);
			col[3]=((unsigned char) (40*alpha))+1;
			va->AddVertexQTC(centerPos+vectors[(y+1)*5+x+1]*interSize,texx,texy,col);
			va->AddVertexQTC(centerPos+vectors[(y+1)*5+x]*interSize,texx,texy,col);
		}
	}
}


void CSpherePartProjectile::CreateSphere(float3 pos, float alpha, int ttl, float expansionSpeed , CUnit* owner, float3 color)
{
	for(int y=0;y<16;y+=4){
		for(int x=0;x<32;x+=4){
			new CSpherePartProjectile(pos,x,y,expansionSpeed,alpha,ttl,owner,color);
		}
	}
}

CSpherePartSpawner::CSpherePartSpawner()
:	CProjectile()
{
}

CSpherePartSpawner::~CSpherePartSpawner()
{
}

CR_BIND_DERIVED(CSpherePartSpawner, CProjectile, );

CR_REG_METADATA(CSpherePartSpawner,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(alpha),
		CR_MEMBER(ttl),
		CR_MEMBER(expansionSpeed),
		CR_MEMBER(color),
	CR_MEMBER_ENDFLAG(CM_Config)
));

void CSpherePartSpawner::Init(const float3& pos, CUnit *owner)
{
	CProjectile::Init(pos, owner);
	deleteMe = true;
	CSpherePartProjectile::CreateSphere(pos, alpha, ttl, expansionSpeed, owner, color);
}
