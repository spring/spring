/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LIGHTNINGCANNON_H
#define LIGHTNINGCANNON_H

#include "Weapon.h"

class CLightningCannon: public CWeapon
{
	CR_DECLARE_DERIVED(CLightningCannon)
public:
	CLightningCannon(CUnit* owner = nullptr, const WeaponDef* def = nullptr);

private:
	void FireImpl(const bool scriptCall) override final;
	float GetPredictedImpactTime(float3 p) const override final;

private:
	float3 color;
};


#endif /* LIGHTNINGCANNON_H */
