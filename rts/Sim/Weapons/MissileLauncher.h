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
	~CMissileLauncher(void);

	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

private:
	virtual void FireImpl();
};


#endif /* MISSILELAUNCHER_H */
