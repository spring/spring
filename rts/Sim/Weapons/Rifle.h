/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RIFLE_H
#define RIFLE_H

#include "Weapon.h"

class CRifle: public CWeapon
{
	CR_DECLARE(CRifle);
public:
	CRifle(CUnit* owner, const WeaponDef* def);

	void Update();

private:
	void FireImpl(bool scriptCall);
};

#endif /* RIFLE_H */
