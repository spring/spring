//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#ifndef JC_TASKMANAGER_H
#define JC_TASKMANAGER_H

#include "ptrvec.h"
#include "Tasks.h"

class BuildUnit;
class DbgWindow;
class CfgBuildOptions;

// a unit that can build stuff
class BuildUnit : public aiUnit
{
public:
	BuildUnit();
	~BuildUnit();

	void DependentDied (aiObject *obj);
	void UnitFinished ();
	ResourceInfo GetResourceUse ();
	float CalcBusyTime (IAICallback *cb);

	// these deal with death dependencies as well
	void RemoveTask (aiTask *task);
	void AddTask (aiTask *task);

	vector <aiTask *> tasks;
	int index;

	aiTask *activeTask;
	bool isDefending;
};

struct TaskManagerConfig
{
	bool Load (CfgList *sidecfg);

	float BuildSpeedPerMetalIncome;
	CfgBuildOptions *InitialOrders;
};

// Handles all building tasks
class TaskManager : public aiHandler
{
public:
	TaskManager (CGlobals *g);
	~TaskManager ();

	aiUnit* Init (int id);
	void InitBuilderTypes (const UnitDef *commanderDef);

	void Update ();
	void BalanceResourceUsage (int type);

	void AddTask (aiTask *t);

	BuildUnit* FindInactiveBuilderForTask (const UnitDef *def);
	const UnitDef* FindBestBuilderType (const UnitDef* constr, BuildUnit**builder);
	bool CanBuildUnit(const UnitDef* def);

	void CheckBuildError (BuildUnit *builder);
	void UpdateBuilderActiveTask (BuildUnit *builder);

	void RemoveUnitBlocking (const UnitDef* def, const float3& pos);

	void BalanceBuildSpeed (float totalBuildSpeed);
	void FindNewJob (BuildUnit *u);
	void OrderNewBuilder ();

	// returns false if this task is deleted
	void FinishConstructedUnit (BuildTask *task);

	bool DoInitialBuildOrders ();
	void InitialBuildOrderFinished (int defID);

	aiUnit* UnitCreated (int id);
	void UnitMoveFailed (aiUnit *unit);
	void UnitDestroyed (aiUnit *unit);

	void ChatMsg (const char *msg, int player);

	const char* GetName() { return "TaskManager"; }

	ptrvec<aiTask> tasks;
	ptrvec<BuildUnit> builders;

	// stores inactive builders during Update() - empty when not updating
	vector <BuildUnit *> inactive;
	// builder counts mapped to builder types
	vector <int> currentBuilders;
	vector <int> currentUnitBuilds;

	// precomputed list of builder types for this side
	vector <int> builderTypes;

	TaskManagerConfig config;

	vector<int> initialBuildOrderState;
	bool initialBuildOrdersFinished;
	BuildTask *initialBuildOrderTask;

	int jobFinderIterator;

	struct Stat {
		Stat(){minBuildSpeed=totalBuildSpeed=0.0;}
		float minBuildSpeed;
		float totalBuildSpeed;
	} stat;
};



#endif
