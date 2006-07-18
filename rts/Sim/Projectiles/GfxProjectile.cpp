#include "StdAfx.h"
// GfxProjectile.cpp: implementation of the CGfxProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "GfxProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "mmgr.h"
#include "ProjectileHandler.h"

CR_BIND_DERIVED(CGfxProjectile, CProjectile);

CR_REG_METADATA(CGfxProjectile, 
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(creationTime),
		CR_MEMBER(lifeTime),
		CR_MEMBER(color),
	CR_MEMBER_ENDFLAG(CM_Config)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGfxProjectile::CGfxProjectile()
{
	creationTime=lifeTime=0;
	color[0]=color[1]=color[2]=color[3]=255;
	checkCol=false;
}

CGfxProjectile::CGfxProjectile(const float3& pos,const float3& speed,int lifeTime,const float3& color)
: CProjectile(pos,speed,0),
	lifeTime(lifeTime),
	creationTime(gs->frameNum)
{
	checkCol=false;
	this->color[0]=(unsigned char) (color[0]*255);
	this->color[1]=(unsigned char) (color[1]*255);
	this->color[2]=(unsigned char) (color[2]*255);
	this->color[3]=20;
	drawRadius=3;
}

CGfxProjectile::~CGfxProjectile()
{

}


void CGfxProjectile::Update()
{
	pos+=speed;
	if(gs->frameNum>=creationTime+lifeTime)
		deleteMe=true;
}

void CGfxProjectile::Draw()
{
	inArray=true;

	float3 interPos=pos+speed*gu->timeOffset;
	va->AddVertexTC(interPos-camera->right*drawRadius-camera->up*drawRadius,ph->circularthingytex.xstart,ph->circularthingytex.ystart,color);
	va->AddVertexTC(interPos+camera->right*drawRadius-camera->up*drawRadius,ph->circularthingytex.xend,ph->circularthingytex.ystart,color);
	va->AddVertexTC(interPos+camera->right*drawRadius+camera->up*drawRadius,ph->circularthingytex.xend,ph->circularthingytex.yend,color);
	va->AddVertexTC(interPos-camera->right*drawRadius+camera->up*drawRadius,ph->circularthingytex.xstart,ph->circularthingytex.yend,color);
}
