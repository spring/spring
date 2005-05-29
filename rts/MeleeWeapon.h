#ifndef MELEEWEAPON_H
#define MELEEWEAPON_H
// MeleeWeapon.h: interface for the CMeleeWeapon class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MELEEWEAPON_H__9E3B7980_BB91_4634_9CD6_4141621D7BA3__INCLUDED_)
#define AFX_MELEEWEAPON_H__9E3B7980_BB91_4634_9CD6_4141621D7BA3__INCLUDED_

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

#include "Weapon.h"

class CMeleeWeapon : public CWeapon  
{
public:
	void Update();
	CMeleeWeapon(CUnit* owner);
	virtual ~CMeleeWeapon();

	void Fire(void);
};

#endif // !defined(AFX_MELEEWEAPON_H__9E3B7980_BB91_4634_9CD6_4141621D7BA3__INCLUDED_)


#endif /* MELEEWEAPON_H */
