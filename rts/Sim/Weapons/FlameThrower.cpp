/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FlameThrower.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/FlameProjectile.h"
#include "Sim/Units/Unit.h"
#include "WeaponDefHandler.h"
#include "System/mmgr.h"

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

CFlameThrower::~CFlameThrower(void)
{
}

void CFlameThrower::FireImpl(void)
{
	const float3 dir = (targetPos - weaponMuzzlePos).Normalize();
	const float3 spread =
		((gs->randVector() * sprayAngle + salvoError) *
		weaponDef->ownerExpAccWeight) -
		(dir * 0.001f);

	new CFlameProjectile(weaponMuzzlePos, dir * projectileSpeed,
		spread, owner, weaponDef, (int) (range / projectileSpeed * weaponDef->duration));
}

bool CFlameThrower::TryTarget(const float3 &pos, bool userTarget, CUnit* unit)
{
	if (!CWeapon::TryTarget(pos, userTarget, unit))
		return false;

	if (!weaponDef->waterweapon && TargetUnitOrPositionInWater(pos, unit))
		return false;

	float3 dir(pos - weaponMuzzlePos);
	float length = dir.Length();
	if (length == 0)
		return true;

	dir /= length;

	if (!HaveFreeLineOfFire(weaponMuzzlePos, dir, length, unit)) {
		return false;
	}

	if (avoidFeature && TraceRay::LineFeatureCol(weaponMuzzlePos, dir, length)) {
		return false;
	}
	if (avoidFriendly && TraceRay::TestCone(weaponMuzzlePos, dir, length, (accuracy + sprayAngle), owner->allyteam, true, false, false, owner)) {
		return false;
	}
	if (avoidNeutral && TraceRay::TestCone(weaponMuzzlePos, dir, length, (accuracy + sprayAngle), owner->allyteam, false, true, false, owner)) {
		return false;
	}

	return true;
}

void CFlameThrower::Update(void)
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
