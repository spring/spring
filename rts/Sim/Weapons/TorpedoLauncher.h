/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TORPEDOLAUNCHER_H
#define TORPEDOLAUNCHER_H

#include "Weapon.h"

class CTorpedoLauncher :
	public CWeapon
{
	CR_DECLARE(CTorpedoLauncher);
public:
	CTorpedoLauncher(CUnit* owner);

	void Update();

	float tracking;

private:
	bool TestTarget(const float3& pos, bool userTarget, const CUnit* unit) const;
	void FireImpl();
};


#endif /* TORPEDOLAUNCHER_H */
