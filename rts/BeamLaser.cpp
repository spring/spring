#include "StdAfx.h"
#include "BeamLaser.h"
#include "Unit.h"
#include "Sound.h"
#include "GameHelper.h"
#include "LaserProjectile.h"
#include "Ground.h"
#include "WeaponDefHandler.h"
#include "AirMoveType.h"
#include "InfoConsole.h"
#include "BeamLaserProjectile.h"
//#include "mmgr.h"

CBeamLaser::CBeamLaser(CUnit* owner)
: CWeapon(owner),
	oldDir(ZeroVector)
{
}

CBeamLaser::~CBeamLaser(void)
{
}

void CBeamLaser::Update(void)
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		if(!onlyForward){		
			wantedDir=targetPos-weaponPos;
			wantedDir.Normalize();
		}
		predict=salvoSize/2;
	}
	CWeapon::Update();
}

bool CBeamLaser::TryTarget(const float3& pos,bool userTarget,CUnit* unit)
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

void CBeamLaser::Init(void)
{
	salvoDelay=0;
	salvoSize=(int)weaponDef->beamtime*30;
	damages=damages*(1.0f/salvoSize);		//restate the damage from damage per salvo to damage per frame (shot)

	CWeapon::Init();
}

void CBeamLaser::Fire(void)
{
	float3 dir;
	if(onlyForward && dynamic_cast<CAirMoveType*>(owner->moveType)){		//the taairmovetype cant align itself properly, change back when that is fixed
		dir=owner->frontdir;
	} else {
		if(salvoLeft==salvoSize-1){
			if(fireSoundId)
				sound->PlaySound(fireSoundId,owner,fireSoundVolume);
			dir=targetPos-weaponPos;
			dir.Normalize();
			oldDir=dir;
		} else {
			dir=oldDir;
		}
	}
	dir+=(salvoError)*(1-owner->limExperience*0.7);
	dir.Normalize();

	CUnit* hit;
	float length=helper->TraceRay(weaponPos,dir,range*1.3,damages[0],owner,hit);
	float3 hitPos=weaponPos+dir*length;
	float intensity=1-length/(range*2);

	if(length<range*1.05)
		helper->Explosion(hitPos,damages*intensity,areaOfEffect,owner);

	float startAlpha=weaponDef->intensity*255;
	float endAlpha=(1-length/(range*1.3))*startAlpha;

	new CBeamLaserProjectile(weaponPos,hitPos,startAlpha,endAlpha,color,owner,weaponDef->thickness);
}
