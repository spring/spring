/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LaserCannon.h"
#include "WeaponDef.h"
#include "Map/Ground.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/SpringMath.h"

CR_BIND_DERIVED(CLaserCannon, CWeapon, )
CR_REG_METADATA(CLaserCannon, (
	CR_MEMBER(color)
))

CLaserCannon::CLaserCannon(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
	//happens when loading
	if (def != nullptr)
		color = def->visuals.color;
}


void CLaserCannon::UpdateProjectileSpeed(const float val)
{
	projectileSpeed = std::max(0.001f, val); // sanitize
	range = std::max(1.0f, std::floor(val / projectileSpeed)) * projectileSpeed;
}
void CLaserCannon::UpdateRange(const float val)
{
	// round range *DOWN* to integer multiple of projectile speed
	//
	// (val / speed) is the total number of frames the projectile
	// is allowed to do damage to objects, ttl decreases from N-1
	// to 0 and collisions are checked at 0 inclusive
	range = std::max(1.0f, std::floor(val / projectileSpeed)) * projectileSpeed;
}


void CLaserCannon::FireImpl(const bool scriptCall)
{
	float3 dir = currentTargetPos - weaponMuzzlePos;

	const float dist = dir.LengthNormalize();
	const int ttlreq = std::ceil(dist / projectileSpeed);
	const int ttlmax = std::floor(range / projectileSpeed) - 1;

	// [?] StrafeAirMovetype cannot align itself properly, change back when that is fixed
	if (onlyForward && owner->unitDef->IsStrafingAirUnit())
		dir = owner->frontdir;

	dir += (gsRNG.NextVector() * SprayAngleExperience() + SalvoErrorExperience());
	dir.Normalize();

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.speed = dir * projectileSpeed;
	params.ttl = mix(std::max(ttlreq, ttlmax), std::min(ttlreq, ttlmax), weaponDef->selfExplode);

	WeaponProjectileFactory::LoadProjectile(params);
}
