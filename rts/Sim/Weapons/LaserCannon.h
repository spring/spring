#ifndef LASERCANNON_H
#define LASERCANNON_H

#include "Weapon.h"

class CLaserCannon :
	public CWeapon
{
	CR_DECLARE(CLaserCannon);
public:
	CLaserCannon(CUnit* owner);
	~CLaserCannon(void);

	void Update(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

	void Init(void);

	float3 color;

private:
	virtual void FireImpl();
};


#endif /* LASERCANNON_H */
