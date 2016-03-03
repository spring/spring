/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EmgCannon.h"
#include "WeaponDef.h"
#include "Sim/Misc/Team.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/Sync/SyncTracer.h"

CR_BIND_DERIVED(CEmgCannon, CWeapon, (NULL, NULL))
CR_REG_METADATA(CEmgCannon, )

CEmgCannon::CEmgCannon(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
}


void CEmgCannon::FireImpl(const bool scriptCall)
{
	float3 dir = currentTargetPos - weaponMuzzlePos;
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
