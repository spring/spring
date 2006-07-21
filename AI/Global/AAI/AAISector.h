#pragma once

#include "aidef.h"

class AAI;
class AAIUnitTable;
class AAIMap;

struct DefenceCoverage 
{
	Direction direction;
	float defence;
};

class AAISector
{
public:
	AAISector();
	~AAISector(void);

	void SetCoordinates(int left, int right, int top, int bottom);
	void SetGridLocation(int x, int y);
	void AddMetalSpot(AAIMetalSpot *spot);
	AAIMetalSpot* GetFreeMetalSpot(float3 pos);
	AAIMetalSpot* GetFreeMetalSpot();
	void FreeMetalSpot(float3 pos, const UnitDef *extractor);
	void SetAI(AAI *ai);
	void SetBase(bool base);
	int GetNumberOfMetalSpots();
	void Update();

	// associates an extractor with a metal spot in that sector
	void AddExtractor(int unit_id, int def_id, float3 pos);

	// returns buildsite for a unit in that sector (or zerovector if nothing found)	
	float3 GetBuildsite(int building, bool water = false);	
	
	// returns a buildsite for a defence building
	float3 GetDefenceBuildsite(int building, UnitCategory category, bool water = false);
	float3 GetRandomBuildsite(int building, int tries, bool water = false);
	float3 GetCenterBuildsite(int building, bool water = false);
	float3 GetHighestBuildsite(int building);
	
	// gets rectangle for possible buildsite
	void GetBuildsiteRectangle(int *xStart, int *xEnd, int *yStart, int *yEnd);

	// returns a float ranging from 1 to 2 indicating how close the sector is to the map border (1 is closest)
	float GetMapBorderDist();

	// helper functions
	void Pos2SectorMapPos(float3 *pos, const UnitDef* def);
	void SectorMapPos2Pos(float3 *pos, const UnitDef* def);

	// add and remove defence buidlings to that sector
	void RemoveDefence(int unit_id);
	void AddDefence(int unit_id, int def_id, float3 pos);

	// returns the category with the weakest defence in comparison with threat
	UnitCategory GetWeakestCategory();

	// returns the defence power vs a certain unit category, -1 if failed
	float GetDefencePowerVs(UnitCategory category);

	// returns threat by a certain category
	float GetThreat(UnitCategory category, float learned, float current);
	float GetOverallThreat(float learned, float current);

	// updates threat map
	void UpdateThreatValues(UnitCategory unit, UnitCategory attacker);

	// returns center of the sector
	float3 GetCenter();			

	// get water/flat gorund ratio
	float GetWaterRatio();
	float GetFlatRatio();

	// sector number
	int x, y;  

	float enemy_structures; 
	float own_structures;

	list<AAIDefence> defences;

	// units in the sector
	int enemyUnitsOfType[(int)SEA_BUILDER+1];
	int unitsOfType[(int)SEA_BUILDER+1];

	// how many times the sector was not scouted 
	float last_scout;
	
	// 0 is current importance, 1 is learned importance
	float importance[2];
	float *attacked_by[2]; // 0 ground, 1 air, 2 sea

	// how many battles took place in that sector
	float *combats[2];

	// how many units of certain type recently lost in that sector
	float *threat_against; // 0 ground, 1 air, 2 sea
	float *lost_units;

	// how efficiently stationary arty worked when placed in that sector
	float arty_efficiency[2];

	// water and flat terrain ratio
	float flat_ratio;
	float water_ratio;

	int left, right, top, bottom;

	list<AAIMetalSpot*> metalSpots;

	AAI *ai;

	AAIUnitTable *ut;
	AAIMap *map;

	bool freeMetalSpots;

	int distance_to_base;	// 0 = base, 1 = neighbour to base 
	bool interior;			// true if sector is no inner sector

	// internal for def. placement
	DefenceCoverage defCoverage[4];
};

