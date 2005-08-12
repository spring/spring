#ifndef __EMG_CANNON_H__
#define __EMG_CANNON_H__

#include "Weapon.h"

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

#endif // __EMG_CANNON_H__

