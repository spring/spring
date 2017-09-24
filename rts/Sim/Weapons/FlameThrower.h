/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FLAME_THROWER_H
#define _FLAME_THROWER_H

#include "Weapon.h"

class CFlameThrower: public CWeapon
{
	CR_DECLARE_DERIVED(CFlameThrower)
public:
	CFlameThrower(CUnit* owner = nullptr, const WeaponDef* def = nullptr): CWeapon(owner, def) {}

	float3 color;
	float3 color2;

private:
	void FireImpl(const bool scriptCall) override final;
};

#endif // _FLAME_THROWER_H
