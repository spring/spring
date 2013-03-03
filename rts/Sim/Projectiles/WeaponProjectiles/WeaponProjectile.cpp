/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Game/GameHelper.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Rendering/Colors.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Models/IModelParser.h"
#include "Map/Ground.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
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
	bounces = 0;
	keepBouncing = true;
	weaponDefID = 0;
	cegID = -1U;
}


CWeaponProjectile::CWeaponProjectile(const ProjectileParams& params, const bool isRay)
	: CProjectile(params.pos, params.speed, params.owner, true, true, false)
	, targeted(false)
	, weaponDef(params.weaponDef)
	, target(params.target)
	, targetPos(params.end)
	, weaponDefID(params.weaponDef? params.weaponDef->id: -1U)
	, cegID(-1U)
	, colorTeam(0)
	, startpos(params.pos)
	, ttl(params.ttl)
	, bounces(0)
	, keepBouncing(true)
{
	assert(weaponDef != NULL);

	if (isRay) {
		pos = (startpos + targetPos) * 0.5f;
		//pos = startpos;
		//speed = (targetPos - startpos) + weaponDef->collisionSize; //FIXME move to CProjectileHandler::Check...Collision?
		drawRadius = startpos.distance(targetPos);
	}
	
	projectileType = WEAPON_BASE_PROJECTILE;

	if (params.owner) {
		colorTeam = params.owner->team;
	}

	CSolidObject* so = dynamic_cast<CSolidObject*>(target);
	if (so) {
		AddDeathDependence(so, DEPENDENCE_WEAPONTARGET);
	}

	CWeaponProjectile* po = dynamic_cast<CWeaponProjectile*>(target);
	if (po) {
		if (weaponDef->interceptSolo)
			po->targeted = true;
		AddDeathDependence(po, DEPENDENCE_INTERCEPTTARGET);
	}

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

	if (weaponDef->targetable)
		interceptHandler.AddInterceptTarget(this, targetPos);
}


CWeaponProjectile::~CWeaponProjectile()
{
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
		float3 impactPos = pos;
		float3 impactDir = speed;

		switch(projectileType) {
			case WEAPON_BEAMLASER_PROJECTILE:
			case WEAPON_LARGEBEAMLASER_PROJECTILE:
			case WEAPON_LIGHTNING_PROJECTILE:
			{
				impactPos = feature ? feature->pos : targetPos;
				impactDir = (targetPos - startpos);
			} break;
		}

		const DamageArray& damageArray = (weaponDef->dynDamageExp <= 0.0f)?
			weaponDef->damages:
			weaponDefHandler->DynamicDamages(
				weaponDef->damages,
				startpos,
				impactPos,
				(weaponDef->dynDamageRange > 0.0f)?
					weaponDef->dynDamageRange:
					weaponDef->range,
				weaponDef->dynDamageExp, weaponDef->dynDamageMin,
				weaponDef->dynDamageInverted
			);

		const CGameHelper::ExplosionParams params = {
			impactPos,
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

	if (!weaponDef->noExplode || TraveledRange()) {
		CProjectile::Collision();
	}
}

void CWeaponProjectile::Collision(CUnit* unit)
{
	{
		float3 impactPos = pos;
		float3 impactDir = speed;

		switch(projectileType) {
			case WEAPON_BEAMLASER_PROJECTILE:
			case WEAPON_LARGEBEAMLASER_PROJECTILE:
			case WEAPON_LIGHTNING_PROJECTILE:
			{
				impactPos = unit->pos;
				impactDir = (targetPos - startpos);
			} break;
		}

		const DamageArray& damageArray = (weaponDef->dynDamageExp <= 0.0f)?
			weaponDef->damages:
			weaponDefHandler->DynamicDamages(
				weaponDef->damages,
				startpos,
				impactPos,
				(weaponDef->dynDamageRange > 0.0f)?
					weaponDef->dynDamageRange:
					weaponDef->range,
				weaponDef->dynDamageExp, weaponDef->dynDamageMin,
				weaponDef->dynDamageInverted
			);

		const CGameHelper::ExplosionParams params = {
			impactPos,
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

	if (!weaponDef->noExplode || TraveledRange()) {
		CProjectile::Collision();
	}
}

void CWeaponProjectile::Update()
{
	CProjectile::Update();
	UpdateGroundBounce();
	UpdateInterception();
}


void CWeaponProjectile::UpdateInterception()
{
	if (!target)
		return;

	CWeaponProjectile* po = dynamic_cast<CWeaponProjectile*>(target);
	if (!po)
		return;

	switch(projectileType) {
		case WEAPON_BEAMLASER_PROJECTILE:
		case WEAPON_LARGEBEAMLASER_PROJECTILE:
		case WEAPON_LIGHTNING_PROJECTILE:
		{
			if (ClosestPointOnLine(startpos, targetPos, po->pos).SqDistance(po->pos) < Square(weaponDef->collisionSize)) {
				po->Collision();
				Collision();
				return;
			}
		} break;

		case WEAPON_LASER_PROJECTILE: // important!
		case WEAPON_BASE_PROJECTILE:
		case WEAPON_EMG_PROJECTILE:
		case WEAPON_EXPLOSIVE_PROJECTILE:
		case WEAPON_FIREBALL_PROJECTILE:
		case WEAPON_FLAME_PROJECTILE:
		case WEAPON_MISSILE_PROJECTILE:
		case WEAPON_STARBURST_PROJECTILE:
		case WEAPON_TORPEDO_PROJECTILE:
		default:
			if (pos.SqDistance(po->pos) < Square(weaponDef->damageAreaOfEffect)) {
			//FIXME if (pos.SqDistance(po->pos) < Square(weaponDef->collisionSize)) {
				po->Collision();
				Collision();
			}
	}
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
	if (o == target) {
		target = NULL;
	}
}

void CWeaponProjectile::PostLoad()
{
	weaponDef = weaponDefHandler->GetWeaponDefByID(weaponDefID);

//	if(weaponDef->interceptedByShieldType)
//		interceptHandler.AddShieldInterceptableProjectile(this);

	model = weaponDef->LoadModel();

//	collisionFlags = weaponDef->collisionFlags;
}
