/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MELEEWEAPON_H
#define MELEEWEAPON_H

#include "Weapon.h"

class CMeleeWeapon: public CWeapon
{
	CR_DECLARE_DERIVED(CMeleeWeapon)
public:
	CMeleeWeapon(CUnit* owner = nullptr, const WeaponDef* def = nullptr): CWeapon(owner, def) {}

private:
	bool HaveFreeLineOfFire(const float3 srcPos, const float3 tgtPos, const SWeaponTarget& trg) const override final { return true; }
	void FireImpl(const bool scriptCall) override final;
};

#endif /* MELEEWEAPON_H */
