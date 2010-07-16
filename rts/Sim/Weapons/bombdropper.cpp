/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "bombdropper.h"
#include "Game/GameHelper.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/Team.h"
#include "LogOutput.h"
#include "Map/MapInfo.h"
#include "Sim/MoveTypes/TAAirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/ExplosiveProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/TorpedoProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/Unit.h"
#include "WeaponDefHandler.h"
#include "myMath.h"
#include "mmgr.h"

CR_BIND_DERIVED(CBombDropper, CWeapon, (NULL, false));

CR_REG_METADATA(CBombDropper,(
	CR_MEMBER(dropTorpedoes),
	CR_MEMBER(bombMoveRange),
	CR_MEMBER(tracking),
	CR_RESERVED(16)
	));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBombDropper::CBombDropper(CUnit* owner,bool useTorps)
: CWeapon(owner),
	dropTorpedoes(useTorps),
	bombMoveRange(3),
	tracking(0)
{
	onlyForward=true;

	if(useTorps && owner)
		owner->hasUWWeapons=true;
}

CBombDropper::~CBombDropper()
{

}

void CBombDropper::Update()
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		subClassReady=false;
		if(targetType==Target_Unit)
			targetPos=targetUnit->pos;		//aim at base of unit instead of middle and ignore uncertainity
		if(weaponPos.y>targetPos.y){
			float d=targetPos.y-weaponPos.y;
			float s=-owner->speed.y;
			float sq=(s-2*d)/-(weaponDef->myGravity==0 ? mapInfo->map.gravity : -(weaponDef->myGravity));
			if(sq>0)
				predict=s/(weaponDef->myGravity==0 ? mapInfo->map.gravity : -(weaponDef->myGravity))+sqrt(sq);
			else
				predict=0;
			float3 hitpos=owner->pos+owner->speed*predict;
			float speedf=owner->speed.Length();
			if(hitpos.SqDistance2D(targetPos)<Square(std::max(1,(salvoSize-1))*speedf*salvoDelay*0.5f+bombMoveRange)){
				subClassReady=true;
			}
		}
	}
	CWeapon::Update();
}

bool CBombDropper::TryTarget(const float3& pos,bool userTarget,CUnit* unit)
{
	if(unit){
		if(unit->isUnderWater && !dropTorpedoes)
			return false;
	} else {
		if(pos.y<0)
			return false;
	}
	return true;
}

void CBombDropper::FireImpl(void)
{
	if(targetType==Target_Unit) {
		targetPos=targetUnit->pos;		//aim at base of unit instead of middle and ignore uncertainity
	}

	if(dropTorpedoes){
		float3 speed=owner->speed;
		if(dynamic_cast<CTAAirMoveType*>(owner->moveType)){
			speed=targetPos-weaponPos;
			speed.Normalize();
			speed*=5;
		}
		int ttl;
		if(weaponDef->flighttime == 0) {
			ttl = (int)(range/projectileSpeed+15+predict);
		} else {
			ttl = weaponDef->flighttime;
		}
		new CTorpedoProjectile(weaponPos, speed, owner, areaOfEffect,
			projectileSpeed, tracking, ttl, targetUnit, weaponDef);
	} else {
		float3 dif=targetPos-weaponPos;		//fudge a bit better lateral aim to compensate for imprecise aircraft steering
		dif.y=0;
		float3 dir=owner->speed;
		dir.SafeNormalize();
		dir+=(gs->randVector()*sprayAngle+salvoError)*(1-owner->limExperience*0.9f); //add a random spray
		dir.y=std::min(0.0f,dir.y);
		dir.SafeNormalize();
		dif-=dir*dif.dot(dir);
		dif/=std::max(0.01f,predict);
		float size=dif.Length();
		if(size>1.0f)
			dif/=size*1.0f;
		new CExplosiveProjectile(weaponPos, owner->speed + dif, owner,
				weaponDef, 1000, areaOfEffect,
				weaponDef->myGravity==0 ? mapInfo->map.gravity : -(weaponDef->myGravity));
	}
}

void CBombDropper::Init(void)
{
	CWeapon::Init();
	maxAngleDif=-1;
}

/** See SlowUpdate */
bool CBombDropper::AttackUnit(CUnit* unit, bool userTarget)
{
	if (!userTarget) return false;
	return CWeapon::AttackUnit(unit, userTarget);
}

/** See SlowUpdate */
bool CBombDropper::AttackGround(float3 pos, bool userTarget)
{
	if (!userTarget) return false;
	return CWeapon::AttackGround(pos, userTarget);
}

/** Make sure it wont try to find targets not targeted by the cai
(to save cpu mostly) */
void CBombDropper::SlowUpdate(void)
{
	CWeapon::SlowUpdate(true);
}
