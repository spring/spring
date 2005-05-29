#include "StdAfx.h"
// Projectile.cpp: implementation of the CProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>		// Header File For Windows
#include "myGL.h"			// Header File For The OpenGL32 Library
#include "Projectile.h"
#include "ProjectileHandler.h"
#include "Camera.h"
#include "VertexArray.h"
#include "Unit.h"
#include "InfoConsole.h"
#include "Projectile.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
bool CProjectile::inArray=false;
CVertexArray* CProjectile::va=0;

unsigned int CProjectile::textures[10];

CProjectile::CProjectile(const float3& pos,const float3& speed,CUnit* owner)
:	CWorldObject(pos),
	owner(owner),
	speed(speed),
	checkCol(true),
	isUnitPart(false),
	deleteMe(false),
	castShadow(false)
{
	SetRadius(1.7f);
	ph->AddProjectile(this);
	if(owner)
		AddDeathDependence(owner);
}

CProjectile::~CProjectile()
{
//	if(owner)
//		DeleteDeathDependence(owner);
}

void CProjectile::Update()
{
	speed.y+=gs->gravity;

	pos+=speed;
}

void CProjectile::Collision()
{
	deleteMe=true;
	checkCol=false;
	pos.y=MAX_WORLD_SIZE;
}

void CProjectile::Collision(CUnit *unit)
{
	deleteMe=true;
	checkCol=false;
	pos.y=MAX_WORLD_SIZE;
}

void CProjectile::Collision(CFeature* feature)
{
	Collision();
}

void CProjectile::Draw()
{
	inArray=true;
	unsigned char col[4];
	col[0]=1*255;
	col[1]=(unsigned char) (0.5*255);
	col[2]=0*255;
	col[3]=10;
	float3 interPos=pos+speed*gu->timeOffset;
	va->AddVertexTC(interPos-camera->right*drawRadius-camera->up*drawRadius,0,0,col);
	va->AddVertexTC(interPos+camera->right*drawRadius-camera->up*drawRadius,0.125,0,col);
	va->AddVertexTC(interPos+camera->right*drawRadius+camera->up*drawRadius,0.125,0.125,col);
	va->AddVertexTC(interPos-camera->right*drawRadius+camera->up*drawRadius,0,0.125,col);
}

void CProjectile::DrawArray()
{
	va->DrawArrayTC(GL_QUADS);
	ph->currentParticles+=va->drawIndex/24;		//each particle quad is 24 values large
	va=GetVertexArray();
	va->Initialize();
	inArray=false;
}


void CProjectile::DependentDied(CObject *o)
{
	if(o==owner)
		owner=0;
}

void CProjectile::DrawCallback(void)
{
}

void CProjectile::DrawUnitPart(void)
{
}
