/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"

#include "MeleeWeapon.h"
#include "Sim/Units/Unit.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"

CR_BIND_DERIVED(CMeleeWeapon, CWeapon, (NULL));

CR_REG_METADATA(CMeleeWeapon,(
	CR_RESERVED(8)
	));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMeleeWeapon::CMeleeWeapon(CUnit* owner)
: CWeapon(owner)
{
}

CMeleeWeapon::~CMeleeWeapon()
{

}

void CMeleeWeapon::Update()
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
		if(!onlyForward){
			wantedDir=targetPos-weaponPos;
			wantedDir.Normalize();
		}
//		predict=(targetPos-weaponPos).Length()/projectileSpeed;
	}
	CWeapon::Update();
}

void CMeleeWeapon::FireImpl()
{
	if(targetType==Target_Unit){
		float3 impulseDir = targetUnit->pos-weaponMuzzlePos;
		impulseDir.Normalize();
		// the heavier the unit, the more impulse it does
		targetUnit->DoDamage(weaponDef->damages, owner, impulseDir * owner->mass * weaponDef->damages.impulseFactor, weaponDef->id);
	}
}
