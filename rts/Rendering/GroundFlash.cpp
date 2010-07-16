/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "GroundFlash.h"
#include "Map/Ground.h"
#include "Game/Camera.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/ProjectileDrawer.hpp"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "System/GlobalUnsynced.h"

CR_BIND_DERIVED(CGroundFlash, CExpGenSpawnable, );

CR_REG_METADATA(CGroundFlash,
(
 	CR_MEMBER_BEGINFLAG(CM_Config),
				CR_MEMBER(size),
	CR_MEMBER_ENDFLAG(CM_Config)
));

CR_BIND_DERIVED(CStandardGroundFlash, CGroundFlash, );

CR_REG_METADATA(CStandardGroundFlash,
(
 	CR_MEMBER_BEGINFLAG(CM_Config),
 		CR_MEMBER(flashSize),
		CR_MEMBER(circleAlpha),
		CR_MEMBER(flashAlpha),
		CR_MEMBER(circleGrowth),
		CR_MEMBER(color),
	CR_MEMBER_ENDFLAG(CM_Config)
));

CR_BIND_DERIVED(CSeismicGroundFlash, CGroundFlash, (float3(0,0,0),1,0,1,1,1,float3(0,0,0)));

CR_REG_METADATA(CSeismicGroundFlash,(
				CR_MEMBER(side1),
				CR_MEMBER(side2),
				CR_MEMBER(texture),
				CR_MEMBER(sizeGrowth),
				CR_MEMBER(alpha),
				CR_MEMBER(fade),
				CR_MEMBER(ttl),
				CR_MEMBER(color)
				));

CR_BIND_DERIVED(CSimpleGroundFlash, CGroundFlash, );

CR_REG_METADATA(CSimpleGroundFlash,
(
 	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(sizeGrowth),
		CR_MEMBER(ttl),
		CR_MEMBER(colorMap),
		CR_MEMBER(texture),
	CR_MEMBER_ENDFLAG(CM_Config)
));

CVertexArray* CGroundFlash::va=0;

CGroundFlash::CGroundFlash(const float3& p)
{
	pos=p;
	alwaysVisible=false;
}

CGroundFlash::CGroundFlash()
{
}

CStandardGroundFlash::CStandardGroundFlash()
{
}

CStandardGroundFlash::CStandardGroundFlash(const float3& p,float circleAlpha,float flashAlpha,float flashSize,float circleSpeed,float ttl, const float3& col)
	: CGroundFlash(p),
	flashSize(flashSize),
	circleSize(circleSpeed),
	circleGrowth(circleSpeed),
	circleAlpha(circleAlpha),
	flashAlpha(flashAlpha),
	flashAge(0),
	flashAgeSpeed(ttl?1.0f/ttl:0),
	circleAlphaDec(ttl?circleAlpha/ttl:0),
	ttl((int)ttl)
{
	for (int a=0;a<3;a++)
		color[a] = (unsigned char)(col[a]*255.0f);

	float3 fw = camera->forward*-1000.0f;
	this->pos.y=ground->GetHeight2(p.x,p.z)+1;

	float3 p1(p.x+flashSize,0,p.z);
		p1.y=ground->GetApproximateHeight(p1.x,p1.z);
		p1 += fw;
	float3 p2(p.x-flashSize,0,p.z);
		p2.y=ground->GetApproximateHeight(p2.x,p2.z);
		p2 += fw;
	float3 p3(p.x,0,p.z+flashSize);
		p3.y=ground->GetApproximateHeight(p3.x,p3.z);
		p3 += fw;
	float3 p4(p.x,0,p.z-flashSize);
		p4.y=ground->GetApproximateHeight(p4.x,p4.z);
		p4 += fw;

	float3 n1((p3-p1).cross(p4-p1));
	n1.ANormalize();
	float3 n2((p4-p2).cross(p3-p2));
	n2.ANormalize();

	size = flashSize; // flashSize is just backward compability

	float3 normal=n1+n2;
	normal.ANormalize();
	side1=normal.cross(float3(1,0,0));
	side1.ANormalize();
	side2=side1.cross(normal);
	ph->AddGroundFlash(this);
}

CStandardGroundFlash::~CStandardGroundFlash()
{
}

bool CStandardGroundFlash::Update()
{
	circleSize+=circleGrowth;
	circleAlpha-=circleAlphaDec;
	flashAge+=flashAgeSpeed;
	return ttl?(--ttl>0):false;
}

void CStandardGroundFlash::Draw()
{
	float iAlpha=circleAlpha-circleAlphaDec*globalRendering->timeOffset;
	if (iAlpha > 1.0f) iAlpha = 1.0f;
	if (iAlpha < 0.0f) iAlpha = 0.0f;

	unsigned char col[4];
	col[0]=color[0];
	col[1]=color[1];
	col[2]=color[2];
	col[3]=(unsigned char) (iAlpha*255);

	float iSize=circleSize+circleGrowth*globalRendering->timeOffset;

	if(iAlpha>0){
		float3 p1=pos+(-side1-side2)*iSize;
		float3 p2=pos+( side1-side2)*iSize;
		float3 p3=pos+( side1+side2)*iSize;
		float3 p4=pos+(-side1+side2)*iSize;

		va->AddVertexQTC(p1, projectileDrawer->groundringtex->xstart, projectileDrawer->groundringtex->ystart, col);
		va->AddVertexQTC(p2, projectileDrawer->groundringtex->xend,   projectileDrawer->groundringtex->ystart, col);
		va->AddVertexQTC(p3, projectileDrawer->groundringtex->xend,   projectileDrawer->groundringtex->yend,   col);
		va->AddVertexQTC(p4, projectileDrawer->groundringtex->xstart, projectileDrawer->groundringtex->yend,   col);
	}

	float iAge=flashAge+flashAgeSpeed*globalRendering->timeOffset;

	if(iAge<1){
		if(iAge<0.091f)
			iAlpha=flashAlpha*iAge*10.0f;
		else
			iAlpha=flashAlpha*(1.0f-iAge);

		if (iAlpha > 1.0f) iAlpha = 1.0f;
		if (iAlpha < 0.0f) iAlpha = 0.0f;

		col[3]=(unsigned char)(iAlpha*255);
		iSize=size;

		float3 p1=pos+(-side1-side2)*iSize;
		float3 p2=pos+( side1-side2)*iSize;
		float3 p3=pos+( side1+side2)*iSize;
		float3 p4=pos+(-side1+side2)*iSize;

		va->AddVertexQTC(p1, projectileDrawer->groundflashtex->xstart, projectileDrawer->groundflashtex->yend,   col);
		va->AddVertexQTC(p2, projectileDrawer->groundflashtex->xend,   projectileDrawer->groundflashtex->yend,   col);
		va->AddVertexQTC(p3, projectileDrawer->groundflashtex->xend,   projectileDrawer->groundflashtex->ystart, col);
		va->AddVertexQTC(p4, projectileDrawer->groundflashtex->xstart, projectileDrawer->groundflashtex->ystart, col);
	}
}

CSeismicGroundFlash::CSeismicGroundFlash(const float3& p, int ttl, int fade, float size, float sizeGrowth, float alpha, const float3& col)
	: CGroundFlash(p),
	texture(projectileDrawer->seismictex),
	sizeGrowth(sizeGrowth),
	alpha(alpha),
	fade(fade),
	ttl(ttl)
{
	size = size;

	alwaysVisible = true;

	for (int a=0;a<3;a++)
		color[a] = (unsigned char)(col[a]*255.0f);

	float flashsize = size+sizeGrowth*ttl;

	float3 fw = camera->forward*-1000.0f;
	this->pos.y=ground->GetHeight2(p.x,p.z)+1;
	float3 p1(p.x+flashsize,0,p.z);
	p1.y=ground->GetApproximateHeight(p1.x,p1.z);
	p1 += fw;
	float3 p2(p.x-flashsize,0,p.z);
	p2.y=ground->GetApproximateHeight(p2.x,p2.z);
	p2 += fw;
	float3 p3(p.x,0,p.z+flashsize);
	p3.y=ground->GetApproximateHeight(p3.x,p3.z);
	p3 += fw;
	float3 p4(p.x,0,p.z-flashsize);
	p4.y=ground->GetApproximateHeight(p4.x,p4.z);
	p4 += fw;
	float3 n1((p3-p1).cross(p4-p1));
	n1.SafeANormalize();
	float3 n2((p4-p2).cross(p3-p2));
	n2.SafeANormalize();

	float3 normal=n1+n2;
	normal.SafeANormalize();
	side1=normal.cross(float3(1,0,0));
	side1.SafeANormalize();
	side2=side1.cross(normal);
	ph->AddGroundFlash(this);
}

CSeismicGroundFlash::~CSeismicGroundFlash()
{
}

void CSeismicGroundFlash::Draw()
{
	color[3] = ttl<fade ? int(((ttl) / (float)(fade)) * 255) : 255;

	const float3 p1 = pos + (-side1 - side2) * size;
	const float3 p2 = pos + ( side1 - side2) * size;
	const float3 p3 = pos + ( side1 + side2) * size;
	const float3 p4 = pos + (-side1 + side2) * size;

	va->AddVertexQTC(p1, texture->xstart, texture->ystart, color);
	va->AddVertexQTC(p2, texture->xend,   texture->ystart, color);
	va->AddVertexQTC(p3, texture->xend,   texture->yend,   color);
	va->AddVertexQTC(p4, texture->xstart, texture->yend,   color);
}

bool CSeismicGroundFlash::Update()
{
	size += sizeGrowth;
	return (--ttl > 0);
}

CSimpleGroundFlash::CSimpleGroundFlash()
{
}

CSimpleGroundFlash::~CSimpleGroundFlash()
{
}

void CSimpleGroundFlash::Init(const float3& explosionPos, CUnit *owner)
{
	pos += explosionPos;

	float flashsize = size+sizeGrowth*ttl;

	float3 fw = camera->forward*-1000.0f;
	this->pos.y=ground->GetHeight2(pos.x,pos.z)+1;
	float3 p1(pos.x+flashsize,0,pos.z);
	p1.y=ground->GetApproximateHeight(p1.x,p1.z);
	p1 += fw;
	float3 p2(pos.x-flashsize,0,pos.z);
	p2.y=ground->GetApproximateHeight(p2.x,p2.z);
	p2 += fw;
	float3 p3(pos.x,0,pos.z+flashsize);
	p3.y=ground->GetApproximateHeight(p3.x,p3.z);
	p3 += fw;
	float3 p4(pos.x,0,pos.z-flashsize);
	p4.y=ground->GetApproximateHeight(p4.x,p4.z);
	p4 += fw;
	float3 n1((p3-p1).cross(p4-p1));
	n1.ANormalize();
	float3 n2((p4-p2).cross(p3-p2));
	n2.ANormalize();

	//pos += fw;

	float3 normal=n1+n2;
	normal.ANormalize();
	side1=normal.cross(float3(1,0,0));
	side1.ANormalize();
	side2=side1.cross(normal);
	ph->AddGroundFlash(this);

	age = ttl ? 0.0f : 1.0f;
	agerate = ttl ? 1.0f/ttl : 1.0f;
}

void CSimpleGroundFlash::Draw()
{
	unsigned char color[4];
	colorMap->GetColor(color, age);

	float3 p1=pos+(-side1-side2)*size;
	float3 p2=pos+( side1-side2)*size;
	float3 p3=pos+( side1+side2)*size;
	float3 p4=pos+(-side1+side2)*size;

	va->AddVertexQTC(p1,texture->xstart,texture->ystart,color);
	va->AddVertexQTC(p2,texture->xend,texture->ystart,color);
	va->AddVertexQTC(p3,texture->xend,texture->yend,color);
	va->AddVertexQTC(p4,texture->xstart,texture->yend,color);
}

bool CSimpleGroundFlash::Update()
{
	age += agerate;
	size+=sizeGrowth;

	return age<1;
}
