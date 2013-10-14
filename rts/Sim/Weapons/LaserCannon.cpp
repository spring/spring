/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LaserCannon.h"
#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

CR_BIND_DERIVED(CLaserCannon, CWeapon, (NULL, NULL));

CR_REG_METADATA(CLaserCannon,(
	CR_MEMBER(color),
	CR_RESERVED(8)
));

CLaserCannon::CLaserCannon(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
	color = def->visuals.color;
}



void CLaserCannon::Update()
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

		float3 wantedDirTemp = targetPos - weaponPos;
		const float targetDist = wantedDirTemp.LengthNormalize();

		if (!onlyForward && targetDist != 0.0f) {
			wantedDir = wantedDirTemp;
		}

		predict = targetDist / projectileSpeed;
	}

	CWeapon::Update();
}


void CLaserCannon::Init()
{
	CWeapon::Init();
}

void CLaserCannon::FireImpl(bool scriptCall)
{
	float3 dir = targetPos - weaponMuzzlePos;
	const float dist = dir.LengthNormalize();

	if (onlyForward && owner->unitDef->IsStrafingAirUnit()) {
		// [?] StrafeAirMovetype cannot align itself properly, change back when that is fixed
		dir = owner->frontdir;
	}

	dir += (gs->randVector() * SprayAngleExperience() + SalvoErrorExperience());
	dir.Normalize();

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.speed = dir * projectileSpeed;
	params.ttl = std::ceil(std::max(dist, range) / weaponDef->projectilespeed);

	WeaponProjectileFactory::LoadProjectile(params);
}
