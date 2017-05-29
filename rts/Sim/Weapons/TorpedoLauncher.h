/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TORPEDOLAUNCHER_H
#define TORPEDOLAUNCHER_H

#include "Weapon.h"

class CTorpedoLauncher: public CWeapon
{
	CR_DECLARE_DERIVED(CTorpedoLauncher)
public:
	CTorpedoLauncher(CUnit* owner = nullptr, const WeaponDef* def = nullptr);

private:
	bool TestTarget(const float3 pos, const SWeaponTarget& trg) const override final;
	void FireImpl(const bool scriptCall) override final;

private:
	float tracking;
};


#endif /* TORPEDOLAUNCHER_H */
