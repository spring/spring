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
	CR_IGNORED(weaponDef), //PostLoad
	CR_MEMBER(target),
	CR_MEMBER(targetPos),
	CR_MEMBER(startpos),
	CR_MEMBER(ttl),
	CR_MEMBER(bounces),
	CR_MEMBER(weaponDefID),
	CR_POSTLOAD(PostLoad)
));



CWeaponProjectile::CWeaponProjectile(): CProjectile()
	, weaponDef(NULL)
	, target(NULL)

	, weaponDefID(0)

	, ttl(0)
	, bounces(0)

	, targeted(false)
{
}

CWeaponProjectile::CWeaponProjectile(const ProjectileParams& params, const bool isRay)
	: CProjectile(params.pos, params.speed, params.owner, true, true, false)

	, weaponDef(params.weaponDef)
	, target(params.target)

	, weaponDefID(-1u)

	, ttl(params.ttl)
	, bounces(0)

	, targeted(false)

	, startpos(params.pos)
	, targetPos(params.end)
{
	assert(weaponDef != NULL);

	projectileType = WEAPON_BASE_PROJECTILE;

	collisionFlags = weaponDef->collisionFlags;
	weaponDefID = params.weaponDef->id;
	alwaysVisible = weaponDef->visuals.alwaysVisible;
	ignoreWater = weaponDef->waterweapon;

	if (isRay) {
		pos = (startpos + targetPos) * 0.5f;
		//pos = startpos;
		//speed = (targetPos - startpos) + weaponDef->collisionSize; //FIXME move to CProjectileHandler::Check...Collision?
		drawRadius = startpos.distance(targetPos);
	}

	CSolidObject* so = NULL;
	CWeaponProjectile* po = NULL;

	if ((so = dynamic_cast<CSolidObject*>(target)) != NULL) {
		AddDeathDependence(so, DEPENDENCE_WEAPONTARGET);
	}
	if ((po = dynamic_cast<CWeaponProjectile*>(target)) != NULL) {
		po->SetBeingIntercepted(po->IsBeingIntercepted() || weaponDef->interceptSolo);
		AddDeathDependence(po, DEPENDENCE_INTERCEPTTARGET);
	}

	if (params.model != NULL) {
		model = params.model;
	} else {
		model = weaponDef->LoadModel();
	}

	if (params.owner == NULL) {
		// the else-case (default) is handled in CProjectile::Init
		ownerID = params.ownerID;
		teamID = params.teamID;
	}

	if (params.cegID != -1u) {
		cegID = params.cegID;
	} else {
		cegID = gCEG->Load(explGenHandler, weaponDef->cegTag);
	}

	projectileHandler->AddProjectile(this);
	ASSERT_SYNCED(id);

	if (weaponDef->interceptedByShieldType) {
		// this needs a valid projectile id set
		assert(id >= 0);
		interceptHandler.AddShieldInterceptableProjectile(this);
	}

	if (weaponDef->targetable) {
		interceptHandler.AddInterceptTarget(this, targetPos);
	}
}



void CWeaponProjectile::Explode(
	CUnit* hitUnit,
	CFeature* hitFeature,
	float3 impactPos,
	float3 impactDir
) {
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
		hitUnit,
		hitFeature,
		weaponDef->craterAreaOfEffect,
		weaponDef->damageAreaOfEffect,
		weaponDef->edgeEffectiveness,
		weaponDef->explosionSpeed,
		weaponDef->noExplode? 0.3f: 1.0f,                 // gfxMod
		weaponDef->impactOnly,
		weaponDef->noExplode || weaponDef->noSelfDamage,  // ignoreOwner
		true,                                             // damgeGround
		id
	};

	helper->Explosion(params);

	if (!weaponDef->noExplode || TraveledRange()) {
		// remove ourselves from the simulation (otherwise
		// keep traveling and generating more explosions)
		CProjectile::Collision();
	}
}

void CWeaponProjectile::Collision()
{
	Collision((CFeature*) NULL);
}

void CWeaponProjectile::Collision(CFeature* feature)
{
	float3 impactPos = pos;
	float3 impactDir = speed;

	if (feature != NULL) {
		if (IsHitScan()) {
			impactPos = feature->pos;
			impactDir = targetPos - startpos;
		}

		if (gs->randFloat() < weaponDef->fireStarter) {
			feature->StartFire();
		}
	} else {
		if (IsHitScan()) {
			impactPos = targetPos;
			impactDir = targetPos - startpos;
		}
	}

	Explode(NULL, feature, impactPos, impactDir);
}

void CWeaponProjectile::Collision(CUnit* unit)
{
	float3 impactPos = pos;
	float3 impactDir = speed;

	if (unit != NULL) {
		if (IsHitScan()) {
			impactPos = unit->pos;
			impactDir = targetPos - startpos;
		}
	} else {
		assert(false);
	}

	Explode(unit, NULL, impactPos, impactDir);
}

void CWeaponProjectile::Update()
{
	CProjectile::Update();
	UpdateGroundBounce();
	UpdateInterception();
}


void CWeaponProjectile::UpdateInterception()
{
	if (target == NULL)
		return;

	CWeaponProjectile* po = dynamic_cast<CWeaponProjectile*>(target);

	if (po == NULL)
		return;

	if (IsHitScan()) {
		if (ClosestPointOnLine(startpos, targetPos, po->pos).SqDistance(po->pos) < Square(weaponDef->collisionSize)) {
			po->Collision();
			Collision();
		}
	} else {
		// FIXME: if (pos.SqDistance(po->pos) < Square(weaponDef->collisionSize)) {
		if (pos.SqDistance(po->pos) < Square(weaponDef->damageAreaOfEffect)) {
			po->Collision();
			Collision();
		}
	}
}


void CWeaponProjectile::UpdateGroundBounce()
{
	if (luaMoveCtrl)
		return;
	if (ttl <= 0 && weaponDef->numBounce < 0)
		return;
	if (!weaponDef->groundBounce && !weaponDef->waterBounce)
		return;

	float wh = 0.0f;

	if (!weaponDef->waterBounce) {
		wh = ground->GetHeightReal(pos.x, pos.z);
	} else if (weaponDef->groundBounce) {
		wh = ground->GetHeightAboveWater(pos.x, pos.z);
	}

	if (pos.y >= wh)
		return;
	if (weaponDef->numBounce >= 0 && (bounces += 1) > weaponDef->numBounce)
		return;

	const float3& normal = ground->GetNormal(pos.x, pos.z);
	const float dot = speed.dot(normal);

	pos -= speed;
	speed -= (speed + normal * math::fabs(dot)) * (1 - weaponDef->bounceSlip);
	speed += (normal * (math::fabs(dot))) * (1 + weaponDef->bounceRebound);
	pos += speed;

	if (weaponDef->bounceExplosionGenerator == NULL)
		return;

	weaponDef->bounceExplosionGenerator->Explosion(-1u, pos, speed.Length(), 1, owner(), 1, NULL, normal);
}



bool CWeaponProjectile::TraveledRange() const
{
	return ((pos - startpos).SqLength() > (weaponDef->range * weaponDef->range));
}

bool CWeaponProjectile::IsHitScan() const
{
	switch (projectileType) {
		case WEAPON_BEAMLASER_PROJECTILE:      { return true; } break;
		case WEAPON_LARGEBEAMLASER_PROJECTILE: { return true; } break;
		case WEAPON_LIGHTNING_PROJECTILE:      { return true; } break;
	}

	return false;
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
	model = weaponDef->LoadModel();
}
