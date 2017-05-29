/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EMG_CANNON_H
#define _EMG_CANNON_H

#include "Weapon.h"

class CEmgCannon: public CWeapon
{
	CR_DECLARE_DERIVED(CEmgCannon)
public:
	CEmgCannon(CUnit* owner = nullptr, const WeaponDef* def = nullptr): CWeapon(owner, def) {}

private:
	void FireImpl(const bool scriptCall) override final;
};

#endif // _EMG_CANNON_H

