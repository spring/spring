/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MELEEWEAPON_H
#define MELEEWEAPON_H

#include "Weapon.h"

class CMeleeWeapon : public CWeapon  
{
	CR_DECLARE(CMeleeWeapon);
public:
	CMeleeWeapon(CUnit* owner);
	~CMeleeWeapon();

	void Update();

private:
	bool HaveFreeLineOfFire(const float3& pos, bool userTarget, CUnit* unit) const;
	void FireImpl();
};

#endif /* MELEEWEAPON_H */
