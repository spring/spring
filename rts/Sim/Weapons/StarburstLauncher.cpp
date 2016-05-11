/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StarburstLauncher.h"
#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"

CR_BIND_DERIVED(CStarburstLauncher, CWeapon, (NULL, NULL))

CR_REG_METADATA(CStarburstLauncher, (
	CR_MEMBER(uptime),
	CR_MEMBER(tracking)
))

CStarburstLauncher::CStarburstLauncher(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
	//happens when loading
	if (def != nullptr) {
		tracking = weaponDef->turnrate * def->tracks;
		uptime = (def->uptime * GAME_SPEED);
	}
}


void CStarburstLauncher::FireImpl(const bool scriptCall)
{
	const float3 speed = ((weaponDef->fixedLauncher)? weaponDir: UpVector) * weaponDef->startvelocity;
	const float3 aimError = (gs->randVector() * SprayAngleExperience() + SalvoErrorExperience());

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos + UpVector * 2.0f;
	params.end = currentTargetPos;
	params.speed = speed;
	params.error = aimError;
	params.ttl = 200; //???
	params.tracking = tracking;
	params.maxRange = (weaponDef->flighttime > 0 || weaponDef->fixedLauncher)? MAX_PROJECTILE_RANGE: range;

	WeaponProjectileFactory::LoadProjectile(params);
}

bool CStarburstLauncher::HaveFreeLineOfFire(const float3 pos, const SWeaponTarget& trg, bool useMuzzle) const
{
	const float3& wdir = weaponDef->fixedLauncher? weaponDir: UpVector;

	if (TraceRay::TestCone(weaponMuzzlePos, wdir, 100.0f, 0.0f, owner->allyteam, avoidFlags, owner)) {
		return false;
	}

	return true;
}

float CStarburstLauncher::GetRange2D(const float yDiff) const
{
	return range + (yDiff * weaponDef->heightmod);
}
