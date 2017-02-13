// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#ifndef AAI_AIRFORCEMANAGER_H
#define AAI_AIRFORCEMANAGER_H

#include <vector>
#include "System/float3.h"
#include "aidef.h"

namespace springLegacyAI {
	struct UnitDef;
}
using namespace springLegacyAI;
using namespace std;

class AAI;
class AAIBuildTable;

struct AAIAirTarget
{
	float3 pos;
	int def_id;
	int unit_id;
	float cost;
	float health;
	UnitCategory category;
};

class AAIAirForceManager
{
public:
	AAIAirForceManager(AAI *ai);
	~AAIAirForceManager(void);

	// checks if a certain unit is worth attacking it and tries to order air units to do it (units, stationary defences)
	void CheckTarget(int unit, const UnitDef *def);

	// removes target from bombing target list
	void RemoveTarget(int unit_id);

	// attacks the most promising target
	void BombBestUnit(float cost, float danger);


	// list of possible bombing targets
	vector<AAIAirTarget> targets;

private:
	AAIGroup* GetAirGroup(float importance, UnitType group_type);

	// returns true if uni already in target list
	bool IsTarget(int unit_id);
	// tries to attack units of a certain category
	void BombUnitsOfCategory(UnitCategory category);
	// checks if target is possible bombing target and adds to list of bomb targets (used for buildings e.g. stationary arty, nuke launchers..)
	void CheckBombTarget(int unit_id, int def_id);
	// adds new target to bombing targets (if free space in list)
	void AddTarget(int unit_id, int def_id);

	list<AAIGroup*> *air_groups;
	AAI *ai;
	int my_team;
	int num_of_targets;
};

#endif
