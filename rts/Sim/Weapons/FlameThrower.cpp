/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FlameThrower.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/FlameProjectile.h"
#include "Sim/Units/Unit.h"
#include "WeaponDefHandler.h"

CR_BIND_DERIVED(CFlameThrower, CWeapon, (NULL));

CR_REG_METADATA(CFlameThrower,(
	CR_MEMBER(color),
	CR_MEMBER(color2),
	CR_RESERVED(8)
	));

CFlameThrower::CFlameThrower(CUnit* owner)
: CWeapon(owner)
{
}


void CFlameThrower::FireImpl()
{
	const float3 dir = (targetPos - weaponMuzzlePos).Normalize();
	const float3 spread =
		((gs->randVector() * sprayAngle + salvoError) *
		weaponDef->ownerExpAccWeight) -
		(dir * 0.001f);

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.speed = dir * projectileSpeed;
	params.ttl = (int) (range / projectileSpeed * weaponDef->duration);
	new CFlameProjectile(params, spread);
}


void CFlameThrower::Update()
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
		wantedDir = targetPos - weaponPos;
		wantedDir.Normalize();
	}
	CWeapon::Update();
}
