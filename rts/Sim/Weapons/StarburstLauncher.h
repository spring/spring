/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STARBURSTLAUNCHER_H
#define STARBURSTLAUNCHER_H

#include "Weapon.h"

class CStarburstLauncher: public CWeapon
{
	CR_DECLARE(CStarburstLauncher)
public:
	CStarburstLauncher(CUnit* owner, const WeaponDef* def);

	float GetRange2D(float yDiff) const;

private:
	bool HaveFreeLineOfFire(const float3 pos, const SWeaponTarget& trg) const override final;
	void FireImpl(bool scriptCall);

private:
	float tracking;
	float uptime;
};

#endif /* STARBURSTLAUNCHER_H */
