// _____________________________________________________
//
// RAI - Skirmish AI for Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_BUILDER_H
#define RAI_BUILDER_H

struct sBuildQuarry;
class cBuilder;

#include "RAI.h"
#include "PowerManager.h"
#include <deque>
#include <list>

struct sBuildQuarry
{
	sBuildQuarry(sRAIBuildList *buildlist);
	~sBuildQuarry();
	bool IsValid(int frame);
	void SetRS(ResourceSiteExt *rs);

	int builderID;			// ID of the assigned builder, otherwise -1
	UnitInfo *builderUI;	// valid if 'builderID' is set
	list<int> creationID;   // ID of who is being built, I defined this as an array to work around a bug involving misordered calls of UnitDestroyed, UnitCreated - in other words at rare temporary moments this list will hold 2 unit ids.   Spring-Version(v0.72b1-0.73b1)
	int creationUDID;		// ID of what is being built
	sRAIUnitDef *creationUD;// always valid
	ResourceSiteExt *RS;		// The resource this must be built at
	sRAIBuildList *BL;		// used to update unitsActive

	int index;				// stores the index value of its own array
	int type;				// 1=Normal,2=Energy,3=Metal,4=Constructor,5=Energy Storage,6=Metal Storage,7=prerequisite
	int deletionFrame;		// If a unit does not choose to build the option by this frame, then delete the Build Quarry
	int tryCount;			// failed to build, probably due to enemy attacks
};

#define BUILD_QUARRY_SIZE 40

class cBuilder
{
public:
	cBuilder(IAICallback *callback, cRAI *global);
	virtual ~cBuilder();

	void UnitCreated(const int& unit, UnitInfo *U);
	void UnitFinished(const int& unit, UnitInfo *U);
	void UnitDestroyed(const int& unit, UnitInfo *U);
	void UnitAssignBuildList(const int& unit, UnitInfo *U, bool bInitialized=false); // bInitialized is unused, was ment to support a unit changing tasks

	void UBuilderFinished(const int& unit, UnitInfo *U);
	void UBuilderDestroyed(const int& unit, UnitInfo *U);
	void UBuilderIdle(const int& unit, UnitInfo *U);
//	void UBuilderDamaged(const int& unit,int attacker,float3 dir);
	bool UBuilderMoveFailed(const int& unit, UnitInfo *U); // returns true if a solution was found
	void HandleEvent(const IGlobalAI::PlayerCommandEvent *pce);
	void UpdateUDRCost();
	void UpdateKnownFeatures(const int& unit, UnitInfo *U);

	bool bInitiated;	// initialized as false, set to true after set conditions have been meet, usually about 30-60 frames into the game
	cPowerManager *PM;
	cBuilderPlacement *BP;

	map<int,UnitInfo*> UBuilder;	// List of builders, key value = unit ID
	map<int,UnitInfo*> UNanos;
	set<int> Decomission;			// Builders will reclaim these units in there free time
private:
	cRAI *G;
	cRAIUnitDefHandler *UDR;// G->UDH
	cLogFile *l;			// G->l
	IAICallback *cb;		// G->cb

	void CreateBuildOrders() const;
	int LastBuildOrder;

	float MCostLimit;
	float ECostLimit;
//	float MCostUpdate;
//	float ECostUpdate;

	// Work-Around for bugs functions GetMetalUsage() & GetEnergyUsage()
	// spring does not update the values by the time build idle is called, as a result the
	// economy would think that the resources are more strained than they really are.
	float BuilderEnergyDebug;	// updated at the beginning of UBuilderIdle
	float BuilderMetalDebug;	// updated at the beginning of UBuilderIdle
	int BuilderIDDebug;			// ID of the last builder to finish a task
	int BuilderFrameDebug;		// the frame that 'BuilderIDDebug' was last changed
//	double ConstructMetalUsage;
//	double ConstructEnergyUsage;

	int ConEnergyLost;
	int ConMetalLost;
	int ConEnergyDrain;		// How much Energy will be drained when all constructions have started, positive value, does not include contructions that have already begun
	int ConMetalDrain;		// How much Metal will be drained when all constructions have started, positive value, does not include contructions that have already begun
	int ConEnergyRate;		// How much extra Energy will be produced when all constructions have completed
	int ConMetalRate;		// How much extra Metal will be produced when all constructions have completed
	int ConEnergyStorage;	// How much extra Energy Storage will there be when all constructions have completed
	int ConMetalStorage;	// How much extra Metal Storage will there be when all constructions have completed
	bool MetalIsFavorable(float storage=0.50f,float production=1.0f); // returns true if there is no metal production or the ratio of both is met
	bool EnergyIsFavorable(float storage=0.50f,float production=1.0f); // returns true if there is no energy production or the ratio of both is met

	sBuildQuarry *BQ[BUILD_QUARRY_SIZE];
	sBuildQuarry *Prerequisite; // Limits RAI from building more than one at a time
	int BQSize[8]; // index 0 = total, other indexs accessed by iType.  Value of index is equal to counter
	void BQAssignBuilder(int index, const int& unit, UnitInfo* U);
//	void BQAssignConstruct(int index, const int& unit, sRAIUnitDef *udr);
	void BQAdd(sRAIUnitDef *udr, sRAIBuildList *BL, int type);
	void BQRemove(int index);

	struct UnitConstructionInfo
	{
		UnitConstructionInfo(sBuildQuarry *BuildQuarry, const int& unit, UnitInfo* UI)
		{
			U=UI;
			unitID = unit;
			BQ = BuildQuarry;
			BQ->creationID.push_front(unit);
			BQAbandoned=false;
		};
		~UnitConstructionInfo()
		{
		};

		bool BQAbandoned;
		sBuildQuarry *BQ; // valid if BQAbandoned=false
		UnitInfo *U;	// Always valid
		int unitID;
	};
	map<int,UnitConstructionInfo> UConstruction;	// List of what is being built, key value = unit ID

	// due to crash bugs in spring 0.74b3, the position is about the only safe information that could be gathered and stored.
	typedef pair<int,float3> ifPair;
	map<int,float3> FeatureDebris;			// List of features that are blocking the paths of our units
	typedef pair<int,FeatureDef*> ifdPair;
	map<int,float3> MetalDebris;			// List of metal reclaimables found
	map<int,float3> EnergyDebris;			// List of energy reclaimables found
	map<int,float3> ResDebris;				// List of resurrectables found
	typedef pair<string,sRAIUnitDef*> srPair;
	map<string,sRAIUnitDef*> UDRResurrect;	// List of what can be resurrected
};

#endif
