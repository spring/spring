/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "FlameThrower.h"
#include "Game/GameHelper.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/FlameProjectile.h"
#include "Sim/Units/Unit.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"

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

	if (!weaponDef->waterweapon) {
		if (unit) {
			if (unit->isUnderWater)
				return false;
		} else {
			if (pos.y < 0)
				return false;
		}
	}

	float3 dir(pos - weaponMuzzlePos);
	float length = dir.Length();
	if (length == 0)
		return true;

	dir /= length;

	const float g = ground->LineGroundCol(weaponMuzzlePos, pos);
	if (g > 0 && g < length * 0.9f)
		return false;

	if (avoidFeature && helper->LineFeatureCol(weaponMuzzlePos, dir, length)) {
		return false;
	}
	if (avoidFriendly && helper->TestCone(weaponMuzzlePos, dir, length, (accuracy + sprayAngle), owner, CGameHelper::TEST_ALLIED)) {
		return false;
	}
	if (avoidNeutral && helper->TestCone(weaponMuzzlePos, dir, length, (accuracy + sprayAngle), owner, CGameHelper::TEST_NEUTRAL)) {
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
