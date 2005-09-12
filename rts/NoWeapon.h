#ifndef NOWEAPON_H
#define NOWEAPON_H

#include "Weapon.h"

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


#endif /* NOWEAPON_H */
