// Cannon.h: interface for the CCannon class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __CANNON_H__
#define __CANNON_H__

#include "Weapon.h"

class CCannon : public CWeapon
{
	CR_DECLARE(CCannon);
protected:
	float rangeFactor;	/// this is used to keep range true to range tag

public:
	CCannon(CUnit* owner);
	virtual ~CCannon();
	void Init(void);
	void Fire(void);
	bool TryTarget(const float3& pos,bool userTarget,CUnit* unit);
	void Update();
	virtual bool AttackGround(float3 pos,bool userTarget);
	float GetRange2D(float yDiff) const;

	float maxPredict;
	float minPredict;
	bool highTrajectory;
	bool selfExplode;
	void SlowUpdate(void);
	float3 GetWantedDir(const float3& diff);
};

#endif // __CANNON_H__
