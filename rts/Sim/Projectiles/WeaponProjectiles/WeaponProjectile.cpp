/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"

#include "Game/GameHelper.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Rendering/Colors.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Models/IModelParser.h"
#include "Map/Ground.h"
#include "System/Matrix44f.h"
#include "System/Sound/SoundChannels.h"
#ifdef TRACE_SYNC
	#include "System/Sync/SyncTracer.h"
#endif



CR_BIND_DERIVED(CWeaponProjectile, CProjectile, );

CR_REG_METADATA(CWeaponProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(targeted),
//	CR_MEMBER(weaponDef),
	CR_MEMBER(target),
	CR_MEMBER(targetPos),
	CR_MEMBER(startpos),
	CR_MEMBER(colorTeam),
	CR_MEMBER(ttl),
	CR_MEMBER(bounces),
	CR_MEMBER(keepBouncing),
	CR_MEMBER(weaponDefID),
	CR_MEMBER(cegID),
	CR_MEMBER(interceptTarget),
	CR_RESERVED(15),
	CR_POSTLOAD(PostLoad)
));

CWeaponProjectile::CWeaponProjectile(): CProjectile()
{
	targeted = false;
	weaponDef = NULL;
	target = NULL;
	projectileType = WEAPON_BASE_PROJECTILE;
	ttl = 0;
	colorTeam = 0;
	interceptTarget = NULL;
	bounces = 0;
	keepBouncing = true;
	cegID = -1U;
}

CWeaponProjectile::CWeaponProjectile(
	const float3& pos,
	const float3& speed,
	CUnit* owner,
	CUnit* target,
	const float3& targetPos,
	const WeaponDef* weaponDef,
	CWeaponProjectile* interceptTarget,
	int ttl
):
	CProjectile(pos, speed, owner, true, true, false),
	targeted(false),
	weaponDef(weaponDef),
	target(target),
	targetPos(targetPos),
	weaponDefID(weaponDef? weaponDef->id: -1U),
	cegID(-1U),
	colorTeam(0),
	startpos(pos),
	ttl(ttl),
	bounces(0),
	keepBouncing(true),
	interceptTarget(interceptTarget)
{
	projectileType = WEAPON_BASE_PROJECTILE;

	if (owner) {
		colorTeam = owner->team;
	}

	if (target) {
		AddDeathDependence(target, DEPENDENCE_WEAPONTARGET);
	}

	if (interceptTarget) {
		interceptTarget->targeted = true;
		AddDeathDependence(interceptTarget, DEPENDENCE_INTERCEPTTARGET);
	}

	assert(weaponDef != NULL);

	alwaysVisible = weaponDef->visuals.alwaysVisible;
	ignoreWater = weaponDef->waterweapon;

	model = weaponDef->LoadModel();

	collisionFlags = weaponDef->collisionFlags;

	ph->AddProjectile(this);
	ASSERT_SYNCED(id);

	if (weaponDef->interceptedByShieldType) {
		// this needs a valid projectile id set
		assert(id >= 0);
		interceptHandler.AddShieldInterceptableProjectile(this);
	}
}



void CWeaponProjectile::Collision()
{
	Collision((CFeature*)NULL);
}

void CWeaponProjectile::Collision(CFeature* feature)
{
	if (feature && (gs->randFloat() < weaponDef->fireStarter)) {
		feature->StartFire();
	}

	{
		float3 impactDir = speed;

		const DamageArray& damageArray = (weaponDef->dynDamageExp <= 0.0f)?
			weaponDef->damages:
			weaponDefHandler->DynamicDamages(
				weaponDef->damages,
				startpos,
				pos,
				(weaponDef->dynDamageRange > 0.0f)?
					weaponDef->dynDamageRange:
					weaponDef->range,
				weaponDef->dynDamageExp, weaponDef->dynDamageMin,
				weaponDef->dynDamageInverted
			);

		const CGameHelper::ExplosionParams params = {
			pos,
			impactDir.SafeNormalize(),
			damageArray,
			weaponDef,
			owner(),
			NULL,                                             // hitUnit
			feature,
			weaponDef->craterAreaOfEffect,
			weaponDef->damageAreaOfEffect,
			weaponDef->edgeEffectiveness,
			weaponDef->explosionSpeed,
			weaponDef->noExplode? 0.3f: 1.0f,                 // gfxMod
			weaponDef->impactOnly,
			weaponDef->noExplode || weaponDef->noSelfDamage,  // ignoreOwner
			true                                              // damgeGround
		};

		helper->Explosion(params);
	}

	if (!weaponDef->noExplode) {
		CProjectile::Collision();
	} else {
		if (TraveledRange()) {
			CProjectile::Collision();
		}
	}
}

void CWeaponProjectile::Collision(CUnit* unit)
{
	{
		float3 impactDir = speed;

		const DamageArray& damageArray = (weaponDef->dynDamageExp <= 0.0f)?
			weaponDef->damages:
			weaponDefHandler->DynamicDamages(
				weaponDef->damages,
				startpos,
				pos,
				(weaponDef->dynDamageRange > 0.0f)?
					weaponDef->dynDamageRange:
					weaponDef->range,
				weaponDef->dynDamageExp, weaponDef->dynDamageMin,
				weaponDef->dynDamageInverted
			);

		const CGameHelper::ExplosionParams params = {
			pos,
			impactDir.SafeNormalize(),
			damageArray,
			weaponDef,
			owner(),
			unit,
			NULL,                                            // hitFeature
			weaponDef->craterAreaOfEffect,
			weaponDef->damageAreaOfEffect,
			weaponDef->edgeEffectiveness,
			weaponDef->explosionSpeed,
			weaponDef->noExplode? 0.3f: 1.0f,                 // gfxMod
			weaponDef->impactOnly,
			weaponDef->noExplode || weaponDef->noSelfDamage,  // ignoreOwner
			true                                              // damageGround
		};

		helper->Explosion(params);
	}

	if (!weaponDef->noExplode) {
		CProjectile::Collision(unit);
	} else {
		if (TraveledRange())
			CProjectile::Collision();
	}
}

void CWeaponProjectile::Update()
{
	CProjectile::Update();
	UpdateGroundBounce();
}

void CWeaponProjectile::UpdateGroundBounce()
{
	if (luaMoveCtrl) {
		return;
	}

	if ((weaponDef->groundBounce || weaponDef->waterBounce)
			&& (ttl > 0 || weaponDef->numBounce >= 0))
	{
		float wh = 0;

		if (!weaponDef->waterBounce) {
			wh = ground->GetHeightReal(pos.x, pos.z);
		} else if (weaponDef->groundBounce) {
			wh = ground->GetHeightAboveWater(pos.x, pos.z);
		}

		if (pos.y < wh) {
			bounces++;

			if (!keepBouncing || (this->weaponDef->numBounce >= 0
							&& bounces > this->weaponDef->numBounce)) {
				//Collision();
				keepBouncing = false;
			} else {
				const float3& normal = ground->GetNormal(pos.x, pos.z);
				const float dot = speed.dot(normal);

				pos -= speed;
				speed -= (speed + normal * math::fabs(dot)) * (1 - weaponDef->bounceSlip);
				speed += (normal * (math::fabs(dot))) * (1 + weaponDef->bounceRebound);
				pos += speed;

				if (weaponDef->bounceExplosionGenerator) {
					weaponDef->bounceExplosionGenerator->Explosion(
						-1U, pos, speed.Length(), 1, owner(), 1, NULL, normal
					);
				}
			}
		}
	}

}

bool CWeaponProjectile::TraveledRange()
{
	float trangeSq = (pos - startpos).SqLength();
	return (trangeSq > (weaponDef->range * weaponDef->range));
}



void CWeaponProjectile::DrawOnMinimap(CVertexArray& lines, CVertexArray& points)
{
	points.AddVertexQC(pos, color4::yellow);
}

void CWeaponProjectile::DependentDied(CObject* o)
{
	if (o == interceptTarget) {
		interceptTarget = NULL;
	}

	if (o == target) {
		target = NULL;
	}
}

void CWeaponProjectile::PostLoad()
{
	weaponDef = weaponDefHandler->GetWeaponById(weaponDefID);

//	if(weaponDef->interceptedByShieldType)
//		interceptHandler.AddShieldInterceptableProjectile(this);

	model = weaponDef->LoadModel();

//	collisionFlags = weaponDef->collisionFlags;
}
