#include "stdafx.h"
#include ".\wakeprojectile.h"
#include "mygl.h"	
#include "projectilehandler.h"
#include "camera.h"
#include "vertexarray.h"
#include "ground.h"
#include "wind.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWakeProjectile::CWakeProjectile(float3 pos,float3 speed,float startSize,float sizeExpansion, CUnit* owner, float alpha,float alphaFalloff,float fadeupTime)
: CProjectile(pos,speed,owner),
	alpha(0),
	alphaAdd(alpha/fadeupTime),
	alphaAddTime(fadeupTime),
	alphaFalloff(alphaFalloff),
	size(startSize),
	sizeExpansion(sizeExpansion)
{
	this->pos.y=0;
	this->speed.y=0;
	rotation=gu->usRandFloat()*PI*2;
	rotSpeed=(gu->usRandFloat()-0.5)*PI*2*0.01;
	checkCol=false;
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
	col[0]=255*alpha;
	col[1]=255*alpha;
	col[2]=255*alpha;
	col[3]=255*alpha/*-alphaFalloff*gu->timeOffset*/;

	float3 interPos=pos+speed*gu->timeOffset;
	float interSize=size+sizeExpansion*gu->timeOffset;
	float interRot=rotation+rotSpeed*gu->timeOffset;

	float3 dir1=float3(cos(interRot),0,sin(interRot))*interSize;
	float3 dir2=dir1.cross(UpVector);
	va->AddVertexTC(interPos+dir1+dir2, 0.5,0.5,col);
	va->AddVertexTC(interPos+dir1-dir2, 0.5,0.75,col);
	va->AddVertexTC(interPos-dir1-dir2, 0.75,0.75,col);
	va->AddVertexTC(interPos-dir1+dir2, 0.75,0.5,col);
}
