#include "StdAfx.h"
// HeatCloudProjectile.cpp: implementation of the CHeatCloudCloudProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "HeatCloudProjectile.h"
#include "Rendering/GL/myGL.h"
#include "ProjectileHandler.h"
#include "Game/Camera.h"
#include "SyncTracer.h"
#include "Rendering/GL/VertexArray.h"
#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHeatCloudProjectile::CHeatCloudProjectile(const float3 pos,const float3 speed,const  float temperature,const float size, CUnit* owner)
: CProjectile(pos,speed,owner),
	heat(temperature),
	maxheat(temperature),
	heatFalloff(1),
	size(0)
{
	sizeGrowth=size/temperature;
	checkCol=false;
	useAirLos=true;
	SetRadius(size+sizeGrowth*heat/heatFalloff);
	sizemod=0;
	sizemodmod=0;
}

CHeatCloudProjectile::CHeatCloudProjectile(const float3 pos,const float3 speed,const float temperature,const float size, float sizegrowth, CUnit* owner)
: CProjectile(pos,speed,owner),
	heat(temperature),
	maxheat(temperature),
	heatFalloff(1),
	sizemod(1)
{
	this->sizeGrowth = sizegrowth;
	this->size = size;
	checkCol=false;
	useAirLos=true;
	SetRadius(size+sizeGrowth*heat/heatFalloff);
	sizemodmod=min(1.,max(0.,(double)(1-(1/pow(2.0f,heat*0.1f)))));
}

CHeatCloudProjectile::~CHeatCloudProjectile()
{

}

void CHeatCloudProjectile::Update()
{
//	speed.y+=GRAVITY*0.3f;
	pos+=speed;
	heat-=heatFalloff;
	if(heat<=0){
		deleteMe=true;
		heat=0;
	}
	size+=sizeGrowth;
	sizemod*=sizemodmod;
}

void CHeatCloudProjectile::Draw()
{
	inArray=true;
	unsigned char col[4];
	float dheat=heat-gu->timeOffset;
	if(dheat<0)
		dheat=0;
	float alpha=(dheat/maxheat)*255.0f;
	col[0]=(unsigned char)alpha;
	col[1]=(unsigned char)alpha;
	col[2]=(unsigned char)alpha;
	col[3]=1;//(dheat/maxheat)*255.0f;
	float drawsize=(size+sizeGrowth*gu->timeOffset)*(1-sizemod);
	float3 interPos=pos+speed*gu->timeOffset;
	va->AddVertexTC(interPos-camera->right*drawsize-camera->up*drawsize,0.25,0.25,col);
	va->AddVertexTC(interPos+camera->right*drawsize-camera->up*drawsize,0.5,0.25,col);
	va->AddVertexTC(interPos+camera->right*drawsize+camera->up*drawsize,0.5,0.5,col);
	va->AddVertexTC(interPos-camera->right*drawsize+camera->up*drawsize,0.25,0.5,col);
}
