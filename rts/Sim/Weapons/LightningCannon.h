/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LIGHTNINGCANNON_H
#define LIGHTNINGCANNON_H

#include "Weapon.h"

class CLightningCannon :
	public CWeapon
{
	CR_DECLARE(CLightningCannon);
public:
	CLightningCannon(CUnit* owner);
	~CLightningCannon();

	void Update();
	void Init();

	float3 color;
	void SlowUpdate();

private:
	virtual void FireImpl();
};


#endif /* LIGHTNINGCANNON_H */
