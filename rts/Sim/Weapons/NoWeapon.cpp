/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "NoWeapon.h"

CR_BIND_DERIVED(CNoWeapon, CWeapon, (NULL, NULL))
CR_REG_METADATA(CNoWeapon, )

CNoWeapon::CNoWeapon(CUnit* owner, const WeaponDef* def) : CWeapon(owner, def)
{
}

