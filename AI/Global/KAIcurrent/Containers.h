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
class	CSurveillanceHandler;
class	CDamageControl;

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
	CDamageControl*		dc;
	CSurveillanceHandler*	sh;
	vector<CUNIT*>		MyUnits;
	std::ofstream*		LOGGER;
};

/**
This is a unit/building that a unit can make.
Stores information on how its build (unified for builders and factories).
Intended for moveable factories (nanoblobs++), and combat builders.

struct BuildOption
{
	const UnitDef* def; // The spring unit def.
	bool needsBuildPos; // If this unit is built by giveing a float3 pos, or just by its id.
};
*/

/**
The KAI unit def.
*/
struct UnitType
{
	int side; // 1 means arm, 2 core; 0 if side has not been set 
	vector<int> canBuildList;
	//vector<BuildOption> canBuildOptions; // TODO: The list of buildable units for this unit
	vector<int> builtByList;
	vector<float> DPSvsUnit;
	float AverageDPS;
	float Cost;
	//vector<string> TargetCategories; // Useless now
	const UnitDef* def;
	int category;
	unsigned categoryMask; // The new category system. Its a bit mask.
	
	// The move types are bit masks. In order for the pather to use them both must be used (moveDepthType | moveSlopeType)
	unsigned moveDepthType; // Indicates the sea/land canMove type (hovers are 0)
	unsigned moveSlopeType; // Indicates the slope canMove type (ships/subs are 0). This can be used for selecting the unit speed map for the pather.
	unsigned crushStrength; // Indicates the crush strength type. This will be used by the pather when we have debris maps.
	unsigned hackMoveType; // use as move mask for the pather, they can be joined by ( unit1->hackMoveType & unit2->hackMoveType)
	/**
		moveType: 0 = Vehicle/kbot
		moveType: 1 = hover
		moveType: 2 = ship/sub
		moveType: 3 = air
	*/
	unsigned moveType;
};


class integer2{
public:
	CR_DECLARE_STRUCT(integer2);
	integer2() : x(0), y(0) {};
	integer2(const int x,const int y) : x(x),y(y) {}
	~integer2(){};

	inline bool operator== (const integer2 &f) const {
		return (x-f.x == 0 && y-f.y == 0);
	}
	union {
		struct{
			int x;
			int y;
		};
	};
};


class KAIMoveType{
public:
	CR_DECLARE_STRUCT(KAIMoveType);
	KAIMoveType() : Air(false), MaxSlope(10000), MaxDepth(10000), MinDepth(-10000), CrushStrength(0), size(1) {};
	~KAIMoveType(){};
	void Assign(const UnitDef* unit){
		if(!unit->canfly){
			MaxSlope = unit->movedata->maxSlope;
			if(unit->movedata->moveType == unit->movedata->Ground_Move)
				MaxDepth = unit->movedata->depth; //unit->maxWaterDepth;
			else
				MaxDepth = 5000; // Its a hover/ship/sub so no max
			if(unit->movedata->moveType == unit->movedata->Ship_Move)
				MinDepth = unit->movedata->depth;//unit->minWaterDepth;
			else
				MinDepth = -10000; // Its a ground unit
			CrushStrength = unit->movedata->crushStrength;
			size = unit->movedata->size;
			Air = false;
		}
		else{
			Air = true;
		}
	}
	inline bool operator== (const KAIMoveType &f) const {
		return	((MaxSlope == f.MaxSlope
				&& (MaxDepth == f.MaxDepth || (MaxDepth >= 255 && f.MaxDepth >= 255))
				&& (MinDepth == f.MinDepth || (MinDepth < 0 && f.MinDepth < 0)))
				//&& size == f.size
				//&& CrushStrength == f.CrushStrength) //these arent used yet so comment them out 
				|| (Air && f.Air));
	}
	union {
		struct{
			bool Air;
			float MaxSlope;
			float MaxDepth;
			float MinDepth;
			float CrushStrength;
			int size;
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
	
	const UnitDef* def; // For speed up, and debugging
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
	//list<int> builders; // Temp only, for compability atm (will be removed)
	list<BuilderTracker*> builderTrackers; // The new container
	float currentBuildPower; // Temp ?
	const UnitDef* def; // For speed up, and debugging
	float3 pos; // Needed for command verify and debugging
};

struct TaskPlan
{
	int id; // This will be some smart number (a counter?)
	//list<int> builders; // Temp only, for compability atm (will be removed)
	list<BuilderTracker*> builderTrackers; // The new container
	float currentBuildPower; // Temp ?
	const UnitDef* def;
	float3 pos;
};

/**
This is a builder that can't move.
*/
struct Factory
{
	int id;
	const UnitDef* factoryDef; // For speed up, and debugging
	//list<int> supportbuilders; // Temp only, for compability atm (will be removed)
	list<BuilderTracker*> supportBuilderTrackers; // The new container
	//list<BuildOption> buildOptions; // The list of buildable (and approved) units for this factory
	float currentBuildPower;
	float currentTargetBuildPower; // This is how much buildpower this factory wants (or is wanted to have). Unused atm.
};

class EconomyTotal
{
public:
	void Clear(){Cost=Damage=Armor=BuidPower=AverageSpeed=Hitpoints=0;}
	float Cost;
	float Hitpoints;
	float Damage;
	float Armor;
	float BuidPower;
	float AverageSpeed;
};
