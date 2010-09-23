/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_LOADER_H
#define UNIT_LOADER_H

#include "System/float3.h"

#include <string>

class CUnit;
class CWeapon;
struct GuiSoundSet;
struct UnitDef;
struct UnitDefWeapon;
struct WeaponDef;

class CUnitLoader
{
public:
	CUnitLoader();
	virtual ~CUnitLoader();

	/// @param builder may be NULL
	CUnit* LoadUnit(const std::string& name, float3 pos, int team, bool build, int facing, const CUnit* builder);
	/// @param builder may be NULL
	CUnit* LoadUnit(const UnitDef* ud, float3 pos, int team, bool build, int facing, const CUnit* builder);
	void FlattenGround(const CUnit* unit);
	void RestoreGround(const CUnit* unit);

	CWeapon* LoadWeapon(const WeaponDef* weapondef, CUnit* owner, const UnitDefWeapon* udw);

protected:
	void LoadSound(GuiSoundSet& sound);
};

extern CUnitLoader unitLoader;

#endif /* UNIT_LOADER_H */
