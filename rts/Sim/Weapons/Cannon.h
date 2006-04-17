// Cannon.h: interface for the CCannon class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __CANNON_H__
#define __CANNON_H__

#include "Weapon.h"

class CCannon : public CWeapon  
{
public:
	CCannon(CUnit* owner);
	virtual ~CCannon();
	void Init(void);
	void Fire(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);
	void Update();
	virtual bool AttackGround(float3 pos,bool userTarget);

	float maxPredict;
	float minPredict;
	bool highTrajectory;
	bool selfExplode;
	void SlowUpdate(void);
};

#endif // __CANNON_H__
