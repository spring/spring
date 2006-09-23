#include "StdAfx.h"
#include "FlameThrower.h"
#include "Sim/Units/Unit.h"
#include "Sound.h"
#include "Map/Ground.h"
#include "Game/GameHelper.h"
#include "Sim/Projectiles/FlameProjectile.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"

CFlameThrower::CFlameThrower(CUnit* owner)
: CWeapon(owner)
{
}

CFlameThrower::~CFlameThrower(void)
{
}

void CFlameThrower::Fire(void)
{
	float3 dir=targetPos-weaponPos;
	dir.Normalize();
	float3 spread=(gs->randVector()*sprayangle+salvoError)*0.2f;
	spread-=dir*0.001f;

	new CFlameProjectile(weaponPos,dir*projectileSpeed,spread,owner,damages,weaponDef,(int)(range/projectileSpeed*weaponDef->duration));
	if(fireSoundId && (!weaponDef->soundTrigger || salvoLeft==salvoSize-1))
		sound->PlaySample(fireSoundId,owner,fireSoundVolume);
}

bool CFlameThrower::TryTarget(const float3 &pos,bool userTarget,CUnit* unit)
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

	float3 dir=pos-weaponPos;
	float length=dir.Length();
	if(length==0)
		return true;

	dir/=length;

	float g=ground->LineGroundCol(weaponPos,pos);
	if(g>0 && g<length*0.9f)
		return false;

	if(helper->LineFeatureCol(weaponPos,dir,length))
		return false;

	if(avoidFriendly && helper->TestCone(weaponPos,dir,length,(accuracy+sprayangle),owner->allyteam,owner)){
		return false;
	}
	return true;
}

void CFlameThrower::Update(void)
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		wantedDir=targetPos-weaponPos;
		wantedDir.Normalize();
	}
	CWeapon::Update();
}
