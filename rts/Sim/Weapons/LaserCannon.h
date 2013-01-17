/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LASERCANNON_H
#define LASERCANNON_H

#include "Weapon.h"

class CLaserCannon :
	public CWeapon
{
	CR_DECLARE(CLaserCannon);
public:
	CLaserCannon(CUnit* owner);

	void Update();
	void Init();

	float3 color;

private:
	virtual void FireImpl();
};

#endif /* LASERCANNON_H */
