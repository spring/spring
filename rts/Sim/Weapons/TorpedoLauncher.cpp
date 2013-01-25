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
	float3 dir = targetPos - weaponMuzzlePos;
	dir.Normalize();
	if (weaponDef->trajectoryHeight > 0){
		dir.y += weaponDef->trajectoryHeight;
		dir.Normalize();
	}

	float3 startSpeed = dir * weaponDef->startvelocity;
	if (weaponDef->fixedLauncher) {
		startSpeed = weaponDir * weaponDef->startvelocity;
	}

	ProjectileParams params = GetProjectileParams();
	params.speed = startSpeed;
	params.pos = weaponMuzzlePos;
	params.end = targetPos;
	params.ttl = (weaponDef->flighttime == 0)? (int) (range / projectileSpeed + 25): weaponDef->flighttime;

	new CTorpedoProjectile(params, damageAreaOfEffect, projectileSpeed, tracking);
}
