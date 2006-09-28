#include "StdAfx.h"
#include "GroundFlash.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Map/Ground.h"
#include "Game/Camera.h"
#include "GL/VertexArray.h"
#include "LogOutput.h"
#include "mmgr.h"

CVertexArray* CGroundFlash::va=0;

CStandardGroundFlash::CStandardGroundFlash(float3 pos,float circleAlpha,float flashAlpha,float flashSize,float circleSpeed,float ttl, float3 col)
	: CGroundFlash(pos),
	circleAlpha(circleAlpha),
	flashAlpha(flashAlpha),
	flashSize(flashSize),
	circleGrowth(circleSpeed),
	circleSize(circleSpeed),
	flashAge(0),
	ttl((int)ttl),
	circleAlphaDec(circleAlpha/ttl),
	flashAgeSpeed(1.0f/ttl)
{
	for (int a=0;a<3;a++)
		color[a] = (unsigned char)(col[a]*255.0f);

	float3 fw = camera->forward*-1000.0f;
	this->pos.y=ground->GetHeight2(pos.x,pos.z)+1;
	float3 p1(pos.x+flashSize,0,pos.z);
	p1.y=ground->GetApproximateHeight(p1.x,p1.z);
	p1 += fw;
	float3 p2(pos.x-flashSize,0,pos.z);
	p2.y=ground->GetApproximateHeight(p2.x,p2.z);
	p2 += fw;
	float3 p3(pos.x,0,pos.z+flashSize);
	p3.y=ground->GetApproximateHeight(p3.x,p3.z);
	p3 += fw;
	float3 p4(pos.x,0,pos.z-flashSize);
	p4.y=ground->GetApproximateHeight(p4.x,p4.z);
	p4 += fw;
	float3 n1((p3-p1).cross(p4-p1));
	n1.Normalize();
	float3 n2((p4-p2).cross(p3-p2));
	n2.Normalize();

	pos += fw;

	float3 normal=n1+n2;
	normal.Normalize();
	side1=normal.cross(float3(1,0,0));
	side1.Normalize();
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
	return --ttl>0;
}

void CStandardGroundFlash::Draw()
{
	float iAlpha=circleAlpha-circleAlphaDec*gu->timeOffset;
	if (iAlpha > 1.0f) iAlpha = 1.0f;
	if (iAlpha < 0.0f) iAlpha = 0.0f;

	unsigned char col[4];
	col[0]=color[0];
	col[1]=color[1];
	col[2]=color[2];
	col[3]=(unsigned char) (iAlpha*255);

	float iSize=circleSize+circleGrowth*gu->timeOffset;

	if(iAlpha>0){
		float3 p1=pos+(-side1-side2)*iSize;
		float3 p2=pos+( side1-side2)*iSize;
		float3 p3=pos+( side1+side2)*iSize;
		float3 p4=pos+(-side1+side2)*iSize;

		va->AddVertexTC(p1,ph->groundringtex.xstart,ph->groundringtex.ystart,col);
		va->AddVertexTC(p2,ph->groundringtex.xend,ph->groundringtex.ystart,col);
		va->AddVertexTC(p3,ph->groundringtex.xend,ph->groundringtex.yend,col);
		va->AddVertexTC(p4,ph->groundringtex.xstart,ph->groundringtex.yend,col);
	}

	float iAge=flashAge+flashAgeSpeed*gu->timeOffset;

	if(iAge<1){
		if(iAge<0.091f)
			iAlpha=flashAlpha*iAge*10.0f;
		else
			iAlpha=flashAlpha*(1.0f-iAge);

		if (iAlpha > 1.0f) iAlpha = 1.0f;
		if (iAlpha < 0.0f) iAlpha = 0.0f;

		col[3]=(unsigned char)(iAlpha*255);
		iSize=flashSize;

		float3 p1=pos+(-side1-side2)*iSize;
		float3 p2=pos+( side1-side2)*iSize;
		float3 p3=pos+( side1+side2)*iSize;
		float3 p4=pos+(-side1+side2)*iSize;

		va->AddVertexTC(p1,ph->groundflashtex.xstart,ph->groundflashtex.yend,col);
		va->AddVertexTC(p2,ph->groundflashtex.xend,ph->groundflashtex.yend,col);
		va->AddVertexTC(p3,ph->groundflashtex.xend,ph->groundflashtex.ystart,col);
		va->AddVertexTC(p4,ph->groundflashtex.xstart,ph->groundflashtex.ystart,col);
	}
}

CSimpleGroundFlash::CSimpleGroundFlash(float3 pos, AtlasedTexture texture, int ttl, int fade, float size, float sizeGrowth, float alpha, float3 col)
	: CGroundFlash(pos),
	sizeGrowth(sizeGrowth),
	size(size),
	texture(texture),
	alpha(alpha),
	ttl(ttl),
	fade(fade)
{
	alwaysVisible = true;

	for (int a=0;a<3;a++)
		color[a] = (unsigned char)(col[a]*255.0f);

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
	n1.Normalize();
	float3 n2((p4-p2).cross(p3-p2));
	n2.Normalize();

	pos += fw;

	float3 normal=n1+n2;
	normal.Normalize();
	side1=normal.cross(float3(1,0,0));
	side1.Normalize();
	side2=side1.cross(normal);
	ph->AddGroundFlash(this);
}

CSimpleGroundFlash::~CSimpleGroundFlash()
{
}

void CSimpleGroundFlash::Draw()
{
	color[3] = ttl<fade ? int(((ttl)/(float)(fade))*255) : 255;

	float3 p1=pos+(-side1-side2)*size;
	float3 p2=pos+( side1-side2)*size;
	float3 p3=pos+( side1+side2)*size;
	float3 p4=pos+(-side1+side2)*size;

	va->AddVertexTC(p1,texture.xstart,texture.ystart,color);
	va->AddVertexTC(p2,texture.xend,texture.ystart,color);
	va->AddVertexTC(p3,texture.xend,texture.yend,color);
	va->AddVertexTC(p4,texture.xstart,texture.yend,color);
}

bool CSimpleGroundFlash::Update()
{
	size+=sizeGrowth;
	return --ttl>0;
}

