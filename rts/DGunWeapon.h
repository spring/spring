#pragma once
#include "weapon.h"

class CDGunWeapon :
	public CWeapon
{
public:
	CDGunWeapon(CUnit* owner);
	~CDGunWeapon(void);
	void Fire(void);
	void Update(void);
	void Init(void);
};
