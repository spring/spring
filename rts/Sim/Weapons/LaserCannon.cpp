/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Game/TraceRay.h"
#include "LaserCannon.h"
#include "Map/Ground.h"
#include "Sim/MoveTypes/StrafeAirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/LaserProjectile.h"
#include "Sim/Units/Unit.h"
#include "WeaponDefHandler.h"

CR_BIND_DERIVED(CLaserCannon, CWeapon, (NULL));

CR_REG_METADATA(CLaserCannon,(
	CR_MEMBER(color),
	CR_RESERVED(8)
	));

CLaserCannon::CLaserCannon(CUnit* owner)
: CWeapon(owner)
{
}



void CLaserCannon::Update()
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

	if (!weaponDef->waterweapon && TargetUnitOrPositionInWater(pos, unit))
		return false;

	float3 dir(pos - weaponMuzzlePos);
	const float length = dir.Length();
	if (length == 0)
		return true;

	dir /= length;

	if (!onlyForward) {
		if (!HaveFreeLineOfFire(weaponMuzzlePos, dir, length, unit)) {
			return false;
		}
	}

	const float spread =
		(accuracy + sprayAngle) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight);

	if (avoidFeature && TraceRay::LineFeatureCol(weaponMuzzlePos, dir, length)) {
		return false;
	}
	if (avoidFriendly && TraceRay::TestCone(weaponMuzzlePos, dir, length, spread, owner->allyteam, true, false, false, owner)) {
		return false;
	}
	if (avoidNeutral && TraceRay::TestCone(weaponMuzzlePos, dir, length, spread, owner->allyteam, false, true, false, owner)) {
		return false;
	}

	return true;
}

void CLaserCannon::Init()
{
	CWeapon::Init();
}

void CLaserCannon::FireImpl()
{
	float3 dir;
	if (onlyForward && dynamic_cast<CStrafeAirMoveType*>(owner->moveType)) {
		// HoverAirMovetype cannot align itself properly, change back when that is fixed
		dir = owner->frontdir;
	} else {
		dir = targetPos - weaponMuzzlePos;
		dir.Normalize();
	}

	dir +=
		(gs->randVector() * sprayAngle + salvoError) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight);
	dir.Normalize();

	// subtract a magic 24 elmos in FPS mode (helps against range-exploits)
	const int fpsRangeSub = (owner->fpsControlPlayer != NULL)? (SQUARE_SIZE * 3): 0;
	const float boltLength = weaponDef->duration * (weaponDef->projectilespeed * GAME_SPEED);
	const int boltTTL = ((weaponDef->range - fpsRangeSub) / weaponDef->projectilespeed) - (fpsRangeSub >> 2);

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.speed = dir * projectileSpeed;
	params.ttl = boltTTL;
	new CLaserProjectile(params, boltLength, weaponDef->visuals.color, weaponDef->visuals.color2, weaponDef->intensity);
}
