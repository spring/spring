/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FLAME_THROWER_H
#define _FLAME_THROWER_H

#include "Weapon.h"

class CFlameThrower : public CWeapon
{
	CR_DECLARE(CFlameThrower);
public:
	CFlameThrower(CUnit* owner);
	~CFlameThrower();
	void Update();

	float3 color;
	float3 color2;

private:
	virtual void FireImpl();
};

#endif // _FLAME_THROWER_H
