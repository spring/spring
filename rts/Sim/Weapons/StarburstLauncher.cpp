#include "StdAfx.h"
#include "StarburstLauncher.h"
#include "Sound.h"
#include "Sim/Projectiles/StarburstProjectile.h"
#include "Game/GameHelper.h"
#include "Sim/Units/Unit.h"
#include "Map/Ground.h"
#include "WeaponDefHandler.h"
#include "Sim/Misc/InterceptHandler.h"
#include "mmgr.h"

CR_BIND_DERIVED(CStarburstLauncher, CWeapon, (NULL));

CR_REG_METADATA(CStarburstLauncher,(
	CR_MEMBER(uptime),
	CR_MEMBER(tracking),
	CR_RESERVED(8)
	));

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
	CStarburstProjectile* p=SAFE_NEW CStarburstProjectile(weaponPos+float3(0,2,0),float3(0,0.01f,0),owner,targetPos,areaOfEffect,projectileSpeed,tracking,(int)uptime,targetUnit, weaponDef,interceptTarget);

	if(weaponDef->targetable)
		interceptHandler.AddInterceptTarget(p,targetPos);

	if(fireSoundId && (!weaponDef->soundTrigger || salvoLeft==salvoSize-1))
		sound->PlaySample(fireSoundId,owner,fireSoundVolume);
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

	if(avoidFriendly && helper->TestCone(weaponPos,UpVector,100,0,owner->allyteam,owner))
		return false;

	return true;
}

float CStarburstLauncher::GetRange2D(float yDiff) const
{
	return range+yDiff*heightMod;
}
