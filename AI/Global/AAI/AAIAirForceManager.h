//-------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------


#pragma once

#include "aidef.h"

class AAI;
class AAIBuildTable;
class AAIMap;


class AAIAirForceManager
{
public:
	AAIAirForceManager(AAI *ai, IAICallback *cb, AAIBuildTable *bt);
	~AAIAirForceManager(void);

	// checks if a certain unit is worth bombing it and tries to order air units to do it
	void CheckTarget(int unit, const UnitDef *def);

	// erases old targets and adds new ones
	void AddTarget(float3 pos, int def_id, float cost, float health, UnitCategory category);

	// tries to attack units of a certain category
	void BombUnitsOfCategory(UnitCategory category);

	// attacks the most promising target 
	void BombBestUnit(float cost, float danger);

	// list of possible bombing targets
	AAIAirTarget *targets;

	list<AAIGroup*> *air_groups;

	AAIGroup* GetAirGroup(float importance, UnitType group_type);

private:

	IAICallback *cb;
	AAI *ai;
	AAIBuildTable *bt;
	AAIMap *map;
};
