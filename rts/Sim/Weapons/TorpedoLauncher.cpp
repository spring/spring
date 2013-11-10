/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TorpedoLauncher.h"
#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"

CR_BIND_DERIVED(CTorpedoLauncher, CWeapon, (NULL, NULL));

CR_REG_METADATA(CTorpedoLauncher,(
	CR_MEMBER(tracking),
	CR_RESERVED(8)
));

CTorpedoLauncher::CTorpedoLauncher(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
	tracking = ((def->tracks)? weaponDef->turnrate: 0);
}


void CTorpedoLauncher::Update()
{
	if (targetType != Target_None) {
		weaponPos = owner->GetObjectSpacePos(relWeaponPos);
		weaponMuzzlePos = owner->GetObjectSpacePos(relWeaponMuzzlePos);

		wantedDir = targetPos - weaponPos;
		predict = wantedDir.LengthNormalize() / projectileSpeed;
	}

	CWeapon::Update();
}

bool CTorpedoLauncher::TestTarget(const float3& pos, bool userTarget, const CUnit* unit) const
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
	if (weaponMuzzlePos.y >  0.0f &&                           !TargetUnitOrPositionInWater(pos, unit))
		return false;
	// water-based launchers cannot target anything not in water unless submissile
	if (weaponMuzzlePos.y <= 0.0f && !weaponDef->submissile && !TargetUnitOrPositionInWater(pos, unit))
		return false;

	return (CWeapon::TestTarget(pos, userTarget, unit));
}

void CTorpedoLauncher::FireImpl(bool scriptCall)
{
	float3 dir = targetPos - weaponMuzzlePos;
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
	params.end = targetPos;
	params.ttl = (weaponDef->flighttime == 0)? std::ceil(std::max(dist, range) / projectileSpeed + 25): weaponDef->flighttime;
	params.tracking = tracking;

	WeaponProjectileFactory::LoadProjectile(params);
}
