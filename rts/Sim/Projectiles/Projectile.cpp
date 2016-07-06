/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Projectile.h"
#include "Map/MapInfo.h"
#include "Rendering/Colors.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ExpGenSpawnableMemberInfo.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Matrix44f.h"

CR_BIND_DERIVED_INTERFACE(CProjectile, CExpGenSpawnable)

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
	CR_IGNORED(callEvent), //we want the render event called for all projectiles

	CR_MEMBER(castShadow),
	CR_MEMBER(drawSorted),

	CR_MEMBER_BEGINFLAG(CM_Config),
		CR_MEMBER(dir),
	CR_MEMBER_ENDFLAG(CM_Config),
	CR_MEMBER(drawPos),

	CR_MEMBER(mygravity),
	CR_IGNORED(sortDist),
	CR_MEMBER(sortDistOffset),
	CR_MEMBER(tempNum),

	CR_MEMBER(ownerID),
	CR_MEMBER(teamID),
	CR_MEMBER(allyteamID),
	CR_MEMBER(cegID),

	CR_MEMBER(projectileType),
	CR_MEMBER(collisionFlags),

	CR_MEMBER(quads)
))



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
	, callEvent(true)
	, castShadow(false)
	, drawSorted(true)

	, mygravity(mapInfo? mapInfo->map.gravity: 0.0f)
	, sortDist(0.0f)
	, sortDistOffset(0.0f)
	, tempNum(0)

	, ownerID(-1u)
	, teamID(-1u)
	, allyteamID(-1)
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
	, callEvent(true)
	, castShadow(false)
	, drawSorted(true)

	, dir(ZeroVector) // set via Init()
	, mygravity(mapInfo? mapInfo->map.gravity: 0.0f)
	, sortDistOffset(0.f)

	, ownerID(-1u)
	, teamID(-1u)
	, allyteamID(-1)
	, cegID(-1u)

	, projectileType(-1u)
	, collisionFlags(0)
{
	SetRadiusAndHeight(1.7f, 0.0f);
	Init(owner, ZeroVector);
}


CProjectile::~CProjectile()
{
	if (synced) {
		quadField->RemoveProjectile(this);
#ifdef TRACE_SYNC
		tracefile << "Projectile died id: " << id << ", pos: <" << pos.x << ", " << pos.y << ", " << pos.z << ">\n";
#endif
	}
}

void CProjectile::Init(const CUnit* owner, const float3& offset)
{
	if (owner != NULL) {
		// must be set before the AddProjectile call
		ownerID = owner->id;
		teamID = owner->team;
		allyteamID =  teamHandler->IsValidTeam(teamID)? teamHandler->AllyTeam(teamID): -1;
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


void CProjectile::Delete()
{
	deleteMe = true;
	checkCol = false;
}


void CProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	points.AddVertexQC(pos, color4::whiteA);
}


void CProjectile::DrawArray()
{
	va->DrawArrayTC(GL_QUADS);
	va = GetVertexArray();
	va->Initialize();
	inArray = false;
}


CUnit* CProjectile::owner() const {
	// NOTE:
	//   this death dependency optimization using "ownerID" is logically flawed:
	//   because ID's are reused it could return a unit that is not the original
	//   owner (unlikely however unless ID's get recycled very rapidly)
	return (unitHandler->GetUnit(ownerID));
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


bool CProjectile::GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo)
{
	if (CExpGenSpawnable::GetMemberInfo(memberInfo))
		return true;

	CHECK_MEMBER_INFO_FLOAT3(CProjectile, dir)

	return false;
}
