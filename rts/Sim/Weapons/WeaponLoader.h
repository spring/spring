/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WEAPON_LOADER_H
#define WEAPON_LOADER_H

class CUnit;
class CWeapon;
struct UnitDefWeapon;
struct WeaponDef;

class CWeaponLoader {
public:
	static void InitStatic();
	static void KillStatic();

	static void LoadWeapons(CUnit* unit);
	static void InitWeapons(CUnit* unit);
	static void FreeWeapons(CUnit* unit);

private:
	static CWeapon* LoadWeapon(CUnit* owner, const WeaponDef* weaponDef);
	static void InitWeapon(CUnit* owner, CWeapon* weapon, const UnitDefWeapon* defWeapon);
};

#endif

