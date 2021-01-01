/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EmgCannon.h"

#include "WeaponDef.h"
#include "Map/Ground.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/Team.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/SpringMath.h"

CR_BIND_DERIVED(CEmgCannon, CWeapon, )
CR_REG_METADATA(CEmgCannon, )


void CEmgCannon::FireImpl(const bool scriptCall)
{
	float3 dir = currentTargetPos - weaponMuzzlePos;
	const float dist = dir.LengthNormalize();

	// [?] StrafeAirMoveType cannot align itself properly, change back when that is fixed
	if (onlyForward && owner->unitDef->IsStrafingAirUnit())
		dir = owner->frontdir;

	dir += (gsRNG.NextVector() * SprayAngleExperience() + SalvoErrorExperience());
	dir.Normalize();

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.speed = dir * projectileSpeed;
	params.ttl = weaponDef->flighttime > 0 ? weaponDef->flighttime : math::ceil(std::max(dist, range) / projectileSpeed);

	WeaponProjectileFactory::LoadProjectile(params);
}
