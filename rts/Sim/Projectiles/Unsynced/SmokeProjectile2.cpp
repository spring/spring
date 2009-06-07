// SmokeProjectile.cpp: implementation of the CSmokeProjectile2 class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "mmgr.h"

#include "SmokeProjectile2.h"

#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "GlobalUnsynced.h"

CR_BIND_DERIVED(CSmokeProjectile2, CProjectile, );

CR_REG_METADATA(CSmokeProjectile2,
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(color),
		CR_MEMBER(ageSpeed),
		CR_MEMBER(size),
		CR_MEMBER(startSize),
		CR_MEMBER(sizeExpansion),
		CR_MEMBER(wantedPos),
		CR_MEMBER(glowFalloff),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_MEMBER(age),
	CR_MEMBER(textureNum),
	CR_RESERVED(8)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSmokeProjectile2::CSmokeProjectile2() :
	CProjectile(),
	color(0.5f),
	age(0.0f),
	ageSpeed(1.0f),
	size(0.0f),
	startSize(0.0f),
	sizeExpansion(0.0f),
	textureNum(0),
	glowFalloff(0.0f)
{
	deleteMe=false;
	checkCol=false;
	synced=false;
}

void CSmokeProjectile2::Init(const float3& pos, CUnit *owner GML_PARG_C)
{
	textureNum=(int)(gu->usRandInt() % ph->smoketex.size());

	if(pos.y-ground->GetApproximateHeight(pos.x,pos.z)>10)
		useAirLos=true;

	if(!owner)
		alwaysVisible=true;

	wantedPos += pos;

	CProjectile::Init(pos, owner GML_PARG_P);
}

CSmokeProjectile2::CSmokeProjectile2(float3 pos,float3 wantedPos,float3 speed,float ttl,float startSize,float sizeExpansion, CUnit* owner, float color GML_PARG_C)
: CProjectile(pos,speed,owner, false, false GML_PARG_P),
	color(color),
	age(0),
	size(0),
	startSize(startSize),
	sizeExpansion(sizeExpansion),
	wantedPos(wantedPos)
{
	ageSpeed=1/ttl;
	checkCol=false;
	castShadow=true;
	if(pos.y-ground->GetApproximateHeight(pos.x,pos.z)>10)
		useAirLos=true;
	glowFalloff=4.5f+gu->usRandFloat()*6;
	textureNum=(int)(gu->usRandInt() % ph->smoketex.size());
}

CSmokeProjectile2::~CSmokeProjectile2()
{

}

void CSmokeProjectile2::Update()
{
	wantedPos+=speed;
	wantedPos += wind.GetCurrentWind()*age*0.05f;

	pos.x+=(wantedPos.x-pos.x)*0.07f;
	pos.y+=(wantedPos.y-pos.y)*0.02f;
	pos.z+=(wantedPos.z-pos.z)*0.07f;
	age+=ageSpeed;
	size+=sizeExpansion;
	if(size<startSize)
		size+=(startSize-size)*0.2f;
	SetRadius(size);
	if(age>1){
		age=1;
		deleteMe=true;
	}
}

void CSmokeProjectile2::Draw()
{
	inArray=true;
	float interAge=std::min(1.0f,age+ageSpeed*gu->timeOffset);
	unsigned char col[4];
	unsigned char alpha;
	if(interAge<0.05f)
		alpha=(unsigned char)(interAge*19*127);
	else
		alpha=(unsigned char)((1-interAge)*127);
	float rglow=std::max(0.f,(1-interAge*glowFalloff)*127);
	float gglow=std::max(0.f,(1-interAge*glowFalloff*2.5f)*127);
	col[0]=(unsigned char)(color*alpha+rglow);
	col[1]=(unsigned char)(color*alpha+gglow);
	col[2]=(unsigned char)std::max(0.f,color*alpha-gglow*0.5f);
	col[3]=alpha/*-alphaFalloff*gu->timeOffset*/;
	//int frame=textureNum;
	//float xmod=0.125f+(float(int(frame%6)))/16.0f;
	//float ymod=(int(frame/6))/16.0f;
	//int smokenum = frame%12;

	float3 interPos=pos+(wantedPos+speed*gu->timeOffset-pos)*0.1f*gu->timeOffset;
	const float interSize=size+sizeExpansion*gu->timeOffset;
	const float3 pos1 ((camera->right - camera->up) * interSize);
	const float3 pos2 ((camera->right + camera->up) * interSize);
	va->AddVertexTC(interPos-pos2,ph->smoketex[textureNum].xstart,ph->smoketex[textureNum].ystart,col);
	va->AddVertexTC(interPos+pos1,ph->smoketex[textureNum].xend,ph->smoketex[textureNum].ystart,col);
	va->AddVertexTC(interPos+pos2,ph->smoketex[textureNum].xend,ph->smoketex[textureNum].yend,col);
	va->AddVertexTC(interPos-pos1,ph->smoketex[textureNum].xstart,ph->smoketex[textureNum].yend,col);
}
