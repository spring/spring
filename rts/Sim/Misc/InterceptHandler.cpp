#include "StdAfx.h"
#include "mmgr.h"

#include "InterceptHandler.h"

#include "Game/GlobalSynced.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "LogOutput.h"
#include "creg/STL_List.h"

CR_BIND(CInterceptHandler, )
CR_REG_METADATA(CInterceptHandler, (
	CR_MEMBER(interceptors),
	CR_MEMBER(plasmaRepulsors),
	CR_RESERVED(8)
	));


CInterceptHandler interceptHandler;


CInterceptHandler::CInterceptHandler(void)
{
}


CInterceptHandler::~CInterceptHandler(void)
{
}


void CInterceptHandler::AddInterceptorWeapon(CWeapon* weapon)
{
	interceptors.push_back(weapon);
}


void CInterceptHandler::RemoveInterceptorWeapon(CWeapon* weapon)
{
	interceptors.remove(weapon);
}


void CInterceptHandler::AddInterceptTarget(CWeaponProjectile* target,float3 destination)
{
	int targTeam=-1;
	if(target->owner)
		targTeam=target->owner->allyteam;

	for(std::list<CWeapon*>::iterator wi=interceptors.begin();wi!=interceptors.end();++wi){
		CWeapon* w=*wi;
		if ((targTeam==-1 || !gs->Ally(w->owner->allyteam,targTeam)) && (target->weaponDef->targetable & w->weaponDef->interceptor) && w->weaponPos.distance2D(destination) < w->weaponDef->coverageRange){
			w->incoming.push_back(target);
			w->AddDeathDependence(target);
		}
	}
}


void CInterceptHandler::AddShieldInterceptableProjectile(CWeaponProjectile* p)
{
	for(std::list<CPlasmaRepulser*>::iterator wi=plasmaRepulsors.begin();wi!=plasmaRepulsors.end();++wi){
		CPlasmaRepulser* shield=*wi;
		if(shield->weaponDef->shieldInterceptType & p->weaponDef->interceptedByShieldType){
			shield->NewProjectile(p);
		}
	}
}


float CInterceptHandler::AddShieldInterceptableBeam(CWeapon* emitter, float3 start, float3 dir, float length, float3& newDir, CPlasmaRepulser*& repulsedBy)
{
	float minRange=99999999;
	float3 tempDir;

	for(std::list<CPlasmaRepulser*>::iterator wi=plasmaRepulsors.begin();wi!=plasmaRepulsors.end();++wi){
		CPlasmaRepulser* shield=*wi;
		if(shield->weaponDef->shieldInterceptType & emitter->weaponDef->interceptedByShieldType){
			float dist=shield->NewBeam(emitter,start,dir,length,tempDir);
			if(dist>0 && dist<minRange){
				minRange=dist;
				newDir=tempDir;
				repulsedBy=shield;
			}
		}
	}
	return minRange;
}


void CInterceptHandler::AddPlasmaRepulser(CPlasmaRepulser* r)
{
	plasmaRepulsors.push_back(r);
}


void CInterceptHandler::RemovePlasmaRepulser(CPlasmaRepulser* r)
{
	plasmaRepulsors.remove(r);
}
