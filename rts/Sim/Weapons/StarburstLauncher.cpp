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
	CR_MEMBER(tracking),
	CR_RESERVED(8)
))

CStarburstLauncher::CStarburstLauncher(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
	tracking = ((def->tracks)? weaponDef->turnrate: 0);
	uptime = (def->uptime * GAME_SPEED);
}


void CStarburstLauncher::Update(void)
{
	if (targetType != Target_None) {
		aimFromPos = owner->GetObjectSpacePos(relAimFromPos);
		weaponMuzzlePos = owner->GetObjectSpacePos(relWeaponMuzzlePos);

		// the aiming upward is apperently implicid so aim toward target
		wantedDir = (targetPos - aimFromPos).Normalize();
	}

	CWeapon::Update();
}

void CStarburstLauncher::FireImpl(bool scriptCall)
{
	const float3 speed = ((weaponDef->fixedLauncher)? weaponDir: UpVector) * weaponDef->startvelocity;
	const float3 aimError = (gs->randVector() * SprayAngleExperience() + SalvoErrorExperience());

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos + UpVector * 2.0f;
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
	return range + (yDiff * weaponDef->heightmod);
}
