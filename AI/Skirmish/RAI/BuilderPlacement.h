// _____________________________________________________
//
// RAI - Skirmish AI for Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_BUILDER_PLACEMENT_H
#define RAI_BUILDER_PLACEMENT_H

struct ResourceSiteExt;
class cBuilderPlacement;

#include "RAI.h"
#include "Builder.h"
using std::map;
using std::pair;
using std::set;

struct ResourceSiteExtBO
{
	ResourceSiteExtBO(sRAIUnitDef* UDR );
	void CheckBuild();
	sRAIUnitDef *udr;	// This is the unit in question
	bool RBBlocked;
	bool RBRanked;
	bool RBRanged;
	bool CanBuild;	// not blocked, not outranked, not outranged - used to adjust SetULConstructs()
};

struct ResourceSiteExt
{
	ResourceSiteExt(ResourceSite *RSite, IAICallback* callback);
	void CheckBlocked();	// Note: this will not work in a UnitDestroyed Event, since the dead extractor still exists at that moment
	void CheckRanked();		// For the moment, only called from SetResourceOwner()
	void SetRanged(bool inRange=true);

	int unitID;					// Valid if a unit is occupying this site, otherwise -1
	sRAIUnitDef *unitUD;		// Valid if 'UnitID' is set
	int builderID;				// Valid if a builder has been assigned to this site, otherwise -1
	UnitInfo *builderUI;		// Valid if 'BuilderID' is set
	bool enemy;
	bool ally;

	float searchRadius;	// distance from resource a unit is willing to be built
	int disApart;		// distance from structures when building a unit
	ResourceSite *S;	// Basic resource site data

	map<int,ResourceSiteExtBO> BuildOptions;	// The posible units that can be build at this position
	map<int,ResourceSiteExt*> Linked;		// key value = Resources[] index
	map<int,ResourceSiteExt*> LinkedD2;	// key value = Resources[] index, All 'Linked' Resources as well as their Linked Resources
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

	bool NeedResourceSite(const UnitDef* bd) const;
	ResourceSiteExt* FindResourceSite(float3& pos, const UnitDef* bd, TerrainMapArea* BuilderMA); // called when a builder has been assigned
	float3 FindBuildPosition(sBuildQuarry *BQ);
	void CheckBlockedRList( map<int,ResourceSiteExt*> *RL = 0 ); // CheckBlocked for all resource sites on this list

	// position must be valid
	bool CanBeBuiltAt(sRAIUnitDef *udr, float3& position, const float& range=0.0); // NOTE: returns false if the area was too small to be recorded
	bool CanBuildAt(UnitInfo *U, const float3& position, const float3& destination);

	// modified version of ALCallback.cpp's ClosestBuildSite
//	float3 ClosestBuildSite(const UnitDef* ud, float3 p, float sRadius, int facing=0);
private:
	bool FindWeaponPlacement(UnitInfo *U, float3& position);
	void UpdateAllyResources(); // determines what resources allies have gained or lost
	int GetResourceIndex(const int &unit, const UnitDef* ud);
	void SetResourceOwner(int RSindex, ResourceSiteExt *RS, int unit, sRAIUnitDef *udr=0);

	cLogFile *l;
	cRAI *G;
	IAICallback *cb;
	map<int,UnitInfo*> UExtractor;
	map<int,UnitInfo*> UGeoPlant;

	ResourceSiteExt **Resources; // List of all limited resources (metal,geo)
	int ResourceSize;
	typedef pair<int,ResourceSiteExt*> irPair;
	// A ResourceSiteExt is only on one list at a given time
	map<int,ResourceSiteExt*> RSAvailable;	// Resources a unit is willing to build at
	map<int,ResourceSiteExt*> RSRemaining;	// Resources that can not be reached
/*
	struct BuildGroup
	{
		BuildGroup(int unit)
		{
			RS=0;
			units.insert(unit);
		};
		BuildGroup(ResourceSiteExt *Resource)
		{
			RS=Resource;
			position=RS->position;
			x=32.0;
			z=32.0;
		};
		float3 position;
		float x;
		float z;
		int type;
		ResourceSiteExt *RS;
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
	int sectorXSize;
	int sectorZSize;
	float MapW;	// x
	float MapH;	// z
*/
	// Only used on loading

	// only used during initialization
	float FindResourceDistance(ResourceSite* R1, ResourceSite* R2, const int &pathType);
	typedef pair<int,float> ifPair; // used for ResourceSiteDistance.distance
	struct ResourceLinkInfo
	{
		int index;
		int bestI;				// closest resource not currently linked
		set<int> restrictedR;	// which indexes should the resource not be compared to
	};
};

#endif
