/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TorpedoLauncher.h"
#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"

CR_BIND_DERIVED(CTorpedoLauncher, CWeapon, (NULL));

CR_REG_METADATA(CTorpedoLauncher,(
	CR_MEMBER(tracking),
	CR_RESERVED(8)
));

CTorpedoLauncher::CTorpedoLauncher(CUnit* owner)
: CWeapon(owner),
	tracking(0)
{
}


void CTorpedoLauncher::Update()
{
	if (targetType != Target_None) {
		weaponPos = owner->pos +
			owner->frontdir * relWeaponPos.z +
			owner->updir    * relWeaponPos.y +
			owner->rightdir * relWeaponPos.x;
		weaponMuzzlePos = owner->pos +
			owner->frontdir * relWeaponMuzzlePos.z +
			owner->updir    * relWeaponMuzzlePos.y +
			owner->rightdir * relWeaponMuzzlePos.x;

		wantedDir = targetPos - weaponPos;
		const float dist = wantedDir.Length();
		predict = dist / projectileSpeed;
		wantedDir /= dist;
	}

	CWeapon::Update();
}

bool CTorpedoLauncher::TestTarget(const float3& pos, bool userTarget, const CUnit* unit) const
{
	// NOTE: only here for #3557
	if (!TargetUnitOrPositionInWater(pos, unit))
		return false;

	return (CWeapon::TestTarget(pos, userTarget, unit));
}

void CTorpedoLauncher::FireImpl()
{
	float3 dir = targetPos - weaponMuzzlePos;
	const float dist = dir.Length();
	dir /= dist;

	if (weaponDef->trajectoryHeight > 0) {
		dir.y += weaponDef->trajectoryHeight;
		dir.Normalize();
	}

	ProjectileParams params = GetProjectileParams();
	params.speed = ((weaponDef->fixedLauncher)? weaponDir: dir) * weaponDef->startvelocity;
	params.pos = weaponMuzzlePos;
	params.end = targetPos;
	params.ttl = (weaponDef->flighttime == 0)? std::ceil(std::max(dist, range) / projectileSpeed + 25): weaponDef->flighttime;
	params.tracking = tracking;

	WeaponProjectileFactory::LoadProjectile(params);
}
