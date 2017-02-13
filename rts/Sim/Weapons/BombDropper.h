/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BOMB_DROPPER_H
#define BOMB_DROPPER_H

#include "Weapon.h"

class CBombDropper: public CWeapon
{
	CR_DECLARE_DERIVED(CBombDropper)
public:
	CBombDropper(CUnit* owner, const WeaponDef* def, bool useTorps);

	float GetPredictedImpactTime(float3 p) const override final;

private:
	bool CanFire(bool ignoreAngleGood, bool ignoreTargetType, bool ignoreRequestedDir) const override final;

	bool TestTarget(const float3 pos, const SWeaponTarget& trg) const override final;
	bool TestRange(const float3 pos, const SWeaponTarget& trg) const override final;
	bool HaveFreeLineOfFire(const float3 pos, const SWeaponTarget& trg, bool useMuzzle = false) const override final;
	void FireImpl(const bool scriptCall) override final;

private:
	/// if we should drop torpedoes
	bool dropTorpedoes;
	/// range of bombs (torpedoes) after they hit ground/water
	float torpMoveRange;
	float tracking;
};

#endif /* BOMB_DROPPER_H */
