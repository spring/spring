#include "StdAfx.h"
// GfxProjectile.cpp: implementation of the CGfxProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "GfxProjectile.h"
#include "Rendering/GL/myGL.h"	

#include "ProjectileHandler.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

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
	va->AddVertexTC(interPos-camera->right*drawRadius-camera->up*drawRadius,0,0,color);
	va->AddVertexTC(interPos+camera->right*drawRadius-camera->up*drawRadius,0.125,0,color);
	va->AddVertexTC(interPos+camera->right*drawRadius+camera->up*drawRadius,0.125,0.125,color);
	va->AddVertexTC(interPos-camera->right*drawRadius+camera->up*drawRadius,0,0.125,color);
}
