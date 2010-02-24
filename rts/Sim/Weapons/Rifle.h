/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RIFLE_H
#define RIFLE_H

#include "Weapon.h"

class CRifle : public CWeapon  
{
	CR_DECLARE(CRifle);
public:
	bool TryTarget(const float3 &pos,bool userTarget,CUnit* unit);
	void Update();
	CRifle(CUnit* owner);
	virtual ~CRifle();

private:
	virtual void FireImpl();
};

#endif /* RIFLE_H */
