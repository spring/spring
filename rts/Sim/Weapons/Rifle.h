#ifndef RIFLE_H
#define RIFLE_H
// Rifle.h: interface for the CRifle class.
//
//////////////////////////////////////////////////////////////////////

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
