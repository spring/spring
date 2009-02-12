#ifndef NOWEAPON_H
#define NOWEAPON_H

#include "Weapon.h"

class CNoWeapon :
	public CWeapon
{
	CR_DECLARE(CNoWeapon);
public:
	CNoWeapon(CUnit *owner = 0);
	~CNoWeapon(void);

	void Update(void);
	void SlowUpdate(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);

	void Init(void);

private:
	virtual void FireImpl();
};


#endif /* NOWEAPON_H */
