#ifndef RIFLE_H
#define RIFLE_H
// Rifle.h: interface for the CRifle class.
//
//////////////////////////////////////////////////////////////////////

#include "Weapon.h"

class CRifle : public CWeapon  
{
public:
	bool TryTarget(const float3 &pos,bool userTarget,CUnit* unit);
	void Update();
	CRifle(CUnit* owner);
	virtual ~CRifle();

	void Fire(void);
};

#endif /* RIFLE_H */
