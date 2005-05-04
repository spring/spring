#include "stdafx.h"
#include ".\dgunweapon.h"
#include "fireballprojectile.h"
#include "unit.h"
#include "sound.h"
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
	if(fireSoundId)
		sound->PlaySound(fireSoundId,owner,fireSoundVolume);
}


void CDGunWeapon::Init(void)
{
	CWeapon::Init();
	muzzleFlareSize=1.5;
}
