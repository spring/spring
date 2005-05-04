// Cannon.h: interface for the CCannon class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __CANNON_H__
#define __CANNON_H__

#include "archdef.h"

#include "Weapon.h"

class CCannon : public CWeapon  
{
public:
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);
	void Update();
	CCannon(CUnit* owner);
	virtual ~CCannon();
	void Init(void);
	void Fire(void);

	float maxPredict;
	float minPredict;
	bool highTrajectory;
	bool selfExplode;
	void SlowUpdate(void);
};

#endif // __CANNON_H__
