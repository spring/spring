#include "stdafx.h"
#include "groundflash.h"
#include "projectilehandler.h"
#include "ground.h"
#include "camera.h"
#include "vertexarray.h"
#include "infoconsole.h"
//#include "mmgr.h"

unsigned int CGroundFlash::texture=0;
CVertexArray* CGroundFlash::va=0;

CGroundFlash::CGroundFlash(float3 pos,float circleAlpha,float flashAlpha,float flashSize,float circleSpeed,float ttl)
: circleAlpha(circleAlpha),
	flashAlpha(flashAlpha),
	flashSize(flashSize),
	circleGrowth(circleSpeed),
	circleSize(circleSpeed),
	ttl(ttl),
	circleAlphaDec(circleAlpha/ttl),
	flashAlphaDec(flashAlpha/ttl),
	pos(pos)
{
	pos.y=ground->GetHeight(pos.x,pos.z);
	normal=ground->GetNormal(pos.x,pos.z);
	side1=normal.cross(float3(1,0,0));
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
	flashAlpha-=flashAlphaDec;
	if(--ttl<0){
		ph->RemoveGroundFlash(this);
	}
}

void CGroundFlash::Draw(void)
{
	float3 camVect=camera->pos-pos;
	float camLength=camVect.Length();
	camVect/=camLength;
	float moveLength=50;
	if(camLength<60)
		moveLength=camLength-50;
	float3 modPos=pos+camVect*moveLength;

	float iAlpha=circleAlpha-flashAlphaDec*gu->timeOffset;

	unsigned char col[4];
	col[0]=255;
	col[1]=255;
	col[2]=128;
	col[3]=iAlpha*255;

	float iSize=circleSize+circleGrowth*gu->timeOffset;
	iSize*=1.0-(moveLength/(camLength));

	if(iAlpha>0){
		va->AddVertexTC(modPos-side1*iSize-side2*iSize,0,0,col);
		va->AddVertexTC(modPos+side1*iSize-side2*iSize,1,0,col);
		va->AddVertexTC(modPos+side1*iSize+side2*iSize,1,0.5,col);
		va->AddVertexTC(modPos-side1*iSize+side2*iSize,0,0.5,col);
	}
	iAlpha=flashAlpha-flashAlphaDec*gu->timeOffset;
	col[3]=iAlpha*255;
	iSize=flashSize*(1.0-(moveLength/(camLength)));

	if(iAlpha>0){
		va->AddVertexTC(modPos+(-side1-side2)*iSize,0,1,col);
		va->AddVertexTC(modPos+( side1-side2)*iSize,1,1,col);
		va->AddVertexTC(modPos+( side1+side2)*iSize,1,0.5,col);
		va->AddVertexTC(modPos+(-side1+side2)*iSize,0,0.5,col);
	}
}

