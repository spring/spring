#ifndef MELEEWEAPON_H
#define MELEEWEAPON_H
// MeleeWeapon.h: interface for the CMeleeWeapon class.
//
//////////////////////////////////////////////////////////////////////

#include "Weapon.h"

class CMeleeWeapon : public CWeapon  
{
public:
	void Update();
	CMeleeWeapon(CUnit* owner);
	virtual ~CMeleeWeapon();

	void Fire(void);
};

#endif /* MELEEWEAPON_H */
