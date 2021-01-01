/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <limits>
#include <algorithm>

#include "InterceptHandler.h"

#include "Map/Ground.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/EventHandler.h"
#include "System/float3.h"
#include "System/SpringMath.h"
#include "System/creg/STL_Deque.h"


CR_BIND_DERIVED(CInterceptHandler, CObject, )
CR_REG_METADATA(CInterceptHandler, (
	CR_MEMBER(interceptors),
	CR_MEMBER(interceptables)
))

CInterceptHandler interceptHandler;



void CInterceptHandler::Update(bool forced) {
	if (((gs->frameNum % UNIT_SLOWUPDATE_RATE) != 0) && !forced)
		return;

	for (CWeapon* w: interceptors) {
		const WeaponDef* wDef = w->weaponDef;
		const CUnit* wOwner = w->owner;

		assert(wDef->interceptor || wDef->isShield);

		for (CWeaponProjectile* p: interceptables) {
			if (!p->CanBeInterceptedBy(wDef))
				continue;
			if (w->HasIncomingProjectile(p->id))
				continue;

			const int pAllyTeam = p->GetAllyteamID();

			if (teamHandler.IsValidAllyTeam(pAllyTeam) && teamHandler.Ally(wOwner->allyteam, pAllyTeam))
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
			const float weaponDist = w->aimFromPos.distance(p->pos);
			const float impactDist = CGround::LineGroundCol(p->pos, p->pos + p->dir * weaponDist);

			const float3& pImpactPos = p->pos + p->dir * impactDist;
			const float3& pTargetPos = p->GetTargetPos();
			const float3  pWeaponVec = p->pos - w->aimFromPos;

			if (w->aimFromPos.SqDistance2D(pTargetPos) < Square(wDef->coverageRange)) {
				w->AddDeathDependence(p, DEPENDENCE_INTERCEPT);
				w->AddIncomingProjectile(p->id);
				continue; // 1
			}

			if (false /*wDef->noFlyThroughIntercept*/) {
				// <w> is just a static interceptor and fires only at projectiles
				// TARGETED within its current interception area; any projectiles
				// CROSSING its interception area aren't targeted
				//XXX implement in lua?
				continue;
			}

			if (pWeaponVec.SqLength2D() < Square(wDef->coverageRange)) {
				w->AddDeathDependence(p, DEPENDENCE_INTERCEPT);
				w->AddIncomingProjectile(p->id);
				continue; // 2
			}

			if (w->aimFromPos.SqDistance2D(pImpactPos) < Square(wDef->coverageRange)) {
				const float3 pTargetDir = (pTargetPos - p->pos).SafeNormalize();
				const float3 pImpactDir = (pImpactPos - p->pos).SafeNormalize();

				// the projected impact position can briefly shift into the covered
				// area during transition from vertical to horizontal flight, so we
				// perform an extra test (NOTE: assumes non-parabolic trajectory)
				if (pTargetDir.dot(pImpactDir) >= 0.999f) {
					w->AddDeathDependence(p, DEPENDENCE_INTERCEPT);
					w->AddIncomingProjectile(p->id);
					continue; // 3
				}
			}

			const float3 pMinSepPos = p->pos + p->dir * Clamp(-(pWeaponVec.dot(p->dir)), 0.0f, impactDist);
			const float3 pMinSepVec = w->aimFromPos - pMinSepPos;

			if (pMinSepVec.SqLength() < Square(wDef->coverageRange)) {
				w->AddDeathDependence(p, DEPENDENCE_INTERCEPT);
				w->AddIncomingProjectile(p->id);
				continue; // 4
			}
		}
	}
}



void CInterceptHandler::AddInterceptorWeapon(CWeapon* weapon)
{
	interceptors.push_back(weapon);
}


void CInterceptHandler::RemoveInterceptorWeapon(CWeapon* weapon)
{
	auto it = std::find(interceptors.begin(), interceptors.end(), weapon);
	if (it != interceptors.end()) {
		interceptors.erase(it);
	}
}


void CInterceptHandler::AddInterceptTarget(CWeaponProjectile* target, const float3& destination)
{
	// keep track of all interceptable projectiles
	interceptables.push_back(target);

	// if the target projectile dies in any way, we need to remove it
	// (we cannot rely on any interceptor telling us, because they may
	// die before the interceptable itself does)
	AddDeathDependence(target, DEPENDENCE_INTERCEPTABLE);

	Update(true);
}


void CInterceptHandler::DependentDied(CObject* o)
{
	auto it = std::find(interceptables.begin(), interceptables.end(), static_cast<CWeaponProjectile*>(o));
	if (it != interceptables.end()) {
		interceptables.erase(it);
	}
}

