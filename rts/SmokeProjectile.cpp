// SmokeProjectile.cpp: implementation of the CSmokeProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "SmokeProjectile.h"
#include "myGL.h"	
#include "ProjectileHandler.h"
#include "Camera.h"
#include "VertexArray.h"
#include "Ground.h"
#include "Wind.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSmokeProjectile::CSmokeProjectile(float3 pos,float3 speed,float ttl,float startSize,float sizeExpansion, CUnit* owner, float color)
: CProjectile(pos,speed,owner),
	color(color),
	age(0),
	startSize(startSize),
	size(0),
	sizeExpansion(sizeExpansion)
{
	ageSpeed=1.0/ttl;
	checkCol=false;
	castShadow=true;

	PUSH_CODE_MODE;
	ENTER_MIXED;
	textureNum=gu->usRandFloat()*12;
	POP_CODE_MODE;

	if(pos.y-ground->GetApproximateHeight(pos.x,pos.z)>10)
		useAirLos=true;

	if(!owner)
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
	unsigned char alpha=(1-age)*255;
	col[0]=color*alpha;
	col[1]=color*alpha;
	col[2]=color*alpha;
	col[3]=alpha/*-alphaFalloff*gu->timeOffset*/;
	int frame=textureNum;
	float xmod=0.125+(float(int(frame%6)))/16;
	float ymod=(int(frame/6))/16.0;

	float3 interPos=pos+speed*gu->timeOffset;
	float interSize=size+sizeExpansion*gu->timeOffset;
	va->AddVertexTC(interPos-camera->right*interSize-camera->up*interSize,xmod,ymod,col);
	va->AddVertexTC(interPos+camera->right*interSize-camera->up*interSize,xmod+1.0/16,ymod,col);
	va->AddVertexTC(interPos+camera->right*interSize+camera->up*interSize,xmod+1.0/16,ymod+1.0/16,col);
	va->AddVertexTC(interPos-camera->right*interSize+camera->up*interSize,xmod,ymod+1.0/16,col);
}
