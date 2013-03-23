/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MeleeWeapon.h"
#include "WeaponDef.h"
#include "Sim/Units/Unit.h"

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


void CMeleeWeapon::Update()
{
	if (targetType != Target_None) {
		weaponPos = owner->pos +
			owner->frontdir * relWeaponPos.z +
			owner->updir    * relWeaponPos.y +
			owner->rightdir * relWeaponPos.x;
		weaponMuzzlePos = owner->pos +
			owner->frontdir * relWeaponMuzzlePos.z +
			owner->updir    * relWeaponMuzzlePos.y +
			owner->rightdir * relWeaponMuzzlePos.x;

		if (!onlyForward) {
			wantedDir = targetPos - weaponPos;
			wantedDir.Normalize();
		}
	}

	CWeapon::Update();
}

bool CMeleeWeapon::HaveFreeLineOfFire(const float3& pos, bool userTarget, const CUnit* unit) const
{
	return true;
}

void CMeleeWeapon::FireImpl()
{
	if (targetType == Target_Unit) {
		const float3 impulseDir = (targetUnit->pos - weaponMuzzlePos).Normalize();
		const float3 impulseVec = impulseDir * owner->mass * weaponDef->damages.impulseFactor;

		// the heavier the unit, the more impulse it does
		targetUnit->DoDamage(weaponDef->damages, impulseVec, owner, weaponDef->id, -1);
	}
}
