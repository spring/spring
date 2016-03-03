/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rifle.h"
#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Game/GameHelper.h"
#include "Map/Ground.h"
#include "Rendering/Env/Particles/Classes/HeatCloudProjectile.h"
#include "Rendering/Env/Particles/Classes/SmokeProjectile.h"
#include "Rendering/Env/Particles/Classes/TracerProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"
#include "System/Sync/SyncTracer.h"
#include "System/myMath.h"

CR_BIND_DERIVED(CRifle, CWeapon, (NULL, NULL))
CR_REG_METADATA(CRifle, )

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRifle::CRifle(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
}

float CRifle::GetPredictedImpactTime(float3 p) const
{
	return 0;
}

void CRifle::FireImpl(const bool scriptCall)
{
	float3 dir = (currentTargetPos - weaponMuzzlePos).SafeNormalize();
	dir +=
		(gs->randVector() * SprayAngleExperience() + SalvoErrorExperience());
	dir.Normalize();

	CUnit* hitUnit;
	CFeature* hitFeature;

	const float length = TraceRay::TraceRay(weaponMuzzlePos, dir, range, 0, owner, hitUnit, hitFeature);
	const float impulse = CGameHelper::CalcImpulseScale(*damages, 1.0f);

	if (hitUnit != NULL) {
		hitUnit->DoDamage(*damages, dir * impulse, owner, weaponDef->id, -1);
		new CHeatCloudProjectile(owner, weaponMuzzlePos + dir * length, hitUnit->speed * 0.9f, 30, 1);
	}else if (hitFeature != NULL) {
		hitFeature->DoDamage(*damages, dir * impulse, owner, weaponDef->id, -1);
		new CHeatCloudProjectile(owner, weaponMuzzlePos + dir * length, hitFeature->speed * 0.9f, 30, 1);
	}

	new CTracerProjectile(owner, weaponMuzzlePos, dir * projectileSpeed, length);
	new CSmokeProjectile(owner, weaponMuzzlePos, ZeroVector, 70, 0.1f, 0.02f, 0.6f);
}
