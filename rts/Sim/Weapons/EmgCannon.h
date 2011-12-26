/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _EMG_CANNON_H
#define _EMG_CANNON_H

#include "Weapon.h"

class CEmgCannon : public CWeapon
{
	CR_DECLARE(CEmgCannon);
public:
	CEmgCannon(CUnit* owner);
	~CEmgCannon();

	void Update();
	bool TryTarget(const float3& pos, bool userTarget, CUnit* unit);

	void Init();

private:
	virtual void FireImpl();
};

#endif // _EMG_CANNON_H

