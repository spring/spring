#pragma once

#include "aidef.h"

class AAIBuildTable;
class AAISector;
class AAI;

class AAIMap
{
public:
	AAIMap(AAI *ai);
	~AAIMap(void);

	void Init();
	
	// sets cells of the builmap to value
	bool SetBuildMap(int xPos, int yPos, int xSize, int ySize, int value, int ignore_value = -1);

	// converts map-pos to unit-pos and vice versa
	void Pos2BuildMapPos(float3 *pos, const UnitDef* def);
	void BuildMapPos2Pos(float3 *pos, const UnitDef* def);
	void Pos2FinalBuildPos(float3 *pos, const UnitDef *def);
	
	// returns buildsites for normal and defence buildings
	float3 GetHighestBuildsite(const UnitDef *def, int xStart, int xEnd, int yStart, int yEnd);
	float3 GetCenterBuildsite(const UnitDef *def, int xStart, int xEnd, int yStart, int yEnd, bool water = false);
	float3 GetRandomBuildsite(const UnitDef *def, int xStart, int xEnd, int yStart, int yEnd, int tries, bool water = false);
	float3 GetBuildSiteInRect(const UnitDef *def, int xStart, int xEnd, int yStart, int yEnd, bool water = false);
	
	float3 GetClosestBuildsite(const UnitDef *def, float3 pos, int max_distance, bool water);

	void GetSize(const UnitDef *def, int *xSize, int *ySize);

	// prevents ai from building too many buildings in a row
	void CheckRows(int xPos, int yPos, int xSize, int ySize, bool add, bool water);

	// adds or removes blocked cells
	void BlockCells(int xPos, int yPos, int width, int height, bool block, bool water);

	// returns number of cells with big slope
	int GetCliffyCells(int xPos, int yPos, int xSize, int ySize);
	int GetCliffyCellsInSector(AAISector *sector);

	// updates spotted buildings
	void UpdateRecon();

	void UpdateSectors();

	void UpdateCategoryUsefulness(const UnitDef *killer_def, int killer, const UnitDef *killed_def, int killed);

	void UpdateArty(int attacker);

	char* GetMapTypeTextString(int mapType);
	char* GetMapTypeString(int mapType);

private:
	// return next cell in direction with a certain value
	int GetNextX(int direction, int xPos, int yPos, int value);	// 0 means left, other right; returns -1 if not found 
	int GetNextY(int direction, int xPos, int yPos, int value);	// 0 means up, other down; returns -1 if not found

	
	// returns true if buildmap allows construction
	bool CanBuildAt(int xPos, int yPos, int xSize, int ySize, bool water = false);

	// if auto_set == true, the loaded values are assigned to the current sectordata as well 
	void ReadMapLearnFile(bool auto_set);

	// loads mex spots, cliffs etc. from file or creates new one
	void GetMapData();

	// calculates learning effect
	void Learn();

	// sets cliffs
	void DetectCliffs();

	// sets water
	void DetectWater();

	void SearchMetalSpots();


public:
	AAISector **sector;			// sectors

	int xMapSize, yMapSize;				// x and y size of the map
	int xSectors, ySectors;				// number of sectors
	int xSectorSize, ySectorSize;		// size of sectors (in unit pos coordinates)
	int xSectorSizeMap, ySectorSizeMap;	// size of sectors (in map coodrinates = 1/8 xSize)

	list<AAIMetalSpot> metal_spots;

	bool metalMap;
	MapType mapType;	// 0 -> unknown ,1-> land map (default), 2 -> air map, 
						// 3 -> water map with land connections 
						// 4 -> "full water map

	float **map_usefulness;

	bool initialized;

	char *buildmap;		// map of the cells in the sector; 
						// 0 unoccupied, flat terrain 
						// 1 occupied flat terrain, 
						// 2 spaces between buildings 
						// 3 terrain not suitable for constr.
						// 4 water
						// 5 occupied water
	char *blockmap;		// number of buildings which ordered a cell to blocked

	// temp for scouting
	float *units_spotted;

private:
	AAI *ai;
	IAICallback *cb;
	AAIBuildTable *bt;

	// used for scouting
	int *unitsInLos;
	int *unitsInSector;
};
