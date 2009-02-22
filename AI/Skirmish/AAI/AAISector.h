// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the TA Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

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
	AAIMetalSpot* GetFreeMetalSpot();
	void FreeMetalSpot(float3 pos, const UnitDef *extractor);
	void Init(AAI *ai, int x, int y, int left, int right, int top, int bottom);

	// adds/removes the sector from base sectors; returns true if succesful
	bool SetBase(bool base);

	int GetNumberOfMetalSpots();
	void Update();

	// associates an extractor with a metal spot in that sector
	void AddExtractor(int unit_id, int def_id, float3 *pos);

	// returns buildsite for a unit in that sector (or zerovector if nothing found)
	float3 GetBuildsite(int building, bool water = false);

	// returns a buildsite for a defence building
	float3 GetDefenceBuildsite(int building, UnitCategory category, float terrain_modifier, bool water);
	float3 GetRandomBuildsite(int building, int tries, bool water = false);
	float3 GetCenterBuildsite(int building, bool water = false);
	float3 GetHighestBuildsite(int building);

	// gets rectangle for possible buildsite
	void GetBuildsiteRectangle(int *xStart, int *xEnd, int *yStart, int *yEnd);

	// returns a float ranging from 1 to 2 indicating how close the sector is to the map border (1 is closest)
	float GetMapBorderDist();

	// returns number of own buildings in that sector
	int GetNumberOfBuildings();

	// helper functions
	void Pos2SectorMapPos(float3 *pos, const UnitDef* def);
	void SectorMapPos2Pos(float3 *pos, const UnitDef* def);

	// add and remove defence buidlings to that sector
	void RemoveDefence(int unit_id);
	void AddDefence(int unit_id, int def_id);

	// removes building from sector -> update own_structure & unitsOfType[]
	void RemoveBuildingType(int def_id);

	// returns the category with the weakest defence in comparison with threat
	UnitCategory GetWeakestCategory();

	// returns the defence power vs a certain unit category, -1 if failed
	float GetDefencePowerVs(UnitCategory category);
	float GetDefencePowerVsID(int combat_cat_id);

	// returns threatzo the sector by a certain category
	float GetThreatBy(UnitCategory category, float learned, float current);
	float GetThreatByID(int combat_cat_id, float learned, float current);
	float GetOverallThreat(float learned, float current);

	// returns threat by the sector to categories
	float GetThreatTo(float ground, float air, float hover, float sea, float submarine);

	// returns combat power of units in that and neighbouring sectors vs combat cat
	float GetAreaCombatPowerVs(int combat_category, float neighbour_importance);

	// updates threat map
	void UpdateThreatValues(UnitCategory unit, UnitCategory attacker);

	// returns lost units in that sector
	float GetLostUnits(float ground, float air, float hover, float sea, float submarine);

	// returns center of the sector
	float3 GetCenter();

	// returns a free position in sector where units can be send to regardless of movement_type (ZeroVector if none found)
	void GetMovePos(float3 *pos);

	// returns a free position in sector on specified continent for the movement type (ZeroVector if none found)
	void GetMovePosOnContinent(float3 *pos, unsigned int movement_type, int continent);

	// returns true is pos is within sector
	bool PosInSector(float3 *pos);

	// get water/flat ground ratio
	float GetWaterRatio();
	float GetFlatRatio();

	// returns true if sector is connected with a big ocean (and not only a small pond)
	bool ConnectedToOcean();

	// sector number
	int x, y;

	float enemy_structures;
	float own_structures;
	float allied_structures;

	int rally_points;	// how many groups got a rally point in that sector

	list<AAIDefence> defences;
	int failed_defences; // how many times aai tried to build defences and could not find possible constructionsite

	// units in the sector
	vector<int> enemyUnitsOfType;
	vector<int> unitsOfType;

	// how many times the sector was not scouted
	float last_scout;

	// importance of the sector
	float importance_this_game;
	float importance_learned;

	// how many times ai has been attacked by a certain assault category in this sector
	vector<float> attacked_by_this_game;
	vector<float> attacked_by_learned;

	// how many battles took place in that sector (of each assault category)
	vector<float> combats_this_game;
	vector<float> combats_learned;

	// how many units of certain type recently lost in that sector
	vector<float> lost_units;

	// stores combat power of all stationary defs/combat unit vs different categories
	vector<float> stat_combat_power; // 0 ground, 1 air, 2 hover, 3 sea, 4 submarine
	vector<float> mobile_combat_power; // 0 ground, 1 air, 2 hover, 3 sea, 4 submarine, 5 building

	// combat.eff of all enemy units in this sector (0 if safe sector)
	float threat;

	// water and flat terrain ratio
	float flat_ratio;
	float water_ratio;

	float left, right, top, bottom;

	list<AAIMetalSpot*> metalSpots;

	AAI *ai;

	AAIUnitTable *ut;
	AAIMap *map;

	bool freeMetalSpots;

	int distance_to_base;	// 0 = base, 1 = neighbour to base
	bool interior;			// true if sector is no inner sector

	unsigned int allowed_movement_types;	// movement types that may enter this sector
};
