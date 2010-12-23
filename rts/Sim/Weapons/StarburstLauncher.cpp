/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Game/GameHelper.h"
#include "Map/Ground.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Projectiles/WeaponProjectiles/StarburstProjectile.h"
#include "Sim/Units/Unit.h"
#include "StarburstLauncher.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"

CR_BIND_DERIVED(CStarburstLauncher, CWeapon, (NULL));

CR_REG_METADATA(CStarburstLauncher,(
	CR_MEMBER(uptime),
	CR_MEMBER(tracking),
	CR_RESERVED(8)
	));

CStarburstLauncher::CStarburstLauncher(CUnit* owner)
: CWeapon(owner),
	tracking(0),
	uptime(3)
{
}

CStarburstLauncher::~CStarburstLauncher(void)
{
}

void CStarburstLauncher::Update(void)
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
		wantedDir=(targetPos-weaponPos).Normalize();		//the aiming upward is apperently implicid so aim toward target
	}
	CWeapon::Update();
}

void CStarburstLauncher::FireImpl()
{
	float3 speed(0,weaponDef->startvelocity,0);
	float maxrange;
	if (weaponDef->fixedLauncher) {
		speed = weaponDir * weaponDef->startvelocity;
		maxrange = (float)MAX_WORLD_SIZE;
	} else if (weaponDef->flighttime > 0) {
		maxrange = (float)MAX_WORLD_SIZE;
	} else {
		maxrange = (float)range;
	}

	const float3 aimError =
		(gs->randVector() * sprayAngle + salvoError) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight);

	CStarburstProjectile* p =
		new CStarburstProjectile(weaponMuzzlePos + float3(0, 2, 0), speed, owner,
		targetPos, areaOfEffect, projectileSpeed, tracking, (int) uptime, targetUnit,
		weaponDef, interceptTarget, maxrange, aimError);

	if (weaponDef->targetable)
		interceptHandler.AddInterceptTarget(p, targetPos);
}

bool CStarburstLauncher::TryTarget(const float3& pos, bool userTarget, CUnit* unit)
{
	if (!CWeapon::TryTarget(pos, userTarget, unit))
		return false;

	if (unit) {
		if (unit->isUnderWater && !weaponDef->waterweapon)
			return false;
	} else {
		if (pos.y < 0 && !weaponDef->waterweapon)
			return false;
	}

	if (avoidFriendly && helper->TestCone(weaponMuzzlePos,
		(weaponDef->fixedLauncher? weaponDir: UpVector), 100, 0, owner, CGameHelper::TEST_ALLIED)) {
		return false;
	}
	if (avoidNeutral && helper->TestCone(weaponMuzzlePos,
		(weaponDef->fixedLauncher? weaponDir: UpVector), 100, 0, owner, CGameHelper::TEST_NEUTRAL)) {
		return false;
	}

	return true;
}

float CStarburstLauncher::GetRange2D(float yDiff) const
{
	return range + (yDiff * heightMod);
}
