/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/StarburstProjectile.h"
#include "Sim/Units/Unit.h"
#include "StarburstLauncher.h"
#include "WeaponDefHandler.h"

CR_BIND_DERIVED(CStarburstLauncher, CWeapon, (NULL));

CR_REG_METADATA(CStarburstLauncher,(
	CR_MEMBER(uptime),
	CR_MEMBER(tracking),
	CR_RESERVED(8)
	));

CStarburstLauncher::CStarburstLauncher(CUnit* owner)
: CWeapon(owner),
	tracking(0),
	uptime(3)
{
}

CStarburstLauncher::~CStarburstLauncher(void)
{
}

void CStarburstLauncher::Update(void)
{
	if (targetType != Target_None) {
		weaponPos = owner->pos +
			owner->frontdir * relWeaponPos.z +
			owner->updir    * relWeaponPos.y +
			owner->rightdir * relWeaponPos.x;
		weaponMuzzlePos = owner->pos +
			owner->frontdir * relWeaponMuzzlePos.z +
			owner->updir    * relWeaponMuzzlePos.y +
			owner->rightdir * relWeaponMuzzlePos.x;

		// the aiming upward is apperently implicid so aim toward target
		wantedDir = (targetPos - weaponPos).Normalize();
	}

	CWeapon::Update();
}

void CStarburstLauncher::FireImpl()
{
	float3 speed(0.0f, weaponDef->startvelocity, 0.0f);

	if (weaponDef->fixedLauncher) {
		speed = weaponDir * weaponDef->startvelocity;
	}

	const float maxRange = (weaponDef->flighttime > 0 || weaponDef->fixedLauncher)?
		MAX_PROJECTILE_RANGE: range;
	const float3 aimError =
		(gs->randVector() * sprayAngle + salvoError) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight);

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos + float3(0, 2, 0);
	params.end = targetPos;
	params.speed = speed;
	params.ttl = 200; //???

	new CStarburstProjectile(params, damageAreaOfEffect, projectileSpeed, tracking, (int) uptime, maxRange, aimError);
}

bool CStarburstLauncher::TryTarget(const float3& pos, bool userTarget, CUnit* unit)
{
	if (!CWeapon::TryTarget(pos, userTarget, unit))
		return false;

	if (!weaponDef->waterweapon && TargetUnitOrPositionInWater(pos, unit))
		return false;

	const float3& wdir = weaponDef->fixedLauncher? weaponDir: UpVector;

	if (avoidFriendly && TraceRay::TestCone(weaponMuzzlePos, wdir, 100.0f, 0.0f, owner->allyteam, true, false, false, owner)) {
		return false;
	}
	if (avoidNeutral && TraceRay::TestCone(weaponMuzzlePos, wdir, 100.0f, 0.0f, owner->allyteam, false, true, false, owner)) {
		return false;
	}

	return true;
}

float CStarburstLauncher::GetRange2D(float yDiff) const
{
	return range + (yDiff * heightMod);
}
