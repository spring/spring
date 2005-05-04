#pragma once
#include "weapon.h"

class CStarburstLauncher :
	public CWeapon
{
public:
	CStarburstLauncher(CUnit* owner);
	~CStarburstLauncher(void);

	void Fire(void);
	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

	float tracking;
	float uptime;
};

