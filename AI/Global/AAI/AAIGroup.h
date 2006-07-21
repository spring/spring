#pragma once

#include "aidef.h"
#include "AAISector.h"

class AAI;
class AAIBuildTable;

class AAIGroup
{
public:
	AAIGroup(IAICallback *cb, AAI* ai, const UnitDef *def, UnitType unit_type);
	~AAIGroup(void);

	bool AddUnit(int unit_id, int def_id, UnitType type);

	bool RemoveUnit(int unit, int attacker);

	void GiveOrder(Command *c, float importance);

	void Update();

	void BombTarget(float3 pos, float importance);
	void BombTarget(int unit, float importance);
	void AirRaidTarget(int unit, float importance);

	float GetPowerVS(int assault_cat_id);

	float3 GetGroupPos();

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

	AAISector *target_sector;

	// rally point of the group, ZeroVector if none...
	float3 rally_point;

private:
	
	IAICallback *cb;
	AAI* ai;
	AAIBuildTable *bt;
};
