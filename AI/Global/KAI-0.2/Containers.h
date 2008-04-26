#ifndef CONTAINERS_H
#define CONTAINERS_H

using std::string;
using std::vector;
using std::list;

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
class CSurveillanceHandler;
class CDamageControl;
//added by Kloot
class DGunController;
class CCoverageHandler;

struct AIClasses {
	CR_DECLARE_STRUCT(AIClasses);
	void PostLoad();
	IAICallback*			cb;
	IAICheats*				cheat;
	CEconomyTracker*		econTracker;
	CBuildUp*				bu;
	CSunParser*				parser;
	CMetalMap*				mm;
	CMaths*					math;
	CDebug*					debug;
	CPathFinder*			pather;
	CUnitTable*				ut;
	CThreatMap*				tm;
	CUnitHandler*			uh;
	CDefenseMatrix*			dm;
	CAttackHandler*			ah;
	CEconomyManager*		em;
	CDamageControl*			dc;
	CSurveillanceHandler*	sh;
	vector<CUNIT*>			MyUnits;
	std::ofstream*			LOGGER;
	// added by Kloot
	DGunController*			dgunController;
	CCoverageHandler*       radarCoverage;
	CCoverageHandler*       sonarCoverage;
	CCoverageHandler*       rjammerCoverage;
	CCoverageHandler*       sjammerCoverage;
	CCoverageHandler*       antinukeCoverage;
	CCoverageHandler*       shieldCoverage;
};


/*
 * The KAI unit def.
 */
struct UnitType {
	// 1 means arm, 2 core; 0 if side has not been set 
	int side;

	vector<int> canBuildList;
	// TODO: The list of buildable units for this unit
	// vector<BuildOption> canBuildOptions;

	vector<int> builtByList;
	vector<float> DPSvsUnit;
	float AverageDPS;
	float Cost;
	// vector<string> TargetCategories;
	const UnitDef* def;
	int category;
	// the new category system (bitmask)
	unsigned categoryMask;

	// the move types are bit masks, in order for the pather to use them both must be used (moveDepthType | moveSlopeType)

	// indicates the sea/land canMove type (hovers are 0)
	unsigned moveDepthType;
	// indicates the slope canMove type (ships/subs are 0), this can be used for selecting the unit speed map for the pather
	unsigned moveSlopeType;
	// indicates the crush strength type, this will be used by the pather when we have debris maps
	unsigned crushStrength;
	// use as move mask for the pather, they can be joined by (unit1 -> hackMoveType & unit2 -> hackMoveType)
	unsigned hackMoveType;

	// moveType 0 = Vehicle/kbot, moveType 1 = hover, moveType 2 = ship/sub, moveType 3 = air
	unsigned moveType;
};


class integer2 {
	public:
		CR_DECLARE_STRUCT(integer2);

		integer2(): x(0), y(0) {
		};
		integer2(const int x, const int y): x(x), y(y) {
		}
		~integer2() {
		};

		inline bool operator == (const integer2 &f) const {
			return (x - f.x == 0 && y - f.y == 0);
		}

		union {
			struct {
				int x;
				int y;
			};
		};
};


class KAIMoveType {
	public:
//		CR_DECLARE_STRUCT(KAIMoveType);
		KAIMoveType(): Air(false), MaxSlope(10000), MaxDepth(10000), MinDepth(-10000), CrushStrength(0), size(1) {
		};
		~KAIMoveType() {
		};

		void Assign(const UnitDef* unit) {
			if (!unit -> canfly) {
				MaxSlope = unit -> movedata -> maxSlope;

				if (unit -> movedata -> moveType == unit -> movedata -> Ground_Move)
					MaxDepth = unit -> movedata -> depth;
				else {
					// it's a hover/ship/sub so no max
					MaxDepth = 5000;
				}
				if (unit -> movedata -> moveType == unit -> movedata -> Ship_Move)
					MinDepth = unit -> movedata -> depth;
				else {
 					// it's a ground unit
					MinDepth = -10000;
				}

				CrushStrength = unit -> movedata -> crushStrength;
				size = unit -> movedata -> size;
				Air = false;
			}
			else {
				Air = true;
			}
		}

		inline bool operator == (const KAIMoveType &f) const {
			bool b1 = MaxSlope == f.MaxSlope;
			bool b2 = (MaxDepth == f.MaxDepth || (MaxDepth >= 255 && f.MaxDepth >= 255));
			bool b3 = (MinDepth == f.MinDepth || (MinDepth < 0 && f.MinDepth < 0));
			bool b4 = (Air && f.Air);

			return ((b1 && b2 && b3) || b4);
		}

		union {
			struct {
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
 * builder container: main task is to make sure that tracking builders is easy (making asserts and tests)
 */
struct BuilderTracker {
	CR_DECLARE_STRUCT(BuilderTracker);
	int builderID;
	// if not NULL then this worker belongs to this BuildTask
	int buildTaskId;
	// if not NULL then this worker belongs to this TaskPlan
	int taskPlanId;
	// if not NULL then this worker belongs to this Factory.
	int factoryId;
	// if not NULL then this worker is on a reclaim job or something, it's not tracked until it gets idle
	int customOrderId;

	// if not NULL then this worker is stuck or something
	int stuckCount;
	// if this builder ends up without orders for some time, try IdleUnitAdd()
	int idleStartFrame;
	// the frame an order is given, needed for lag tolerance when verifying unit command
	int commandOrderPushFrame;
	// TODO: this is (or will be in the future) a hint of what this builder will make
	int categoryMaker;

	// for speed up, and debugging
	const UnitDef* def;

	// these are unused

	// what frame this builder will start working.
	int estimateRealStartFrame;
	// this will be constant or based on the last startup time
	int estimateFramesForNanoBuildActivation;
	// simple ETA, updated every 16 frames or so (0 means it's there)
	int estimateETAforMoveingToBuildSite;
	// the def -> buildDistance or something
	float distanceToSiteBeforeItCanStartBuilding;
	void PostLoad();
};

struct BuildTask {
	CR_DECLARE_STRUCT(BuildTask);
	void PostLoad();
	int id;
	int category;

	// temp only, for compability (will be removed)
	// list<int> builders;
	// the new container
	list<BuilderTracker*> builderTrackers;

	float currentBuildPower;
	// for speed up, and debugging
	const UnitDef* def;
	// needed for command verify and debugging
	float3 pos;
};

struct TaskPlan {
	CR_DECLARE_STRUCT(TaskPlan);
	void PostLoad();
	// this will be some smart number (a counter?)
	int id;

	// temp only, for compability (will be removed)
	// list<int> builders;
	// the new container
	list<BuilderTracker*> builderTrackers;

	float currentBuildPower;
	const UnitDef* def;
	std::string defname;
	float3 pos;
};

/*
 * This is a builder that can't move.
 */
struct Factory {
	CR_DECLARE_STRUCT(Factory);
	void PostLoad();
	int id;
	// for speed up, and debugging
	const UnitDef* factoryDef;

	// temp only, for compability (will be removed)
	// list<int> supportbuilders;
	// the new container
	list<BuilderTracker*> supportBuilderTrackers;

	// the list of buildable (and approved) units for this factory
	// list<BuildOption> buildOptions;
	float currentBuildPower;
	// this is how much buildpower this factory wants (or is wanted to have), unused
	float currentTargetBuildPower;
};


class EconomyTotal {
	public:
		void Clear() {
			Cost = Damage = Armor = BuidPower = AverageSpeed = Hitpoints = 0;
		}
		float Cost;
		float Hitpoints;
		float Damage;
		float Armor;
		float BuidPower;
		float AverageSpeed;
};


#endif
