#pragma once

#include "aidef.h"
#include "AAISector.h"

class AAI;
class AAIBuildTable;
class AAIAttack;

class AAIGroup
{
public:
	AAIGroup(IAICallback *cb, AAI* ai, const UnitDef *def, UnitType unit_type);
	~AAIGroup(void);

	bool AddUnit(int unit_id, int def_id, UnitType type);

	bool RemoveUnit(int unit, int attacker);

	void GiveOrder(Command *c, float importance, UnitTask task);

	void AttackSector(AAISector *dest, float importance);

	// retreat combat groups to pos
	void Retreat(float3 pos);

	int GetRandomUnit();

	void Update();

	void TargetUnitKilled();

	void UnitIdle(int unit);

	float GetPowerVS(int assault_cat_id);

	float3 GetGroupPos();

	// returns true if group is strong enough to attack
	bool SufficientAttackPower();

	int maxSize;
	int size;
	int speed_group; 
	float avg_speed;
	
	list<int2> units;
	
	Command lastCommand;

	float task_importance;	// importance of current task

	GroupTask task;

	UnitCategory category;

	UnitType group_type;

	UnitMoveType move_type;

	AAISector *target_sector;

	// attack the group takes part in
	AAIAttack *attack;

	// rally point of the group, ZeroVector if none...
	float3 rally_point;

private:
	
	IAICallback *cb;
	AAI* ai;
	AAIBuildTable *bt;
};
