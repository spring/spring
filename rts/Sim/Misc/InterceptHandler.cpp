#include "StdAfx.h"
#include "InterceptHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Projectiles/WeaponProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Game/UI/InfoConsole.h"
#include "mmgr.h"

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
	int targTeam=0;
	if(target->owner)
		targTeam=target->owner->allyteam;

	for(std::list<CWeapon*>::iterator wi=interceptors.begin();wi!=interceptors.end();++wi){
		CWeapon* w=*wi;
		if(!gs->Ally(w->owner->allyteam,targTeam) && (target->weaponDef->targetable & w->weaponDef->interceptor) && w->weaponPos.distance2D(destination) < w->weaponDef->coverageRange){
			w->incoming.push_back(target);
			w->AddDeathDependence(target);
		}
	}
}

void CInterceptHandler::AddPlasma(CProjectile* p)
{
	for(std::list<CPlasmaRepulser*>::iterator wi=plasmaRepulsors.begin();wi!=plasmaRepulsors.end();++wi)
		(*wi)->NewPlasmaProjectile(p);
}

void CInterceptHandler::AddPlasmaRepulser(CPlasmaRepulser* r)
{
	plasmaRepulsors.push_back(r);
}

void CInterceptHandler::RemovePlasmaRepulser(CPlasmaRepulser* r)
{
	plasmaRepulsors.remove(r);
}
