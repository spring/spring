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
	virtual ~CTorpedoLauncher(void);

	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

	float tracking;

private:
	virtual void FireImpl(void);
};


#endif /* TORPEDOLAUNCHER_H */
