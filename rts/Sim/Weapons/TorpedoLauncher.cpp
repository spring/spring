/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/TorpedoProjectile.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "TorpedoLauncher.h"
#include "WeaponDefHandler.h"

CR_BIND_DERIVED(CTorpedoLauncher, CWeapon, (NULL));

CR_REG_METADATA(CTorpedoLauncher,(
		CR_MEMBER(tracking),
		CR_RESERVED(8)
		));

CTorpedoLauncher::CTorpedoLauncher(CUnit* owner)
: CWeapon(owner),
	tracking(0)
{
	if (owner) owner->hasUWWeapons=true;
}

CTorpedoLauncher::~CTorpedoLauncher()
{
}


void CTorpedoLauncher::Update()
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

		wantedDir = targetPos - weaponPos;
		const float dist = wantedDir.Length();
		predict = dist / projectileSpeed;
		wantedDir /= dist;
	}

	CWeapon::Update();
}

void CTorpedoLauncher::FireImpl()
{
	float3 dir;
	dir=targetPos-weaponMuzzlePos;
	dir.Normalize();
	if(weaponDef->trajectoryHeight>0){
		dir.y+=weaponDef->trajectoryHeight;
		dir.Normalize();
	}

	float3 startSpeed;
	if (!weaponDef->fixedLauncher) {
		startSpeed = dir * weaponDef->startvelocity;
	}
	else {
		startSpeed = weaponDir * weaponDef->startvelocity;
	}

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.end = targetPos;
	params.speed = startSpeed;
	params.ttl = weaponDef->flighttime == 0? (int) (range / projectileSpeed + 25): weaponDef->flighttime;

	new CTorpedoProjectile(params, damageAreaOfEffect, projectileSpeed, tracking);
}

bool CTorpedoLauncher::TryTarget(const float3& pos, bool userTarget, CUnit* unit)
{
	if (!CWeapon::TryTarget(pos, userTarget, unit))
		return false;

	if (unit) {
		// if we cannot leave water and target unit is not in water, bail
		if (!weaponDef->submissile && !unit->inWater)
			return false;
	} else {
		// if we cannot leave water and target position is not in water, bail
		if (!weaponDef->submissile && ground->GetHeightReal(pos.x, pos.z) > 0.0f)
			return false;
	}

	float3 targetVec = pos - weaponMuzzlePos;
	float targetDist = targetVec.Length();

	if (targetDist == 0.0f)
		return true;

	targetVec /= targetDist;
	// +0.05f since torpedoes have an unfortunate tendency to hit own ships due to movement
	float spread = (accuracy + sprayAngle) + 0.05f;

	if (avoidFriendly && TraceRay::TestCone(weaponMuzzlePos, targetVec, targetDist, spread, owner->allyteam, true, false, false, owner)) {
		return false;
	}
	if (avoidNeutral && TraceRay::TestCone(weaponMuzzlePos, targetVec, targetDist, spread, owner->allyteam, false, true, false, owner)) {
		return false;
	}

	return true;
}
