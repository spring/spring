#pragma once
#include "weapon.h"

class CNoWeapon :
	public CWeapon
{
public:
	CNoWeapon(CUnit *owner);
	CNoWeapon();
	~CNoWeapon(void);

	void Update(void);
	void SlowUpdate(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

	void Init(void);
	void Fire(void);
};
