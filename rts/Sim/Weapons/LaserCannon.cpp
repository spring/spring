#include "StdAfx.h"
#include "LaserCannon.h"
#include "Sim/Units/Unit.h"
#include "Sound.h"
#include "Game/GameHelper.h"
#include "Sim/Projectiles/LaserProjectile.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectile.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"

CLaserCannon::CLaserCannon(CUnit* owner)
: CWeapon(owner)
{
}

CLaserCannon::~CLaserCannon(void)
{
}

void CLaserCannon::Update(void)
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		if(!onlyForward){		
			wantedDir=targetPos-weaponPos;
			wantedDir.Normalize();
		}
		predict=(targetPos-weaponPos).Length()/projectileSpeed;
	}
	CWeapon::Update();
}

bool CLaserCannon::TryTarget(const float3& pos,bool userTarget,CUnit* unit)
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

	if(!onlyForward){		//skip ground col testing for aircrafts
		float g=ground->LineGroundCol(weaponPos,pos);
		if(g>0 && g<length*0.9)
			return false;
	}
	if(helper->LineFeatureCol(weaponPos,dir,length))
		return false;

	if(helper->TestCone(weaponPos,dir,length,(accuracy+sprayangle)*(1-owner->limExperience*0.7),owner->allyteam,owner))
		return false;
	return true;
}

void CLaserCannon::Init(void)
{
	CWeapon::Init();
//	muzzleFlareSize=0.5;
}

void CLaserCannon::Fire(void)
{
	float3 dir;
	if(onlyForward && dynamic_cast<CAirMoveType*>(owner->moveType)){		//the taairmovetype cant align itself properly, change back when that is fixed
		dir=owner->frontdir;
	} else {
		dir=targetPos-weaponPos;
		dir.Normalize();
	}
	dir+=(gs->randVector()*sprayangle+salvoError)*(1-owner->limExperience*0.7);
	dir.Normalize();

	int fpsSub=0;
#ifdef DIRECT_CONTROL_ALLOWED
	if(owner->directControl)
		fpsSub=6;
#endif

	new CLaserProjectile(weaponPos, dir*projectileSpeed, owner, weaponDef->damages, 30, weaponDef->visuals.color, weaponDef->visuals.color2, weaponDef->intensity, weaponDef, (int)((weaponDef->range-fpsSub*4)/weaponDef->projectilespeed)-fpsSub);

	if(fireSoundId && (!weaponDef->soundTrigger || salvoLeft==salvoSize-1))
		sound->PlaySound(fireSoundId,owner,fireSoundVolume);
}

