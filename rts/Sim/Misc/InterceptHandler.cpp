/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <limits>


#include "InterceptHandler.h"

#include "Map/Ground.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDef.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/EventHandler.h"
#include "System/float3.h"
#include "System/myMath.h"
#include "System/creg/STL_List.h"

#include <limits>

CR_BIND_DERIVED(CInterceptHandler, CObject, )
CR_REG_METADATA(CInterceptHandler, (
	CR_MEMBER(interceptors),
	CR_MEMBER(repulsors),
	CR_MEMBER(interceptables)
))

CInterceptHandler interceptHandler;



void CInterceptHandler::Update(bool forced) {
	if (((gs->frameNum % UNIT_SLOWUPDATE_RATE) != 0) && !forced)
		return;

	std::list<CWeapon*>::iterator wit;
	std::map<int, CWeaponProjectile*>::const_iterator pit;

	for (wit = interceptors.begin(); wit != interceptors.end(); ++wit) {
		CWeapon* w = *wit;

		const WeaponDef* wDef = w->weaponDef;
		const CUnit* wOwner = w->owner;

		assert(wDef->interceptor || wDef->isShield);

		for (pit = interceptables.begin(); pit != interceptables.end(); ++pit) {
			CWeaponProjectile* p = pit->second;
			const WeaponDef* pDef = p->GetWeaponDef();

			if ((pDef->targetable & wDef->interceptor) == 0)
				continue;
			if (w->incomingProjectiles.find(p->id) != w->incomingProjectiles.end())
				continue;

			const CUnit* pOwner = p->owner();
			const int pAllyTeam = (pOwner != NULL)? pOwner->allyteam: -1;

			if (pAllyTeam != -1 && teamHandler->Ally(wOwner->allyteam, pAllyTeam))
				continue;

			// note: will be called every Update so long as gadget does not return true
			if (!eventHandler.AllowWeaponInterceptTarget(wOwner, w, p))
				continue;

			// there are four cases when an interceptor <w> should fire at a projectile <p>:
			//     1. p's target position inside w's interception circle (w's owner can move!)
			//     2. p's current position inside w's interception circle
			//     3. p's projected impact position inside w's interception circle
			//     4. p's trajectory intersects w's interception circle
			//
			// these checks all need to be evaluated periodically, not just
			// when a projectile is created and handed to AddInterceptTarget
			const float weaponDist = w->weaponPos.distance(p->pos);
			const float impactDist = CGround::LineGroundCol(p->pos, p->pos + p->dir * weaponDist);

			const float3& pImpactPos = p->pos + p->dir * impactDist;
			const float3& pTargetPos = p->GetTargetPos();
			const float3  pWeaponVec = p->pos - w->weaponPos;

			if ((pTargetPos - w->weaponPos).SqLength2D() < Square(wDef->coverageRange)) {
				w->AddDeathDependence(p, DEPENDENCE_INTERCEPT);
				w->incomingProjectiles[p->id] = p;
				continue; // 1
			}

			if (wDef->interceptor == 1) {
				// <w> is just a static interceptor and fires only at projectiles
				// TARGETED within its current interception area; any projectiles
				// CROSSING its interception area are fired at only if interceptor
				// is >= 2
				//// continue;
			}

			if (pWeaponVec.SqLength2D() < Square(wDef->coverageRange)) {
				w->AddDeathDependence(p, DEPENDENCE_INTERCEPT);
				w->incomingProjectiles[p->id] = p;
				continue; // 2
			}

			if ((pImpactPos - w->weaponPos).SqLength2D() < Square(wDef->coverageRange)) {
				const float3 pTargetDir = (pTargetPos - p->pos).SafeNormalize();
				const float3 pImpactDir = (pImpactPos - p->pos).SafeNormalize();

				// the projected impact position can briefly shift into the covered
				// area during transition from vertical to horizontal flight, so we
				// perform an extra test (NOTE: assumes non-parabolic trajectory)
				if (pTargetDir.dot(pImpactDir) >= 0.999f) {
					w->AddDeathDependence(p, DEPENDENCE_INTERCEPT);
					w->incomingProjectiles[p->id] = p;
					continue; // 3
				}
			}

			const float3 pMinSepPos = p->pos + p->dir * std::min(impactDist, std::max(-(pWeaponVec.dot(p->dir)), 0.0f));
			const float3 pMinSepVec = w->weaponPos - pMinSepPos;

			if (pMinSepVec.SqLength() < Square(wDef->coverageRange)) {
				w->AddDeathDependence(p, DEPENDENCE_INTERCEPT);
				w->incomingProjectiles[p->id] = p;
				continue; // 4
			}
		}
	}
}



void CInterceptHandler::AddInterceptTarget(CWeaponProjectile* target, const float3& destination)
{
	// keep track of all interceptable projectiles
	interceptables[target->id] = target;

	// if the target projectile dies in any way, we need to remove it
	// (we cannot rely on any interceptor telling us, because they may
	// die before the interceptable itself does)
	AddDeathDependence(target, DEPENDENCE_INTERCEPTABLE);

	Update(true);
}

void CInterceptHandler::AddShieldInterceptableProjectile(CWeaponProjectile* p)
{
	for (std::list<CPlasmaRepulser*>::iterator wi = repulsors.begin(); wi != repulsors.end(); ++wi) {
		CPlasmaRepulser* shield = *wi;

		if (shield->weaponDef->shieldInterceptType & p->GetWeaponDef()->interceptedByShieldType) {
			shield->NewProjectile(p);
		}
	}
}



float CInterceptHandler::AddShieldInterceptableBeam(CWeapon* emitter, const float3& start, const float3& dir, float length, float3& newDir, CPlasmaRepulser*& repulsedBy)
{
	float minRange = std::numeric_limits<float>::max();
	float3 tempDir;

	for (std::list<CPlasmaRepulser*>::iterator wi = repulsors.begin(); wi != repulsors.end(); ++wi) {
		CPlasmaRepulser* shield = *wi;

		if ((shield->weaponDef->shieldInterceptType & emitter->weaponDef->interceptedByShieldType) == 0)
			continue;

		const float dist = shield->NewBeam(emitter, start, dir, length, tempDir);

		if (dist <=     0.0f) continue;
		if (dist >= minRange) continue;

		minRange = dist;
		newDir = tempDir;
		repulsedBy = shield;
	}

	return minRange;
}



void CInterceptHandler::DependentDied(CObject* o) {
	std::map<int, CWeaponProjectile*>::iterator it = interceptables.find(static_cast<CWeaponProjectile*>(o)->id);

	if (it != interceptables.end()) {
		interceptables.erase(it->first);
	}
}

