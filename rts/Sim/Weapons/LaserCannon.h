/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LASERCANNON_H
#define LASERCANNON_H

#include "Weapon.h"

class CLaserCannon: public CWeapon
{
	CR_DECLARE_DERIVED(CLaserCannon)
public:
	CLaserCannon(CUnit* owner = nullptr, const WeaponDef* def = nullptr);

	void UpdateProjectileSpeed(const float val) override final;
	void UpdateRange(const float val) override final;

private:
	void FireImpl(const bool scriptCall) override final;

private:
	float3 color;
};

#endif /* LASERCANNON_H */
