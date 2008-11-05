#ifndef LIGHTNINGCANNON_H
#define LIGHTNINGCANNON_H

#include "Weapon.h"

class CLightningCannon :
	public CWeapon
{
	CR_DECLARE(CLightningCannon);
public:
	CLightningCannon(CUnit* owner);
	~CLightningCannon(void);

	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

	void Init(void);
	void Fire(void);

	float3 color;
	void SlowUpdate(void);
};


#endif /* LIGHTNINGCANNON_H */
