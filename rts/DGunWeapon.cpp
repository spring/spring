#include "StdAfx.h"
#include "DGunWeapon.h"
#include "FireBallProjectile.h"
#include "Unit.h"
#include "Sound.h"
#include "WeaponDefHandler.h"
//#include "mmgr.h"

CDGunWeapon::CDGunWeapon(CUnit* owner)
: CWeapon(owner)
{
}

CDGunWeapon::~CDGunWeapon(void)
{
}

void CDGunWeapon::Update(void)
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		if(!onlyForward){		
			wantedDir=targetPos-weaponPos;
			wantedDir.Normalize();
		}
		predict=0;		//have to manually predict
	}
	CWeapon::Update();
}

void CDGunWeapon::Fire(void)
{
	float3 dir;
	if(onlyForward){
		dir=owner->frontdir;
	} else {
		dir=targetPos-weaponPos;
		dir.Normalize();
	}
	dir+=(gs->randVector()*sprayangle+salvoError)*(1-owner->limExperience*0.5);
	dir.Normalize();

	new CFireBallProjectile(weaponPos,dir*projectileSpeed,owner,0,targetPos,weaponDef);
	if(fireSoundId && (!weaponDef->soundTrigger || salvoLeft==salvoSize-1))
		sound->PlaySound(fireSoundId,owner,fireSoundVolume*0.2);
}


void CDGunWeapon::Init(void)
{
	CWeapon::Init();
	muzzleFlareSize=1.5;
}
