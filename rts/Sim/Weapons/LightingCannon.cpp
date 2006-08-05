#include "StdAfx.h"
#include "LightingCannon.h"
#include "Sim/Units/Unit.h"
#include "Sim/Projectiles/TracerProjectile.h"
#include "Sound.h"
#include "Game/GameHelper.h"
#include "Sim/Projectiles/LightingProjectile.h"
#include "Map/Ground.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"

CLightingCannon::CLightingCannon(CUnit* owner)
: CWeapon(owner)
{
}

CLightingCannon::~CLightingCannon(void)
{
}

void CLightingCannon::Update(void)
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		if(!onlyForward){		
			wantedDir=targetPos-weaponPos;
			wantedDir.Normalize();
		}
	}
	CWeapon::Update();
}

bool CLightingCannon::TryTarget(const float3& pos,bool userTarget,CUnit* unit)
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
	if(g>0 && g<length*0.9)
		return false;

	if(helper->LineFeatureCol(weaponPos,dir,length))
		return false;

	if(avoidFriendly && helper->TestCone(weaponPos,dir,length,(accuracy+sprayangle),owner->allyteam,owner))
		return false;
	return true;
}

void CLightingCannon::Init(void)
{
	CWeapon::Init();
}

void CLightingCannon::Fire(void)
{
	float3 dir=targetPos-weaponPos;
	dir.Normalize();
	CUnit* u=0;
	float r=helper->TraceRay(weaponPos,dir,range,0,owner,u);

//	if(u)
//		u->DoDamage(damages,owner,ZeroVector);
	helper->Explosion(weaponPos+dir*r,damages,areaOfEffect,weaponDef->edgeEffectivness,weaponDef->explosionSpeed,owner,false,0.5,true);

	new CLightingProjectile(weaponPos,weaponPos+dir*(r+10),owner,color,weaponDef,10,this);
	if(fireSoundId && (!weaponDef->soundTrigger || salvoLeft==salvoSize-1))
		sound->PlaySound(fireSoundId,owner,fireSoundVolume);

}



void CLightingCannon::SlowUpdate(void)
{
	CWeapon::SlowUpdate();
	if(targetType==Target_Unit){
		predict=(gs->randFloat()-0.5)*20*range/weaponPos.distance(targetUnit->midPos)*(1.2-owner->limExperience);		//make the weapon somewhat less effecient against aircrafts hopefully
	}
}
