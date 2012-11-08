/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Rifle.h"
#include "WeaponDefHandler.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/Unsynced/HeatCloudProjectile.h"
#include "Sim/Projectiles/Unsynced/SmokeProjectile.h"
#include "Sim/Projectiles/Unsynced/TracerProjectile.h"
#include "Sim/Units/Unit.h"
#include "System/Sync/SyncTracer.h"
#include "System/myMath.h"

CR_BIND_DERIVED(CRifle, CWeapon, (NULL));

CR_REG_METADATA(CRifle,(
	CR_RESERVED(8)
	));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRifle::CRifle(CUnit* owner)
: CWeapon(owner)
{
}

CRifle::~CRifle()
{

}

void CRifle::Update()
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

		wantedDir = (targetPos - weaponPos).Normalize();
	}

	CWeapon::Update();
}

bool CRifle::TryTarget(const float3 &pos, bool userTarget, CUnit* unit)
{
	if (!CWeapon::TryTarget(pos, userTarget, unit))
		return false;

	if (unit) {
		if (unit->isUnderWater)
			return false;
	} else {
		if (pos.y < 0)
			return false;
	}

	float3 dir = pos - weaponMuzzlePos;
	float length = dir.Length();
	if (length == 0)
		return true;

	dir /= length;

	if (!HaveFreeLineOfFire(weaponMuzzlePos, dir, length, unit)) {
		return false;
	}

	const float spread =
		(accuracy + sprayAngle) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight);

	if (avoidFriendly && TraceRay::TestCone(weaponMuzzlePos, dir, length, spread, owner->allyteam, true, false, false, owner)) {
		return false;
	}
	if (avoidNeutral && TraceRay::TestCone(weaponMuzzlePos, dir, length, spread, owner->allyteam, false, true, false, owner)) {
		return false;
	}

	return true;
}

void CRifle::FireImpl()
{
	float3 dir = (targetPos - weaponMuzzlePos).Normalize();
	dir +=
		((gs->randVector() * sprayAngle + salvoError) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight));
	dir.Normalize();

	CUnit* hitUnit;
	CFeature* hitFeature;
	const float length = TraceRay::TraceRay(weaponMuzzlePos, dir, range, 0, owner, hitUnit, hitFeature);

	if (hitUnit) {
		hitUnit->DoDamage(weaponDef->damages, ZeroVector, owner, weaponDef->id);
		new CHeatCloudProjectile(weaponMuzzlePos + dir*length, hitUnit->speed * 0.9f, 30, 1, owner);
	}

	new CTracerProjectile(weaponMuzzlePos, dir*projectileSpeed, length, owner);
	new CSmokeProjectile(weaponMuzzlePos, float3(0,0,0), 70, 0.1f, 0.02f, owner, 0.6f);
}
