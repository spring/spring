/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"

#include "InterceptHandler.h"

#include "Map/Ground.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDef.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/myMath.h"
#include "System/creg/STL_List.h"

CR_BIND(CInterceptHandler, )
CR_REG_METADATA(CInterceptHandler, (
	CR_MEMBER(interceptors),
	CR_MEMBER(repulsors),
	CR_RESERVED(8)
));

CInterceptHandler interceptHandler;



void CInterceptHandler::Update() {
	if ((gs->frameNum % UNIT_SLOWUPDATE_RATE) != 0)
		return;

	std::list<CWeapon*>::iterator wit;
	std::map<int, CWeaponProjectile*>::const_iterator pit;

	for (wit = interceptors.begin(); wit != interceptors.end(); ++wit) {
		CWeapon* w = *wit;
		const WeaponDef* wDef = w->weaponDef;
		const CUnit* wOwner = w->owner;
		const float3& wOwnerPos = wOwner->pos;

		std::map<int, CWeaponProjectile*>& incomingProjectiles = w->incomingProjectiles;

		for (pit = incomingProjectiles.begin(); pit != incomingProjectiles.end(); ++pit) {
			CWeaponProjectile* p = pit->second;
			const WeaponDef* pDef = p->weaponDef;

			// <incomingProjectiles> might contain non-interceptable
			// projectiles too (if <w> is interceptor *and* repulsor)
			if ((pDef->targetable & wDef->interceptor) == 0)
				continue;
			if (incomingProjectiles.find(p->id) != incomingProjectiles.end())
				continue;

			const CUnit* pOwner = p->owner();
			const int pAllyTeam = (pOwner != NULL)? pOwner->allyteam: -1;

			if (pAllyTeam != -1 && teamHandler->Ally(wOwner->allyteam, pAllyTeam))
				continue;

			// there are four cases when an interceptor <w> should fire at a projectile <p>:
			//     1. p's current position inside w's interception circle
			//     2. p's target position inside w's interception circle (w's owner can move!)
			//     3. p's projected impact position inside w's interception circle
			//     4. p's trajectory intersects w's interception circle
			//
			// these checks all need to be evaluated periodically, not just
			// when a projectile is created and handed to AddInterceptTarget
			const float interceptDist = (w->weaponPos - p->pos).Length() + wDef->coverageRange * 2.0f;
			const float impactDist = ground->LineGroundCol(p->pos, p->pos + p->dir * interceptDist);

			const float3& pFlightPos = p->pos;
			const float3& pImpactPos = p->pos + p->dir * impactDist;
			const float3& pTargetPos = p->targetPos;

			if ((pFlightPos - wOwnerPos).SqLength2D() < Square(wDef->coverageRange)) {
				incomingProjectiles[p->id] = p; continue; // 1
			}
			if ((pTargetPos - wOwnerPos).SqLength2D() < Square(wDef->coverageRange)) {
				incomingProjectiles[p->id] = p; continue; // 2
			}
			if ((pImpactPos - wOwnerPos).SqLength2D() < Square(wDef->coverageRange)) {
				incomingProjectiles[p->id] = p; continue; // 3
			}

			const float3 pCurSeparationVec = wOwnerPos - pFlightPos;
			const float pMinSeparationTime = Clamp(pCurSeparationVec.dot(p->dir), 0.0f, 1.0f);
			const float3 pMinSeparationVec = pCurSeparationVec - (p->speed * pMinSeparationTime);

			if (pMinSeparationVec.SqLength() < Square(wDef->coverageRange)) {
				incomingProjectiles[p->id] = p; continue; // 4
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
	interceptors.remove(weapon);
}



void CInterceptHandler::AddInterceptTarget(CWeaponProjectile* target, const float3& destination)
{
	const CUnit* targetOwner = target->owner();
	const int targetAllyTeam = (targetOwner != NULL)? targetOwner->allyteam: -1;

	for (std::list<CWeapon*>::iterator wi = interceptors.begin(); wi != interceptors.end(); ++wi) {
		CWeapon* w = *wi;

		if ((target->weaponDef->targetable & w->weaponDef->interceptor) == 0)
			continue;
		if (w->weaponPos.SqDistance2D(destination) >= Square(w->weaponDef->coverageRange))
			continue;
		if (targetAllyTeam != -1 && teamHandler->Ally(w->owner->allyteam, targetAllyTeam))
			continue;

		w->incomingProjectiles[target->id] = target;
		w->AddDeathDependence(target, CObject::DEPENDENCE_INTERCEPT);
	}
}


void CInterceptHandler::AddShieldInterceptableProjectile(CWeaponProjectile* p)
{
	for (std::list<CPlasmaRepulser*>::iterator wi = repulsors.begin(); wi != repulsors.end(); ++wi) {
		CPlasmaRepulser* shield = *wi;
		if (shield->weaponDef->shieldInterceptType & p->weaponDef->interceptedByShieldType) {
			shield->NewProjectile(p);
		}
	}
}


float CInterceptHandler::AddShieldInterceptableBeam(CWeapon* emitter, const float3& start, const float3& dir, float length, float3& newDir, CPlasmaRepulser*& repulsedBy)
{
	float minRange = 99999999;
	float3 tempDir;

	for (std::list<CPlasmaRepulser*>::iterator wi = repulsors.begin(); wi != repulsors.end(); ++wi) {
		CPlasmaRepulser* shield = *wi;
		if(shield->weaponDef->shieldInterceptType & emitter->weaponDef->interceptedByShieldType){
			float dist = shield->NewBeam(emitter, start, dir, length, tempDir);
			if ((dist > 0) && (dist < minRange)) {
				minRange = dist;
				newDir = tempDir;
				repulsedBy = shield;
			}
		}
	}
	return minRange;
}


void CInterceptHandler::AddPlasmaRepulser(CPlasmaRepulser* r)
{
	repulsors.push_back(r);
}

void CInterceptHandler::RemovePlasmaRepulser(CPlasmaRepulser* r)
{
	repulsors.remove(r);
}
