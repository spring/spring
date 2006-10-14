#pragma once

class	IAICallback;
class	IAICheats;
class	CSunParser;
class	CMetalMap;
class	CMaths;
class	CDebug;
class	CPathFinder;
class	CUnitTable;
class	CThreatMap;
class	CUnitHandler;
class	CUNIT;
class	CDefenseMatrix;
class	CEconomyTracker;
class	CBuildUp;
class	CAttackHandler;
class	CEconomyManager;

struct AIClasses{
	IAICallback*		cb;
	IAICheats*			cheat;
	CEconomyTracker*	econTracker; // Temp only
	CBuildUp *			bu;
	CSunParser*			parser;
	CMetalMap*			mm;
	CMaths*				math;
	CDebug*				debug;
	CPathFinder*		pather;
	CUnitTable*			ut;
	CThreatMap*			tm;
	CUnitHandler*		uh;
	CDefenseMatrix*		dm;
	CAttackHandler*		ah;
	CEconomyManager*	em;
	vector<CUNIT*>		MyUnits;
	std::ofstream*		LOGGER;
};

struct UnitType
{
	int side; // 1 means arm, 2 core; 0 if side has not been set 
	vector<int> canBuildList;
	vector<int> builtByList;
	vector<float> DPSvsUnit;
	vector<string> TargetCategories;
	const UnitDef* def; 
	int category;
};


class integer2{
public:
	CR_DECLARE_STRUCT(integer2);
	integer2() : x(0), y(0) {};
	integer2(const int x,const int y) : x(x),y(y) {}
	~integer2(){};

	inline bool operator== (const integer2 &f) const {
		return (x-f.x == 0  && y-f.y == 0);
	}
	union {
		struct{
			int x;
			int y;
		};
	};
};

/*
This is a builder container.
Its main task is to make shure that tracking builders is easy (making asserts and tests)
*/
struct BuilderTracker
{
	int builderID;
	int buildTaskId;	// If not NULL then this worker belongs to this BuildTask.
	int taskPlanId;		// If not NULL then this worker belongs to this TaskPlan.
	int factoryId;		// If not NULL then this worker belongs to this Factory.
	int customOrderId;	// If not NULL then this worker is on a reclame jop or something, its not tracked until it gets idle (spring call).
	
	int stuckCount;		// If not NULL then this worker is stuck or something
	int idleStartFrame;	// If this builder ends up without orders for some time, try IdleUnitAdd() - this is a "hack" as spring dont call UnitIdle() ???
	int commandOrderPushFrame; // The frame an order is given, needed for lag tolerance when verifying unit command.
	int categoryMaker;	// TODO:  This is (or will be) a hint of what this builder will make in the future. Will be a temp only, until a better system is made.
	
	// This is unused atm.
	int estimateRealStartFrame; // What frame this builder will start working.
	int estimateFramesForNanoBuildActivation; // This will be constant or based on the last startup time
	int estimateETAforMoveingToBuildSite; // Simple eta, updated every 16 frames or so (0 = its there)
	float distanceToSiteBeforeItCanStartBuilding; // The def->buildDistance or something.
	
};

struct BuildTask
{
	int id;
	int category;
	list<int> builders; // Temp only, for compability atm (will be removed)
	list<BuilderTracker*> builderTrackers; // The new container
	float currentBuildPower; // Temp ?
	const UnitDef* def; // For speed up, and debugging
	float3 pos; // Needed for command verify and debugging
};

struct TaskPlan
{
	int id; // This will be some smart number (a counter?)
	list<int> builders; // Temp only, for compability atm (will be removed)
	list<BuilderTracker*> builderTrackers; // The new container
	float currentBuildPower; // Temp ?
	const UnitDef* def;
	float3 pos;
};

struct Factory
{
	int id;
	list<int> supportbuilders; // Temp only, for compability atm (will be removed)
	list<BuilderTracker*> supportBuilderTrackers; // The new container
};








