/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LaserCannon.h"
#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/MoveTypes/StrafeAirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"

CR_BIND_DERIVED(CLaserCannon, CWeapon, (NULL));

CR_REG_METADATA(CLaserCannon,(
	CR_MEMBER(color),
	CR_RESERVED(8)
));

CLaserCannon::CLaserCannon(CUnit* owner): CWeapon(owner)
{
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
		float targetDist = wantedDirTemp.Length();

		if (!onlyForward && (targetDist != 0.0f)) {
			wantedDir = wantedDirTemp;
			wantedDir /= targetDist;
		}

		predict = targetDist / projectileSpeed;
	}

	CWeapon::Update();
}


void CLaserCannon::Init()
{
	CWeapon::Init();
}

void CLaserCannon::FireImpl()
{
	float3 dir = targetPos - weaponMuzzlePos;
	const float dist = dir.Length();
	dir /= dist;

	if (onlyForward && dynamic_cast<CStrafeAirMoveType*>(owner->moveType)) {
		// HoverAirMovetype cannot align itself properly, change back when that is fixed
		dir = owner->frontdir;
	}

	dir +=
		(gs->randVector() * sprayAngle + salvoError) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight);
	dir.Normalize();

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.speed = dir * projectileSpeed;
	params.ttl = std::ceil(std::max(dist, weaponDef->range) / weaponDef->projectilespeed);

	WeaponProjectileFactory::LoadProjectile(params);
}
