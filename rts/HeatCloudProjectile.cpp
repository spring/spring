#include "StdAfx.h"
// HeatCloudProjectile.cpp: implementation of the CHeatCloudCloudProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "HeatCloudProjectile.h"
#include "myGL.h"
#include "ProjectileHandler.h"
#include "Camera.h"
#include "SyncTracer.h"
#include "VertexArray.h"
//#include "mmgr.h"

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
}

void CHeatCloudProjectile::Draw()
{
	inArray=true;
	unsigned char col[4];
	float dheat=heat-gu->timeOffset;
	if(dheat<0)
		dheat=0;
	float alpha=(dheat/maxheat)*255.0f;
	col[0]=alpha;
	col[1]=alpha;
	col[2]=alpha;
	col[3]=1;//(dheat/maxheat)*255.0f;
	float drawsize=size+sizeGrowth*gu->timeOffset;
	float3 interPos=pos+speed*gu->timeOffset;
	va->AddVertexTC(interPos-camera->right*drawsize-camera->up*drawsize,0.25,0.25,col);
	va->AddVertexTC(interPos+camera->right*drawsize-camera->up*drawsize,0.5,0.25,col);
	va->AddVertexTC(interPos+camera->right*drawsize+camera->up*drawsize,0.5,0.5,col);
	va->AddVertexTC(interPos-camera->right*drawsize+camera->up*drawsize,0.25,0.5,col);
}
