/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RIFLE_H
#define RIFLE_H

#include "Weapon.h"

class CRifle: public CWeapon
{
	CR_DECLARE_DERIVED(CRifle)
public:
	CRifle(CUnit* owner = nullptr, const WeaponDef* def = nullptr): CWeapon(owner, def) {}

private:
	void FireImpl(const bool scriptCall) override final;
	float GetPredictedImpactTime(float3 p) const override final { return 0.0f; }
};

#endif /* RIFLE_H */
