/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MISSILELAUNCHER_H
#define MISSILELAUNCHER_H

#include "Weapon.h"

class CMissileLauncher :
	public CWeapon
{
	CR_DECLARE(CMissileLauncher);
public:
	CMissileLauncher(CUnit* owner);
	~CMissileLauncher();

	void Update();


private:
	bool HaveFreeLineOfFire(const float3& pos,bool userTarget,CUnit* unit) const;
	void FireImpl();
};


#endif /* MISSILELAUNCHER_H */
