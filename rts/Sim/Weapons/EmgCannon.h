/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EMG_CANNON_H
#define _EMG_CANNON_H

#include "Weapon.h"

class CEmgCannon: public CWeapon
{
	CR_DECLARE(CEmgCannon)
public:
	CEmgCannon(CUnit* owner, const WeaponDef* def);

private:
	void FireImpl(const bool scriptCall) override final;
};

#endif // _EMG_CANNON_H

