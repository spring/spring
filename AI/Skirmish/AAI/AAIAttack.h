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
#include <set>
using namespace std;

class AAI;
class AAISector;
class AAIGroup;

class AAIAttack
{
public:
	AAIAttack(AAI *ai);
	~AAIAttack(void);

	void AddGroup(AAIGroup *group);

	void RemoveGroup(AAIGroup *group);

	// returns true if attack has failed
	bool Failed();

	// orders units to attack specidied sector
	void AttackSector(AAISector *sector);

	// orders all units involved to retreat
	void StopAttack();

	// target sector
	AAISector *dest;

	// tick when last attack order has been given (to prevent overflow when unit gets stuck and sends "unit idel" all the time)
	int lastAttack;

	// specifies what kind of sector the involved unit groups may attack
	bool land;
	bool water;

	// groups participating
	set<AAIGroup*> combat_groups;
	set<AAIGroup*> aa_groups;
	set<AAIGroup*> arty_groups;

private:
	AAI *ai;
};
