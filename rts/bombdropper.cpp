// Cannon.cpp: implementation of the CBombDropper class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BombDropper.h"
#include "unit.h"
#include "explosiveprojectile.h"
#include "infoconsole.h"
#include "sound.h"
#include "gamehelper.h"
#include "unit.h"
#include "team.h"
#include ".\bombdropper.h"
#include "weaponprojectile.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBombDropper::CBombDropper(CUnit* owner)
: CWeapon(owner)
{
	onlyForward=true;
}

CBombDropper::~CBombDropper()
{

}

void CBombDropper::Update()
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		subClassReady=false;
		if(weaponPos.y>targetPos.y){
			float d=targetPos.y-weaponPos.y;
			float s=-owner->speed.y;
			float sq=(s-2*d)/-gs->gravity;
			if(sq>0)
				predict=-s/-gs->gravity+sqrt(sq);
			else
				predict=0;
			float3 hitpos=owner->pos+owner->speed*predict;
			float speedf=owner->speed.Length();
			if(hitpos.distance2D(targetPos)<(salvoSize-1)*speedf*salvoDelay*0.5+3){
				subClassReady=true;
			}
		}
	}
	CWeapon::Update();
}

bool CBombDropper::TryTarget(const float3& pos,bool userTarget,CUnit* unit)
{
	if(unit){
		if(unit->isUnderWater)
			return false;
	} else {
		if(pos.y<0)
			return false;
	}
	return true;
}

void CBombDropper::Fire(void)
{
	new CExplosiveProjectile(owner->pos,owner->speed,owner,damages, weaponDef, 1000,areaOfEffect);
	//CWeaponProjectile::CreateWeaponProjectile(owner->pos,owner->speed,owner, NULL, float3(0,0,0), damages, weaponDef);
	if(fireSoundId)
		sound->PlaySound(fireSoundId,owner,fireSoundVolume);
}

void CBombDropper::Init(void)
{
	CWeapon::Init();
	maxAngleDif=-1;
}
