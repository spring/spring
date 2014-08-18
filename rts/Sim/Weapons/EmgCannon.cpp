/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EmgCannon.h"
#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Sim/Misc/Team.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/Sync/SyncTracer.h"

CR_BIND_DERIVED(CEmgCannon, CWeapon, (NULL, NULL))

CR_REG_METADATA(CEmgCannon,(
	CR_RESERVED(8)
))

CEmgCannon::CEmgCannon(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
}


void CEmgCannon::Update()
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



void CEmgCannon::FireImpl(bool scriptCall)
{
	float3 dir = targetPos - weaponMuzzlePos;
	const float dist = dir.LengthNormalize();

	if (onlyForward && owner->unitDef->IsStrafingAirUnit()) {
		// [?] StrafeAirMoveType cannot align itself properly, change back when that is fixed
		dir = owner->frontdir;
	}

	dir += (gs->randVector() * SprayAngleExperience() + SalvoErrorExperience());
	dir.Normalize();

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.speed = dir * projectileSpeed;
	params.ttl = std::ceil(std::max(dist, range) / projectileSpeed);

	WeaponProjectileFactory::LoadProjectile(params);
}
