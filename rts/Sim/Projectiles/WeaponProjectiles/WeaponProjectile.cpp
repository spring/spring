/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

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
#include "System/GlobalUnsynced.h"
#include "System/Sound/IEffectChannel.h"
#include "System/LogOutput.h"
#ifdef TRACE_SYNC
	#include "Sync/SyncTracer.h"
#endif



CR_BIND_DERIVED(CWeaponProjectile, CProjectile, );

CR_REG_METADATA(CWeaponProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(targeted),
//	CR_MEMBER(weaponDef),
	CR_MEMBER(weaponDefName),
	CR_MEMBER(target),
	CR_MEMBER(targetPos),
	CR_MEMBER(startpos),
	CR_MEMBER(colorTeam),
	CR_MEMBER(ttl),
	CR_MEMBER(bounces),
	CR_MEMBER(keepBouncing),
	CR_MEMBER(cegTag),
	CR_MEMBER(cegID),
	CR_MEMBER(interceptTarget),
	CR_MEMBER(id),
	CR_RESERVED(15),
	CR_POSTLOAD(PostLoad)
	));

CWeaponProjectile::CWeaponProjectile(): CProjectile()
{
	targeted = false;
	weaponDef = 0;
	target = 0;
	projectileType = WEAPON_BASE_PROJECTILE;
	ttl = 0;
	colorTeam = 0;
	interceptTarget = 0;
	bounces = 0;
	keepBouncing = true;
	cegID = -1U;
}

CWeaponProjectile::CWeaponProjectile(const float3& pos, const float3& speed,
		CUnit* owner, CUnit* target, const float3 &targetPos,
		const WeaponDef* weaponDef, CWeaponProjectile* interceptTarget,
		int ttl):
	CProjectile(pos, speed, owner, true, true, false),
	targeted(false),
	weaponDef(weaponDef),
	weaponDefName(weaponDef? weaponDef->name: std::string("")),
	target(target),
	targetPos(targetPos),
	cegTag(weaponDef? weaponDef->cegTag: std::string("")),
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
		AddDeathDependence(target);
	}

	if (interceptTarget) {
		interceptTarget->targeted = true;
		AddDeathDependence(interceptTarget);
	}
	if (weaponDef) {
		if(weaponDef->interceptedByShieldType) {
			interceptHandler.AddShieldInterceptableProjectile(this);
		}

		alwaysVisible = weaponDef->visuals.alwaysVisible;

		model = LoadModel(weaponDef);

		collisionFlags = weaponDef->collisionFlags;
	}

	ph->AddProjectile(this);
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

	if (/* !weaponDef->noExplode || gs->frameNum & 1 */ true) {
		// don't do damage only on odd-numbered frames
		// for noExplode projectiles (it breaks coldet)
		float3 impactDir = speed;
		impactDir.Normalize();

		// Dynamic Damage
		DamageArray dynDamages;
		if (weaponDef->dynDamageExp > 0)
			dynDamages = weaponDefHandler->DynamicDamages(weaponDef->damages,
					startpos, pos, (weaponDef->dynDamageRange > 0)
							? weaponDef->dynDamageRange
							: weaponDef->range,
					weaponDef->dynDamageExp, weaponDef->dynDamageMin,
					weaponDef->dynDamageInverted);

		helper->Explosion(
			pos,
			(weaponDef->dynDamageExp > 0)? dynDamages: weaponDef->damages,
			weaponDef->areaOfEffect,
			weaponDef->edgeEffectiveness,
			weaponDef->explosionSpeed,
			owner(),
			true,
			weaponDef->noExplode? 0.3f: 1.0f,
			weaponDef->noExplode || weaponDef->noSelfDamage,
			weaponDef->impactOnly,
			weaponDef->explosionGenerator,
			0,
			impactDir,
			weaponDef->id,
			feature
		);
	}

	if (weaponDef->soundhit.getID(0) > 0) {
		Channels::Battle.PlaySample(weaponDef->soundhit.getID(0), this, weaponDef->soundhit.getVolume(0));
	}

	if (!weaponDef->noExplode){
		CProjectile::Collision();
	} else {
		if (TraveledRange()) {
			CProjectile::Collision();
		}
	}
}

void CWeaponProjectile::Collision(CUnit* unit)
{
	if (/* !weaponDef->noExplode || gs->frameNum & 1 */ true) {
		// don't do damage only on odd-numbered frames
		// for noExplode projectiles (it breaks coldet)
		float3 impactDir = speed;
		impactDir.Normalize();

		// Dynamic Damage
		DamageArray damages;
		if (weaponDef->dynDamageExp > 0) {
			damages = weaponDefHandler->DynamicDamages(weaponDef->damages,
					startpos, pos,
					weaponDef->dynDamageRange > 0?
						weaponDef->dynDamageRange:
						weaponDef->range,
					weaponDef->dynDamageExp, weaponDef->dynDamageMin,
					weaponDef->dynDamageInverted);
		} else {
			damages = weaponDef->damages;
		}

		helper->Explosion(
			pos,
			damages,
			weaponDef->areaOfEffect,
			weaponDef->edgeEffectiveness,
			weaponDef->explosionSpeed,
			owner(),
			true,
			weaponDef->noExplode? 0.3f: 1.0f,
			weaponDef->noExplode || weaponDef->noSelfDamage,
			weaponDef->impactOnly,
			weaponDef->explosionGenerator,
			unit,
			impactDir,
			weaponDef->id
		);
	}

	if (weaponDef->soundhit.getID(0) > 0) {
		Channels::Battle.PlaySample(weaponDef->soundhit.getID(0), this,
				weaponDef->soundhit.getVolume(0));
	}

	if (!weaponDef->noExplode)
		CProjectile::Collision(unit);
	else {
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
				speed -= (speed + normal * fabs(dot)) * (1 - weaponDef->bounceSlip);
				speed += (normal * (fabs(dot))) * (1 + weaponDef->bounceRebound);
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
	weaponDef = weaponDefHandler->GetWeapon(weaponDefName);

//	if(weaponDef->interceptedByShieldType)
//		interceptHandler.AddShieldInterceptableProjectile(this);

	if (!weaponDef->visuals.modelName.empty()) {
		if (weaponDef->visuals.model == NULL) {
			std::string modelname = "objects3d/" + weaponDef->visuals.modelName;
			if (modelname.find(".") == std::string::npos) {
				modelname += ".3do";
			}
			const_cast<WeaponDef*>(weaponDef)->visuals.model = modelParser->Load3DModel(modelname);
		}
		if (weaponDef->visuals.model) {
			model = weaponDef->visuals.model;
		}
	}

//	collisionFlags = weaponDef->collisionFlags;
}
