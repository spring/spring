#include "StdAfx.h"
#include "DGunWeapon.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/WeaponProjectiles/FireBallProjectile.h"
#include "Sim/Units/Unit.h"
#include "WeaponDefHandler.h"
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

void CDGunWeapon::FireImpl()
{
	float3 dir;
	if(onlyForward){
		dir=owner->frontdir;
	} else {
		dir=targetPos-weaponMuzzlePos;
		dir.Normalize();
	}
	dir+=(gs->randVector()*sprayAngle+salvoError)*(1-owner->limExperience*0.5f);
	dir.Normalize();

	new CFireBallProjectile(weaponMuzzlePos, dir * projectileSpeed, owner, 0, targetPos, weaponDef);
}


void CDGunWeapon::Init(void)
{
	CWeapon::Init();
	muzzleFlareSize=1.5f;
}
