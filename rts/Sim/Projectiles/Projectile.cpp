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
	CR_MEMBER(hitscan),

	CR_MEMBER(luaMoveCtrl),
	CR_MEMBER(checkCol),
	CR_MEMBER(ignoreWater),
	CR_MEMBER(deleteMe),
	CR_MEMBER(castShadow),

	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(dir),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_MEMBER(drawPos),

	CR_MEMBER(mygravity),
	CR_IGNORED(tempdist),

	CR_MEMBER(ownerID),
	CR_MEMBER(teamID),
	CR_MEMBER(cegID),

	CR_MEMBER(projectileType),
	CR_MEMBER(collisionFlags),

	CR_MEMBER(qfCellData)
));

CR_BIND(CProjectile::QuadFieldCellData, )



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
bool CProjectile::inArray = false;
CVertexArray* CProjectile::va = NULL;


CProjectile::CProjectile()
	: synced(false)
	, weapon(false)
	, piece(false)
	, hitscan(false)

	, luaMoveCtrl(false)
	, checkCol(true)
	, ignoreWater(false)
	, deleteMe(false)
	, castShadow(false)

	, mygravity(mapInfo? mapInfo->map.gravity: 0.0f)

	, ownerID(-1u)
	, teamID(-1u)
	, cegID(-1u)

	, projectileType(-1u)
	, collisionFlags(0)
{
}

CProjectile::CProjectile(
	const float3& pos,
	const float3& spd,
	const CUnit* owner,
	bool isSynced,
	bool isWeapon,
	bool isPiece,
	bool isHitScan
): CExpGenSpawnable(pos, spd)

	, synced(isSynced)
	, weapon(isWeapon)
	, piece(isPiece)
	, hitscan(isHitScan)

	, luaMoveCtrl(false)
	, checkCol(true)
	, ignoreWater(false)
	, deleteMe(false)
	, castShadow(false)

	, dir(ZeroVector) // set via Init()
	, mygravity(mapInfo? mapInfo->map.gravity: 0.0f)

	, ownerID(-1u)
	, teamID(-1u)
	, cegID(-1u)

	, projectileType(-1u)
	, collisionFlags(0)
{
	SetRadiusAndHeight(1.7f, 0.0f);
	Init(owner, ZeroVector);
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

void CProjectile::Init(const CUnit* owner, const float3& offset)
{
	if (owner != NULL) {
		// must be set before the AddProjectile call
		ownerID = owner->id;
		teamID = owner->team;
	}
	if (!hitscan) {
		SetPosition(pos + offset);
		SetVelocityAndSpeed(speed);
	}
	if (!weapon && !piece) {
		// NOTE:
		//   new CWeapon- and CPieceProjectile*'s add themselves
		//   to CProjectileHandler (other code needs to be able
		//   to dyna-cast CProjectile*'s to those derived types,
		//   and adding them here would throw away too much RTTI)
		projectileHandler->AddProjectile(this);
	}
	if (synced && !weapon) {
		quadField->AddProjectile(this);
	}
}


void CProjectile::Update()
{
	if (luaMoveCtrl)
		return;

	SetPosition(pos + speed);
	SetVelocityAndSpeed(speed + (UpVector * mygravity));
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
	const int idx = (va->drawIndex() / 24);

	va = GetVertexArray();
	va->Initialize();
	inArray = false;

	return idx;
}

CUnit* CProjectile::owner() const {
	// NOTE:
	//   this death dependency optimization using "ownerID" is logically flawed:
	//   because ID's are reused it could return a unit that is not the original
	//   owner (unlikely however unless ID's get recycled very rapidly)
	CUnit* unit = unitHandler->GetUnit(ownerID);

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

