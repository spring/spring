#pragma once

#include "aidef.h"

class AAI;
class AAIBuildTask;

class AAIBuilder
{
public:
	AAIBuilder(AAI *ai, int unit_id, int def_id);
	~AAIBuilder(void);

	void Killed();

	void Update();

	void GiveBuildOrder(int id_building, float3 pos, bool water);

	void BuildingFinished();

	void GiveReclaimOrder(int unit_id);

	void TakeOverConstruction(AAIBuildTask *task);

	void Idle();

	// assisting and stop assisting factories or other builders
	void AssistConstruction(int builder, int target_unit, int importance = 5);
	void AssistFactory(int factory, int importance = 5);
	void StopAssistant();
	void ReleaseAssistant();
	void ReleaseAllAssistants();

	void ConstructionFailed();

	// ids of the currently constructed building (0 if none)
	int building_def_id;		
	int building_unit_id;
	float3 build_pos;
	UnitCategory building_category;
	int order_tick;
	int task_importance;
	
	// ids of the builder
	int unit_id;
	int def_id;
	int buildspeed;

	// pointer to buildtask 
	AAIBuildTask *build_task;

	// id of the unit, the builder currently assists (-1 if none)
	int assistance;

	// checks if assisting builders needed
	void CheckAssistance();

	// removes an assiting con unit
	void RemoveAssitant(int unit_id);

	// assistant builders
	set<int> assistants;
	
	// current task (idle, building, assisting)
	UnitTask task;

	const deque<Command> *commands;

	IAICallback *cb;
	AAI *ai;
};
