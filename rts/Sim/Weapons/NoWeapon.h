/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NOWEAPON_H
#define NOWEAPON_H

#include "Weapon.h"

class CNoWeapon: public CWeapon
{
	CR_DECLARE_DERIVED(CNoWeapon)
public:
	CNoWeapon(CUnit* owner = nullptr, const WeaponDef* def = nullptr): CWeapon(owner, def) {}

	void Update() override final {}
	void SlowUpdate() override final {}
	void Init() override final {}

private:
	bool TestTarget(const float3 pos, const SWeaponTarget& trg) const override final { return false; }
	void FireImpl(const bool scriptCall) override final {}
};


#endif /* NOWEAPON_H */
