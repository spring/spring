#include "stdafx.h"
#include ".\intercepthandler.h"
#include "weapon.h"
#include "weaponprojectile.h"
#include "unit.h"
#include "weapondefhandler.h"
//#include "mmgr.h"

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
		if(!gs->allies[w->owner->allyteam][targTeam] && (target->weaponDef->targetable & w->weaponDef->interceptor) && w->weaponPos.distance2D(destination) < w->weaponDef->coverageRange){
			w->incoming.push_back(target);
			w->AddDeathDependence(target);
		}
	}
}
