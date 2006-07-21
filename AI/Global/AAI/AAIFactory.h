#pragma once

#include "aidef.h"

class AAI;
class AAIBuildTable;

class AAIFactory
{
public:
	AAIFactory(AAI *ai, int unit_id, int def_id);
	~AAIFactory(void);

	// starts new construction if factopry is not busy
	void Update();

	// called when factory is destroyed
	void Killed();

	// checks if assisting builders needed
	void CheckAssistance();

	void ReleaseAllAssistants();
	void StopAllAssistants();

	// removes an assiting con unit
	void RemoveAssitant(int unit_id);

	// gets the total buildtime of all units in the buildque of the factory
	double GetMyQueBuildtime();

	// the buildque of that factory type
	list<int> *buildque;

	// assistant builders
	set<int> assistants;

	bool isBusy(int factory);

	bool busy;
	AAI *ai;
	AAIBuildTable *bt;

	int def_id;
	int unit_id;
	int building_def_id;
	float3 my_pos;
};
