/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StarburstLauncher.h"
#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"

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

	const float3 aimError =
		(gs->randVector() * SprayAngleExperience() + SalvoErrorExperience());

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos + float3(0, 2, 0);
	params.end = targetPos;
	params.speed = speed;
	params.error = aimError;
	params.ttl = 200; //???
	params.tracking = tracking;
	params.maxRange = (weaponDef->flighttime > 0 || weaponDef->fixedLauncher)? MAX_PROJECTILE_RANGE: range;

	WeaponProjectileFactory::LoadProjectile(params);
}

bool CStarburstLauncher::HaveFreeLineOfFire(const float3& pos, bool userTarget, const CUnit* unit) const
{
	const float3& wdir = weaponDef->fixedLauncher? weaponDir: UpVector;

	if (TraceRay::TestCone(weaponMuzzlePos, wdir, 100.0f, 0.0f, owner->allyteam, avoidFlags, owner)) {
		return false;
	}

	return true;
}

float CStarburstLauncher::GetRange2D(float yDiff) const
{
	return range + (yDiff * heightMod);
}
