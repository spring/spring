#include "StdAfx.h"
#include "EmgCannon.h"
#include "Sim/Units/Unit.h"
#include "Game/Team.h"
#include "Sim/Projectiles/TracerProjectile.h"
#include "Sound.h"
#include "Game/GameHelper.h"
#include "Sim/Projectiles/EmgProjectile.h"
#include "Map/Ground.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "SyncTracer.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"

CEmgCannon::CEmgCannon(CUnit* owner)
: CWeapon(owner)
{
}

CEmgCannon::~CEmgCannon(void)
{
}

void CEmgCannon::Update(void)
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

bool CEmgCannon::TryTarget(const float3& pos,bool userTarget,CUnit* unit)
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

	if(avoidFriendly && helper->TestCone(weaponPos,dir,length,(accuracy+sprayangle)*(1-owner->limExperience*0.5),owner->allyteam,owner))
		return false;
	return true;
}

void CEmgCannon::Init(void)
{
	CWeapon::Init();
}

void CEmgCannon::Fire(void)
{
#ifdef TRACE_SYNC
	tracefile << "Emg fire: ";
	tracefile << sprayangle << " " << gs->randSeed << " " << salvoError.x << " " << salvoError.z << " " << owner->limExperience << " " << projectileSpeed << "\n";
#endif
	float3 dir;
	if(onlyForward && dynamic_cast<CAirMoveType*>(owner->moveType)){		//the taairmovetype cant align itself properly, change back when that is fixed
		dir=owner->frontdir;
	} else {
		dir=targetPos-weaponPos;
		dir.Normalize();
	}
	dir+=(gs->randVector()*sprayangle+salvoError)*(1-owner->limExperience*0.5);
	dir.Normalize();

	new CEmgProjectile(weaponPos,dir*projectileSpeed,owner,damages,weaponDef->visuals.color,weaponDef->intensity,(int)(range/projectileSpeed), weaponDef);
	if(fireSoundId && (!weaponDef->soundTrigger || salvoLeft==salvoSize-1))
		sound->PlaySample(fireSoundId,owner,fireSoundVolume);
}

