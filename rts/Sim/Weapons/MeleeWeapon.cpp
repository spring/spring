#include "StdAfx.h"
// MeleeWeapon.cpp: implementation of the CMeleeWeapon class.
//
//////////////////////////////////////////////////////////////////////

#include "MeleeWeapon.h"
#include "Sim/Units/Unit.h"
#include "Sound.h"
#include "Sim/Weapons/WeaponDefHandler.h"
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
	CWeapon::Update();

}

void CMeleeWeapon::Fire(void)
{
	if(targetType==Target_Unit){
		float3 impulseDir = targetUnit->pos-weaponPos;
		impulseDir.Normalize();
		targetUnit->DoDamage(damages,owner,impulseDir,weaponDef->id);
		if(fireSoundId)
			sound->PlaySample(fireSoundId,owner,fireSoundVolume);
	}
}
