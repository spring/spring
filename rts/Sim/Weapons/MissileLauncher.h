#ifndef MISSILELAUNCHER_H
#define MISSILELAUNCHER_H

#include "Weapon.h"

class CMissileLauncher :
	public CWeapon
{
	CR_DECLARE(CMissileLauncher);
public:
	CMissileLauncher(CUnit* owner);
	~CMissileLauncher(void);

	void Fire(void);
	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);
};


#endif /* MISSILELAUNCHER_H */
