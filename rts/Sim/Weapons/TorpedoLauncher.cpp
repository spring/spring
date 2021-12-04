/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TorpedoLauncher.h"

#include "WeaponDef.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "System/SpringMath.h"

CR_BIND_DERIVED(CTorpedoLauncher, CWeapon, )
CR_REG_METADATA(CTorpedoLauncher,(
	CR_MEMBER(tracking)
))

CTorpedoLauncher::CTorpedoLauncher(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
	// null happens when loading
	if (def != nullptr)
		tracking = weaponDef->turnrate * def->tracks;
}


bool CTorpedoLauncher::TestTarget(const float3 pos, const SWeaponTarget& trg) const
{
	// by default we are a waterweapon, therefore:
	//   if muzzle is above water, target position is only allowed to be IN water
	//   if muzzle is below water, target position being valid depends on submissile
	//
	// NOTE:
	//   generally a TorpedoLauncher has its muzzle UNDER water and can *only* fire at
	//   targets IN water but depth-charge weapons break first part of this assumption
	//   (as do aircraft-based launchers)
	//
	//   this check used to be in the base-class but is really out of place there: any
	//   "normal" weapon with fireSubmersed = true should be able to fire out of water
	//   (regardless of submissile which applies only to TorpedoLaunchers) but was not
	//   able to, see #3951
	//
	// land- or air-based launchers cannot target anything not in water
	if (weaponMuzzlePos.y >  0.0f &&                           !TargetInWater(pos, trg))
		return false;
	// water-based launchers cannot target anything not in water unless submissile
	if (weaponMuzzlePos.y <= 0.0f && !weaponDef->submissile && !TargetInWater(pos, trg))
		return false;

	return (CWeapon::TestTarget(pos, trg));
}

void CTorpedoLauncher::FireImpl(const bool scriptCall)
{
	float3 dir = currentTargetPos - weaponMuzzlePos;
	float3 vel;

	const float dist = dir.LengthNormalize();

	if (weaponDef->fixedLauncher) {
		vel = weaponDir * weaponDef->startvelocity;
	} else {
		dir += (UpVector * std::max(0.0f, weaponDef->trajectoryHeight));
		vel = dir.Normalize() * weaponDef->startvelocity;
	}

	ProjectileParams params = GetProjectileParams();
	params.speed = vel;
	params.pos = weaponMuzzlePos;
	params.end = currentTargetPos;
	params.ttl = (weaponDef->flighttime == 0)? math::ceil(std::max(dist, range) / projectileSpeed + 25): weaponDef->flighttime;
	params.tracking = tracking;

	WeaponProjectileFactory::LoadProjectile(params);
}
