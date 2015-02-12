/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NOWEAPON_H
#define NOWEAPON_H

#include "Weapon.h"

class CNoWeapon: public CWeapon
{
	CR_DECLARE(CNoWeapon)
public:
	CNoWeapon(CUnit* owner, const WeaponDef* def);

	void Update() {}
	void SlowUpdate() {}
	void Init() {}

private:
	bool TestTarget(const float3& pos, bool userTarget, const CUnit* unit) const { return false; }
	void FireImpl(bool scriptCall) {}
};


#endif /* NOWEAPON_H */
