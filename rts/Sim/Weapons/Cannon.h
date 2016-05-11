/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CANNON_H
#define _CANNON_H

#include "Weapon.h"

class CCannon: public CWeapon
{
	CR_DECLARE_DERIVED(CCannon)
protected:
	/// this is used to keep range true to range tag
	float rangeFactor;
	/// cached input for GetWantedDir
	float3 lastDiff;
	/// cached result for GetWantedDir
	float3 lastDir;

public:
	CCannon(CUnit* owner, const WeaponDef* def);

	void Init() override final;
	void UpdateRange(const float val) override final;
	void UpdateWantedDir() override final;
	void SlowUpdate() override final;

	float GetRange2D(float yDiff, float rFact) const;
	float GetRange2D(const float yDiff) const override final;


	/// indicates high trajectory on/off state
	bool highTrajectory;
	/// projectile gravity
	float gravity;

private:
	/// tells where to point the gun to hit the point at pos+diff
	float3 GetWantedDir(const float3& diff);
	float3 GetWantedDir2(const float3& diff) const;

	bool HaveFreeLineOfFire(const float3 pos, const SWeaponTarget& trg, bool useMuzzle = false) const override final;
	void FireImpl(const bool scriptCall) override final;
};

#endif // _CANNON_H
