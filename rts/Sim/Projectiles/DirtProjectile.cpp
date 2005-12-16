#include "StdAfx.h"
// DirtProjectile.cpp: implementation of the CDirtCloudProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "DirtProjectile.h"
#include "myGL.h"
#include "ProjectileHandler.h"
#include "Camera.h"
#include "VertexArray.h"
#include "Ground.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDirtProjectile::CDirtProjectile(const float3 pos,const float3 speed,const float ttl,const float size,const float expansion,float slowdown,CUnit* owner,const float3& color)
: CProjectile(pos,speed,owner),
	alpha(255),
	size(size),
	sizeExpansion(expansion),
	slowdown(slowdown),
	color(color)
{
	checkCol=false;
	alphaFalloff=255/ttl;
}

CDirtProjectile::~CDirtProjectile()
{

}

void CDirtProjectile::Update()
{
	speed*=slowdown;
	speed.y+=gs->gravity;
	pos+=speed;
	alpha-=alphaFalloff;
	size+=sizeExpansion;
	if(ground->GetApproximateHeight(pos.x,pos.z)-40>pos.y){
		deleteMe=true;
	}
	if(alpha<=0){
		deleteMe=true;
		alpha=0;
	}
}

void CDirtProjectile::Draw()
{
	float partAbove=(pos.y/(size*camera->up.y));
	if(partAbove<-1)
		return;
	else if(partAbove>1)
		partAbove=1;

	inArray=true;
	unsigned char col[4];
	col[0]=(unsigned char) (color.x*alpha);
	col[1]=(unsigned char) (color.y*alpha);
	col[2]=(unsigned char) (color.z*alpha);
	col[3]=(unsigned char) (alpha)/*-gu->timeOffset*alphaFalloff*/;

	float3 interPos=pos+speed*gu->timeOffset;
	float interSize=size+gu->timeOffset*sizeExpansion;
	va->AddVertexTC(interPos-camera->right*interSize-camera->up*interSize*partAbove,0.25*(1-partAbove),0.51,col);
	va->AddVertexTC(interPos+camera->right*interSize-camera->up*interSize*partAbove,0.25*(1-partAbove),1,col);
	va->AddVertexTC(interPos+camera->right*interSize+camera->up*interSize,0.5,1,col);
	va->AddVertexTC(interPos-camera->right*interSize+camera->up*interSize,0.5,0.51,col);
}
