/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Game/GameHelper.h"
#include "LightningCannon.h"
#include "Map/Ground.h"
#include "PlasmaRepulser.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Projectiles/WeaponProjectiles/LightningProjectile.h"
#include "Sim/Units/Unit.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"

CR_BIND_DERIVED(CLightningCannon, CWeapon, (NULL));

CR_REG_METADATA(CLightningCannon,(
	CR_MEMBER(color),
	CR_RESERVED(8)
	));

CLightningCannon::CLightningCannon(CUnit* owner)
: CWeapon(owner)
{
}

CLightningCannon::~CLightningCannon(void)
{
}

void CLightningCannon::Update(void)
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
		if(!onlyForward){
			wantedDir=targetPos-weaponPos;
			wantedDir.Normalize();
		}
	}
	CWeapon::Update();
}

bool CLightningCannon::TryTarget(const float3& pos, bool userTarget, CUnit* unit)
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

	float3 dir = pos - weaponMuzzlePos;
	float length = dir.Length();
	if (length == 0)
		return true;

	dir /= length;

	float g = ground->LineGroundCol(weaponMuzzlePos, pos);
	if (g > 0 && g < length * 0.9f)
		return false;

	if (avoidFeature && helper->LineFeatureCol(weaponMuzzlePos, dir, length)) {
		return false;
	}
	if (avoidFriendly && helper->TestAllyCone(weaponMuzzlePos, dir, length, (accuracy + sprayAngle), owner->allyteam, owner)) {
		return false;
	}
	if (avoidNeutral && helper->TestNeutralCone(weaponMuzzlePos, dir, length, (accuracy + sprayAngle), owner)) {
		return false;
	}

	return true;
}

void CLightningCannon::Init(void)
{
	CWeapon::Init();
}

void CLightningCannon::FireImpl()
{
	float3 dir(targetPos - weaponMuzzlePos);
	dir.Normalize();
	dir +=
		(gs->randVector() * sprayAngle + salvoError) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight);
	dir.Normalize();

	const CUnit* cu = 0;
	const CFeature* cf = 0;
	float r = helper->TraceRay(weaponMuzzlePos, dir, range, 0, (const CUnit*)owner, cu, collisionFlags, &cf);
	CUnit* u = (cu == NULL) ? NULL : uh->units[cu->id];
	CFeature* f = cf ? featureHandler->GetFeature(cf->id) : 0;


	float3 newDir;
	CPlasmaRepulser* shieldHit = NULL;
	const float shieldLength = interceptHandler.AddShieldInterceptableBeam(this, weaponMuzzlePos, dir, range, newDir, shieldHit);

	if (shieldLength < r) {
		r = shieldLength;
		if (shieldHit) {
			shieldHit->BeamIntercepted(this);
		}
	}

	if (u) {
		if (u->unitDef->usePieceCollisionVolumes) {
			u->SetLastAttackedPiece(u->localmodel->pieces[0], gs->frameNum);
		}
	}

	// Dynamic Damage
	DamageArray dynDamages;
	if (weaponDef->dynDamageExp > 0) {
		dynDamages = weaponDefHandler->DynamicDamages(
			weaponDef->damages,
			weaponMuzzlePos,
			targetPos,
			weaponDef->dynDamageRange > 0?
				weaponDef->dynDamageRange:
				weaponDef->range,
			weaponDef->dynDamageExp,
			weaponDef->dynDamageMin,
			weaponDef->dynDamageInverted
		);
	}

	helper->Explosion(
		weaponMuzzlePos + dir * r,
		weaponDef->dynDamageExp > 0?
			dynDamages:
			weaponDef->damages,
		areaOfEffect,
		weaponDef->edgeEffectiveness,
		weaponDef->explosionSpeed,
		owner,
		false,
		0.5f,
		weaponDef->noExplode || weaponDef->noSelfDamage, /*true*/
		weaponDef->impactOnly,                           /*false*/
		weaponDef->explosionGenerator,
		u,
		dir,
		weaponDef->id,
		f
	);

	new CLightningProjectile(
		weaponMuzzlePos,
		weaponMuzzlePos + dir * (r + 10),
		owner,
		color,
		weaponDef,
		10,
		this
	);
}



void CLightningCannon::SlowUpdate(void)
{
	CWeapon::SlowUpdate();
}
