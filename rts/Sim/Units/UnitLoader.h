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
	/// @param builder may be NULL
	CUnit* LoadUnit(const std::string& name, const float3& pos, int team, bool build, int facing, const CUnit* builder);
	/// @param builder may be NULL
	CUnit* LoadUnit(const UnitDef* ud, const float3& pos, int team, bool build, int facing, const CUnit* builder);

	CWeapon* LoadWeapon(CUnit* owner, const UnitDefWeapon* udw);

	void GiveUnits(const std::vector<std::string>& args, int team);

	void FlattenGround(const CUnit* unit);
	void RestoreGround(const CUnit* unit);

protected:
	void LoadSound(GuiSoundSet& sound);
};

extern CUnitLoader* unitLoader;

#endif /* UNIT_LOADER_H */
