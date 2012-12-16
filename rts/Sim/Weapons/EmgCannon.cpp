/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EmgCannon.h"
#include "Game/TraceRay.h"
#include "Sim/Misc/Team.h"
#include "Map/Ground.h"
#include "Sim/MoveTypes/StrafeAirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/EmgProjectile.h"
#include "Sim/Units/Unit.h"
#include "System/Sync/SyncTracer.h"
#include "WeaponDefHandler.h"

CR_BIND_DERIVED(CEmgCannon, CWeapon, (NULL));

CR_REG_METADATA(CEmgCannon,(
				CR_RESERVED(8)
				));

CEmgCannon::CEmgCannon(CUnit* owner)
: CWeapon(owner)
{
}

CEmgCannon::~CEmgCannon()
{
}

void CEmgCannon::Update()
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

bool CEmgCannon::TryTarget(const float3& pos, bool userTarget, CUnit* unit)
{
	if (!CWeapon::TryTarget(pos, userTarget, unit))
		return false;

	if (!weaponDef->waterweapon && TargetUnitOrPositionInWater(pos, unit))
		return false;

	float3 dir(pos - weaponMuzzlePos);
	float length = dir.Length();
	if (length == 0)
		return true;

	dir /= length;

	if (!HaveFreeLineOfFire(weaponMuzzlePos, dir, length, unit)) {
		return false;
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

void CEmgCannon::Init()
{
	CWeapon::Init();
}

void CEmgCannon::FireImpl()
{
	float3 dir;
	if (onlyForward && dynamic_cast<CStrafeAirMoveType*>(owner->moveType)) {
		// HoverAirMoveType canot align itself properly, change back when that is fixed
		dir = owner->frontdir;
	} else {
		dir = (targetPos - weaponMuzzlePos).Normalize();
	}

	dir +=
		((gs->randVector() * sprayAngle + salvoError) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight));
	dir.Normalize();

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.speed = dir * projectileSpeed;
	params.ttl = (int) (range / projectileSpeed);
	new CEmgProjectile(params, weaponDef->visuals.color, weaponDef->intensity);
}
