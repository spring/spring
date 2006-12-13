#include "StdAfx.h"
#include "DGunWeapon.h"
#include "Sim/Projectiles/FireBallProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sound.h"
#include "WeaponDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Game/Team.h"
#include "mmgr.h"

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
	if(uh->limitDgun && owner->unitDef->isCommander && owner->pos.distance(gs->Team(owner->team)->startPos)>uh->dgunRadius){
		return;		//prevents dgunning using fps view if outside dgunlimit
	}

	float3 dir;
	if(onlyForward){
		dir=owner->frontdir;
	} else {
		dir=targetPos-weaponPos;
		dir.Normalize();
	}
	dir+=(gs->randVector()*sprayangle+salvoError)*(1-owner->limExperience*0.5f);
	dir.Normalize();

	SAFE_NEW CFireBallProjectile(weaponPos,dir*projectileSpeed,owner,0,targetPos,weaponDef);
	if(fireSoundId && (!weaponDef->soundTrigger || salvoLeft==salvoSize-1))
		sound->PlaySample(fireSoundId,owner,fireSoundVolume*0.2f);
}


void CDGunWeapon::Init(void)
{
	CWeapon::Init();
	muzzleFlareSize=1.5f;
}
