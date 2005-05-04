#pragma once
#include "weapon.h"

class CFlameThrower :
	public CWeapon
{
public:
	CFlameThrower(CUnit* owner);
	~CFlameThrower(void);
	void Fire(void);
	bool TryTarget(const float3 &pos,bool userTarget,CUnit* unit);
	void Update(void);
};
