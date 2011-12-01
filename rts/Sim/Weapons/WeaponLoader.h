/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WEAPON_LOADER_H
#define WEAPON_LOADER_H

class CUnit;
class CWeapon;
struct UnitDefWeapon;
struct WeaponDef;

class CWeaponLoader {
public:
	static CWeaponLoader* GetInstance();

	void LoadWeapons(CUnit* unit);

private:
	CWeapon* LoadWeapon(CUnit* owner, const UnitDefWeapon* defWeapon);
	CWeapon* InitWeapon(CUnit* owner, CWeapon* weapon, const UnitDefWeapon* defWeapon);
};

#define weaponLoader (CWeaponLoader::GetInstance())

#endif

