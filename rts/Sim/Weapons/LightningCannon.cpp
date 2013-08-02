/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LightningCannon.h"
#include "PlasmaRepulser.h"
#include "WeaponDef.h"
#include "WeaponDefHandler.h"
#include "Game/GameHelper.h"
#include "Game/TraceRay.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Misc/CollisionHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"


CR_BIND_DERIVED(CLightningCannon, CWeapon, (NULL));

CR_REG_METADATA(CLightningCannon,(
	CR_MEMBER(color),
	CR_RESERVED(8)
));

CLightningCannon::CLightningCannon(CUnit* owner): CWeapon(owner)
{
}


void CLightningCannon::Update()
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

		if (!onlyForward) {
			wantedDir = (targetPos - weaponPos).Normalize();
		}
	}

	CWeapon::Update();
}

void CLightningCannon::Init()
{
	CWeapon::Init();
}

void CLightningCannon::FireImpl()
{
	float3 curPos = weaponMuzzlePos;
	float3 curDir = (targetPos - curPos).Normalize();
	float3 newDir = curDir;

	curDir +=
		(gs->randVector() * SprayAngleExperience() + SalvoErrorExperience());
	curDir.Normalize();

	CUnit* hitUnit = NULL;
	CFeature* hitFeature = NULL;
	CPlasmaRepulser* hitShield = NULL;
	CollisionQuery hitColQuery;

	float boltLength = TraceRay::TraceRay(curPos, curDir, range, collisionFlags, owner, hitUnit, hitFeature, &hitColQuery);

	if (!weaponDef->waterweapon) {
		// terminate bolt at water surface if necessary
		if ((curDir.y < 0.0f) && ((curPos.y + curDir.y * boltLength) <= 0.0f)) {
			boltLength = curPos.y / -curDir.y;
		}
	}

	const float shieldLength = interceptHandler.AddShieldInterceptableBeam(this, curPos, curDir, range, newDir, hitShield);

	if (shieldLength < boltLength) {
		boltLength = shieldLength;
		hitShield->BeamIntercepted(this);
	}

	if (hitUnit != NULL) {
		hitUnit->SetLastAttackedPiece(hitColQuery.GetHitPiece(), gs->frameNum);
	}

	const DamageArray& damageArray = (weaponDef->dynDamageExp <= 0.0f)?
		weaponDef->damages:
		weaponDefHandler->DynamicDamages(
			weaponDef->damages,
			weaponMuzzlePos,
			targetPos,
			(weaponDef->dynDamageRange > 0.0f)?
				weaponDef->dynDamageRange:
				weaponDef->range,
			weaponDef->dynDamageExp,
			weaponDef->dynDamageMin,
			weaponDef->dynDamageInverted
		);

	const CGameHelper::ExplosionParams params = {
		curPos + curDir * boltLength,                     // hitPos (same as hitColQuery.GetHitPos() if no water or shield in way)
		curDir,
		damageArray,
		weaponDef,
		owner,
		hitUnit,
		hitFeature,
		craterAreaOfEffect,
		damageAreaOfEffect,
		weaponDef->edgeEffectiveness,
		weaponDef->explosionSpeed,
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
	pparams.ttl = 10;

	WeaponProjectileFactory::LoadProjectile(pparams);
}



void CLightningCannon::SlowUpdate()
{
	CWeapon::SlowUpdate();
}
