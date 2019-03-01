/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rifle.h"
#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Game/GameHelper.h"
#include "Map/Ground.h"
#include "Rendering/Env/Particles/Classes/HeatCloudProjectile.h"
#include "Rendering/Env/Particles/Classes/SmokeProjectile.h"
#include "Rendering/Env/Particles/Classes/TracerProjectile.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/Unit.h"
#include "Sim/Features/Feature.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "System/Sync/SyncTracer.h"
#include "System/SpringMath.h"

CR_BIND_DERIVED(CRifle, CWeapon, )
CR_REG_METADATA(CRifle, )

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


void CRifle::FireImpl(const bool scriptCall)
{
	float3 dir = (currentTargetPos - weaponMuzzlePos).SafeNormalize();
	dir +=
		(gsRNG.NextVector() * SprayAngleExperience() + SalvoErrorExperience());
	dir.Normalize();

	CUnit* hitUnit;
	CFeature* hitFeature;

	const float length = TraceRay::TraceRay(weaponMuzzlePos, dir, range, 0, owner, hitUnit, hitFeature);
	const float impulse = CGameHelper::CalcImpulseScale(*damages, 1.0f);

	if (hitUnit != nullptr) {
		hitUnit->DoDamage(*damages, dir * impulse, owner, weaponDef->id, -1);
		projMemPool.alloc<CHeatCloudProjectile>(owner, weaponMuzzlePos + dir * length, hitUnit->speed * 0.9f, 30, 1);
	} else if (hitFeature != nullptr) {
		hitFeature->DoDamage(*damages, dir * impulse, owner, weaponDef->id, -1);
		projMemPool.alloc<CHeatCloudProjectile>(owner, weaponMuzzlePos + dir * length, hitFeature->speed * 0.9f, 30, 1);
	}

	projMemPool.alloc<CTracerProjectile>(owner, weaponMuzzlePos, dir * projectileSpeed, length);
	projMemPool.alloc<CSmokeProjectile>(owner, weaponMuzzlePos, ZeroVector, 70, 0.1f, 0.02f, 0.6f);
}
