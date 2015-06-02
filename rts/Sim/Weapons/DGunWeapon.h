/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _DGUN_WEAPON_H
#define _DGUN_WEAPON_H

#include "Weapon.h"

class CDGunWeapon: public CWeapon
{
	CR_DECLARE(CDGunWeapon)
public:
	CDGunWeapon(CUnit* owner, const WeaponDef* def);

	void Fire();
	float GetPredictFactor(float3 p) const override final;
	void Init() override final;

private:
	void FireImpl(const bool scriptCall) override final;
};

#endif // _DGUN_WEAPON_H

