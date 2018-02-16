// _____________________________________________________
//
// RAI - Skirmish AI for Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

// NOTES:
// "position blocks" refers to the in-game units of meaturement
// A Map Preivew Block is 512x512 position blocks
// GetMapWidth(),GetMapHeight(),GetHeightMap() uses 8x8 position blocks
// GetMetalMap(),GetSlopeMap() uses 16x16 position blocks

#ifndef RAI_GLOBAL_TERRAIN_MAP_H
#define RAI_GLOBAL_TERRAIN_MAP_H

#include "LegacyCpp/IAICallback.h"
#include "LegacyCpp/UnitDef.h"
#include "LogFile.h"
#include <map>
#include <list>
#include <set>
#include <cstdio>
#include <string.h>
using namespace std;

//#include "GPathfinder.h"

struct TerrainMapArea;
struct TerrainMapAreaSector;
struct TerrainMapMobileType;
struct TerrainMapImmobileType;
struct TerrainMapSector;

struct TerrainMapAreaSector
{
	TerrainMapAreaSector()
	{
		S=NULL;
		area=NULL;
		areaClosest=NULL;
	};
	// NOTE: some of these values are loaded as they become needed, use GlobalTerrainMap functions
	TerrainMapSector *S;	// always valid
	TerrainMapArea* area;		// The TerrainMapArea this sector belongs to, otherwise = 0 until
	TerrainMapArea* areaClosest;// uninitialized, = the TerrainMapArea closest to this sector
	// Use this to find the closest sector useable by a unit with a different MoveType, the 0 pointer may be valid as a key index
	map<TerrainMapMobileType*,TerrainMapAreaSector*> sectorAlternativeM; // uninitialized
	map<TerrainMapImmobileType*,TerrainMapSector*> sectorAlternativeI; // uninitialized
};

struct TerrainMapArea
{
	TerrainMapArea(int areaIUSize, TerrainMapMobileType* TMMobileType)
	{
		index=areaIUSize;
		mobileType = TMMobileType;
		percentOfMap=0.0;
		areaUsable=0;
	};
	bool areaUsable; // Should units of this type be used in this area
	int index;
	TerrainMapMobileType* mobileType;
	map<int,TerrainMapAreaSector*> sector;			// key = sector index, a list of all sectors belonging to it
	map<int,TerrainMapAreaSector*> sectorClosest;	// key = sector indexes not in "sector", indicates the sector belonging to this map-area with the closest distance
												// NOTE: use GlobalTerrainMap->GetClosestSector: these values are not initialized but are instead loaded as they become needed
	float percentOfMap; // 0-100
};
#define MAP_AREA_LIST_SIZE 50
struct TerrainMapMobileType
{
	TerrainMapMobileType()
	{
		typeUsable = false;
		sector = 0;
		areaSize = 0;
		areaLargest = 0;
		udSize = 0;
		MD = NULL;
		canFloat = 0;
		canHover = 0;
		minElevation = 0.0;
		maxElevation = 0.0;
		maxSlope = 0.0;
		memset(area,0,MAP_AREA_LIST_SIZE * sizeof(area[0]));
		
	};
	~TerrainMapMobileType()
	{
		delete [] sector;
		for(int i=0; i<areaSize; i++)
			delete area[i];
	};
	bool typeUsable; // Should units of this type be used on this map
	TerrainMapAreaSector *sector;	// Each MoveType has it's own sector list, GlobalTerrainMap->GetSectorIndex() gives an index
	TerrainMapArea *area[MAP_AREA_LIST_SIZE];	// Each MoveType has it's own MapArea list
	TerrainMapArea *areaLargest;// Largest area usable by this type, otherwise = 0
	int areaSize;

	float maxSlope;		// = MoveData*->maxSlope
	float maxElevation; // = -ud->minWaterDepth
	float minElevation; // = -MoveData*->depth
	bool canHover;
	bool canFloat;
	MoveData* MD;
	int udSize;
//	GlobalPathfinder *PF; // unused
};

struct TerrainMapSector
{
	TerrainMapSector()
	{
		percentLand = 0.0;
		maxSlope = 0.0;
		maxElevation = 0.0;
		minElevation = 0.0;
		isWater = 0;
	};

	bool isWater;		// (Water = true) (Land = false)
	float3 position;	// center of the sector, same as unit positions

	// only used during initialization
	float percentLand; // 0-100
	float minElevation; // 0 or less for water
	float maxElevation;
	float maxSlope;		// 0 or higher
};

struct TerrainMapImmobileType
{
	TerrainMapImmobileType()
	{
		udSize = 0;
		canFloat = 0;
		canHover = 0;
		minElevation = 0.0;
		maxElevation = 0.0;
		typeUsable = 0;
		
	};

	bool typeUsable; // Should units of this type be used on this map
	map<int,TerrainMapSector*> sector;			// a list of sectors useable by these units
	map<int,TerrainMapSector*> sectorClosest;	// key = sector indexes not in "sector", indicates the closest sector in "sector"
	float minElevation;
	float maxElevation;
	bool canHover;
	bool canFloat;
	int udSize;
};

class GlobalTerrainMap
{
public:
	GlobalTerrainMap(IAICallback* cb, cLogFile* logfile);
	~GlobalTerrainMap();

	bool CanMoveToPos(TerrainMapArea *area, const float3& destination);
	TerrainMapAreaSector* GetSectorList(TerrainMapArea* sourceArea=0);
	TerrainMapAreaSector* GetClosestSector(TerrainMapArea* sourceArea, const int& destinationSIndex);
	TerrainMapSector* GetClosestSector(TerrainMapImmobileType* sourceIT, const int& destinationSIndex);
	TerrainMapAreaSector* GetAlternativeSector(TerrainMapArea* sourceArea, const int& sourceSIndex, TerrainMapMobileType* destinationMT);
	TerrainMapSector* GetAlternativeSector(TerrainMapArea* destinationArea, const int& sourceSIndex, TerrainMapImmobileType* destinationIT); // can return 0
	int GetSectorIndex(const float3& position); // use IsSectorValid() to insure the index is valid
	bool IsSectorValid(const int& sIndex);

	list<TerrainMapMobileType> mobileType;			// Used for mobile units, not all movedatas are used
	map<int,TerrainMapMobileType*> udMobileType;	// key = ud->id, Used to find a TerrainMapMobileType for a unit
	list<TerrainMapImmobileType> immobileType;		// Used for immobile units
	map<int,TerrainMapImmobileType*> udImmobileType;// key = ud->id, Used to find a TerrainMapImmobileType for a unit
	TerrainMapAreaSector *sectorAirType;	// used for flying units, GetSectorIndex gives an index
	TerrainMapSector *sector;				// global sector data, GetSectorIndex gives an index
	TerrainMapImmobileType *landSectorType;	// 0 to the sky
	TerrainMapImmobileType *waterSectorType;// minElevation to 0

	bool waterIsHarmful;	// Units are damaged by it (Lava/Acid map)
	bool waterIsAVoid;		// (Space map)
	float minElevation;		// 0 or less (used by cRAIUnitDefHandler, builder start selecter)
	float percentLand;		// 0 to 100 (used by cRAIUnitDefHandler)

	int sectorXSize;
	int sectorZSize;
	int convertStoP; // Sector to Position: times this value for convertion, divide for the reverse

private:
	int GetFileValue(int &fileSize, char *&file, string entry);
	typedef pair<int,TerrainMapAreaSector*> iasPair;
	typedef pair<int,TerrainMapSector*> isdPair;
	typedef pair<TerrainMapMobileType*,TerrainMapAreaSector*> msPair;
	typedef pair<TerrainMapImmobileType*,TerrainMapSector*> isPair;
//	cLogFile *l; // Debugging only
};

#endif
