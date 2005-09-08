#include "StdAfx.h"
#include "MissileLauncher.h"
#include "Sound.h"
#include "MissileProjectile.h"
#include "GameHelper.h"
#include "Unit.h"
#include "Ground.h"
#include "WeaponProjectile.h"
#include "AirMoveType.h"
#include "WeaponDefHandler.h"
//#include "mmgr.h"

CMissileLauncher::CMissileLauncher(CUnit* owner)
: CWeapon(owner)
{
}

CMissileLauncher::~CMissileLauncher(void)
{
}

void CMissileLauncher::Update(void)
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		if(!onlyForward){		
			wantedDir=targetPos-weaponPos;
			float dist=wantedDir.Length();
			predict=dist/projectileSpeed;
			wantedDir/=dist;
			if(weaponDef->trajectoryHeight>0){
				wantedDir.y+=weaponDef->trajectoryHeight;
				wantedDir.Normalize();
			}
		}
	}
	CWeapon::Update();
}

void CMissileLauncher::Fire(void)
{
	float3 dir;
	if(onlyForward){
		dir=owner->frontdir;
	} else {
		dir=targetPos-weaponPos;
		dir.Normalize();
		if(weaponDef->trajectoryHeight>0){
			dir.y+=weaponDef->trajectoryHeight;
			dir.Normalize();
		}
	}
	float3 startSpeed=dir*weaponDef->startvelocity;
	if(onlyForward && dynamic_cast<CAirMoveType*>(owner->moveType))
		startSpeed+=owner->speed;

	new CMissileProjectile(weaponPos,startSpeed,owner,damages,areaOfEffect,projectileSpeed,(int)(range/projectileSpeed+25),targetUnit, weaponDef,targetPos);
	//CWeaponProjectile::CreateWeaponProjectile(weaponPos,startSpeed,owner,targetUnit, float3(0,0,0), weaponDef);
	if(fireSoundId)
		sound->PlaySound(fireSoundId,owner,fireSoundVolume);
}

bool CMissileLauncher::TryTarget(const float3& pos,bool userTarget,CUnit* unit)
{
	if(!CWeapon::TryTarget(pos,userTarget,unit))
		return false;

	if(unit){
		if(unit->isUnderWater){
			return false;
		}
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
	} else {
		float3 goaldir=pos-owner->pos;
		goaldir.Normalize();
		if(owner->frontdir.dot(goaldir) < maxAngleDif)
			return false;
	}
	if(helper->TestCone(weaponPos,dir,length,(accuracy+sprayangle),owner->allyteam,owner))
		return false;
	return true;
}
