#ifndef __DGUN_WEAPON_H__
#define __DGUN_WEAPON_H__

#include "Weapon.h"

class CDGunWeapon :
	public CWeapon
{
	CR_DECLARE(CDGunWeapon);
public:
	CDGunWeapon(CUnit* owner);
	~CDGunWeapon(void);
	void Fire(void);
	void Update(void);
	void Init(void);

private:
	virtual void FireImpl();
};

#endif // __DGUN_WEAPON_H__

