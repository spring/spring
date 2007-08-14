// SmokeProjectile.cpp: implementation of the CSmokeProjectile2 class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "SmokeProjectile2.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Map/Ground.h"
#include "Sim/Misc/Wind.h"
#include "ProjectileHandler.h"
#include "mmgr.h"

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
	CR_MEMBER(textureNum)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSmokeProjectile2::CSmokeProjectile2()
: color(0.5f), age(0.0f), ageSpeed(1.0f), startSize(0.0f), size(0.0f), sizeExpansion(0.0f), glowFalloff(0.0f)
{
	textureNum = 0;
	checkCol=false;
	synced=false;
}

void CSmokeProjectile2::Init(const float3& pos, CUnit *owner)
{
	textureNum=(int)(gu->usRandFloat()*12);

	if(pos.y-ground->GetApproximateHeight(pos.x,pos.z)>10)
		useAirLos=true;

	if(!owner)
		alwaysVisible=true;

	wantedPos += pos;

	CProjectile::Init(pos, owner);
}

CSmokeProjectile2::CSmokeProjectile2(float3 pos,float3 wantedPos,float3 speed,float ttl,float startSize,float sizeExpansion, CUnit* owner, float color)
: CProjectile(pos,speed,owner, false),
	wantedPos(wantedPos),
	color(color),
	age(0),
	startSize(startSize),
	size(0),
	sizeExpansion(sizeExpansion)
{
	ageSpeed=1/ttl;
	checkCol=false;
	castShadow=true;
	if(pos.y-ground->GetApproximateHeight(pos.x,pos.z)>10)
		useAirLos=true;
	PUSH_CODE_MODE;
	ENTER_MIXED;
	glowFalloff=4.5f+gu->usRandFloat()*6;
	textureNum=(int)(gu->usRandFloat()*12);
	POP_CODE_MODE;
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
	float interAge=min(1.0f,age+ageSpeed*gu->timeOffset);
	unsigned char col[4];
	unsigned char alpha;
	if(interAge<0.05f)
		alpha=(unsigned char)(interAge*19*127);
	else
		alpha=(unsigned char)((1-interAge)*127);
	float rglow=max(0.f,(1-interAge*glowFalloff)*127);
	float gglow=max(0.f,(1-interAge*glowFalloff*2.5f)*127);
	col[0]=(unsigned char)(color*alpha+rglow);
	col[1]=(unsigned char)(color*alpha+gglow);
	col[2]=(unsigned char)max(0.f,color*alpha-gglow*0.5f);
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
