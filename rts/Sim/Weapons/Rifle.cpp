/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rifle.h"
#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/Unsynced/HeatCloudProjectile.h"
#include "Sim/Projectiles/Unsynced/SmokeProjectile.h"
#include "Sim/Projectiles/Unsynced/TracerProjectile.h"
#include "Sim/Units/Unit.h"
#include "System/Sync/SyncTracer.h"
#include "System/myMath.h"

CR_BIND_DERIVED(CRifle, CWeapon, (NULL, NULL));

CR_REG_METADATA(CRifle,(
	CR_RESERVED(8)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRifle::CRifle(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
}


void CRifle::Update()
{
	if (targetType != Target_None) {
		weaponPos = owner->GetObjectSpacePos(relWeaponPos);
		weaponMuzzlePos = owner->GetObjectSpacePos(relWeaponMuzzlePos);

		wantedDir = (targetPos - weaponPos).Normalize();
	}

	CWeapon::Update();
}


void CRifle::FireImpl(bool scriptCall)
{
	float3 dir = (targetPos - weaponMuzzlePos).Normalize();
	dir +=
		(gs->randVector() * SprayAngleExperience() + SalvoErrorExperience());
	dir.Normalize();

	CUnit* hitUnit;
	CFeature* hitFeature;
	const float length = TraceRay::TraceRay(weaponMuzzlePos, dir, range, 0, owner, hitUnit, hitFeature);

	if (hitUnit) {
		hitUnit->DoDamage(weaponDef->damages, ZeroVector, owner, weaponDef->id, -1);
		new CHeatCloudProjectile(weaponMuzzlePos + dir*length, hitUnit->speed * 0.9f, 30, 1, owner);
	}

	new CTracerProjectile(weaponMuzzlePos, dir*projectileSpeed, length, owner);
	new CSmokeProjectile(weaponMuzzlePos, ZeroVector, 70, 0.1f, 0.02f, owner, 0.6f);
}
