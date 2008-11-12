#include "StdAfx.h"
#include "DGunWeapon.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/WeaponProjectiles/FireBallProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sound.h"
#include "WeaponDefHandler.h"
#include "myMath.h"
#include "mmgr.h"

CR_BIND_DERIVED(CDGunWeapon, CWeapon, (NULL));

CR_REG_METADATA(CDGunWeapon,(
				CR_RESERVED(8)
				));

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
		weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
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
	if(uh->limitDgun && owner->unitDef->isCommander && owner->pos.SqDistance(teamHandler->Team(owner->team)->startPos)>Square(uh->dgunRadius)){
		return;		//prevents dgunning using fps view if outside dgunlimit
	}

	float3 dir;
	if(onlyForward){
		dir=owner->frontdir;
	} else {
		dir=targetPos-weaponMuzzlePos;
		dir.Normalize();
	}
	dir+=(gs->randVector()*sprayAngle+salvoError)*(1-owner->limExperience*0.5f);
	dir.Normalize();

	SAFE_NEW CFireBallProjectile(weaponMuzzlePos, dir * projectileSpeed, owner, 0, targetPos, weaponDef);

	if (fireSoundId && (!weaponDef->soundTrigger || salvoLeft == salvoSize - 1))
		sound->PlaySample(fireSoundId, owner, fireSoundVolume * 0.2f);
}


void CDGunWeapon::Init(void)
{
	CWeapon::Init();
	muzzleFlareSize=1.5f;
}
