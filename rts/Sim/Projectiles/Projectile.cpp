/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "System/mmgr.h"

#include "Projectile.h"
#include "Game/Camera.h"
#include "Map/MapInfo.h"
#include "Rendering/Colors.h"
#include "Rendering/ProjectileDrawer.hpp"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"

CR_BIND_DERIVED(CProjectile, CExpGenSpawnable, );

CR_REG_METADATA(CProjectile,
(
	CR_MEMBER(synced),
	CR_MEMBER(weapon),
	CR_MEMBER(piece),

	CR_MEMBER(luaMoveCtrl),
	CR_MEMBER(checkCol),
	CR_MEMBER(ignoreWater),
	CR_MEMBER(deleteMe),
	CR_MEMBER(castShadow), // unsynced

	CR_MEMBER(ownerId),
	CR_MEMBER(projectileType),
	CR_MEMBER(collisionFlags),

	CR_MEMBER(quadFieldCellCoors),

	CR_MEMBER(mygravity),
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(speed),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_RESERVED(8)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
bool CProjectile::inArray = false;
CVertexArray* CProjectile::va = NULL;


CProjectile::CProjectile():
	synced(false),
	weapon(false),
	piece(false),
	luaMoveCtrl(false),
	checkCol(true),
	ignoreWater(false),
	deleteMe(false),
	castShadow(false),
	speed(ZeroVector),
	mygravity(mapInfo? mapInfo->map.gravity: 0.0f),
	ownerId(0),
	projectileType(-1U),
	collisionFlags(0)
{
	GML_GET_TICKS(lastProjUpdate);
}

CProjectile::CProjectile(const float3& pos, const float3& spd, CUnit* owner, bool isSynced, bool isWeapon, bool isPiece):
	CExpGenSpawnable(pos),
	synced(isSynced),
	weapon(isWeapon),
	piece(isPiece),
	luaMoveCtrl(false),
	checkCol(true),
	ignoreWater(false),
	deleteMe(false),
	castShadow(false),
	speed(spd),
	mygravity(mapInfo? mapInfo->map.gravity: 0.0f),
	ownerId(0),
	projectileType(-1U),
	collisionFlags(0)
{
	Init(ZeroVector, owner);
	GML_GET_TICKS(lastProjUpdate);
}

void CProjectile::Detach() {
	// SYNCED
	if (synced) {
		qf->RemoveProjectile(this);
	}
	CExpGenSpawnable::Detach();
}

CProjectile::~CProjectile() {
	// UNSYNCED
	assert(!synced || detached);
}

void CProjectile::Init(const float3& offset, CUnit* owner)
{
	if (owner != NULL) {
		//! must be set before the AddProjectile call
		ownerId = owner->id;
	}
	if (!(weapon || piece)) {
		//! we need to be able to dynacast to derived
		//! types, but this throws away too much RTTI
		ph->AddProjectile(this);
	}
	if (synced) {
		qf->AddProjectile(this);
	}

	pos += offset;

	SetRadius(1.7f);
}


void CProjectile::Update()
{
	if (!luaMoveCtrl) {
		speed.y += mygravity;
		pos += speed;
		dir = speed; dir.Normalize();
	}
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


void CProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	points.AddVertexQC(pos, color4::whiteA);
}

int CProjectile::DrawArray()
{
	int idx = 0;

	va->DrawArrayTC(GL_QUADS);

	// draw-index gets divided by 24 because each element is 
	// 12 + 4 + 4 + 4 = 24 bytes in size (pos + u + v + color)
	// for each type of "projectile"
	idx = (va->drawIndex() / 24);
	va = GetVertexArray();
	va->Initialize();
	inArray = false;

	return idx;
}
