#ifndef KAIK_CONTAINERS_HDR
#define KAIK_CONTAINERS_HDR

#include <list>
#include <set>
#include <vector>

#include "System/float3.h"
#include "IncCREG.h"
#include "Defines.h"

struct UnitDef;

class IGlobalAICallback;
class IAICallback;
class IAICheats;
class CMetalMap;
class CMaths;
class CPathFinder;
class CUnitTable;
class CThreatMap;
class CUnitHandler;
class CUNIT;
class CDefenseMatrix;
class CEconomyTracker;
class CBuildUp;
class CAttackHandler;
class CDGunControllerHandler;
class CLogger;
class CCommandTracker;
struct MapData;

struct AIClasses {
	CR_DECLARE_STRUCT(AIClasses);

	AIClasses() { /* CREG-only */ }
	AIClasses(IGlobalAICallback*);
	~AIClasses();
	void Init();
	void Load();

	IAICallback*            cb;
	IAICheats*              ccb;
	CEconomyTracker*        econTracker;
	CBuildUp*               bu;
	CMetalMap*              mm;
	CMaths*                 math;
	CPathFinder*            pather;
	CUnitTable*             ut;
	CThreatMap*             tm;
	CUnitHandler*           uh;
	CDefenseMatrix*         dm;
	CAttackHandler*         ah;
	CLogger*                logger;
	CDGunControllerHandler* dgunConHandler;
	CCommandTracker*        ct;
	MapData*                mapData;

	std::vector<CUNIT*>     MyUnits;
	std::vector<int>        unitIDs;
};

// NOTE: CUNIT does not know about this structure
struct UnitType {
	CR_DECLARE_STRUCT(UnitType);
	void PostLoad();

	std::vector<int> canBuildList;
	std::vector<int> builtByList;
	std::vector<float> DPSvsUnit;

	const UnitDef* def;
	UnitCategory category;
	bool isHub;
	int techLevel;
	float costMultiplier;

	// which sides can build this UnitType (usually only
	// one, needed for types that are shared among sides
	// in certain mods)
	/// std::set<int> sides;
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
 * builder container: main task is to make sure that
 * tracking builders is easy (making asserts and tests)
 */
struct BuilderTracker {
	CR_DECLARE_STRUCT(BuilderTracker);

	int builderID;
	// if not NULL then this worker belongs to this BuildTask.
	int buildTaskId;
	// if not NULL then this worker belongs to this TaskPlan.
	int taskPlanId;
	// if not NULL then this worker belongs to this Factory.
	int factoryId;
	// if not NULL then this worker is on reclaim job or something, not tracked until it gets idle
	int customOrderId;

	// if not NULL then this worker is stuck or something
	int stuckCount;
	// if this builder ends up without orders for some time, try IdleUnitAdd()
	int idleStartFrame;
	// the frame an order is given, needed for lag tolerance when verifying unit command
	int commandOrderPushFrame;
	// TODO: this is (or will be in the future) a hint of what this builder will make
	UnitCategory categoryMaker;

	// these are unused

	// what frame this builder will start working
	int estimateRealStartFrame;
	// this will be constant or based on the last startup time
	int estimateFramesForNanoBuildActivation;
	// Simple ETA, updated every 16 frames or so (0 means it's there)
	int estimateETAforMoveingToBuildSite;
	// the def -> buildDistance or something
	float distanceToSiteBeforeItCanStartBuilding;
};

struct BuildTask {
	CR_DECLARE_STRUCT(BuildTask);
	void PostLoad(void);

	int id;
	UnitCategory category;
	// temp only, for compatibility (will be removed)
	std::list<int> builders;
	// the new container
	std::list<BuilderTracker*> builderTrackers;

	float currentBuildPower;
	// for speed up, and debugging
	const UnitDef* def;
	// needed for command verify and debugging
	float3 pos;
};

struct TaskPlan {
	CR_DECLARE_STRUCT(TaskPlan);
	void PostLoad(void);

	// this will be some smart number (a counter?)
	int id;
	// temp only, for compatibility (will be removed)
	std::list<int> builders;
	std::list<BuilderTracker*> builderTrackers;

	float currentBuildPower;
	const UnitDef* def;
	std::string defName;
	float3 pos;
};

struct UpgradeTask {
	CR_DECLARE_STRUCT(UpgradeTask);
	void PostLoad(void);

	UpgradeTask() {
		oldBuildingID  = -1;
		oldBuildingPos = float3(-1.0f, -1.0f, -1.0f);
		newBuildingDef = NULL;
		creationFrame  = -1;
		reclaimStatus  = false;
	}

	UpgradeTask(int buildingID, int frame, const float3& buildingPos, const UnitDef* buildingDef) {
		oldBuildingID  = buildingID;
		oldBuildingPos = buildingPos;
		newBuildingDef = buildingDef;
		creationFrame  = frame;
		reclaimStatus  = false;
	}

	int            oldBuildingID;
	float3         oldBuildingPos;
	const UnitDef* newBuildingDef;
	int            creationFrame;
	bool           reclaimStatus;

	std::set<int> builderIDs;
};



struct Factory {
	CR_DECLARE_STRUCT(Factory);

	int id;
	// temp only, for compatibility (will be removed)
	std::list<int> supportbuilders;
	std::list<BuilderTracker*> supportBuilderTrackers;
};

struct NukeSilo {
	CR_DECLARE_STRUCT(NukeSilo);

	int id;
	int numNukesReady;
	int numNukesQueued;
};

struct MetalExtractor {
	CR_DECLARE_STRUCT(MetalExtractor);

	int id;
	int buildFrame;
};

#endif
