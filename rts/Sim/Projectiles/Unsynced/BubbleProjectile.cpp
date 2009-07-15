#include "StdAfx.h"
#include "mmgr.h"

#include "BubbleProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "GlobalUnsynced.h"

CR_BIND_DERIVED(CBubbleProjectile, CProjectile, (float3(0,0,0),float3(0,0,0),0,0,0,NULL,0));

CR_REG_METADATA(CBubbleProjectile, (
	CR_MEMBER(ttl),
	CR_MEMBER(alpha),
	CR_MEMBER(size),
	CR_MEMBER(startSize),
	CR_MEMBER(sizeExpansion),
	CR_RESERVED(8)
	));


CBubbleProjectile::CBubbleProjectile(float3 pos,float3 speed, float ttl, float startSize, float sizeExpansion, CUnit* owner, float alpha GML_PARG_C):
	CProjectile(pos, speed, owner, false, false, false GML_PARG_P),
	ttl((int) ttl),
	alpha(alpha),
	size(startSize * 0.4f),
	startSize(startSize),
	sizeExpansion(sizeExpansion)
{
	checkCol = false;
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

	float interSize=size+sizeExpansion*gu->timeOffset;
	va->AddVertexTC(drawPos-camera->right*interSize-camera->up*interSize,ph->bubbletex.xstart    ,ph->bubbletex.ystart    ,col);
	va->AddVertexTC(drawPos+camera->right*interSize-camera->up*interSize,ph->bubbletex.xend,ph->bubbletex.ystart    ,col);
	va->AddVertexTC(drawPos+camera->right*interSize+camera->up*interSize,ph->bubbletex.xend,ph->bubbletex.yend,col);
	va->AddVertexTC(drawPos-camera->right*interSize+camera->up*interSize,ph->bubbletex.xstart    ,ph->bubbletex.yend,col);
}
