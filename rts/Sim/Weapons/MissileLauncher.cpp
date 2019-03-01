/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MissileLauncher.h"

#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/SpringMath.h"

CR_BIND_DERIVED(CMissileLauncher, CWeapon, )
CR_REG_METADATA(CMissileLauncher, )


void CMissileLauncher::UpdateWantedDir()
{
	CWeapon::UpdateWantedDir();

	if (weaponDef->trajectoryHeight > 0.0f) {
		wantedDir.y += weaponDef->trajectoryHeight;
		wantedDir.Normalize();
	}
}

void CMissileLauncher::FireImpl(const bool scriptCall)
{
	float3 targetVec = currentTargetPos - weaponMuzzlePos;
	const float targetDist = targetVec.LengthNormalize();

	if (onlyForward) {
		targetVec = owner->frontdir;
	} else if (weaponDef->fixedLauncher) {
		targetVec = weaponDir;
	} else if (weaponDef->trajectoryHeight > 0.0f) {
		targetVec = (targetVec + UpVector * weaponDef->trajectoryHeight).Normalize();
	}

	targetVec += (gsRNG.NextVector() * SprayAngleExperience() + SalvoErrorExperience());
	targetVec.Normalize();

	float3 startSpeed = targetVec * weaponDef->startvelocity;

	// NOTE: why only for SAMT units?
	if (onlyForward && owner->unitDef->IsStrafingAirUnit())
		startSpeed += owner->speed;

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.end = currentTargetPos;
	params.speed = startSpeed;
	params.ttl = weaponDef->flighttime == 0? math::ceil(std::max(targetDist, range) / projectileSpeed + 25 * weaponDef->selfExplode): weaponDef->flighttime;

	WeaponProjectileFactory::LoadProjectile(params);
}

bool CMissileLauncher::HaveFreeLineOfFire(const float3 srcPos, const float3 tgtPos, const SWeaponTarget& trg) const
{
	// high-trajectory missiles use parabolic rather than linear ground intersection
	if (weaponDef->trajectoryHeight <= 0.0f)
		return (CWeapon::HaveFreeLineOfFire(srcPos, tgtPos, trg));

	float3 targetVec = (tgtPos - srcPos) * XZVector;
	float3 launchDir = (tgtPos - srcPos).SafeNormalize();

	const float xzTargetDist = targetVec.LengthNormalize();

	if (xzTargetDist == 0.0f)
		return true;

	const float   linCoeff = launchDir.y + weaponDef->trajectoryHeight;
	const float   qdrCoeff = -weaponDef->trajectoryHeight / xzTargetDist;
	const float groundDist = ((avoidFlags & Collision::NOGROUND) == 0)?
		CGround::TrajectoryGroundCol(srcPos, targetVec, xzTargetDist, linCoeff, qdrCoeff):
		-1.0f;

	if (groundDist > 0.0f)
		return false;

	return (!TraceRay::TestTrajectoryCone(srcPos, targetVec, xzTargetDist, linCoeff, qdrCoeff, 0.0f, owner->allyteam, avoidFlags, owner));
}

