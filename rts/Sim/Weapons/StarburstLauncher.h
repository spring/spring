/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STARBURSTLAUNCHER_H
#define STARBURSTLAUNCHER_H

#include "Weapon.h"

class CStarburstLauncher: public CWeapon
{
	CR_DECLARE_DERIVED(CStarburstLauncher)
public:
	CStarburstLauncher(CUnit* owner, const WeaponDef* def);

	float GetRange2D(const float yDiff) const override final;

private:
	bool HaveFreeLineOfFire(const float3 pos, const SWeaponTarget& trg, bool useMuzzle = false) const override final;
	void FireImpl(const bool scriptCall) override final;

private:
	float tracking;
	float uptime;
};

#endif /* STARBURSTLAUNCHER_H */
