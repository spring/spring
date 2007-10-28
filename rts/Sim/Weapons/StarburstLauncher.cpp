#include "StdAfx.h"
#include "Game/GameHelper.h"
#include "Map/Ground.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Projectiles/WeaponProjectiles/StarburstProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sound.h"
#include "StarburstLauncher.h"
#include "WeaponDefHandler.h"
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
		weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
		wantedDir=(targetPos-weaponPos).Normalize();		//the aiming upward is apperently implicid so aim toward target
	}
	CWeapon::Update();
}

void CStarburstLauncher::Fire(void)
{
	float3 speed(0,weaponDef->startvelocity,0);
	float maxrange;
	if (weaponDef->fixedLauncher) {
		speed = weaponDir * weaponDef->startvelocity;
		maxrange = (float)MAX_WORLD_SIZE;
	} else if (weaponDef->flighttime > 0) {
		maxrange = (float)MAX_WORLD_SIZE;
	} else {
		maxrange = (float)range;
	}

	CStarburstProjectile* p =
		SAFE_NEW CStarburstProjectile(weaponMuzzlePos + float3(0, 2, 0), speed, owner,
		targetPos, areaOfEffect, projectileSpeed, tracking, (int) uptime, targetUnit,
		weaponDef, interceptTarget, maxrange, weaponDef->cegTag);

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
		if(unit->isUnderWater && !weaponDef->waterweapon)
			return false;
	} else {
		if(pos.y<0 && !weaponDef->waterweapon)
			return false;
	}

	if(avoidFriendly && helper->TestCone(weaponMuzzlePos,
			(weaponDef->fixedLauncher ? weaponDir : UpVector), 100, 0, owner->allyteam, owner))
		return false;

	return true;
}

float CStarburstLauncher::GetRange2D(float yDiff) const
{
	return range+yDiff*heightMod;
}
