/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Game/GameHelper.h"
#include "Rendering/Colors.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/ProjectileParams.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Map/Ground.h"
#include "System/Matrix44f.h"
#include "System/myMath.h"
#include "System/creg/DefTypes.h"


CR_BIND_DERIVED_INTERFACE(CWeaponProjectile, CProjectile)

CR_REG_METADATA(CWeaponProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(damages),
	CR_MEMBER(targeted),
	CR_MEMBER(weaponDef),
	CR_MEMBER(target),
	CR_MEMBER(targetPos),
	CR_MEMBER(startPos),
	CR_MEMBER(ttl),
	CR_MEMBER(bounces),
	CR_MEMBER(weaponNum),

	CR_POSTLOAD(PostLoad)
))


CWeaponProjectile::CWeaponProjectile(const ProjectileParams& params)
	: CProjectile(params.pos, params.speed, params.owner, true, true, false, false)

	, damages(nullptr)
	, weaponDef(params.weaponDef)
	, target(params.target)

	, ttl(params.ttl)
	, bounces(0)

	, targeted(false)

	, startPos(params.pos)
	, targetPos(params.end)
{
	projectileType = WEAPON_BASE_PROJECTILE;

	assert(weaponDef != nullptr);

	if (weaponDef->IsHitScanWeapon()) {
		hitscan = true;
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
	weaponNum = params.weaponNum;
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
		allyteamID = teamHandler->IsValidTeam(teamID)? teamHandler->AllyTeam(teamID): -1;
	}

	if (ownerID != -1u && weaponNum != -1u) {
		const CUnit* owner = unitHandler->GetUnit(ownerID);
		if (owner != nullptr && weaponNum < owner->weapons.size()) {
			damages = DynDamageArray::IncRef(owner->weapons[weaponNum]->damages);
		}
	}
	if (damages == nullptr)
		damages = DynDamageArray::IncRef(&weaponDef->damages);

	if (params.cegID != -1u) {
		cegID = params.cegID;
	} else {
		cegID = weaponDef->ptrailExplosionGeneratorID;
	}

	// must happen after setting position and velocity
	projectileHandler->AddProjectile(this);
	quadField->AddProjectile(this);

	ASSERT_SYNCED(id);

	if (weaponDef->targetable) {
		interceptHandler.AddInterceptTarget(this, targetPos);
	}
}


CWeaponProjectile::~CWeaponProjectile()
{
	DynDamageArray::DecRef(damages);
}


void CWeaponProjectile::Explode(
	CUnit* hitUnit,
	CFeature* hitFeature,
	float3 impactPos,
	float3 impactDir
) {
	const DamageArray& damageArray = damages->GetDynamicDamages(startPos, impactPos);
	const CExplosionParams params = {
		impactPos,
		impactDir.SafeNormalize(),
		damageArray,
		weaponDef,
		owner(),
		hitUnit,
		hitFeature,
		damages->craterAreaOfEffect,
		damages->damageAreaOfEffect,
		damages->edgeEffectiveness,
		damages->explosionSpeed,
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
		if (pos.SqDistance(po->pos) < Square(damages->damageAreaOfEffect)) {
			po->Collision();
			Collision();
		}
	}
}


void CWeaponProjectile::UpdateGroundBounce()
{
	// projectile is not allowed to bounce on either surface
	if (!weaponDef->groundBounce && !weaponDef->waterBounce)
		return;
	// max bounce already reached?
	if ((bounces + 1) > weaponDef->numBounce)
		return;
	if (luaMoveCtrl)
		return;
	if (ttl <= 0)
		return;

	// water or ground bounce?
	float3 normal;
	bool bounced = false;
	const float distWaterHit  = (pos.y > 0.0f && speed.y < 0.0f) ? (pos.y / -speed.y) : -1.0f;
	const bool intersectWater = (distWaterHit >= 0.0f) && (distWaterHit <= 1.0f);
	if (intersectWater && weaponDef->waterBounce) {
		pos += speed * distWaterHit;
		pos.y = 0.5f;
		normal = CGround::GetNormalAboveWater(pos.x, pos.z);
		bounced = true;
	} else {
		const float distGroundHit  = CGround::LineGroundCol(pos, pos + speed); //TODO use traj one for traj weapons?
		const bool intersectGround = (distGroundHit >= 0.0f);
		if (intersectGround && weaponDef->groundBounce) {
			static const float dontTouchSurface = 0.99f;
			pos += dir * distGroundHit * dontTouchSurface;
			normal = CGround::GetNormal(pos.x, pos.z);
			bounced = true;
		}
	}

	if (!bounced)
		return;

	// spawn CEG before bouncing, otherwise we might be too
	// far up in the air if it has the (under)water flag set
	explGenHandler->GenExplosion(weaponDef->bounceExplosionGeneratorID, pos, normal, speed.w, 1.0f, 1.0f, owner(), NULL);

	++bounces;
	const float dot = math::fabs(speed.dot(normal));
	CWorldObject::SetVelocity(speed - (speed + normal * dot) * (1 - weaponDef->bounceSlip   ));
	CWorldObject::SetVelocity(         speed + normal * dot  * (1 + weaponDef->bounceRebound));
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


bool CWeaponProjectile::CanBeInterceptedBy(const WeaponDef* wd) const
{
	return ((weaponDef->targetable & wd->interceptor) != 0);
}


void CWeaponProjectile::DependentDied(CObject* o)
{
	if (o == target) {
		target = NULL;
	}
}


void CWeaponProjectile::PostLoad()
{
	assert(weaponDef != nullptr);
	model = weaponDef->LoadModel();
}
