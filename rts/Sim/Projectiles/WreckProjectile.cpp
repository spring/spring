// WreckProjectile.cpp: implementation of the CWreckProjectile class.
//
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4786)

#include "System/StdAfx.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Game/Camera.h"
#include "WreckProjectile.h"
#include "SmokeProjectile.h"
#include "ProjectileHandler.h"
#include "Sim/Map/Ground.h"
//#include "System/mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWreckProjectile::CWreckProjectile(float3 pos,float3 speed,float temperature,CUnit* owner)
: CProjectile(pos,speed,owner)
{
	checkCol=false;
	drawRadius=2;
}

CWreckProjectile::~CWreckProjectile()
{

}

void CWreckProjectile::Update()
{
	speed.y+=gs->gravity;
	speed.x*=0.994f;
	speed.z*=0.994f;
	if(speed.y>0)
		speed.y*=0.998f;
	pos+=speed;

	if(!(gs->frameNum%3)){
		CSmokeProjectile* hp=new CSmokeProjectile(pos,ZeroVector,50,4,0.3,owner,0.5);
		hp->size+=0.1;
	}
	if(pos.y+0.3<ground->GetApproximateHeight(pos.x,pos.z))
		deleteMe=true;
}

void CWreckProjectile::Draw(void)
{
	inArray=true;
	unsigned char col[4];
	col[0]=(unsigned char) (0.15*200);
	col[1]=(unsigned char) (0.1*200);
	col[2]=(unsigned char) (0.05*200);
	col[3]=200;

	float3 interPos=pos+speed*gu->timeOffset;
	va->AddVertexTC(interPos-camera->right*drawRadius-camera->up*drawRadius,0,0,col);
	va->AddVertexTC(interPos+camera->right*drawRadius-camera->up*drawRadius,0.125,0,col);
	va->AddVertexTC(interPos+camera->right*drawRadius+camera->up*drawRadius,0.125,0.125,col);
	va->AddVertexTC(interPos-camera->right*drawRadius+camera->up*drawRadius,0.0,0.125,col);
}
