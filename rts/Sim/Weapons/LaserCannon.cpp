/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "Game/TraceRay.h"
#include "LaserCannon.h"
#include "Map/Ground.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/LaserProjectile.h"
#include "Sim/Units/Unit.h"
#include "WeaponDefHandler.h"
#include "System/mmgr.h"

CR_BIND_DERIVED(CLaserCannon, CWeapon, (NULL));

CR_REG_METADATA(CLaserCannon,(
	CR_MEMBER(color),
	CR_RESERVED(8)
	));

CLaserCannon::CLaserCannon(CUnit* owner)
: CWeapon(owner)
{
}

CLaserCannon::~CLaserCannon(void)
{
}

void CLaserCannon::Update(void)
{
	if(targetType != Target_None){
		weaponPos = owner->pos +
			owner->frontdir * relWeaponPos.z +
			owner->updir    * relWeaponPos.y +
			owner->rightdir * relWeaponPos.x;
		weaponMuzzlePos = owner->pos +
			owner->frontdir * relWeaponMuzzlePos.z +
			owner->updir    * relWeaponMuzzlePos.y +
			owner->rightdir * relWeaponMuzzlePos.x;

		float3 wantedDirTemp(targetPos - weaponPos);
		float len = wantedDirTemp.Length();
		if(!onlyForward && (len != 0.0f)) {
			wantedDir = wantedDirTemp;
			wantedDir /= len;
		}
		predict=len/projectileSpeed;
	}
	CWeapon::Update();
}

bool CLaserCannon::TryTarget(const float3& pos, bool userTarget, CUnit* unit)
{
	if (!CWeapon::TryTarget(pos, userTarget, unit))
		return false;

	if (unit) {
		if (unit->isUnderWater && !weaponDef->waterweapon)
			return false;
	} else {
		if (pos.y < 0 && !weaponDef->waterweapon)
			return false;
	}

	float3 dir(pos - weaponMuzzlePos);
	const float length = dir.Length();
	if (length == 0)
		return true;

	dir /= length;

	if (!onlyForward) {
		if (!HaveFreeLineOfFire(weaponMuzzlePos, dir, length)) {
			return false;
		}
	}

	const float spread =
		(accuracy + sprayAngle) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight);

	if (avoidFeature && TraceRay::LineFeatureCol(weaponMuzzlePos, dir, length)) {
		return false;
	}
	if (avoidFriendly && TraceRay::TestAllyCone(weaponMuzzlePos, dir, length, spread, owner->allyteam, owner)) {
		return false;
	}
	if (avoidNeutral && TraceRay::TestNeutralCone(weaponMuzzlePos, dir, length, spread, owner)) {
		return false;
	}

	return true;
}

void CLaserCannon::Init(void)
{
	CWeapon::Init();
}

void CLaserCannon::FireImpl()
{
	float3 dir;
	if (onlyForward && dynamic_cast<CAirMoveType*>(owner->moveType)) {
		// the taairmovetype cant align itself properly, change back when that is fixed
		dir = owner->frontdir;
	} else {
		dir = targetPos - weaponMuzzlePos;
		dir.Normalize();
	}

	dir +=
		(gs->randVector() * sprayAngle + salvoError) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight);
	dir.Normalize();

	int fpsSub = 0;

	if (owner->fpsControlPlayer != NULL) {
		// subtract 24 elmos in FPS mode (?)
		fpsSub = 24;
	}

	new CLaserProjectile(weaponMuzzlePos, dir * projectileSpeed, owner,
		weaponDef->duration * weaponDef->maxvelocity,
		weaponDef->visuals.color, weaponDef->visuals.color2,
		weaponDef->intensity, weaponDef,
		(int) ((weaponDef->range - fpsSub) / weaponDef->projectilespeed) - 6);
}
