#include "StdAfx.h"
#include "LaserCannon.h"
#include "Unit.h"
#include "Sound.h"
#include "GameHelper.h"
#include "LaserProjectile.h"
#include "Ground.h"
#include "WeaponProjectile.h"
#include "AirMoveType.h"
//#include "mmgr.h"

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

//<<<<<<< LaserCannon.cpp
//	new CLaserProjectile(weaponPos,dir*projectileSpeed,owner,damages,30,color,0.8,weaponDef, range/projectileSpeed-4);
//=======
	//new CLaserProjectile(weaponPos,dir*projectileSpeed,owner,damages,30,color,0.8,weaponDef, range/projectileSpeed);
	CWeaponProjectile::CreateWeaponProjectile(weaponPos,dir*projectileSpeed,owner, NULL, targetPos,  weaponDef);

//>>>>>>> 1.12
	if(fireSoundId)
		sound->PlaySound(fireSoundId,owner,fireSoundVolume);
}

