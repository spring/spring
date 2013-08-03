/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MissileLauncher.h"
#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/MoveTypes/StrafeAirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"

CR_BIND_DERIVED(CMissileLauncher, CWeapon, (NULL));

CR_REG_METADATA(CMissileLauncher,(
	CR_RESERVED(8)
));

CMissileLauncher::CMissileLauncher(CUnit* owner): CWeapon(owner)
{
}


void CMissileLauncher::Update()
{
	if (targetType != Target_None) {
		weaponPos = owner->pos + owner->frontdir * relWeaponPos.z + owner->updir * relWeaponPos.y + owner->rightdir * relWeaponPos.x;
		weaponMuzzlePos = owner->pos + owner->frontdir * relWeaponMuzzlePos.z + owner->updir * relWeaponMuzzlePos.y + owner->rightdir * relWeaponMuzzlePos.x;

		if (!onlyForward) {
			wantedDir = targetPos - weaponPos;
			float dist = wantedDir.Length();
			predict = dist / projectileSpeed;
			wantedDir /= dist;

			if (weaponDef->trajectoryHeight > 0.0f) {
				wantedDir.y += weaponDef->trajectoryHeight;
				wantedDir.Normalize();
			}
		}
	}
	CWeapon::Update();
}

void CMissileLauncher::FireImpl()
{
	float3 dir = targetPos - weaponMuzzlePos;
	const float dist = dir.Length();
	dir /= dist;

	if (onlyForward) {
		dir = owner->frontdir;
	} else if (weaponDef->fixedLauncher) {
		dir = weaponDir;
	} else if (weaponDef->trajectoryHeight > 0.0f) {
		dir.y += weaponDef->trajectoryHeight;
		dir.Normalize();
	}

	dir +=
		(gs->randVector() * SprayAngleExperience() + SalvoErrorExperience());
	dir.Normalize();

	float3 startSpeed = dir * weaponDef->startvelocity;
	if (onlyForward && dynamic_cast<CStrafeAirMoveType*>(owner->moveType))
		startSpeed += owner->speed;

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.end = targetPos;
	params.speed = startSpeed;
	params.ttl = weaponDef->flighttime == 0? std::ceil(std::max(dist, range) / projectileSpeed + 25 * weaponDef->selfExplode): weaponDef->flighttime;

	WeaponProjectileFactory::LoadProjectile(params);
}

bool CMissileLauncher::HaveFreeLineOfFire(const float3& pos, bool userTarget, const CUnit* unit) const
{
	// do a different test depending on if the missile has high
	// trajectory (parabolic vs. linear ground intersection)
	if (weaponDef->trajectoryHeight <= 0.0f)
		return CWeapon::HaveFreeLineOfFire(pos, userTarget, unit);

	float3 dir(pos - weaponMuzzlePos);

	float3 flatDir(dir.x, 0, dir.z);
	dir.SafeNormalize();
	float flatLength = flatDir.Length();

	if (flatLength == 0)
		return true;

	flatDir /= flatLength;

	const float linear = dir.y + weaponDef->trajectoryHeight;
	const float quadratic = -weaponDef->trajectoryHeight / flatLength;
	const float groundDist = ((avoidFlags & Collision::NOGROUND) == 0)?
		ground->TrajectoryGroundCol(weaponMuzzlePos, flatDir, flatLength - 30, linear, quadratic):
		-1.0f;
	// FIXME: _WHY_ the 30-elmo subtraction?
	const float modFlatLength = flatLength - 30.0f;

	if (groundDist > 0.0f)
		return false;

	if (TraceRay::TestTrajectoryCone(weaponMuzzlePos, flatDir, modFlatLength,
		linear, quadratic, 0, owner->allyteam, avoidFlags, owner)) {
		return false;
	}

	return true;
}
