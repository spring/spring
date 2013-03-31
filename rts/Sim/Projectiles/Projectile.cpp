/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Projectile.h"
#include "Map/MapInfo.h"
#include "Rendering/Colors.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Matrix44f.h"

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
	CR_MEMBER(castShadow),

	CR_MEMBER(lastProjUpdate),
	CR_MEMBER(dir),
	CR_MEMBER(drawPos),
	CR_IGNORED(tempdist),

	CR_MEMBER(ownerID),
	CR_MEMBER(teamID),
	CR_MEMBER(cegID),
	CR_MEMBER(projectileType),
	CR_MEMBER(collisionFlags),

	CR_MEMBER(quadFieldCellCoors),

	CR_MEMBER(mygravity),
	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(speed),
	CR_MEMBER_ENDFLAG(CM_Config),

	CR_IGNORED(quadFieldCellIter) // runtime. set in ctor
));



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
bool CProjectile::inArray = false;
CVertexArray* CProjectile::va = NULL;


CProjectile::CProjectile()
	: synced(false)
	, weapon(false)
	, piece(false)

	, luaMoveCtrl(false)
	, checkCol(true)
	, ignoreWater(false)
	, deleteMe(false)
	, castShadow(false)

	, speed(ZeroVector)
	, mygravity(mapInfo? mapInfo->map.gravity: 0.0f)

	, ownerID(-1u)
	, teamID(-1u)
	, cegID(-1u)

	, projectileType(-1u)
	, collisionFlags(0)
{
	GML::GetTicks(lastProjUpdate);
}

CProjectile::CProjectile(const float3& pos, const float3& spd, CUnit* owner, bool isSynced, bool isWeapon, bool isPiece)
	: CExpGenSpawnable(pos)

	, synced(isSynced)
	, weapon(isWeapon)
	, piece(isPiece)

	, luaMoveCtrl(false)
	, checkCol(true)
	, ignoreWater(false)
	, deleteMe(false)
	, castShadow(false)

	, speed(spd)
	, mygravity(mapInfo? mapInfo->map.gravity: 0.0f)

	, ownerID(-1u)
	, teamID(-1u)
	, cegID(-1u)

	, projectileType(-1u)
	, collisionFlags(0)
{
	Init(ZeroVector, owner);
	GML::GetTicks(lastProjUpdate);
}

void CProjectile::Detach() {
	// SYNCED
	if (synced) {
		quadField->RemoveProjectile(this);
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
		// must be set before the AddProjectile call
		ownerID = owner->id;
		teamID = owner->team;
	}
	if (!(weapon || piece)) {
		// NOTE:
		//   new CWeapon- and CPieceProjectile*'s add themselves
		//   to CProjectileHandler (other code needs to be able
		//   to dyna-cast CProjectile*'s to those derived types,
		//   and adding them here would throw away too much RTTI)
		projectileHandler->AddProjectile(this);
	}
	if (synced) {
		quadField->AddProjectile(this);
	}

	pos += offset;

	SetRadiusAndHeight(1.7f, 0.0f);
}


void CProjectile::Update()
{
	if (!luaMoveCtrl) {
		speed.y += mygravity;
		pos += speed;
		// projectiles always point directly along their speed-vector
		dir = speed;
		dir = dir.SafeNormalize();
	}
}


void CProjectile::Collision()
{
	deleteMe = true;
	checkCol = false;
}

void CProjectile::Collision(CUnit* unit)
{
	Collision();
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
	va->DrawArrayTC(GL_QUADS);

	// draw-index gets divided by 24 because each element is
	// 12 + 4 + 4 + 4 = 24 bytes in size (pos + u + v + color)
	// for each type of "projectile"
	int idx = (va->drawIndex() / 24);
	va = GetVertexArray();
	va->Initialize();
	inArray = false;

	return idx;
}

CUnit* CProjectile::owner() const {
	// Note: this death dependency optimization using "ownerID" is logically flawed,
	//  since ids are being reused it could return a unit that is not the original owner
	CUnit* unit = unitHandler->GetUnit(ownerID);

	// make volatile
	if (GML::SimEnabled())
		return (*(CUnit* volatile*) &unit);

	return unit;
}

CMatrix44f CProjectile::GetTransformMatrix(bool offsetPos) const {
	float3 xdir;
	float3 ydir;

	if (math::fabs(dir.y) < 0.95f) {
		xdir = dir.cross(UpVector);
		xdir.SafeANormalize();
	} else {
		xdir.x = 1.0f;
	}

	ydir = xdir.cross(dir);

	return (CMatrix44f(drawPos + (dir * radius * 0.9f * offsetPos), -xdir, ydir, dir));
}

