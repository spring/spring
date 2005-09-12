#ifndef BOMBDROPPER_H
#define BOMBDROPPER_H
// Cannon.h: interface for the CCannon class.
//
//////////////////////////////////////////////////////////////////////

#include "Weapon.h"

class CBombDropper : public CWeapon  
{
public:
	void Update();
	CBombDropper(CUnit* owner);
	virtual ~CBombDropper();

	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);
	void Fire(void);
	void Init(void);
};

#endif /* BOMBDROPPER_H */
