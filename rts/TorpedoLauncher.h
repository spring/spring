#pragma once
#include "weapon.h"

class CTorpedoLauncher :
	public CWeapon
{
public:
	CTorpedoLauncher(CUnit* owner);
	virtual ~CTorpedoLauncher(void);

	void Fire(void);
	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

	float tracking;
};

