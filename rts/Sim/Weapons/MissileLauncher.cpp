#include "StdAfx.h"
#include "Game/GameHelper.h"
#include "Map/Ground.h"
#include "MissileLauncher.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/MissileProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sound.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"

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

			if (weaponDef->trajectoryHeight > 0) {
				wantedDir.y += weaponDef->trajectoryHeight;
				wantedDir.Normalize();
			}
		}
	}
	CWeapon::Update();
}

void CMissileLauncher::Fire(void)
{
	float3 dir;
	if (onlyForward) {
		dir = owner->frontdir;
	} else if (weaponDef->fixedLauncher) {
		dir = weaponDir;
	} else {
		dir = targetPos - weaponMuzzlePos;
		dir.Normalize();

		if (weaponDef->trajectoryHeight > 0) {
			dir.y += weaponDef->trajectoryHeight;
			dir.Normalize();
		}
	}

	dir += (gs->randVector() * sprayAngle + salvoError) * (1 - owner->limExperience * 0.5f);
	dir.Normalize();

	float3 startSpeed = dir * weaponDef->startvelocity;
	if (onlyForward && dynamic_cast<CAirMoveType*>(owner->moveType))
		startSpeed += owner->speed;

	new CMissileProjectile(weaponMuzzlePos, startSpeed, owner, areaOfEffect,
			projectileSpeed,
			weaponDef->flighttime == 0
                ? (int) (range / projectileSpeed + 25 * weaponDef->selfExplode)
                : weaponDef->flighttime,
			targetUnit, weaponDef, targetPos);

	if (fireSoundId && (!weaponDef->soundTrigger || salvoLeft == salvoSize - 1))
		sound->PlaySample(fireSoundId, owner->pos, dir*100, fireSoundVolume);
}

bool CMissileLauncher::TryTarget(const float3& pos, bool userTarget, CUnit* unit)
{
	if (!CWeapon::TryTarget(pos, userTarget, unit))
		return false;

	if (!weaponDef->waterweapon) {
		if (unit) {
			if (unit->isUnderWater) {
				return false;
			}
		} else {
			if (pos.y < 0)
				return false;
		}
	}

	float3 dir = pos - weaponMuzzlePos;

	if (weaponDef->trajectoryHeight > 0) {
		// do a different test depending on if the missile has a high trajectory or not
		float3 flatdir(dir.x, 0, dir.z);
		dir.Normalize();
		float flatlength = flatdir.Length();

		if (flatlength == 0)
			return true;

		flatdir /= flatlength;

		float linear = dir.y + weaponDef->trajectoryHeight;
		float quadratic = -weaponDef->trajectoryHeight / flatlength;
		float gc = ground->TrajectoryGroundCol(weaponMuzzlePos, flatdir, flatlength - 30, linear, quadratic);

		if (gc > 0)
			return false;

		if (avoidFriendly && helper->TestTrajectoryAllyCone(weaponMuzzlePos, flatdir, flatlength - 30, linear, quadratic, 0, 8, owner->allyteam, owner)) {
			return false;
		}
		if (avoidNeutral && helper->TestTrajectoryNeutralCone(weaponMuzzlePos, flatdir, flatlength - 30, linear, quadratic, 0, 8, owner)) {
			return false;
		}
	} else {
		float length = dir.Length();
		if (length == 0)
			return true;

		dir /= length;

		if (!onlyForward) {
			// skip ground col testing for aircraft
			float g = ground->LineGroundCol(weaponMuzzlePos, pos);
			if (g > 0 && g < length * 0.9f)
				return false;
		} else {
			float3 goaldir = pos - owner->pos;
			goaldir.Normalize();
			if (owner->frontdir.dot(goaldir) < maxAngleDif)
				return false;
		}

		if (avoidFriendly && helper->TestAllyCone(weaponMuzzlePos, dir, length, (accuracy + sprayAngle), owner->allyteam, owner)) {
			return false;
		}
		if (avoidNeutral && helper->TestNeutralCone(weaponMuzzlePos, dir, length, (accuracy + sprayAngle), owner)) {
			return false;
		}
	}
	return true;
}
