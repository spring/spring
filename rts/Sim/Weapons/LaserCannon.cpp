/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LaserCannon.h"
#include "WeaponDef.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/myMath.h"

CR_BIND_DERIVED(CLaserCannon, CWeapon, (NULL, NULL))

CR_REG_METADATA(CLaserCannon,(
	CR_MEMBER(color),
	CR_RESERVED(8)
))

CLaserCannon::CLaserCannon(CUnit* owner, const WeaponDef* def)
	: CWeapon(owner, def)
	, color(def->visuals.color)
{
}



void CLaserCannon::Update()
{
	if (targetType != Target_None) {
		weaponPos = owner->GetObjectSpacePos(relWeaponPos);
		weaponMuzzlePos = owner->GetObjectSpacePos(relWeaponMuzzlePos);

		float3 wantedDirTemp = targetPos - weaponPos;
		const float targetDist = wantedDirTemp.LengthNormalize();

		if (!onlyForward && targetDist != 0.0f) {
			wantedDir = wantedDirTemp;
		}

		predict = targetDist / projectileSpeed;
	}

	CWeapon::Update();
}

void CLaserCannon::UpdateRange(float val)
{
	// round range *DOWN* to integer multiple of projectile speed
	//
	// (val / speed) is the total number of frames the projectile
	// is allowed to do damage to objects, ttl decreases from N-1
	// to 0 and collisions are checked at 0 inclusive
	range = std::max(1.0f, std::floor(val / weaponDef->projectilespeed)) * weaponDef->projectilespeed;
}


void CLaserCannon::FireImpl(bool scriptCall)
{
	float3 dir = targetPos - weaponMuzzlePos;

	const float dist = dir.LengthNormalize();
	const int ttlreq = std::ceil(dist / weaponDef->projectilespeed);
	const int ttlmax = std::floor(range / weaponDef->projectilespeed) - 1;

	if (onlyForward && owner->unitDef->IsStrafingAirUnit()) {
		// [?] StrafeAirMovetype cannot align itself properly, change back when that is fixed
		dir = owner->frontdir;
	}

	dir += (gs->randVector() * SprayAngleExperience() + SalvoErrorExperience());
	dir.Normalize();

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.speed = dir * projectileSpeed;
	params.ttl = mix(std::max(ttlreq, ttlmax), std::min(ttlreq, ttlmax), weaponDef->selfExplode);

	WeaponProjectileFactory::LoadProjectile(params);
}
