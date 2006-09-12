#include "StdAfx.h"
#include "BubbleProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "mmgr.h"
#include "ProjectileHandler.h"

CBubbleProjectile::CBubbleProjectile(float3 pos,float3 speed,float ttl,float startSize,float sizeExpansion, CUnit* owner, float alpha)
: CProjectile(pos,speed,owner),
	alpha(alpha),
	startSize(startSize),
	size(startSize*0.4f),
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
		size+=(startSize-size)*0.2f;
	drawRadius=size;

	if(pos.y>-size*0.7f){
		pos.y=-size*0.7f;
		alpha-=0.03f;
	}
	if(ttl<0){
		alpha-=0.03f;
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
	va->AddVertexTC(interPos-camera->right*interSize-camera->up*interSize,ph->circularthingytex.xstart    ,ph->circularthingytex.ystart    ,col);
	va->AddVertexTC(interPos+camera->right*interSize-camera->up*interSize,ph->circularthingytex.xend,ph->circularthingytex.ystart    ,col);
	va->AddVertexTC(interPos+camera->right*interSize+camera->up*interSize,ph->circularthingytex.xend,ph->circularthingytex.yend,col);
	va->AddVertexTC(interPos-camera->right*interSize+camera->up*interSize,ph->circularthingytex.xstart    ,ph->circularthingytex.yend,col);
}
