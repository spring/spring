#ifndef LIGHTINGCANNON_H
#define LIGHTINGCANNON_H

#include "Weapon.h"

class CLightingCannon :
	public CWeapon
{
	CR_DECLARE(CLightingCannon);
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


#endif /* LIGHTINGCANNON_H */
