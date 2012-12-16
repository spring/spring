/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LightningCannon.h"
#include "WeaponDefHandler.h"
#include "Game/GameHelper.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "PlasmaRepulser.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Projectiles/WeaponProjectiles/LightningProjectile.h"
#include "Sim/Units/Unit.h"


CR_BIND_DERIVED(CLightningCannon, CWeapon, (NULL));

CR_REG_METADATA(CLightningCannon,(
	CR_MEMBER(color),
	CR_RESERVED(8)
	));

CLightningCannon::CLightningCannon(CUnit* owner)
: CWeapon(owner)
{
}

CLightningCannon::~CLightningCannon()
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

bool CLightningCannon::TryTarget(const float3& pos, bool userTarget, CUnit* unit)
{
	if (!CWeapon::TryTarget(pos, userTarget, unit))
		return false;

	if (!weaponDef->waterweapon && TargetUnitOrPositionInWater(pos, unit))
		return false;

	float3 dir = pos - weaponMuzzlePos;
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

void CLightningCannon::Init()
{
	CWeapon::Init();
}

void CLightningCannon::FireImpl()
{
	float3 curPos = weaponMuzzlePos;
	float3 hitPos;
	float3 curDir = (targetPos - curPos).Normalize();
	float3 newDir = curDir;

	curDir +=
		(gs->randVector() * sprayAngle + salvoError) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight);
	curDir.Normalize();

	CUnit* hitUnit = NULL;
	CFeature* hitFeature = NULL;
	CPlasmaRepulser* hitShield = NULL;

	float boltLength = TraceRay::TraceRay(curPos, curDir, range, collisionFlags, owner, hitUnit, hitFeature);

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

	hitPos = curPos + curDir * boltLength;
	// NOTE: we still need the old position and direction for the projectile
	// curPos = hitPos;
	// curDir = newDir;

	if (hitUnit != NULL) {
		hitUnit->SetLastAttackedPiece(hitUnit->localModel->GetRoot(), gs->frameNum);
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
		hitPos,
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
		false                                             // damageGround
	};

	helper->Explosion(params);

	ProjectileParams pparams = GetProjectileParams();
	pparams.pos = curPos;
	pparams.end = curPos + curDir * (boltLength + 10.0f);
	pparams.ttl = 10;
	new CLightningProjectile(pparams, color);
}



void CLightningCannon::SlowUpdate()
{
	CWeapon::SlowUpdate();
}
