#include "StdAfx.h"
// HeatCloudProjectile.cpp: implementation of the CHeatCloudCloudProjectile class.
//
//////////////////////////////////////////////////////////////////////
#include "mmgr.h"

#include "Game/Camera.h"
#include "HeatCloudProjectile.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "System/GlobalUnsynced.h"

CR_BIND_DERIVED(CHeatCloudProjectile, CProjectile, );

CR_REG_METADATA(CHeatCloudProjectile,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(heat),
		CR_MEMBER(maxheat),
		CR_MEMBER(heatFalloff),
		CR_MEMBER(size),
		CR_MEMBER(sizeGrowth),
		CR_MEMBER(sizemod),
		CR_MEMBER(sizemodmod),
		CR_MEMBER(texture),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_RESERVED(8)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHeatCloudProjectile::CHeatCloudProjectile()
{
	heat=maxheat=heatFalloff=size=sizeGrowth=sizemod=sizemodmod=0.0f;
	checkCol=false;
	useAirLos=true;
	synced=false;
	texture = &ph->heatcloudtex;
}

CHeatCloudProjectile::CHeatCloudProjectile(const float3 pos,const float3 speed,const  float temperature,const float size, CUnit* owner)
: CProjectile(pos,speed,owner, false),
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
	texture = &ph->heatcloudtex;
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
	va->AddVertexTC(interPos-camera->right*drawsize-camera->up*drawsize,texture->xstart,texture->ystart,col);
	va->AddVertexTC(interPos+camera->right*drawsize-camera->up*drawsize,texture->xend,texture->ystart,col);
	va->AddVertexTC(interPos+camera->right*drawsize+camera->up*drawsize,texture->xend,texture->yend,col);
	va->AddVertexTC(interPos-camera->right*drawsize+camera->up*drawsize,texture->xstart,texture->yend,col);
}
