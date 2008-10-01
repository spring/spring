//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#include "ptrvec.h"

enum ForceGroupTask
{
	ft_AttackDefend,
	ft_Raid,
	ft_Defend,
	ft_Dominate
};

struct ForceConfig
{
	ForceConfig ();

	struct Group
	{
		CfgBuildOptions *units;
		int batchsize;
		int category;
		int level;
		ForceGroupTask task;
		float minmetal, minenergy;
		float maxspread, groupdist;
		string name;

		// precalculated from unit list
		float metal, energy, buildtime;
	};

	float defaultSpread;
	list <Group> groups;
	typedef list <Group>::iterator GroupIterator;

	// maps category to groups
	map <int, vector <Group *> > catmap;
	typedef map <int, vector <Group *> >::iterator CatMapIterator;

	void Precalc ();
	bool Load (CfgList *cfg);
	bool ParseForceGroup (CfgList *groupnode, const string& name);
};

enum UnitGroupState
{
	ugroup_Building=0, // group is incomplete - building
	ugroup_Moving,   // moving to goal
	ugroup_Pruning,  // clear units in current sector
	ugroup_Grouping, // moving the units to each other - unspreading
	ugroup_WaitingForGoal // waiting until a goal has been found
};


class ForceUnit : public aiUnit
{
public:
	ForceUnit() {index=0;}
	~ForceUnit();

	int index; // index for ptrvec
};


// force unit group -- a group of units that is supposed to attack at the same time
class UnitGroup : public aiHandler
{
public:
	UnitGroup (CGlobals *g);
	~UnitGroup ();

	void DependentDied (aiObject *o);

	void Update ();
	void UnitIdle (aiUnit *unit);
	int FindPreferredUnit (); // -1 when none
	void GiveOrder (Command *c);
	void UnitDestroyed (aiUnit *unit);
	const char *GetName ();
	int CountCurrentOrders();

	aiUnit* CreateUnit (int id, BuildTask *task);
	void UnitFinished (aiUnit *unit);

	// Set states
	void SetPruning ();
	void SetMoving ();
	void SetGrouping ();
	void SetWaitingForGoal ();

	// State helper functions
	void Init ();
	void SetNewGoal ();
	int SelectTarget (const vector <int>& enemies);
	bool IsGoalReached (const float2& mid,float spread);

	bool CalcPositioning (float2& pmin, float2& pmax, float2& mid, int2& closestSector );

	ForceConfig::Group *group;

	ptrvec<ForceUnit> units;
	int2 goal; // game info position
	UnitGroupState state;
	float2 mid;
	int2 current;
	int curTarget; // current enemy unit that is targeted by the group
	int index; // ptrvec index
	int lastCmdFrame;

	int *orderedUnits;
};

class MainAI;

// 73h F0rC3 H4nD13R
class ForceHandler : public TaskFactory
{
public:
	typedef ForceConfig::Group ForceGroup;

	ForceHandler(CGlobals *g);
	~ForceHandler();

	void Update ();
	void Init ();
	BuildTask* GetNewBuildTask ();
	const char* GetName () { return "Force"; }

	struct BuildState
	{
		BuildState() { newDef=0; batch =0 ; cat=0; }

		int cat;
		UnitGroup *batch;
		const UnitDef *newDef;
		int newUnitTypeIndex;
		int waitFrame; // frame since newDef is set
	};

	void InitBatch (BuildState *b);
	void ClearEmptyBatches();
	void UpdateNextCategoryTask (int c);
	void ShowGroupStates ();

	struct Task : public BuildTask {
		Task (const UnitDef* def, UnitGroup *group) : BuildTask(def) { destHandler=group; }
	};

	vector <BuildState> build;
	ptrvec <UnitGroup> batches;

	ForceConfig config;
};

