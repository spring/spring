#include "StdAfx.h"
#include "ExploSpikeProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
//#include "mmgr.h"

CExploSpikeProjectile::CExploSpikeProjectile(const float3& pos,const float3& speed,float length,float width,float alpha,float alphaDecay,CUnit* owner)
:	CProjectile(pos,speed,owner),
	length(length),
	width(width),
	alpha(alpha),
	alphaDecay(alphaDecay),
	dir(speed)
{
	lengthGrowth=dir.Length()*(0.5f+gu->usRandFloat()*0.4f);
	dir/=lengthGrowth;

	checkCol=false;
	useAirLos=true;
	SetRadius(length+lengthGrowth*alpha/alphaDecay);
}

CExploSpikeProjectile::~CExploSpikeProjectile(void)
{
}

void CExploSpikeProjectile::Update(void)
{
	pos+=speed;
	length+=lengthGrowth;
	alpha-=alphaDecay;

	if(alpha<=0){
		alpha=0;
		deleteMe=true;
	}
}

void CExploSpikeProjectile::Draw(void)
{
	inArray=true;

	float3 dif(pos-camera->pos2);
	dif.Normalize();
	float3 dir2(dif.cross(dir));
	dir2.Normalize();

	unsigned char col[4];
	float a=std::max(0.f,alpha-alphaDecay*gu->timeOffset)*255;
	col[0]=(unsigned char)a;
	col[1]=(unsigned char)(a*0.8);
	col[2]=(unsigned char)(a*0.5);
	col[3]=1;

	float3 interpos=pos+speed*gu->timeOffset;
	float3 l=dir*length+lengthGrowth*gu->timeOffset;
	float3 w=dir2*width;

	va->AddVertexTC(interpos+l+w, 8/8.0, 1/8.0, col);
	va->AddVertexTC(interpos+l-w, 8/8.0, 0/8.0, col);
	va->AddVertexTC(interpos-l-w, 7/8.0, 0/8.0, col);
	va->AddVertexTC(interpos-l+w, 7/8.0, 1/8.0, col);
}
