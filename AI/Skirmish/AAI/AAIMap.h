// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#ifndef AAI_MAP_H
#define AAI_MAP_H

#include "aidef.h"
#include "System/float3.h"

#include <vector>
#include <list>
using namespace std;

class AAIBuildTable;
class AAI;
class AAISector;

namespace springLegacyAI {
	struct UnitDef;
}
using namespace springLegacyAI;

struct AAIContinent
{
	int id;
	int size;			// number of cells
	bool water;
};

class AAIMap
{
public:
	AAIMap(AAI *ai);
	~AAIMap(void);

	void Init();
	void Pos2FinalBuildPos(float3 *pos, const UnitDef *def);

	// returns id of continent the cell belongs to
	int GetContinentID(int x, int y);
	int GetContinentID(float3 *pos);

	// returns true if pos is located on s small continent (= pond or island)
	bool LocatedOnSmallContinent(float3 *pos);

	// returns continent id with respect to the units movement type (e.g. land, non amphib unit being in shallow water will return id of nearest land continent)
	int GetSmartContinentID(float3 *pos, unsigned int unit_movement_type);


	// returns sector (0 if out of sector map -> e.g. aircraft flying outside of the map) of a position
	AAISector* GetSectorOfPos(float3 *pos);


	float GetEdgeDistance(float3 *pos);

	// returns buildsites for normal and defence buildings
	float3 GetHighestBuildsite(const UnitDef *def, int xStart, int xEnd, int yStart, int yEnd);
	float3 GetCenterBuildsite(const UnitDef *def, int xStart, int xEnd, int yStart, int yEnd, bool water = false);
	float3 GetRandomBuildsite(const UnitDef *def, int xStart, int xEnd, int yStart, int yEnd, int tries, bool water = false);
	float3 GetBuildSiteInRect(const UnitDef *def, int xStart, int xEnd, int yStart, int yEnd, bool water = false);

	// prefer buildsites that are on plateus and not too close to the edge of the map
	float3 GetRadarArtyBuildsite(const UnitDef *def, int xStart, int xEnd, int yStart, int yEnd, float range, bool water);

	// return rating of a the best buidliste fpr a def. building vs category within specified rect (and stores pos in pointer)
	float GetDefenceBuildsite(float3 *best_pos, const UnitDef *def, int xStart, int xEnd, int yStart, int yEnd, UnitCategory category,  float terrain_modifier, bool water);

	float3 GetClosestBuildsite(const UnitDef *def, float3 pos, int max_distance, bool water);

	// updates buildmap ((un)block cells + insert/remove spaces) if regular building is added/removed (factories need some extra space)
	void UpdateBuildMap(float3 build_pos, const UnitDef *def, bool block, bool water, bool factory);

	// returns number of cells with big slope
	int GetCliffyCells(int xPos, int yPos, int xSize, int ySize);

	// updates spotted ennemy/ally buildings/units on the map
	void UpdateRecon();

	// updates enemy buildings/enemy stat. combat strength in sectors based on scouted_buildings_map
	void UpdateEnemyScoutingData();

	void UpdateSectors();


	// adds/removes a defence buidling to the defence map
	void AddDefence(float3 *pos, int defence);
	void RemoveDefence(float3 *pos, int defence);

	// updates number of lost units in sector
	void UnitKilledAt(float3 *pos, UnitCategory category);

	// sectors
	vector<vector<AAISector> > sector;

	// used for scouting, used to get all friendly/enemy units in los
	vector<int> units_in_los;
	static int xMapSize, yMapSize;				// x and y size of the map (map coordinates)
	static int xSectors, ySectors;				// number of sectors
	static int xSectorSize, ySectorSize;		// size of sectors (in unit pos coordinates)
	static int xSectorSizeMap, ySectorSizeMap;	// size of sectors (in map coodrinates = 1/8 xSize)
	static float land_ratio;
	static int water_metal_spots;
	static int land_metal_spots;
	static bool metalMap;
	static float water_ratio;
	static MapType map_type;	// 0 -> unknown ,1-> land map (default), 2 -> air map,
								// 3 -> water map with land connections
								// 4 -> "full water map

	static vector< vector<int> > team_sector_map;	// stores the number of ai player which has taken that sector (-1 if none)
											// this helps preventing aai from expanding into sectors of other aai players


	static vector<int> buildmap;	// map of the cells in the sector;
							// 0 unoccupied, flat terrain
							// 1 occupied flat terrain,
							// 2 spaces between buildings
							// 3 terrain not suitable for constr.
							// 4 water
							// 5 occupied water
	static vector<AAIContinent> continents;
	static int avg_water_continent_size;
	static list<int> map_categories_id;


private:
	// defence maps
	vector<float> defence_map;	//  ground/sea defence map has 1/2 of resolution of blockmap/buildmap
	vector<float> air_defence_map; // air defence map has 1/2 of resolution of blockmap/buildmap
	vector<float> submarine_defence_map; // submarine defence map has 1/2 of resolution of blockmap/buildmap
	// converts map-pos to unit-pos and vice versa
	void Pos2BuildMapPos(float3 *pos, const UnitDef* def);

	// stores the def_id of the building or combat unit placed on that cell (0 if none), same resolution as los map (= 1/2 resolution of buildmap)
	vector<unsigned short> scout_map;

	// stores the frame of the last update of a cell (same resolution as los map)
	vector<int> last_updated_map;

	// indicates whether sector is within los (to prevent unnecessary updates) 0 = no los, > 0 = los
	vector<unsigned short> sector_in_los;
	vector<unsigned short> sector_in_los_with_enemies;

	// temp for scouting
	vector<unsigned short> enemy_combat_units_spotted;

	bool initialized;
	// krogothe's metal spot finder
	void SearchMetalSpots();
	// determines type of map (land, land/water or water map)
	void DetectMapType();

	void CalculateWaterRatio();

	// calculates which parts of the are connected
	void CalculateContinentMaps();

	// determines water, high slopes, defence map
	void AnalyseMap();

	// calculates learning effect
	void Learn();

	// if auto_set == true, the loaded values are assigned to the current sectordata as well
	void ReadMapLearnFile(bool auto_set);

	// reads continent cache file (and creates new one if necessary)
	void ReadContinentFile();

	// reads map cache file (and creates new one if necessary)
	// loads mex spots, cliffs etc. from file or creates new one
	void ReadMapCacheFile();

	// returns true if buildmap allows construction
	bool CanBuildAt(int xPos, int yPos, int xSize, int ySize, bool water = false);

	// return next cell in direction with a certain value
	int GetNextX(int direction, int xPos, int yPos, int value);	// 0 means left, other right; returns -1 if not found
	int GetNextY(int direction, int xPos, int yPos, int value);	// 0 means up, other down; returns -1 if not found

	const char* GetMapTypeString(MapType map_type);

	const char* GetMapTypeTextString(MapType map_type);

	int GetCliffyCellsInSector(AAISector *sector);

	// blocks/unblocks cells (to prevent AAI from packing buildings too close to each other)
	void BlockCells(int xPos, int yPos, int width, int height, bool block, bool water);

	// prevents ai from building too many buildings in a row
	void CheckRows(int xPos, int yPos, int xSize, int ySize, bool add, bool water);

	// returns footprint size of a building
	void GetSize(const UnitDef *def, int *xSize, int *ySize);
	// returns distance to closest edge of the map (in build_map coordinates)
	int GetEdgeDistance(int xPos, int yPos);
	// true if x/y are a valid sector
	bool ValidSector(int x, int y);
	// sets cells of the builmap to value
	bool SetBuildMap(int xPos, int yPos, int xSize, int ySize, int value, int ignore_value = -1);

	void BuildMapPos2Pos(float3 *pos, const UnitDef* def);

	std::string LocateMapLearnFile() const;
	std::string LocateMapCacheFile() const;

	AAI *ai;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// static (shared with other ai players)
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	static int aai_instances;	// how many AAI instances have been initialized

	static int xSize, ySize;					// x and y size of the map (unit coordinates)
	static int losMapRes;				// resolution of the LOS map
	static int xLOSMapSize, yLOSMapSize;		// x and y size of the LOS map
	static int xDefMapSize, yDefMapSize;		// x and y size of the defence maps (1/4 resolution of map)
	static int xContMapSize, yContMapSize;		// x and y size of the continent maps (1/4 resolution of map)
	static list<AAIMetalSpot> metal_spots;
	static float flat_land_ratio;
	static vector<int> blockmap;		// number of buildings which ordered a cell to blocked
	static vector<float> plateau_map;	// positive values indicate plateaus, 1/4 of resolution of blockmap/buildmap
	static vector<int> continent_map;	// id of continent a cell belongs to

	static vector<int> ship_movement_map;	// movement maps for different categories, 1/4 of resolution of blockmap/buildmap
	static vector<int> kbot_movement_map;
	static vector<int> vehicle_movement_map;
	static vector<int> hover_movement_map;
	static int land_continents;
	static int water_continents;


	static int avg_land_continent_size;
	static int max_land_continent_size;
	static int max_water_continent_size;
	static int min_land_continent_size;
	static int min_water_continent_size;

	static list<UnitCategory> map_categories;
};

#endif

