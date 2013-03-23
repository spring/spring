/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CANNON_H
#define _CANNON_H

#include "Weapon.h"

class CCannon : public CWeapon
{
	CR_DECLARE(CCannon);
protected:
	/// this is used to keep range true to range tag
	float rangeFactor;
	/// cached input for GetWantedDir
	float3 lastDiff;
	/// cached result for GetWantedDir
	float3 lastDir;

public:
	CCannon(CUnit* owner);

	void Init();
	void UpdateRange(float val);
	void Update();
	bool AttackGround(float3 pos, bool userTarget);
	float GetRange2D(float yDiff) const;
	void SlowUpdate();


	/// unused?
	float maxPredict;
	/// unused?
	float minPredict;
	/// indicates high trajectory on/off state
	bool highTrajectory;
	/// burnblow tag. defines flakker-like behaviour
	bool selfExplode;
	/// projectile gravity
	float gravity;

private:
	/// tells where to point the gun to hit the point at pos+diff
	float3 GetWantedDir(const float3& diff);
	float3 GetWantedDir2(const float3& diff) const;

	bool HaveFreeLineOfFire(const float3& pos, bool userTarget, const CUnit* unit) const;
	void FireImpl();
};

#endif // _CANNON_H
