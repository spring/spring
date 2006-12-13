#include "StdAfx.h"
#include "MissileLauncher.h"
#include "Sound.h"
#include "Sim/Projectiles/MissileProjectile.h"
#include "Game/GameHelper.h"
#include "Sim/Units/Unit.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectile.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"

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

	SAFE_NEW CMissileProjectile(weaponPos,startSpeed,owner,areaOfEffect,projectileSpeed,(int)(range/projectileSpeed+25),targetUnit, weaponDef,targetPos);
	//CWeaponProjectile::CreateWeaponProjectile(weaponPos,startSpeed,owner,targetUnit, float3(0,0,0), weaponDef);
	if(fireSoundId && (!weaponDef->soundTrigger || salvoLeft==salvoSize-1))
		sound->PlaySample(fireSoundId,owner,fireSoundVolume);
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
	if(weaponDef->trajectoryHeight>0){	//do a different test depending on if the missile has a high trajectory or not
		float3 flatdir(dir.x,0,dir.z);
		dir.Normalize();
		float flatlength=flatdir.Length();
		if(flatlength==0)
			return true;
		flatdir/=flatlength;

		float linear=dir.y+weaponDef->trajectoryHeight;
		float quadratic=-weaponDef->trajectoryHeight/flatlength;

		float gc=ground->TrajectoryGroundCol(weaponPos,flatdir,flatlength-30,linear,quadratic);
		if(gc>0)
			return false;

		if(avoidFriendly && helper->TestTrajectoryCone(weaponPos,flatdir,flatlength-30,linear,quadratic,0,8,owner->allyteam,owner)){
			return false;
		}
	} else {
		float length=dir.Length();
		if(length==0)
			return true;

		dir/=length;

		if(!onlyForward){		//skip ground col testing for aircrafts
			float g=ground->LineGroundCol(weaponPos,pos);
			if(g>0 && g<length*0.9f)
				return false;
		} else {
			float3 goaldir=pos-owner->pos;
			goaldir.Normalize();
			if(owner->frontdir.dot(goaldir) < maxAngleDif)
				return false;
		}
		if(avoidFriendly && helper->TestCone(weaponPos,dir,length,(accuracy+sprayangle),owner->allyteam,owner))
			return false;
	}
	return true;
}
