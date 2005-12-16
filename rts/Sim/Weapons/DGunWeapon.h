#ifndef __DGUN_WEAPON_H__
#define __DGUN_WEAPON_H__

#include "Weapon.h"

class CDGunWeapon :
	public CWeapon
{
public:
	CDGunWeapon(CUnit* owner);
	~CDGunWeapon(void);
	void Fire(void);
	void Update(void);
	void Init(void);
};

#endif // __DGUN_WEAPON_H__

