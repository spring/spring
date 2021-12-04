/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _DGUN_WEAPON_H
#define _DGUN_WEAPON_H

#include "Weapon.h"

class CDGunWeapon: public CWeapon
{
	CR_DECLARE_DERIVED(CDGunWeapon)
public:
	CDGunWeapon(CUnit* owner = nullptr, const WeaponDef* def = nullptr): CWeapon(owner, def) {}

	void Fire();
	float GetPredictedImpactTime(float3 p) const override final { return 0.0f; }
	void Init() override final;

private:
	void FireImpl(const bool scriptCall) override final;
};

#endif // _DGUN_WEAPON_H

