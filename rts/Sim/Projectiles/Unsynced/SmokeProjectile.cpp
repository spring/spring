// SmokeProjectile.cpp: implementation of the CSmokeProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "mmgr.h"

#include "SmokeProjectile.h"

#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "GlobalUnsynced.h"

CR_BIND_DERIVED(CSmokeProjectile, CProjectile, );

CR_REG_METADATA(CSmokeProjectile,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(color),
		CR_MEMBER(size),
		CR_MEMBER(startSize),
		CR_MEMBER(sizeExpansion),
		CR_MEMBER(ageSpeed),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_MEMBER(age),
	CR_RESERVED(8)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSmokeProjectile::CSmokeProjectile()
:	CProjectile(),
	color(0.5f),
	age(0.0f),
	ageSpeed(1.0f),
	size(0.0f),
	startSize(0.0f),
	sizeExpansion(0.0f),
	textureNum(0)
{
	checkCol=false;
}

void CSmokeProjectile::Init(const float3& pos, CUnit *owner GML_PARG_C)
{
	textureNum=(int)(gu->usRandInt() % ph->smoketex.size());

	if(pos.y-ground->GetApproximateHeight(pos.x,pos.z)>10)
		useAirLos=true;

	if(!owner)
		alwaysVisible=true;

	CProjectile::Init(pos, owner GML_PARG_P);
}

CSmokeProjectile::CSmokeProjectile(const float3& pos,const float3& speed,float ttl,float startSize,float sizeExpansion, CUnit* owner, float color GML_PARG_C):
	CProjectile(pos, speed, owner, false, false GML_PARG_P),
	color(color),
	age(0),
	size(0),
	startSize(startSize),
	sizeExpansion(sizeExpansion)
{
	ageSpeed=1.0f/ttl;
	checkCol=false;
	castShadow=true;
	textureNum=(int)(gu->usRandInt() % ph->smoketex.size());

	if(pos.y-ground->GetApproximateHeight(pos.x,pos.z)>10)
		useAirLos=true;

	if (!owner)
		alwaysVisible=true;
}

CSmokeProjectile::~CSmokeProjectile()
{

}


void CSmokeProjectile::Update()
{
	pos+=speed;
	pos+=wind.GetCurrentWind()*age*0.05f;
	age+=ageSpeed;
	size+=sizeExpansion;
	if(size<startSize)
		size+=(startSize-size)*0.2f;
	drawRadius=size;
	if(age>1){
		age=1;
		deleteMe=true;
	}
}

void CSmokeProjectile::Draw()
{
	inArray=true;
	unsigned char col[4];
	unsigned char alpha=(unsigned char) ((1-age)*255);
	col[0]=(unsigned char) (color*alpha);
	col[1]=(unsigned char) (color*alpha);
	col[2]=(unsigned char) (color*alpha);
	col[3]=(unsigned char)alpha/*-alphaFalloff*gu->timeOffset*/;
	//int frame=textureNum;
	//float xmod=0.125f+(float(int(frame%6)))/16;
	//float ymod=(int(frame/6))/16.0f;

	const float interSize=size+sizeExpansion*gu->timeOffset;
	const float3 pos1 ((camera->right - camera->up) * interSize);
	const float3 pos2 ((camera->right + camera->up) * interSize);
	va->AddVertexTC(drawPos-pos2,ph->smoketex[textureNum].xstart,ph->smoketex[textureNum].ystart,col);
	va->AddVertexTC(drawPos+pos1,ph->smoketex[textureNum].xend,ph->smoketex[textureNum].ystart,col);
	va->AddVertexTC(drawPos+pos2,ph->smoketex[textureNum].xend,ph->smoketex[textureNum].yend,col);
	va->AddVertexTC(drawPos-pos1,ph->smoketex[textureNum].xstart,ph->smoketex[textureNum].yend,col);
}
