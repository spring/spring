/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __EMG_CANNON_H__
#define __EMG_CANNON_H__

#include "Weapon.h"

class CEmgCannon :
	public CWeapon
{
	CR_DECLARE(CEmgCannon);
public:
	CEmgCannon(CUnit* owner);
	~CEmgCannon(void);

	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

	void Init(void);

private:
	virtual void FireImpl();
};

#endif // __EMG_CANNON_H__

