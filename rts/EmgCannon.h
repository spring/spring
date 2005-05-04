#pragma once
#include "weapon.h"

class CEmgCannon :
	public CWeapon
{
public:
	CEmgCannon(CUnit* owner);
	~CEmgCannon(void);

	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

	void Init(void);
	void Fire(void);
};
