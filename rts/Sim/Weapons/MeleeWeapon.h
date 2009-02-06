#ifndef MELEEWEAPON_H
#define MELEEWEAPON_H
// MeleeWeapon.h: interface for the CMeleeWeapon class.
//
//////////////////////////////////////////////////////////////////////

#include "Weapon.h"

class CMeleeWeapon : public CWeapon  
{
	CR_DECLARE(CMeleeWeapon);
public:
	void Update();
	CMeleeWeapon(CUnit* owner);
	virtual ~CMeleeWeapon();

private:
	virtual void FireImpl();
};

#endif /* MELEEWEAPON_H */
