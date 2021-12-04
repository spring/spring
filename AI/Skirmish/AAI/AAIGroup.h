// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#ifndef AAI_GROUP_H
#define AAI_GROUP_H

#include "System/type2.h"
#include "Sim/Units/CommandAI/Command.h"
#include "aidef.h"

enum GroupTask {GROUP_IDLE, GROUP_ATTACKING, GROUP_DEFENDING, GROUP_PATROLING, GROUP_BOMBING, GROUP_RETREATING};

namespace springLegacyAI {
	struct UnitDef;
}

using namespace springLegacyAI;

#include <vector>
#include <list>
using namespace std;

class AAI;
class AAIBuildTable;
class AAIAttack;
class AAISector;

class AAIGroup
{
public:
	AAIGroup(AAI* ai, const UnitDef *def, UnitType unit_type, int continent_id);
	~AAIGroup(void);

	bool AddUnit(int unit_id, int def_id, UnitType type, int continent_id);

	bool RemoveUnit(int unit, int attacker);

	void GiveOrder(Command *c, float importance, UnitTask task, const char *owner);

	void AttackSector(AAISector *dest, float importance);

	// defend unit vs enemy (0; zerovector if enemy unknown)
	void Defend(int unit, float3 *enemy_pos, int importance);

	// retreat combat groups to pos
	void Retreat(float3 *pos);

	// bombs target (only for bomber groups)
	void BombTarget(int target_id, float3 *target_pos);

	// orders fighters to defend air space
	void DefendAirSpace(float3 *pos);

	// orders air units to attack
	void AirRaidUnit(int unit_id);

	int GetRandomUnit();

	void Update();

	void TargetUnitKilled();

	// checks current rally point and chooses new one if necessary
	void UpdateRallyPoint();

	// gets a new rally point and orders units to get there
	void GetNewRallyPoint();

	void UnitIdle(int unit);

	float GetCombatPowerVsCategory(int assault_cat_id);

	void GetCombatPower(vector<float> *combat_power);

	float3 GetGroupPos();

	// checks if the group may participate in an attack (= idle, sufficient combat power, etc.)
	bool AvailableForAttack();

	int maxSize;
	int size;

	float avg_speed;
	list<int2> units;


	float task_importance;	// importance of current task

	GroupTask task;

	UnitCategory category;
	int combat_category;

	UnitType group_unit_type;

	unsigned int group_movement_type;

	// attack the group takes part in
	AAIAttack *attack;

	// rally point of the group, ZeroVector if none...
	float3 rally_point;

	// id of the continent the units of this group are stationed on (only matters if group_movement_type is continent bound)
	int continent;

private:
	// returns true if group is strong enough to attack
	bool SufficientAttackPower();
	int lastCommandFrame;
	Command lastCommand;
	int speed_group;

	AAI* ai;
	AAISector *target_sector;

};

#endif

