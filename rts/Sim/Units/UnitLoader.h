/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_LOADER_H
#define UNIT_LOADER_H

#include "System/float3.h"

#include <string>
#include <vector>

class CCommandAI;
class CUnit;
class CWeapon;

struct UnitDef;
struct UnitDefWeapon;

struct UnitLoadParams {
	const UnitDef* unitDef; /// must be non-NULL
	const CUnit* builder; /// may be NULL

	float3 pos;
	float3 speed;

	int unitID;
	int teamID;
	int facing;

	bool beingBuilt;
	bool flattenGround;
};

class CUnitLoader
{
public:
	static CUnitLoader* GetInstance();
	static CCommandAI* NewCommandAI(CUnit* u, const UnitDef* ud);

	CUnit* LoadUnit(const std::string& name, const UnitLoadParams& params);
	CUnit* LoadUnit(const UnitLoadParams& params);

	CWeapon* LoadWeapon(CUnit* owner, const UnitDefWeapon* udw);

	void ParseAndExecuteGiveUnitsCommand(const std::vector<std::string>& args, int team);
	void GiveUnits(const std::string& objectName, float3 pos, int amount, int team, int allyTeamFeatures);

	void FlattenGround(const CUnit* unit);
	void RestoreGround(const CUnit* unit);
};

#define unitLoader (CUnitLoader::GetInstance())

#endif /* UNIT_LOADER_H */
