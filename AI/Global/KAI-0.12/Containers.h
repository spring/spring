#ifndef CONTAINERS_H
#define CONTAINERS_H


class IAICallback;
class IAICheats;
class CSunParser;
class CMetalMap;
class CMaths;
class CDebug;
class CPathFinder;
class CUnitTable;
class CThreatMap;
class CUnitHandler;
class CUNIT;
class CDefenseMatrix;
class CEconomyTracker;
class CBuildUp;
class CAttackHandler;
class CEconomyManager;

// added by Kloot
class DGunController;

struct AIClasses {
	IAICallback*		cb;
	IAICheats*			cheat;
	CEconomyTracker*	econTracker;
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
	// added by Kloot
	DGunController*		dgunController;
};

struct UnitType {
	// 1 means arm, 2 core; 0 if side has not been set
	int side;
	vector<int> canBuildList;
	vector<int> builtByList;
	vector<float> DPSvsUnit;
	vector<string> TargetCategories;
	const UnitDef* def;
	int category;
};


class integer2 {
	public:
		CR_DECLARE_STRUCT(integer2);

		integer2(): x(0), y(0) {};
		integer2(const int x,const int y): x(x), y(y) {}
		~integer2() {};

	inline bool operator == (const integer2 &f) const {
		return ((x - f.x == 0) && (y - f.y == 0));
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
Its main task is to make sure that tracking builders is easy (making asserts and tests)
*/
struct BuilderTracker {
	int builderID;
	// If not NULL then this worker belongs to this BuildTask.
	int buildTaskId;
	// If not NULL then this worker belongs to this TaskPlan.
	int taskPlanId;
	// If not NULL then this worker belongs to this Factory.
	int factoryId;
	// If not NULL then this worker is on reclaim job or something, not tracked until it gets idle (spring call)
	int customOrderId;

	// If not NULL then this worker is stuck or something
	int stuckCount;
	// If this builder ends up without orders for some time, try IdleUnitAdd() ("hack" as spring doesn't call UnitIdle()?)
	int idleStartFrame;
	// The frame an order is given, needed for lag tolerance when verifying unit command
	int commandOrderPushFrame;
	// TODO: This is (or will be in the future) a hint of what this builder will make
	// NOTE: Will be temp only until a better system is made
	int categoryMaker;

	// These are unused atm.

	// What frame this builder will start working
	int estimateRealStartFrame;
	// This will be constant or based on the last startup time
	int estimateFramesForNanoBuildActivation;
	// Simple eta, updated every 16 frames or so (0 means its there)
	int estimateETAforMoveingToBuildSite;
	// The def->buildDistance or something
	float distanceToSiteBeforeItCanStartBuilding;
};

struct BuildTask {
	int id;
	int category;
	// Temp only, for compability atm (will be removed)
	list<int> builders;
	// The new container
	list<BuilderTracker*> builderTrackers;
	// Temp?
	float currentBuildPower;
	// For speed up, and debugging
	const UnitDef* def;
	// Needed for command verify and debugging
	float3 pos;
};

struct TaskPlan {
	// This will be some smart number (a counter?)
	int id;
	// Temp only, for compability atm (will be removed)
	list<int> builders;
	// The new container
	list<BuilderTracker*> builderTrackers;
	// Temp ?
	float currentBuildPower;
	const UnitDef* def;
	float3 pos;
};

struct Factory {
	int id;
	// Temp only, for compability atm (will be removed)
	list<int> supportbuilders;
	// The new container
	list<BuilderTracker*> supportBuilderTrackers;
};


#endif
