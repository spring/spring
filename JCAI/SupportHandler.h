//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#include "ptrvec.h"

struct SupportConfig
{
	struct UnitDefProperty
	{
		UnitDefProperty () { fptr=0; isFloat=true; }

		bool isFloat;
		union {
			float UnitDef::*fptr;
			int UnitDef::*iptr;
		};
	};

	SupportConfig() { mapcover=basecover=0; }

	CfgBuildOptions *mapcover; // mapcover units
	vector <UnitDefProperty> mapcoverProps;  // which property should be used to cover the map?
	CfgBuildOptions *basecover;
	vector <UnitDefProperty> basecoverProps;
	CfgBuildOptions *baseperimeter;
	vector <UnitDefProperty> baseperimeterProps;

	static void MapUDefProperties (CfgBuildOptions *c, vector<UnitDefProperty>& props);
	bool Load (CfgList *sidecfg);

	// Defines a group of support units that ends up in one sector
	struct Group
	{
		Group(){units=0; minMetal=minEnergy=0;}
		CfgBuildOptions *units;
		float minMetal, minEnergy;
		string name;
	};
	list<Group> groups;
};

class SupportHandler : public TaskFactory
{
public:
	SupportHandler (CGlobals *g);

	void Update ();
	void UnitDestroyed (aiUnit *unit);
	void UnitFinished (aiUnit *unit);

	BuildTask* GetNewBuildTask ();

	const char *GetName () { return "Support"; }
	aiUnit* CreateUnit (int id, BuildTask *task); // should not register the unit - since it's not finished yet
protected:
	struct Task;

	struct UnitGroup
	{
		UnitGroup(SupportConfig::Group* Cfg);

		BuildTask* NewTask();
		int CountOrderedUnits();

		SupportConfig::Group *cfg;
		vector<int> orderedUnits;

		struct Unit : public aiUnit { int index; Task *task; UnitGroup *group; };
		ptrvec<Unit> units;
		int index;
		int2 sector;
	};

	struct Task : public BuildTask
	{
		Task(const UnitDef *def) : BuildTask(def) { group=0;}

		bool InitBuildPos (CGlobals *g);
		int GetBuildSpace (CGlobals *g);

		int unitIndex;
		UnitGroup *group;
	};

	ptrvec <UnitGroup> groups;
	SupportConfig config;
	deque <SupportConfig::Group*> buildorder;
};


