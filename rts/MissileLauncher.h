#ifndef MISSILELAUNCHER_H
#define MISSILELAUNCHER_H
/*pragma once removed*/
#include "Weapon.h"

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


#endif /* MISSILELAUNCHER_H */
