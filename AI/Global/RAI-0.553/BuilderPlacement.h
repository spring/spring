// _____________________________________________________
//
// RAI - Skirmish AI for TA Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_BUILDER_PLACEMENT_H
#define RAI_BUILDER_PLACEMENT_H

struct sResourceSpot;
class cBuilderPlacement;

#include "RAI.h"
#include "Builder.h"
#include "Sim/Features/FeatureDef.h"
//#include "ExternalAI/IAICallback.h"

struct sResourceSpotBO
{
	sResourceSpotBO(sRAIUnitDef* UDR );
	void CheckBuild();
	sRAIUnitDef *udr;	// This is the unit in question
	bool RBBlocked;
	bool RBRanked;
	bool RBRanged;
	bool CanBuild;	// not blocked, not outranked, not outranged - used to adjust SetULConstructs()
};

struct sResourceSpot
{
	sResourceSpot(IAICallback* callback, float3 position, int ID=-1, const FeatureDef* featureUD=0);
	void CheckBlocked();	// Note: this will not work in a UnitDestroyed Event, since the dead extractor still exists at that moment
	void CheckRanked();		// For the moment, only called from SetResourceOwner()
	void SetRanged(bool inRange=true);

	int featureID;				// Valid if it's a Geo Spot, otherwise -1
	const FeatureDef *featureD;	// Valid if 'featureID' is set
	int unitID;					// Valid if a unit is occuping this spot, otherwise -1
	sRAIUnitDef *unitUD;		// Valid if 'UnitID' is set
	int builderID;				// Valid if a builder has been assigned to this spot, otherwise -1
	UnitInfo *builderUI;		// Valid if 'BuilderID' is set
	bool enemy;
	bool ally;

	int type;					// 1=metal/Non-Feature, 2=geo/Feature
	float searchRadius;
	int disApart;

	float3 location;
	map<int,sResourceSpotBO> BuildOptions;	// The posible units that can be build at this location
	map<int,sResourceSpot*> Linked;		// key value = Resources[] index
	map<int,sResourceSpot*> LinkedD2;	// key value = Resources[] index, All 'Linked' Resources as well as their Linked Resources
//	int unitLostFrame;
	IAICallback* cb;	// used in CheckBlocked()
};

class cBuilderPlacement
{
public:
	cBuilderPlacement(IAICallback* callback, cRAI* global);
	~cBuilderPlacement();
	void UResourceCreated(int unit, UnitInfo *U);
	void UResourceDestroyed(int unit, UnitInfo *U);
	void EResourceEnterLOS(int enemy, EnemyInfo *E);

	bool NeedsResourceSpot(const UnitDef* bd, int ConstructMapBody);
	sResourceSpot* FindResourceSpot(float3& pos, const UnitDef* bd, int BuilderMapBody); // called when a builder has been assigned
	float3 FindBuildPosition(sBuildQuarry *BQ);
	void CheckBlockedRList( map<int,sResourceSpot*> *RL = 0 ); // CheckBlocked for all resource spots on this list

private:
	bool FindWeaponPlacement(UnitInfo *U, float3& pos);
	void UpdateAllyResources(); // determines what resources allies have gained or lost
	bool CanBuildAtMapBody(float3 fBuildPos, const UnitDef* bd, int BuilderMapBody); // NOTE: returns false if the land mass was too small to be recorded

	cLogFile *l;
	cRAI *G;
	IAICallback *cb;
	map<int,UnitInfo*> UExtractor;
	map<int,UnitInfo*> UGeoPlant;

	int GetResourceIndex(const int &unit, const UnitDef* ud);
	typedef pair<int,sResourceSpotBO> irbPair;
	sResourceSpot *Resources[400]; // List of all limited resources (metal,geo)
	int ResourceSize;
	typedef pair<int,sResourceSpot*> irPair;
	// A sResourceSpot is only on one list at a given time
	map<int,sResourceSpot*> Resource;		// Available Resource Spots
	map<int,sResourceSpot*> ResRemaining;	// A Resource that can not be reached
	void SetResourceOwner(int RSindex, sResourceSpot *RS, int unit, sRAIUnitDef *udr=0);
/*
	struct BuildGroup
	{
		BuildGroup(int unit)
		{
			RS=0;
			units.insert(unit);
		};
		BuildGroup(sResourceSpot *Resource)
		{
			RS=Resource;
			location=RS->location;
			x=32.0;
			z=32.0;
		};
		float3 location;
		float x;
		float z;
		int type; // 
		sResourceSpot *RS;
		set<int> units;
	};
	struct BuildSector
	{
		BuildSector() {};
		set<BuildGroup> LocalGroup;
		set<BuildGroup*> Group;
	};
	set<BuildGroup*> BorderResource; // Hostile Bordering resource
	set<BuildGroup*> DefenceDemand; // Possible groups to built a defence into
	set<BuildGroup*> JammerDemand; // Possible groups to built a radar/sonar jammer into
	void GroupAdd(BuildGroup *G);
	void GroupRemove(BuildGroup *G);
	BuildSector *Sector;
	int SectorXSize;
	int SectorZSize;
	float MapW;	// x
	float MapH;	// z
*/
};

#endif
