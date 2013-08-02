/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DGunWeapon.h"
#include "WeaponDef.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"

CR_BIND_DERIVED(CDGunWeapon, CWeapon, (NULL));

CR_REG_METADATA(CDGunWeapon,(
	CR_RESERVED(8)
));

CDGunWeapon::CDGunWeapon(CUnit* owner): CWeapon(owner)
{
}


void CDGunWeapon::Update()
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

		if (!onlyForward) {
			wantedDir = (targetPos - weaponPos).Normalize();
		}

		// user has to manually predict
		predict = 0;
	}

	CWeapon::Update();
}

void CDGunWeapon::FireImpl()
{
	float3 dir = targetPos - weaponMuzzlePos;
	const float dist = dir.Length();
	dir /= dist;

	if (onlyForward) {
		dir = owner->frontdir;
	}

	dir +=
		(gs->randVector() * SprayAngleExperience() + SalvoErrorExperience());
	dir.Normalize();

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.end = targetPos;
	params.speed = dir * projectileSpeed;
	params.ttl = 1;

	WeaponProjectileFactory::LoadProjectile(params);
}


void CDGunWeapon::Init()
{
	CWeapon::Init();
	muzzleFlareSize=1.5f;
}
