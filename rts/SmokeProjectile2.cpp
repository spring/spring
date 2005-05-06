// SmokeProjectile.cpp: implementation of the CSmokeProjectile2 class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "SmokeProjectile2.h"
#include "myGL.h"	

#include "ProjectileHandler.h"
#include "Camera.h"
#include "VertexArray.h"
#include "Ground.h"
#include "Wind.h"
//#include "mmgr.h"
#include "Syncify.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSmokeProjectile2::CSmokeProjectile2(float3 pos,float3 wantedPos,float3 speed,float ttl,float startSize,float sizeExpansion, CUnit* owner, float color)
: CProjectile(pos,speed,owner),
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
	glowFalloff=4.5+gu->usRandFloat()*6;
	textureNum=gu->usRandFloat()*12;
	POP_CODE_MODE;
}

CSmokeProjectile2::~CSmokeProjectile2()
{

}

void CSmokeProjectile2::Update()
{
	wantedPos+=speed;
	wantedPos += wind.curWind*age*0.05;

	pos.x+=(wantedPos.x-pos.x)*0.07;
	pos.y+=(wantedPos.y-pos.y)*0.02;
	pos.z+=(wantedPos.z-pos.z)*0.07;
	age+=ageSpeed;
	size+=sizeExpansion;
	if(size<startSize)
		size+=(startSize-size)*0.2;
	SetRadius(size);
	if(age>1){
		age=1;
		deleteMe=true;
	}
}

void CSmokeProjectile2::Draw()
{
	inArray=true;
	unsigned char col[4];
	unsigned char alpha=(unsigned char)(1-age)*127;
	float rglow=max((float)0,(1-age*glowFalloff)*127);
	float gglow=max((float)0,(1-age*glowFalloff*(float)2.5)*127);
	col[0]=color*alpha+rglow;
	col[1]=color*alpha+gglow;
	col[2]=max(0.,color*alpha-gglow*0.5);
	col[3]=alpha/*-alphaFalloff*gu->timeOffset*/;
	int frame=textureNum;
	float xmod=0.125+(float(int(frame%6)))/16.0;
	float ymod=(int(frame/6))/16.0;

	float3 interPos=pos+(wantedPos+speed*gu->timeOffset-pos)*0.1*gu->timeOffset;
	float dsize=size+sizeExpansion*gu->timeOffset;
	va->AddVertexTC(interPos-camera->right*dsize-camera->up*dsize,xmod,ymod,col);
	va->AddVertexTC(interPos+camera->right*dsize-camera->up*dsize,xmod+1.0/16,ymod,col);
	va->AddVertexTC(interPos+camera->right*dsize+camera->up*dsize,xmod+1.0/16,ymod+1.0/16,col);
	va->AddVertexTC(interPos-camera->right*dsize+camera->up*dsize,xmod,ymod+1.0/16,col);
}
