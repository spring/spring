/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EmgCannon.h"
#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Sim/Misc/Team.h"
#include "Map/Ground.h"
#include "Sim/MoveTypes/StrafeAirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"
#include "System/Sync/SyncTracer.h"

CR_BIND_DERIVED(CEmgCannon, CWeapon, (NULL));

CR_REG_METADATA(CEmgCannon,(
	CR_RESERVED(8)
));

CEmgCannon::CEmgCannon(CUnit* owner): CWeapon(owner)
{
}


void CEmgCannon::Update()
{
	if(targetType != Target_None){
		weaponPos = owner->pos +
			owner->frontdir * relWeaponPos.z +
			owner->updir    * relWeaponPos.y +
			owner->rightdir * relWeaponPos.x;
		weaponMuzzlePos = owner->pos +
			owner->frontdir * relWeaponMuzzlePos.z +
			owner->updir    * relWeaponMuzzlePos.y +
			owner->rightdir * relWeaponMuzzlePos.x;

		float3 wantedDirTemp(targetPos - weaponPos);
		float len = wantedDirTemp.Length();
		if(!onlyForward && (len != 0.0f)) {
			wantedDir = wantedDirTemp;
			wantedDir /= len;
		}
		predict=len/projectileSpeed;
	}
	CWeapon::Update();
}


void CEmgCannon::Init()
{
	CWeapon::Init();
}

void CEmgCannon::FireImpl()
{
	float3 dir;
	if (onlyForward && dynamic_cast<CStrafeAirMoveType*>(owner->moveType)) {
		// HoverAirMoveType canot align itself properly, change back when that is fixed
		dir = owner->frontdir;
	} else {
		dir = (targetPos - weaponMuzzlePos).Normalize();
	}

	dir +=
		((gs->randVector() * sprayAngle + salvoError) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight));
	dir.Normalize();

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.speed = dir * projectileSpeed;
	params.ttl = (int) (range / projectileSpeed);

	WeaponProjectileFactory::LoadProjectile(params);
}
