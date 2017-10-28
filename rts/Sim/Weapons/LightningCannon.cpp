/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LightningCannon.h"
#include "PlasmaRepulser.h"
#include "WeaponDef.h"
#include "Game/GameHelper.h"
#include "Game/TraceRay.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"

#include <vector>

CR_BIND_DERIVED(CLightningCannon, CWeapon, )
CR_REG_METADATA(CLightningCannon, (
	CR_MEMBER(color)
))

CLightningCannon::CLightningCannon(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
	// null happens when loading
	if (def != nullptr)
		color = def->visuals.color;
}

float CLightningCannon::GetPredictedImpactTime(float3 p) const
{
	return 0;
}

void CLightningCannon::FireImpl(const bool scriptCall)
{
	float3 curPos = weaponMuzzlePos;
	float3 curDir = (currentTargetPos - weaponMuzzlePos).SafeNormalize();

	curDir +=
		(gsRNG.NextVector() * SprayAngleExperience() + SalvoErrorExperience());
	curDir.Normalize();

	CUnit* hitUnit = nullptr;
	CFeature* hitFeature = nullptr;
	CollisionQuery hitColQuery;

	float boltLength = TraceRay::TraceRay(curPos, curDir, range, collisionFlags, owner, hitUnit, hitFeature, &hitColQuery);

	if (!weaponDef->waterweapon) {
		// terminate bolt at water surface if necessary
		if ((curDir.y < 0.0f) && ((curPos.y + curDir.y * boltLength) <= 0.0f)) {
			boltLength = curPos.y / -curDir.y;
			hitUnit = nullptr;
			hitFeature = nullptr;
		}
	}

	static std::vector<TraceRay::SShieldDist> hitShields;
	hitShields.clear();
	TraceRay::TraceRayShields(this, curPos, curDir, range, hitShields);
	for (const TraceRay::SShieldDist& sd: hitShields) {
		if (sd.dist < boltLength && sd.rep->IncomingBeam(this, curPos, curPos + (curDir * sd.dist), 1.0f)) {
			boltLength = sd.dist;
			hitUnit = nullptr;
			hitFeature = nullptr;
			break;
		}
	}

	if (hitUnit != nullptr)
		hitUnit->SetLastHitPiece(hitColQuery.GetHitPiece(), gs->frameNum);

	const DamageArray& damageArray = damages->GetDynamicDamages(weaponMuzzlePos, currentTargetPos);
	const CExplosionParams params = {
		curPos + curDir * boltLength,                     // hitPos (same as hitColQuery.GetHitPos() if no water or shield in way)
		curDir,
		damageArray,
		weaponDef,
		owner,
		hitUnit,
		hitFeature,
		damages->craterAreaOfEffect,
		damages->damageAreaOfEffect,
		damages->edgeEffectiveness,
		damages->explosionSpeed,
		0.5f,                                             // gfxMod
		weaponDef->impactOnly,
		weaponDef->noExplode || weaponDef->noSelfDamage,  // ignoreOwner
		false,                                            // damageGround
		-1u                                               // projectileID
	};

	helper->Explosion(params);

	ProjectileParams pparams = GetProjectileParams();
	pparams.pos = curPos;
	pparams.end = curPos + curDir * (boltLength + 10.0f);
	pparams.ttl = weaponDef->beamLaserTTL;

	WeaponProjectileFactory::LoadProjectile(pparams);
}

