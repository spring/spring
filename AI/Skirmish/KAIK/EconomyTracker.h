#ifndef KAIK_ECONOMYTRACKER_HDR
#define KAIK_ECONOMYTRACKER_HDR

#include <list>
#include <vector>

struct EconomyUnitTracker;

struct BuildingTracker {
	CR_DECLARE_STRUCT(BuildingTracker);

	int unitUnderConstruction;
	int category;
	float hpLastFrame;
	float damage;								// track the damage to get the right ETA
	float hpSomeTimeAgo;						// for nano-stalling support
	float damageSomeTimeAgo;					// unused
	int startedRealBuildingFrame;
	int etaFrame;								// ETA frame made localy (with nanostall), based on last 16 frames only
	float maxTotalBuildPower;					// without the builders that are still moving (real possible build power)
	float assignedTotalBuildPower;				// with the builders that are still moving (possible build power in the future)
	float energyUsage;							// current usage (with nanostall), last 16 frames
	float metalUsage;							// current usage (with nanostall), last 16 frames
	
	bool buildTask;								// buildTask, if it has one
	int factory;								// factory, if it has one
	EconomyUnitTracker* economyUnitTracker;		// pointer to the planed EconomyUnitTracker for this unit

	void clear(void) {
		unitUnderConstruction		= 0;
		category					= 0;
		hpLastFrame					= 0;
		damage						= 0;		// track the damage to get the right ETA
		hpSomeTimeAgo				= 0;		// for nano-stalling support
		damageSomeTimeAgo			= 0;		// unused
		startedRealBuildingFrame	= -1;
		etaFrame					= -1;		// ETA frame made localy (with nanostall), based on last 16 frames
		maxTotalBuildPower			= 0;		// without the builders that are still moving (real possible build power)
		assignedTotalBuildPower		= 0;		// with the builders that are still moving (possible build power in the future)
		energyUsage					= 0;		// current usage (with nanostall), last 16 frames
		metalUsage					= 0;		// current usage (with nanostall), last 16 frames
		buildTask					= false;	// buildTask, if it has one
		factory						= 0;		// factory, if it has one
		economyUnitTracker			= NULL;		// pointer to the planed EconomyUnitTracker for this unit
	}
};

// Intended to contain units under construction only.
struct UnitStateRequirement {
	/*
	If this is the "current" frame, then its what this unit will do with the economy the next 16 frames.
	energy/metal usage is either from builders, or from the units own doing.
	*/

	int frame;
	bool underConstruction;
	bool nanoStallPoint;
	bool constructionCompletedPoint;
	bool constructionStartPoint;
	bool constructionSpeedChangePoint;
	/*
	If this is true, and its not controllable, no more UnitStateRequirement's will be made for this unit
	If controllableUsage is true, then it will be moved to a list that match its nature.
	*/
	bool unitEntersService; 

	float energyChange;
	float metalChange;

	
	// This is for units that are completed only
	// and then only if they are controllable
	bool controllableUsage;		// We can change the economy effects of the unit
	bool unitStateOrder;		// What the units order are intended to be. Metalmakers are off if energy is lower than some value.
	float energyChangeOff;
	float metalChangeOff;
};


struct controllableUnitEconomyForcast {
};


struct TotalEconomyState {
	int frame;				// frame this state represents
	int madeInFrame;		// frame this state was made
	float energyStored;
	float metalStored;

	float energyMake;
	float metalMake;

	float energyUsage;		// from construction only
	float metalUsage;		// from construction only

	float energyStorageSize;
	float metalStorageSize;
};

/*
 * This holds information about when a builder might
 * manage to get to its build site and start working
 */
struct BuilderETAdata {
	int builderID;
	int estimateRealStartFrame;						// what frame this builder will start working.
	int estimateFramesForNanoBuildActivation;		// constant or based on the last startup time
	int estimateETAforMoveingToBuildSite;			// ETA, updated every 16 frames or so (0 = its there)
	float distanceToSiteBeforeItCanStartBuilding;	// def->buildDistance or something.
};

struct EconomyUnitTracker {
	CR_DECLARE_STRUCT(EconomyUnitTracker);
	void PostLoad();

	int economyUnitId;							// Only economyUnitId and createFrame gives a correct ID
	int createFrame;							// If the unit is under construction, this is the globally made ETA
	BuildingTracker* buildingTracker;			// pointer to the BuildingTracker for this unit (if not done), MUST be updated before use
	bool alive;
	const UnitDef* unitDef;						// We will lose the unit id later on
	int dieFrame;
	int category;
	float totalEnergyMake;						// total lifetime sum
	float totalMetalMake;						// total lifetime sum
	float totalEnergyUsage;						// total lifetime sum
	float totalMetalUsage;						// total lifetime sum
	float lastUpdateEnergyMake;					// last 16 frame sum
	float lastUpdateMetalMake;					// last 16 frame sum
	float lastUpdateEnergyUsage;				// last 16 frame sum
	float lastUpdateMetalUsage;					// last 16 frame sum
	bool dynamicChangingUsage;					// for windmills and units with guns
	bool nonEconomicUnit;						// for units that is to be ignored by the economy planner (?)
	float estimateEnergyChangeFromDefWhileOn;	// sum change from unitDef*
	float estimateMetalChangeFromDefWhileOn;	// sum change from unitDef*
	float estimateEnergyChangeFromDefWhileOff;	// sum change from unitDef*
	float estimateMetalChangeFromDefWhileOff;	// sum change from unitDef*

	void clear(void) {
		economyUnitId = 0;
		createFrame = 0;
		// BuildingTracker* buildingTracker;
		alive = false;
		unitDef = 0;
		dieFrame = 0;
		category = 0;
		totalEnergyMake = 0;
		totalMetalMake = 0;
		totalEnergyUsage = 0;
		totalMetalUsage = 0;
		lastUpdateEnergyMake = 0;
		lastUpdateMetalMake = 0;
		lastUpdateEnergyUsage = 0;
		lastUpdateMetalUsage = 0;
		dynamicChangingUsage = false;
		estimateEnergyChangeFromDefWhileOn = 0;
		estimateMetalChangeFromDefWhileOn = 0;
		estimateEnergyChangeFromDefWhileOff = 0;
		estimateMetalChangeFromDefWhileOff = 0;
	}
};

/*
This is a planed building. Its either a metal or energy making building (or unit).
The same as a TaskPlan, with more data (in order to avoid changing the TaskPlan struct)
*/
struct EconomyBuildingPlan {
};

class CEconomyTracker {
	public:
		CR_DECLARE(CEconomyTracker);

		CEconomyTracker(AIClasses* ai);
		~CEconomyTracker();
		void frameUpdate(int);
		void UnitCreated(int unit);
		void UnitFinished(int unit);
		void UnitDestroyed(int unit);
		void UnitDamaged(int unit, float damage);

	private:
		std::vector<std::list<BuildingTracker> > allTheBuildingTrackers;
		std::list<EconomyUnitTracker*> deadEconomyUnitTrackers;
		std::list<EconomyUnitTracker*> newEconomyUnitTrackers;
		std::list<EconomyUnitTracker*> activeEconomyUnitTrackers;
		std::list<EconomyUnitTracker*> underConstructionEconomyUnitTrackers;

		AIClasses* ai;
		void updateUnitUnderConstruction(BuildingTracker* bt);
		void SetUnitDefDataInTracker(EconomyUnitTracker* economyUnitTracker);
		TotalEconomyState makePrediction(int targetFrame);

		bool trackerOff;

		float oldEnergy;
		float oldMetal;

		float constructionEnergy;
		float constructionMetal;

		float constructionEnergySum;
		float constructionMetalSum;
};


#endif
