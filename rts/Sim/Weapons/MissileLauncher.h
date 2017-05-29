/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MISSILELAUNCHER_H
#define MISSILELAUNCHER_H

#include "Weapon.h"

class CMissileLauncher: public CWeapon
{
	CR_DECLARE_DERIVED(CMissileLauncher)
public:
	CMissileLauncher(CUnit* owner = nullptr, const WeaponDef* def = nullptr): CWeapon(owner, def) {}

	void UpdateWantedDir() override final;

private:
	const float3& GetAimFromPos(bool useMuzzle = false) const override { return weaponMuzzlePos; }

	bool HaveFreeLineOfFire(const float3 srcPos, const float3 tgtPos, const SWeaponTarget& trg) const override final;
	void FireImpl(const bool scriptCall) override final;
};


#endif /* MISSILELAUNCHER_H */
