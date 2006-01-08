#include "StdAfx.h"
#include "BubbleProjectile.h"
#include "Rendering/GL/myGL.h"	
#include "ProjectileHandler.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Map/Ground.h"
#include "Sim/Misc/Wind.h"
#include "mmgr.h"

CBubbleProjectile::CBubbleProjectile(float3 pos,float3 speed,float ttl,float startSize,float sizeExpansion, CUnit* owner, float alpha)
: CProjectile(pos,speed,owner),
	alpha(alpha),
	startSize(startSize),
	size(startSize*0.4),
	sizeExpansion(sizeExpansion),
	ttl((int)ttl)
{
	checkCol=false;
}

CBubbleProjectile::~CBubbleProjectile()
{

}


void CBubbleProjectile::Update()
{
	pos+=speed;
	--ttl;
	size+=sizeExpansion;
	if(size<startSize)
		size+=(startSize-size)*0.2;
	drawRadius=size;

	if(pos.y>-size*0.7){
		pos.y=-size*0.7;
		alpha-=0.03;
	}
	if(ttl<0){
		alpha-=0.03;
	}
	if(alpha<0){
		alpha=0;
		deleteMe=true;
	}
}

void CBubbleProjectile::Draw()
{
	inArray=true;
	unsigned char col[4];
	col[0]=(unsigned char)(255*alpha);
	col[1]=(unsigned char)(255*alpha);
	col[2]=(unsigned char)(255*alpha);
	col[3]=(unsigned char)(255*alpha);

	float3 interPos=pos+speed*gu->timeOffset;
	float interSize=size+sizeExpansion*gu->timeOffset;
	va->AddVertexTC(interPos-camera->right*interSize-camera->up*interSize,0    ,0    ,col);
	va->AddVertexTC(interPos+camera->right*interSize-camera->up*interSize,1.0/8,0    ,col);
	va->AddVertexTC(interPos+camera->right*interSize+camera->up*interSize,1.0/8,1.0/8,col);
	va->AddVertexTC(interPos-camera->right*interSize+camera->up*interSize,0    ,1.0/8,col);
}
