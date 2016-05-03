/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MeleeWeapon.h"
#include "WeaponDef.h"
#include "Sim/Units/Unit.h"

CR_BIND_DERIVED(CMeleeWeapon, CWeapon, (NULL, NULL))
CR_REG_METADATA(CMeleeWeapon, )

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMeleeWeapon::CMeleeWeapon(CUnit* owner, const WeaponDef* def): CWeapon(owner, def)
{
}


bool CMeleeWeapon::HaveFreeLineOfFire(const float3 pos, const SWeaponTarget& trg, bool useMuzzle) const
{
	return true;
}

void CMeleeWeapon::FireImpl(const bool scriptCall)
{
	if (currentTarget.type == Target_Unit) {
		const float3 impulseVec = wantedDir * owner->mass * damages->impulseFactor;

		// the heavier the unit, the more impulse it does
		currentTarget.unit->DoDamage(*damages, impulseVec, owner, weaponDef->id, -1);
	}
}
