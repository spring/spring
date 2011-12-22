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
	static CUnitLoader* GetInstance();

	/// @param builder may be NULL
	CUnit* LoadUnit(const std::string& name, const float3& pos, int team, bool build, int facing, const CUnit* builder);
	/// @param builder may be NULL
	CUnit* LoadUnit(const UnitDef* ud, const float3& pos, int team, bool build, int facing, const CUnit* builder);

	CWeapon* LoadWeapon(CUnit* owner, const UnitDefWeapon* udw);

	void ParseAndExecuteGiveUnitsCommand(const std::vector<std::string>& args, int team);
	void GiveUnits(const std::string& objectName, float3 pos, int amount, int team, int allyTeamFeatures);

	void FlattenGround(const CUnit* unit);
	void RestoreGround(const CUnit* unit);

protected:
	void LoadSound(GuiSoundSet& sound);
};

#define unitLoader (CUnitLoader::GetInstance())

#endif /* UNIT_LOADER_H */
