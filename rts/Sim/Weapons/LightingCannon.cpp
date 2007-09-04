#include "StdAfx.h"
#include "Game/GameHelper.h"
#include "LightingCannon.h"
#include "Map/Ground.h"
#include "PlasmaRepulser.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Projectiles/WeaponProjectiles/LightingProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sound.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"

CR_BIND_DERIVED(CLightingCannon, CWeapon, (NULL));

CR_REG_METADATA(CLightingCannon,(
	CR_MEMBER(color),
	CR_RESERVED(8)
	));

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
		weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
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

	if(!weaponDef->waterweapon) {
		if(unit){
			if(unit->isUnderWater)
				return false;
		} else {
			if(pos.y<0)
				return false;
		}
	}

	float3 dir=pos-weaponMuzzlePos;
	float length=dir.Length();
	if(length==0)
		return true;

	dir/=length;

	float g=ground->LineGroundCol(weaponMuzzlePos,pos);
	if(g>0 && g<length*0.9f)
		return false;

	if(avoidFeature && helper->LineFeatureCol(weaponMuzzlePos,dir,length))
		return false;

	if(avoidFriendly && helper->TestCone(weaponMuzzlePos,dir,length,(accuracy+sprayangle),owner->allyteam,owner))
		return false;
	return true;
}

void CLightingCannon::Init(void)
{
	CWeapon::Init();
}

void CLightingCannon::Fire(void)
{
	float3 dir=targetPos-weaponMuzzlePos;
	dir.Normalize();
	dir+=(gs->randVector()*sprayangle+salvoError)*(1-owner->limExperience*0.5f);
	dir.Normalize();
	CUnit* u=0;
	float r=helper->TraceRay(weaponMuzzlePos,dir,range,0,owner,u);

	float3 newDir;
	CPlasmaRepulser* shieldHit;
	float shieldLength=interceptHandler.AddShieldInterceptableBeam(this,weaponMuzzlePos,dir,range,newDir,shieldHit);
	if(shieldLength<r){
		r=shieldLength;
		if(shieldHit) {
			shieldHit->BeamIntercepted(this);
		}
	}

//	if(u)
//		u->DoDamage(damages,owner,ZeroVector);

	// Dynamic Damage
	DamageArray dynDamages;
	if (weaponDef->dynDamageExp > 0)
		dynDamages = weaponDefHandler->DynamicDamages(weaponDef->damages, weaponMuzzlePos, targetPos, weaponDef->dynDamageRange>0?weaponDef->dynDamageRange:weaponDef->range, weaponDef->dynDamageExp, weaponDef->dynDamageMin, weaponDef->dynDamageInverted);

	helper->Explosion(weaponMuzzlePos+dir*r,weaponDef->dynDamageExp>0?dynDamages:weaponDef->damages,areaOfEffect,weaponDef->edgeEffectiveness,weaponDef->explosionSpeed,owner,false,0.5f,true,weaponDef->explosionGenerator, u,dir, weaponDef->id);

	SAFE_NEW CLightingProjectile(weaponMuzzlePos,weaponMuzzlePos+dir*(r+10),owner,color,weaponDef,10,this);
	if(fireSoundId && (!weaponDef->soundTrigger || salvoLeft==salvoSize-1))
		sound->PlaySample(fireSoundId,owner,fireSoundVolume);

}



void CLightingCannon::SlowUpdate(void)
{
	CWeapon::SlowUpdate();
	//We don't do hardcoded inaccuracies, use targetMoveError if you want inaccuracy!
//	if(targetType==Target_Unit){
//		predict=(gs->randFloat()-0.5f)*20*range/weaponPos.distance(targetUnit->midPos)*(1.2f-owner->limExperience);		//make the weapon somewhat less effecient against aircrafts hopefully
//	}
}
