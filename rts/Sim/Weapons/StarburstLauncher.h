/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STARBURSTLAUNCHER_H
#define STARBURSTLAUNCHER_H

#include "Weapon.h"

class CStarburstLauncher :
	public CWeapon
{
	CR_DECLARE(CStarburstLauncher);
public:
	CStarburstLauncher(CUnit* owner);
	~CStarburstLauncher();

	void Update();
	float GetRange2D(float yDiff) const;

	float tracking;
	float uptime;

private:
	bool HaveFreeLineOfFire(const float3& pos,bool userTarget,CUnit* unit) const;
	void FireImpl();
};

#endif /* STARBURSTLAUNCHER_H */
