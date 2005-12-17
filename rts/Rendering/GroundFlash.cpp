#include "System/StdAfx.h"
#include "GroundFlash.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Map/Ground.h"
#include "Game/Camera.h"
#include "GL/VertexArray.h"
#include "Game/UI/InfoConsole.h"
//#include "System/mmgr.h"

unsigned int CGroundFlash::texture=0;
CVertexArray* CGroundFlash::va=0;

CGroundFlash::CGroundFlash(float3 pos,float circleAlpha,float flashAlpha,float flashSize,float circleSpeed,float ttl)
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
	this->pos.y=ground->GetHeight2(pos.x,pos.z)+1;
	float3 p1(pos.x+flashSize,0,pos.z);
	p1.y=ground->GetApproximateHeight(p1.x,p1.z);
	float3 p2(pos.x-flashSize,0,pos.z);
	p2.y=ground->GetApproximateHeight(p2.x,p2.z);
	float3 p3(pos.x,0,pos.z+flashSize);
	p3.y=ground->GetApproximateHeight(p3.x,p3.z);
	float3 p4(pos.x,0,pos.z-flashSize);
	p4.y=ground->GetApproximateHeight(p4.x,p4.z);
	float3 n1((p3-p1).cross(p4-p1));
	n1.Normalize();
	float3 n2((p4-p2).cross(p3-p2));
	n2.Normalize();

	normal=n1+n2;
	normal.Normalize();
	side1=normal.cross(float3(1,0,0));
	side1.Normalize();
	side2=side1.cross(normal);
	ph->AddGroundFlash(this);
}

CGroundFlash::~CGroundFlash(void)
{
}

void CGroundFlash::Update(void)
{
	circleSize+=circleGrowth;
	circleAlpha-=circleAlphaDec;
	flashAge+=flashAgeSpeed;
	if(--ttl<0){
		ph->RemoveGroundFlash(this);
	}
}

void CGroundFlash::Draw(void)
{
	float iAlpha=circleAlpha-circleAlphaDec*gu->timeOffset;

	unsigned char col[4];
	col[0]=255;
	col[1]=255;
	col[2]=128;
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

