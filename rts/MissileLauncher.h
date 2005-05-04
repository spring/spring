#pragma once
#include "weapon.h"

class CMissileLauncher :
	public CWeapon
{
public:
	CMissileLauncher(CUnit* owner);
	~CMissileLauncher(void);

	void Fire(void);
	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

	float tracking;
};

