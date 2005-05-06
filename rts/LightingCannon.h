#pragma once
#include "Weapon.h"

class CLightingCannon :
	public CWeapon
{
public:
	CLightingCannon(CUnit* owner);
	~CLightingCannon(void);

	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

	void Init(void);
	void Fire(void);

	float3 color;
	void SlowUpdate(void);
};

