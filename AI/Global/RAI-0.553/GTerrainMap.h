// _____________________________________________________
//
// RAI - Skirmish AI for TA Spring
// Author: Reth / Michael Vadovszki
// _____________________________________________________

#ifndef RAI_TERRAIN_MAP_H
#define RAI_TERRAIN_MAP_H

struct MapBody;
struct MapSector;

#include "ExternalAI/IAICallback.h"
#include "Sim/Units/UnitDef.h"
#include "LogFile.h"
#include <map>

//#define SECTOR_SIZE 256

struct MapSector
{
	MapSector()
	{
		Water=false;
		Land=false;
		AltSector=0;
	};
	bool Water;
	bool Land;
	float3 Pos;				// center of the sector, same as unit positions
	MapSector* AltSector;	// if land then best water sector, if water then best land sector, else self
	map<int,MapBody*> mb;	// key = MapBody index
	map<int,MapSector*> ms; // key = MapBody index, closest sector of this land mass, equal to self if this sector is part of the same land mass.
};

struct MapBody
{
	MapBody(int Index)
	{
		index=Index;
		PercentOfMap=0.0;
		PercentMetal=0.0;
		MetalMap=false;
	};
	bool Water;
	int index;
	float PercentOfMap; // 0-100
	float PercentMetal; // 0-100
	bool MetalMap;
	map<int,MapSector*> Sector; // key = sector index
};

class cTerrainMap
{
public:
	cTerrainMap(IAICallback* cb, cLogFile* logfile);
	~cTerrainMap();

	int GetSector(const float3& Position); // use SectorValid() to insure the index is valid
	bool CanMoveToPos(int MapBody, float3 Destination);
	bool SectorValid(const int& SectorI);

	bool MetalMapLand;
	bool MetalMapWater;
	float PercentMetal;
	float PercentLand;
	float MaxWaterDepth;

	MapSector *Sector;
	int SectorXSize;
	int SectorZSize;

	MapBody *MapB[50];
	int MapBodySize;
	MapBody *MapBMainLand;	// Largest Land Mass, otherwise = 0
	MapBody *MapBMainWater; // Largest Water Mass, otherwise = 0

private:
	cLogFile *l;
//	IAICallback* cb;
};

#endif
