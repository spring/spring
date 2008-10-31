#include "StdAfx.h"
// Projectile.cpp: implementation of the CProjectile class.
//
//////////////////////////////////////////////////////////////////////
#include "mmgr.h"

#include "Rendering/GL/myGL.h"			// Header File For The OpenGL32 Library
#include "Projectile.h"
#include "ProjectileHandler.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Units/Unit.h"
#include "LogOutput.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "Map/MapInfo.h"
#include "System/GlobalUnsynced.h"

CR_BIND_DERIVED(CProjectile, CExpGenSpawnable, );

CR_REG_METADATA(CProjectile,
(
	CR_MEMBER(checkCol),
	CR_MEMBER(castShadow),
	CR_MEMBER(owner),
	CR_MEMBER(synced),
//	CR_MEMBER(drawPos),
//	CR_RESERVED(4),
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(speed),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_RESERVED(8)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
bool CProjectile::inArray = false;
CVertexArray* CProjectile::va = 0;

CProjectile::CProjectile():
	owner(0),
	checkCol(true),
	deleteMe(false),
	castShadow(false),
	s3domodel(0),
	collisionFlags(0)
{
	GML_GET_TICKS(lastProjUpdate);
}


void CProjectile::Init(const float3& explosionPos, CUnit* owner GML_PARG_C)
{
	pos += explosionPos;
	SetRadius(1.7f);
	ph->AddProjectile(this);

	if (owner) {
		AddDeathDependence(owner);
	}
}


CProjectile::CProjectile(const float3& pos, const float3& speed, CUnit* owner, bool synced, bool weapon GML_PARG_C)
:	CExpGenSpawnable(pos),
	owner(owner),
	speed(speed),
	checkCol(true),
	deleteMe(false),
	castShadow(false),
	s3domodel(0),
	collisionFlags(0),
	synced(synced),
	weapon(weapon)
{
	SetRadius(1.7f);
	ph->AddProjectile(this);

	if (owner)
		AddDeathDependence(owner);

	GML_GET_TICKS(lastProjUpdate);
}

CProjectile::~CProjectile()
{
//	if (owner)
//		DeleteDeathDependence(owner);
}

void CProjectile::Update()
{
	speed.y += mapInfo->map.gravity;
	pos += speed;
}

void CProjectile::Collision()
{
	deleteMe = true;
	checkCol = false;
	pos.y = MAX_WORLD_SIZE;
}

void CProjectile::Collision(CUnit *unit)
{
	deleteMe = true;
	checkCol = false;
	pos.y = MAX_WORLD_SIZE;
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
	col[1]=(unsigned char) (0.5f*255);
	col[2]=0*255;
	col[3]=10;
	va->AddVertexTC(drawPos-camera->right*drawRadius-camera->up*drawRadius,ph->projectiletex.xstart,ph->projectiletex.ystart,col);
	va->AddVertexTC(drawPos+camera->right*drawRadius-camera->up*drawRadius,ph->projectiletex.xend,ph->projectiletex.ystart,col);
	va->AddVertexTC(drawPos+camera->right*drawRadius+camera->up*drawRadius,ph->projectiletex.xend,ph->projectiletex.yend,col);
	va->AddVertexTC(drawPos-camera->right*drawRadius+camera->up*drawRadius,ph->projectiletex.xstart,ph->projectiletex.yend,col);
}

void CProjectile::DrawArray()
{
	va->DrawArrayTC(GL_QUADS);
	ph->currentParticles+=va->drawIndex()/24;		//each particle quad is 24 values large
	va=GetVertexArray();
	va->Initialize();
	inArray=false;
}


void CProjectile::DependentDied(CObject* o)
{
	if (o == owner)
		owner = 0;
}

void CProjectile::DrawCallback(void)
{
}

void CProjectile::DrawUnitPart(void)
{
}

void CProjectile::UpdateDrawPos() {
#if defined(USE_GML) && GML_ENABLE_SIMDRAW
		drawPos = pos + (speed * ((float)gu->lastFrameStart - (float)lastProjUpdate) * gu->weightedSpeedFactor);
#else
		drawPos = pos + (speed * gu->timeOffset);
#endif
}
