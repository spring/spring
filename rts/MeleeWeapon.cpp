#include "stdafx.h"
// MeleeWeapon.cpp: implementation of the CMeleeWeapon class.
//
//////////////////////////////////////////////////////////////////////

#include "MeleeWeapon.h"
#include "unit.h"
#include "sound.h"
#include ".\meleeweapon.h"
//#include "mmgr.h"

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
		targetUnit->DoDamage(damages,owner,ZeroVector);
		if(fireSoundId)
			sound->PlaySound(fireSoundId,owner,fireSoundVolume);
	}
}
