/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Game/GameHelper.h"
#include "Rendering/Colors.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Units/Unit.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Map/Ground.h"
#include "System/Matrix44f.h"



CR_BIND_DERIVED(CWeaponProjectile, CProjectile, );

CR_REG_METADATA(CWeaponProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(targeted),
	CR_IGNORED(weaponDef), //PostLoad
	CR_MEMBER(target),
	CR_MEMBER(targetPos),
	CR_MEMBER(startPos),
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

CWeaponProjectile::CWeaponProjectile(const ProjectileParams& params)
	: CProjectile(params.pos, params.speed, params.owner, true, true, false, (params.weaponDef)->IsHitScanWeapon())

	, weaponDef(params.weaponDef)
	, target(params.target)

	, weaponDefID(-1u)

	, ttl(params.ttl)
	, bounces(0)

	, targeted(false)

	, startPos(params.pos)
	, targetPos(params.end)
{
	projectileType = WEAPON_BASE_PROJECTILE;

	if (weaponDef->IsHitScanWeapon()) {
		// the else-case (default) is handled in CProjectile::Init
		//
		// ray projectiles must all set this to false because their collision
		// detection is handled by the weapons firing them, ProjectileHandler
		// will skip any tests for these
		checkCol = false;
		// type has not yet been set by derived ctor's at this point
		// useAirLos = (projectileType != WEAPON_LIGHTNING_PROJECTILE);
		useAirLos = true;

		// NOTE:
		//   {BeamLaser, Lightning}Projectile's do NOT actually move (their
		//   speed is never added to pos) and never alter their speed either
		//   they additionally override our ::Update (so CProjectile::Update
		//   is also never called) which means assigning speed a non-zerovec
		//   value should have no side-effects
		SetPosition(startPos);
		SetVelocityAndSpeed(targetPos - startPos);

		// ProjectileDrawer vis-culls by pos == startPos, but we
		// want to see the beam even if camera is near targetPos
		// --> use full distance for drawRadius
		SetRadiusAndHeight((targetPos - startPos).Length(), 0.0f);
	}

	collisionFlags = weaponDef->collisionFlags;
	weaponDefID = params.weaponDef->id;
	alwaysVisible = weaponDef->visuals.alwaysVisible;
	ignoreWater = weaponDef->waterweapon;

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
		cegID = weaponDef->ptrailExplosionGeneratorID;
	}

	// must happen after setting position and velocity
	projectileHandler->AddProjectile(this);
	quadField->AddProjectile(this);

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
			startPos,
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
		static_cast<unsigned int>(id)
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
		if (hitscan) {
			impactPos = feature->pos;
			impactDir = targetPos - startPos;
		}

		if (gs->randFloat() < weaponDef->fireStarter) {
			feature->StartFire();
		}
	} else {
		if (hitscan) {
			impactPos = targetPos;
			impactDir = targetPos - startPos;
		}
	}

	Explode(NULL, feature, impactPos, impactDir);
}

void CWeaponProjectile::Collision(CUnit* unit)
{
	float3 impactPos = pos;
	float3 impactDir = speed;

	if (unit != NULL) {
		if (hitscan) {
			impactPos = unit->pos;
			impactDir = targetPos - startPos;
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

	// we are the interceptor, point us toward the interceptee pos each frame
	// (normally not needed, subclasses handle it directly in their Update()'s
	// *until* our owner dies)
	if (owner() == NULL) {
		targetPos = po->pos + po->speed;
	}

	if (hitscan) {
		if (ClosestPointOnLine(startPos, targetPos, po->pos).SqDistance(po->pos) < Square(weaponDef->collisionSize)) {
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
	if (ttl <= 0)
		return;
	// projectile is not allowed to bounce even once
	if (weaponDef->numBounce < 0)
		return;
	// projectile is not allowed to bounce on either surface
	if (!weaponDef->groundBounce && !weaponDef->waterBounce)
		return;

	const float gh = CGround::GetHeightReal(pos.x, pos.z);
	const float dg = pos.y - gh;
	const float dw = pos.y - std::max(gh, 0.0f);

	// if not close to a bounceable surface, bail
	if (dg > 0.1f && dw > 0.1f)
		return;

	// if close to water but not allowed to bounce on it, bail
	if ((dw <= 0.1f) && !weaponDef->waterBounce)
		return;
	// if close to ground but not allowed to bounce on it, bail
	if ((dg <= 0.1f) && !weaponDef->groundBounce)
		return;

	if ((bounces += 1) > weaponDef->numBounce)
		return;

	const float3& normal = CGround::GetNormal(pos.x, pos.z);
	const float dot = math::fabs(speed.dot(normal));

	// spawn CEG before bouncing, otherwise we might be too
	// far up in the air if it has the (under)water flag set
	explGenHandler->GenExplosion(weaponDef->bounceExplosionGeneratorID, pos, normal, speed.w, 1.0f, 1.0f, owner(), NULL);

	SetPosition(pos - speed);
	CWorldObject::SetVelocity(speed - (speed + normal * dot) * (1 - weaponDef->bounceSlip   ));
	CWorldObject::SetVelocity(         speed + normal * dot  * (1 + weaponDef->bounceRebound));
	SetPosition(pos + speed);
	SetVelocityAndSpeed(speed);
}



bool CWeaponProjectile::TraveledRange() const
{
	return ((pos - startPos).SqLength() > (weaponDef->range * weaponDef->range));
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
