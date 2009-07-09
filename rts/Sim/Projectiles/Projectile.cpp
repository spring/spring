// Projectile.cpp: implementation of the CProjectile class.
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "mmgr.h"

#include "Map/MapInfo.h"
#include "Rendering/GL/myGL.h"
#include "Projectile.h"
#include "ProjectileHandler.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Rendering/UnitModels/IModelParser.h"
#include "Rendering/Colors.h"
#include "Map/MapInfo.h"
#include "GlobalUnsynced.h"

CR_BIND_DERIVED(CProjectile, CExpGenSpawnable, );

CR_REG_METADATA(CProjectile,
(
	CR_MEMBER(checkCol),
	CR_MEMBER(castShadow),
	CR_MEMBER(ownerId),
	CR_MEMBER(synced),
//	CR_MEMBER(drawPos),
//	CR_RESERVED(4),
	CR_MEMBER(gravity),
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
	checkCol(true),
	deleteMe(false),
	castShadow(false),
	collisionFlags(0),
	s3domodel(0),
	ownerId(0),
	synced(false),
	weapon(false),
	piece(false),
	speed(ZeroVector),
	gravity(mapInfo? mapInfo->map.gravity: 0.0f)
{
	GML_GET_TICKS(lastProjUpdate);
}


void CProjectile::Init(const float3& explosionPos, CUnit* owner GML_PARG_C)
{
	if (owner) {
		ownerId = owner->id;
	}
	if (!(weapon || piece)) {
		ph->AddProjectile(this);
	}

	pos += explosionPos;
	SetRadius(1.7f);
}


CProjectile::CProjectile(const float3& pos, const float3& speed, CUnit* owner, bool isSynced, bool isWeapon, bool isPiece GML_PARG_C):
	checkCol(true),
	deleteMe(false),
	castShadow(false),
	collisionFlags(0),
	s3domodel(0),
	ownerId(0),
	CExpGenSpawnable(pos),
	synced(isSynced),
	weapon(isWeapon),
	piece(isPiece),
	speed(speed),
	gravity(mapInfo? mapInfo->map.gravity: 0.0f)
{
	if (owner) {
		ownerId = owner->id;
	}
	if (!(isWeapon || isPiece)) {
		//! we need to be able to dynacast to derived
		//! types, but this throws away too much RTTI
		ph->AddProjectile(this);
	}

	SetRadius(1.7f);

	GML_GET_TICKS(lastProjUpdate);
}

CProjectile::~CProjectile()
{
}

void CProjectile::Update()
{
	speed.y += gravity;
	pos += speed;
}


void CProjectile::Collision()
{
	deleteMe = true;
	checkCol = false;
	pos.y = MAX_WORLD_SIZE;
}

void CProjectile::Collision(CUnit* unit)
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
	inArray = true;
	unsigned char col[4];
	col[0] = 255;
	col[1] = 127;
	col[2] =   0;
	col[3] =  10;
	va->AddVertexTC(drawPos - camera->right * drawRadius - camera->up * drawRadius, ph->projectiletex.xstart, ph->projectiletex.ystart, col);
	va->AddVertexTC(drawPos + camera->right * drawRadius - camera->up * drawRadius, ph->projectiletex.xend,   ph->projectiletex.ystart, col);
	va->AddVertexTC(drawPos + camera->right * drawRadius + camera->up * drawRadius, ph->projectiletex.xend,   ph->projectiletex.yend,   col);
	va->AddVertexTC(drawPos - camera->right * drawRadius + camera->up * drawRadius, ph->projectiletex.xstart, ph->projectiletex.yend,   col);
}

void CProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	points.AddVertexQC(pos, color4::white);
}

int CProjectile::DrawArray()
{
	int idx = 0;

	va->DrawArrayTC(GL_QUADS);

	// divided by 24 because each element is 
	// 12 + 4 + 4 + 4 bytes in size (pos + u + v + color)
	// for each type of "projectile"
	idx = (va->drawIndex() / 24);
	va = GetVertexArray();
	va->Initialize();
	inArray = false;

	return idx;
}

void CProjectile::DrawCallback(void)
{
}

void CProjectile::DrawUnitPart(void)
{
}

void CProjectile::UpdateDrawPos() {
#if defined(USE_GML) && GML_ENABLE_SIM
		drawPos = pos + (speed * ((float)gu->lastFrameStart - (float)lastProjUpdate) * gu->weightedSpeedFactor);
#else
		drawPos = pos + (speed * gu->timeOffset);
#endif
}
