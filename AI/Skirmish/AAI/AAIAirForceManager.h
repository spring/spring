// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------


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

	// checks if a certain unit is worth attacking it and tries to order air units to do it (units, stationary defences)
	void CheckTarget(int unit, const UnitDef *def);

	// checks if target is possible bombing target and adds to list of bomb targets (used for buildings e.g. stationary arty, nuke launchers..)
	void CheckBombTarget(int unit_id, int def_id);

	// adds new target to bombing targets (if free space in list)
	void AddTarget(int unit_id, int def_id);

	// removes target from bombing target list
	void RemoveTarget(int unit_id);

	// tries to attack units of a certain category
	void BombUnitsOfCategory(UnitCategory category);

	// attacks the most promising target
	void BombBestUnit(float cost, float danger);

	// returns true if uni already in target list
	bool IsTarget(int unit_id);

	// list of possible bombing targets
	vector<AAIAirTarget> targets;

	list<AAIGroup*> *air_groups;

	AAIGroup* GetAirGroup(float importance, UnitType group_type);

private:


	IAICallback *cb;
	AAI *ai;
	AAIBuildTable *bt;
	AAIMap *map;

	int my_team;
	int num_of_targets;
};
