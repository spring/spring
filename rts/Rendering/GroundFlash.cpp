#include "StdAfx.h"
#include "GroundFlash.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Map/Ground.h"
#include "Game/Camera.h"
#include "GL/VertexArray.h"
#include "Game/UI/InfoConsole.h"
#include "mmgr.h"

unsigned int CGroundFlash::texture=0;
CVertexArray* CGroundFlash::va=0;

CGroundFlash::CGroundFlash(float3 pos,float circleAlpha,float flashAlpha,float flashSize,float circleSpeed,float ttl, float3 col)
: circleAlpha(circleAlpha),
	flashAlpha(flashAlpha),
	flashSize(flashSize),
	circleGrowth(circleSpeed),
	circleSize(circleSpeed),
	flashAge(0),
	ttl((int)ttl),
	circleAlphaDec(circleAlpha/ttl),
	flashAgeSpeed(1.0f/ttl),
	pos(pos)
{
	for (int a=0;a<3;a++)
		color[a] = (unsigned char)col[a]*255.0f;

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

CGroundFlash::~CGroundFlash()
{
}

bool CGroundFlash::Update()
{
	circleSize+=circleGrowth;
	circleAlpha-=circleAlphaDec;
	flashAge+=flashAgeSpeed;
	return --ttl>0;
}

void CGroundFlash::Draw()
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

		va->AddVertexTC(p1,0,0,col);
		va->AddVertexTC(p2,1,0,col);
		va->AddVertexTC(p3,1,0.5,col);
		va->AddVertexTC(p4,0,0.5,col);
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

		va->AddVertexTC(p1,0,1,col);
		va->AddVertexTC(p2,1,1,col);
		va->AddVertexTC(p3,1,0.5,col);
		va->AddVertexTC(p4,0,0.5,col);
	}
}

