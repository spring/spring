/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FlameThrower.h"
#include "WeaponDef.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"

CR_BIND_DERIVED(CFlameThrower, CWeapon, (NULL, NULL))

CR_REG_METADATA(CFlameThrower,(
	CR_MEMBER(color),
	CR_MEMBER(color2)
))

CFlameThrower::CFlameThrower(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
}


void CFlameThrower::FireImpl(const bool scriptCall)
{
	float3 dir = currentTargetPos - weaponMuzzlePos;

	const float dist = dir.LengthNormalize();
	const float3 spread =
		(gs->randVector() * SprayAngleExperience() + SalvoErrorExperience()) -
		(dir * 0.001f);

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.speed = dir * projectileSpeed;
	params.spread = spread;
	params.ttl = std::ceil(std::max(dist, range) / projectileSpeed * weaponDef->duration);

	WeaponProjectileFactory::LoadProjectile(params);
}

