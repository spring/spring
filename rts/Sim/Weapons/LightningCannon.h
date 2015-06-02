/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LIGHTNINGCANNON_H
#define LIGHTNINGCANNON_H

#include "Weapon.h"

class CLightningCannon: public CWeapon
{
	CR_DECLARE(CLightningCannon)
public:
	CLightningCannon(CUnit* owner, const WeaponDef* def);

private:
	void FireImpl(const bool scriptCall) override final;

private:
	float3 color;
};


#endif /* LIGHTNINGCANNON_H */
