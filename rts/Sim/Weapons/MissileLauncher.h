/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MISSILELAUNCHER_H
#define MISSILELAUNCHER_H

#include "Weapon.h"

class CMissileLauncher: public CWeapon
{
	CR_DECLARE_DERIVED(CMissileLauncher)
public:
	CMissileLauncher(CUnit* owner, const WeaponDef* def);

	void UpdateWantedDir() override final;

private:
	bool HaveFreeLineOfFire(const float3 pos, const SWeaponTarget& trg, bool useMuzzle = false) const override final;
	void FireImpl(const bool scriptCall) override final;
};


#endif /* MISSILELAUNCHER_H */
