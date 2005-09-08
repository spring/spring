#include "StdAfx.h"
#include "StarburstLauncher.h"
#include "Sound.h"
#include "StarburstProjectile.h"
#include "GameHelper.h"
#include "Unit.h"
#include "Ground.h"
#include "WeaponDefHandler.h"
#include "InterceptHandler.h"
//#include "mmgr.h"

CStarburstLauncher::CStarburstLauncher(CUnit* owner)
: CWeapon(owner),
	uptime(3),
	tracking(0)
{
}

CStarburstLauncher::~CStarburstLauncher(void)
{
}

void CStarburstLauncher::Update(void)
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		wantedDir=(targetPos-weaponPos).Normalize();		//the aiming upward is apperently implicid so aim toward target
	}
	CWeapon::Update();
}

void CStarburstLauncher::Fire(void)
{
	CStarburstProjectile* p=new CStarburstProjectile(weaponPos+float3(0,2,0),float3(0,0.01,0),owner,targetPos,damages,areaOfEffect,projectileSpeed,tracking,(int)uptime,targetUnit, weaponDef,interceptTarget);

	if(weaponDef->targetable)
		interceptHandler.AddInterceptTarget(p,targetPos);

	if(fireSoundId)
		sound->PlaySound(fireSoundId,owner,fireSoundVolume);
}

bool CStarburstLauncher::TryTarget(const float3& pos,bool userTarget,CUnit* unit)
{
	if(!CWeapon::TryTarget(pos,userTarget,unit))
		return false;

	if(unit){
		if(unit->isUnderWater)
			return false;
	} else {
		if(pos.y<0)
			return false;
	}

	if(helper->TestCone(weaponPos,UpVector,100,0,owner->allyteam,owner))
		return false;

	return true;
}
