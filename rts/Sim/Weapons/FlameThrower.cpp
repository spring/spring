/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FlameThrower.h"
#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"

CR_BIND_DERIVED(CFlameThrower, CWeapon, (NULL, NULL))

CR_REG_METADATA(CFlameThrower,(
	CR_MEMBER(color),
	CR_MEMBER(color2),
	CR_RESERVED(8)
))

CFlameThrower::CFlameThrower(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
}


void CFlameThrower::FireImpl(bool scriptCall)
{
	float3 dir = targetPos - weaponMuzzlePos;

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


void CFlameThrower::Update()
{
	if (targetType != Target_None) {
		weaponPos = owner->GetObjectSpacePos(relWeaponPos);
		weaponMuzzlePos = owner->GetObjectSpacePos(relWeaponMuzzlePos);
		wantedDir = (targetPos - weaponPos).Normalize();
	}

	CWeapon::Update();
}
