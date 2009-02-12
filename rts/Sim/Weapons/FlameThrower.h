#ifndef __FLAME_THROWER_H__
#define __FLAME_THROWER_H__

#include "Weapon.h"

class CFlameThrower :
	public CWeapon
{
	CR_DECLARE(CFlameThrower);
public:
	CFlameThrower(CUnit* owner);
	~CFlameThrower(void);
	bool TryTarget(const float3 &pos,bool userTarget,CUnit* unit);
	void Update(void);
	float3 color;
	float3 color2;

private:
	virtual void FireImpl();
};

#endif // __FLAME_THROWER_H__
