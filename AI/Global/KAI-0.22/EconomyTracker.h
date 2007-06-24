#ifndef ECONOMYTRACKER_H
#define ECONOMYTRACKER_H

#include "GlobalAI.h"

struct EconomyUnitTracker;

struct BuildingTracker
{
	CR_DECLARE_STRUCT(BuildingTracker);
	int unitUnderConstruction;
	int category;
	float hpLastFrame;
	float damage; // Must track the damage to get the right eta
	float hpSomeTimeAgo; // This is for nano staling support
	float damageSomeTimeAgo; // unused atm
	int startedRealBuildingFrame;
	int etaFrame; // This is the eta frame made localy (with nanostall), based on last 16 frames only
	float maxTotalBuildPower; // This is without the builders that are still moveing (real possible build power).
	float assignedTotalBuildPower; // This is with the builders that are still moveing (possible build power in the future).
	float energyUsage; // This is the current usage (with nanostall), last 16 frames *
	float metalUsage; // This is the current usage (with nanostall), last 16 frames *
	
	bool buildTask; // The buildTask, if it have one (cant use a pointer, as its inside a dynamic list ??)
	int factory; // The factory, if it have one (cant use a pointer, as its inside a dynamic list ??)
	EconomyUnitTracker *economyUnitTracker; // A pointer to the planed EconomyUnitTracker for this unit, this pointer is stable
	
	
	void clear()
	{
		unitUnderConstruction = 0;
		category = 0;
		hpLastFrame = 0;
		damage = 0; // Must track the damage to get the right eta
		hpSomeTimeAgo = 0; // This is for nano staling support
		damageSomeTimeAgo = 0; // unused atm
		startedRealBuildingFrame = -1;
		etaFrame = -1; // This is the eta frame made localy (with nanostall), based on last 16 frames
		maxTotalBuildPower = 0; // This is without the builders that are still moveing (real possible build power).
		assignedTotalBuildPower = 0; // This is with the builders that are still moveing (possible build power in the future).
		energyUsage = 0; // This is the current usage (with nanostall), last 16 frames
		metalUsage = 0; // This is the current usage (with nanostall), last 16 frames
		buildTask = false; // The buildTask, if it have one (cant use a pointer, as its inside a dynamic list ??)
		factory = 0; // The factory, if it have one (cant use a pointer, as its inside a dynamic list ??)
		economyUnitTracker = NULL; // A pointer to the planed EconomyUnitTracker for this unit
	}
};

struct EconomyUnitTracker
{
	CR_DECLARE_STRUCT(EconomyUnitTracker);
	void PostLoad();
	AIClasses *ai;
	int economyUnitId; // Only economyUnitId and createFrame gives a correct ID
	int createFrame; // If this is in the future, the unit is under construction, and this is the globaly made eta (is a hack now: FIX)
	BuildingTracker * buildingTracker; // A pointer to the BuildingTracker for this unit (if its not done), MUST be updated before use
	bool alive;
	const UnitDef * unitDef; // We will lose the unit id later on
	std::string unitDefName;
	int dieFrame;
	int category; // Not realy needed
	float totalEnergyMake; // The total lifetime sum
	float totalMetalMake; // The total lifetime sum
	float totalEnergyUsage; // The total lifetime sum
	float totalMetalUsage; // The total lifetime sum
	float lastUpdateEnergyMake; // The last 16 frame sum
	float lastUpdateMetalMake; // The last 16 frame sum
	float lastUpdateEnergyUsage; // The last 16 frame sum
	float lastUpdateMetalUsage; // The last 16 frame sum
	bool dynamicChangingUsage; // This is for windmills and units with guns *
	bool nonEconomicUnit; // This is for units that is to be ignored by the economy planer (?) *
	float estimateEnergyChangeFromDefWhileOn; // This is the sum change from unitDef *
	float estimateMetalChangeFromDefWhileOn; // This is the sum change from unitDef *
	float estimateEnergyChangeFromDefWhileOff; // This is the sum change from unitDef *
	float estimateMetalChangeFromDefWhileOff; // This is the sum change from unitDef *
	void clear()
	{
		economyUnitId = 0;
		createFrame = 0; 
		//BuildingTracker * buildingTracker; 
		alive = false;
		unitDef = 0;
		dieFrame = 0;
		category = 0; // Not realy needed
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
Intended to contain units under construction only.
High speed loop version (late design)
*/
struct UnitStateRequirement
{
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
	bool controllableUsage; // We can change the economy effects of the unit 
	bool unitStateOrder; // What the units order are intended to be. Metalmakers are off if energy is lower than some value.
	float energyChangeOff;
	float metalChangeOff;
};


/*
Intended for 
*/
struct controllableUnitEconomyForcast
{

};


struct TotalEconomyState
{
	int frame; // The frame this state represents
	int madeInFrame; // The frame this state was made
	float energyStored;
	float metalStored;
	
	float energyMake;
	float metalMake;
	
	float energyUsage; // ... this is from construction only
	float metalUsage; // ... this is from construction only
	
	float energyStorageSize; // 
	float metalStorageSize; // 
};

/*
This holds information about when a builder might manage to get to its build site and start working

*/
struct BuilderETAdata
{
	int builderID;
	int estimateRealStartFrame; // What frame this builder will start working.
	int estimateFramesForNanoBuildActivation; // This will be constant or based on the last startup time
	int estimateETAforMoveingToBuildSite; // Simple eta, updated every 16 frames or so (0 = its there)
	float distanceToSiteBeforeItCanStartBuilding; // The def->buildDistance or something.
	
};


/*
This is a planed building. Its either a metal or energy making building (or unit).
The same as a TaskPlan, with more data (in order to avoid changing the TaskPlan struct)

*/
struct EconomyBuildingPlan
{
	
};

class CEconomyTracker
{
	CR_DECLARE(CEconomyTracker);
	public:

	CEconomyTracker(AIClasses* ai);
	virtual ~CEconomyTracker();
	void frameUpdate();
	void UnitCreated(int unit);
	void UnitFinished(int unit);
	void UnitDestroyed(int unit);
	void UnitDamaged(int unit, float damage);
	

	private:
	vector<list<BuildingTracker> > allTheBuildingTrackers;
	list<EconomyUnitTracker*> deadEconomyUnitTrackers;
	list<EconomyUnitTracker*> newEconomyUnitTrackers;
	list<EconomyUnitTracker*> activeEconomyUnitTrackers;
	list<EconomyUnitTracker*> underConstructionEconomyUnitTrackers;
	
	AIClasses* ai;
	void updateUnitUnderConstruction(BuildingTracker * bt);
	void SetUnitDefDataInTracker(EconomyUnitTracker * economyUnitTracker);
	TotalEconomyState makePrediction(int targetFrame);
	
	bool trackerOff;
	
	float oldEnergy;
	float oldMetal;
	
	float myCalcEnergy;
	float myCalcMetal;
	
	float constructionEnergy;
	float constructionMetal;
	//float oldHP;

	float constructionEnergySum;
	float constructionMetalSum;

};




#endif /* ECONOMYTRACKER_H */
