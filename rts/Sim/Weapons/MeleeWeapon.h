/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MELEEWEAPON_H
#define MELEEWEAPON_H

#include "Weapon.h"

class CMeleeWeapon: public CWeapon
{
	CR_DECLARE_DERIVED(CMeleeWeapon)
public:
	CMeleeWeapon(CUnit* owner, const WeaponDef* def);

private:
	bool HaveFreeLineOfFire(const float3 pos, const SWeaponTarget& trg, bool useMuzzle = false) const override final;
	void FireImpl(const bool scriptCall) override final;
};

#endif /* MELEEWEAPON_H */
