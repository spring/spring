#pragma once
#include "weapon.h"

class CLaserCannon :
	public CWeapon
{
public:
	CLaserCannon(CUnit* owner);
	~CLaserCannon(void);

	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

	void Init(void);
	void Fire(void);

	float3 color;
};

