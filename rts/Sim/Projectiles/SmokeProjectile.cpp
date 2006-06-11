// SmokeProjectile.cpp: implementation of the CSmokeProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "SmokeProjectile.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Map/Ground.h"
#include "Sim/Misc/Wind.h"
#include "mmgr.h"

CR_BIND_DERIVED(CSmokeProjectile, CProjectile);

CR_REG_METADATA(CSmokeProjectile, 
(
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(color),
		CR_MEMBER(size),
		CR_MEMBER(startSize),
		CR_MEMBER(sizeExpansion),
		CR_MEMBER(ageSpeed),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_MEMBER(age)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSmokeProjectile::CSmokeProjectile()
: color(0.5f), age(0.0f), ageSpeed(1.0f), startSize(0.0f), size(0.0f), sizeExpansion(0.0f)
{
	textureNum = 0;
	checkCol=false;
}

void CSmokeProjectile::Init(const float3& pos, CUnit *owner)
{
	textureNum=(int)(gu->usRandFloat()*12);

	if(pos.y-ground->GetApproximateHeight(pos.x,pos.z)>10)
		useAirLos=true;

	if(!owner)
		alwaysVisible=true;

	CProjectile::Init(pos, owner);
}

CSmokeProjectile::CSmokeProjectile(const float3& pos,const float3& speed,float ttl,float startSize,float sizeExpansion, CUnit* owner, float color)
: CProjectile(pos, speed, owner),
	color(color),
	age(0),
	startSize(startSize),
	size(0),
	sizeExpansion(sizeExpansion)
{
	ageSpeed=1.0/ttl;
	checkCol=false;
	castShadow=true;
	textureNum=(int)(gu->usRandFloat()*12);

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
	pos+=wind.curWind*age*0.05;
	age+=ageSpeed;
	size+=sizeExpansion;
	if(size<startSize)
		size+=(startSize-size)*0.2;
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
	int frame=textureNum;
	float xmod=0.125+(float(int(frame%6)))/16;
	float ymod=(int(frame/6))/16.0;

	const float3 interPos(pos+speed*gu->timeOffset);
	const float interSize=size+sizeExpansion*gu->timeOffset;
	const float3 pos1 ((camera->right - camera->up) * interSize);
	const float3 pos2 ((camera->right + camera->up) * interSize);
	va->AddVertexTC(interPos-pos2,xmod,ymod,col);
	va->AddVertexTC(interPos+pos1,xmod+1.0/16,ymod,col);
	va->AddVertexTC(interPos+pos2,xmod+1.0/16,ymod+1.0/16,col);
	va->AddVertexTC(interPos-pos1,xmod,ymod+1.0/16,col);
}
