/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "MissileLauncher.h"
#include "Sim/MoveTypes/StrafeAirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/MissileProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/Unit.h"
#include "WeaponDefHandler.h"
#include "System/mmgr.h"

CR_BIND_DERIVED(CMissileLauncher, CWeapon, (NULL));

CR_REG_METADATA(CMissileLauncher,(
	CR_RESERVED(8)
	));

CMissileLauncher::CMissileLauncher(CUnit* owner)
: CWeapon(owner)
{
}

CMissileLauncher::~CMissileLauncher(void)
{
}

void CMissileLauncher::Update(void)
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
	float3 dir;
	if (onlyForward) {
		dir = owner->frontdir;
	} else if (weaponDef->fixedLauncher) {
		dir = weaponDir;
	} else {
		dir = targetPos - weaponMuzzlePos;
		dir.Normalize();

		if (weaponDef->trajectoryHeight > 0.0f) {
			dir.y += weaponDef->trajectoryHeight;
			dir.Normalize();
		}
	}

	dir +=
		((gs->randVector() * sprayAngle + salvoError) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight));
	dir.Normalize();

	float3 startSpeed = dir * weaponDef->startvelocity;
	if (onlyForward && dynamic_cast<CStrafeAirMoveType*>(owner->moveType))
		startSpeed += owner->speed;

	new CMissileProjectile(weaponMuzzlePos, startSpeed, owner, areaOfEffect,
			projectileSpeed,
			weaponDef->flighttime == 0
                ? (int) (range / projectileSpeed + 25 * weaponDef->selfExplode)
                : weaponDef->flighttime,
			targetUnit, weaponDef, targetPos);
}

bool CMissileLauncher::TryTarget(const float3& pos, bool userTarget, CUnit* unit)
{
	if (!CWeapon::TryTarget(pos, userTarget, unit))
		return false;

	if (!weaponDef->waterweapon && TargetUnitOrPositionInWater(pos, unit))
		return false;

	float3 dir = pos - weaponMuzzlePos;

	if (weaponDef->trajectoryHeight > 0.0f) {
		// do a different test depending on if the missile has high
		// trajectory (parabolic vs. linear ground intersection; in
		// the latter case, HaveFreeLineOfFire() checks the NOGROUND
		// collision flag for us)
		float3 flatdir(dir.x, 0, dir.z);
		dir.Normalize();
		float flatlength = flatdir.Length();

		if (flatlength == 0)
			return true;

		flatdir /= flatlength;

		const float linear = dir.y + weaponDef->trajectoryHeight;
		const float quadratic = -weaponDef->trajectoryHeight / flatlength;
		const float gc = ((collisionFlags & Collision::NOGROUND) == 0)?
			ground->TrajectoryGroundCol(weaponMuzzlePos, flatdir, flatlength - 30, linear, quadratic):
			-1.0f;

		if (gc > 0.0f)
			return false;

		if (avoidFriendly && TraceRay::TestTrajectoryAllyCone(weaponMuzzlePos, flatdir, flatlength - 30, linear, quadratic, 0, 8, owner->allyteam, owner)) {
			return false;
		}
		if (avoidNeutral && TraceRay::TestTrajectoryNeutralCone(weaponMuzzlePos, flatdir, flatlength - 30, linear, quadratic, 0, 8, owner)) {
			return false;
		}
	} else {
		float length = dir.Length();
		if (length == 0)
			return true;

		dir /= length;

		if (!onlyForward) {
			if (!HaveFreeLineOfFire(weaponMuzzlePos, dir, length, unit)) {
				return false;
			}
		} else {
			float3 goaldir = pos - owner->pos;
			goaldir.Normalize();
			if (owner->frontdir.dot(goaldir) < maxAngleDif)
				return false;
		}

		if (avoidFriendly && TraceRay::TestAllyCone(weaponMuzzlePos, dir, length, (accuracy + sprayAngle), owner->allyteam, owner)) {
			return false;
		}
		if (avoidNeutral && TraceRay::TestNeutralCone(weaponMuzzlePos, dir, length, (accuracy + sprayAngle), owner)) {
			return false;
		}
	}
	return true;
}
