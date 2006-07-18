#include "StdAfx.h"
// Projectile.cpp: implementation of the CProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "Rendering/GL/myGL.h"			// Header File For The OpenGL32 Library
#include "Projectile.h"
#include "ProjectileHandler.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Units/Unit.h"
#include "Game/UI/InfoConsole.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "mmgr.h"

CR_BIND_DERIVED(CProjectile, CWorldObject);

CR_REG_METADATA(CProjectile,
(
	CR_MEMBER(checkCol),
	CR_MEMBER(castShadow),
	CR_MEMBER(owner),

	CR_MEMBER_BEGINFLAG(CM_Config),
	CR_MEMBER(speed)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
bool CProjectile::inArray=false;
CVertexArray* CProjectile::va=0;

CProjectile::CProjectile()
:	owner(0),
	checkCol(true),
	deleteMe(false),
	castShadow(false),
	s3domodel(0)
{}

void CProjectile::Init (const float3& explosionPos, CUnit *owner)
{
	pos += explosionPos;
	SetRadius(1.7f);
	ph->AddProjectile(this);
	if(owner)
		AddDeathDependence(owner);
}

CProjectile::CProjectile(const float3& pos,const float3& speed,CUnit* owner)
:	CWorldObject(pos),
	owner(owner),
	speed(speed),
	checkCol(true),
	deleteMe(false),
	castShadow(false),
	s3domodel(0)
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
	va->AddVertexTC(interPos-camera->right*drawRadius-camera->up*drawRadius,ph->circularthingytex.xstart,ph->circularthingytex.ystart,col);
	va->AddVertexTC(interPos+camera->right*drawRadius-camera->up*drawRadius,ph->circularthingytex.xend,ph->circularthingytex.ystart,col);
	va->AddVertexTC(interPos+camera->right*drawRadius+camera->up*drawRadius,ph->circularthingytex.xend,ph->circularthingytex.yend,col);
	va->AddVertexTC(interPos-camera->right*drawRadius+camera->up*drawRadius,ph->circularthingytex.xstart,ph->circularthingytex.yend,col);
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
