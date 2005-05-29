#ifndef LASERCANNON_H
#define LASERCANNON_H
/*pragma once removed*/
#include "Weapon.h"

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


#endif /* LASERCANNON_H */
