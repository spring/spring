#include "StdAfx.h"
#include "mmgr.h"

#include "Game/Camera.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Rendering/Env/BaseWater.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "WakeProjectile.h"
#include "GlobalUnsynced.h"

CR_BIND_DERIVED(CWakeProjectile, CProjectile, (ZeroVector, ZeroVector, 0, 0, NULL, 0, 0, 0));

CR_REG_METADATA(CWakeProjectile,(
	CR_MEMBER(alpha),
	CR_MEMBER(alphaFalloff),
	CR_MEMBER(alphaAdd),
	CR_MEMBER(alphaAddTime),
	CR_MEMBER(size),
	CR_MEMBER(sizeExpansion),
	CR_MEMBER(rotation),
	CR_MEMBER(rotSpeed),
	CR_RESERVED(8)
	));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWakeProjectile::CWakeProjectile(const float3 pos, const float3 speed, float startSize, float sizeExpansion, CUnit* owner, float alpha, float alphaFalloff, float fadeupTime GML_PARG_C):
	CProjectile(pos, speed, owner, false, false, false GML_PARG_P),
	alpha(0),
	alphaFalloff(alphaFalloff),
	alphaAdd(alpha / fadeupTime),
	alphaAddTime((int)fadeupTime),
	size(startSize),
	sizeExpansion(sizeExpansion)
{
	this->pos.y=0;
	this->speed.y=0;
	rotation=gu->usRandFloat()*PI*2;
	rotSpeed=(gu->usRandFloat()-0.5f)*PI*2*0.01f;
	checkCol=false;
	if(water->noWakeProjectiles){
		alpha=0;
		alphaAddTime=0;
		size=0;
	}
}

CWakeProjectile::~CWakeProjectile()
{

}


void CWakeProjectile::Update()
{
	pos+=speed;
	rotation+=rotSpeed;
	alpha-=alphaFalloff;
	size+=sizeExpansion;
	drawRadius=size;

	if(alphaAddTime!=0){
		alpha+=alphaAdd;
		--alphaAddTime;
	} else if(alpha<0){
		alpha=0;
		deleteMe=true;
	}
}

void CWakeProjectile::Draw()
{
	inArray=true;
	unsigned char col[4];
	col[0]=(unsigned char) (255*alpha);
	col[1]=(unsigned char) (255*alpha);
	col[2]=(unsigned char) (255*alpha);
	col[3]=(unsigned char) (255*alpha)/*-alphaFalloff*gu->timeOffset*/;

	float interSize=size+sizeExpansion*gu->timeOffset;
	float interRot=rotation+rotSpeed*gu->timeOffset;

	float3 dir1=float3(cos(interRot),0,sin(interRot))*interSize;
	float3 dir2=dir1.cross(UpVector);
	va->AddVertexTC(drawPos+dir1+dir2, ph->waketex.xstart,ph->waketex.ystart,col);
	va->AddVertexTC(drawPos+dir1-dir2, ph->waketex.xstart,ph->waketex.yend,col);
	va->AddVertexTC(drawPos-dir1-dir2, ph->waketex.xend,ph->waketex.yend,col);
	va->AddVertexTC(drawPos-dir1+dir2, ph->waketex.xend,ph->waketex.ystart,col);
}
