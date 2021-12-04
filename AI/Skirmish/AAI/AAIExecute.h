// -------------------------------------------------------------------------
// AAI
//
// A skirmish AI for the Spring engine.
// Copyright Alexander Seizinger
//
// Released under GPL license: see LICENSE.html for more information.
// -------------------------------------------------------------------------

#ifndef AAI_EXECUTE_H
#define AAI_EXECUTE_H

#include "aidef.h"

namespace springLegacyAI {
	struct UnitDef;
}
using namespace springLegacyAI;

enum BuildOrderStatus {BUILDORDER_FAILED, BUILDORDER_NOBUILDPOS, BUILDORDER_NOBUILDER, BUILDORDER_SUCCESFUL};

class AAI;
class AAIBuildTable;
class AAIBrain;
class AAIMap;
class AAIUnitTable;
class AAISector;

class AAIExecute
{
public:
	AAIExecute(AAI* ai);
	~AAIExecute(void);

	void InitAI(int commander_unit_id, const UnitDef *commander_def);

	// return true if building will be placed at a valid pos = inside sectors
	bool InitBuildingAt(const UnitDef *def, float3 *pos, bool water);

	void CreateBuildTask(int unit, const UnitDef *def, float3 *pos);

	void MoveUnitTo(int unit, float3 *position);

	void AddUnitToGroup(int unit_id, int def_id, UnitCategory category);

	void BuildScouts();

	void SendScoutToNewDest(int scout);

	// returns a position to retreat unit of certain type
	float3 GetSafePos(int def_id, float3 unit_pos);

	// updates average ressource usage
	void UpdateRessources();

	// checks if ressources are sufficient and orders construction of new buildings
	void CheckRessources();


	// checks if buildings of that type could be replaced with more efficient one (e.g. mex -> moho)
	void CheckMexUpgrade();
	void CheckRadarUpgrade();
	void CheckJammerUpgrade();

	// checks which building type is most important to be constructed and tries to start construction
	void CheckConstruction();

	// the following functions determine how urgent it is to build a further building of the specified type
	void CheckFactories();
	void CheckAirBase();
	void CheckRecon();
	void CheckJammer();
	void CheckStationaryArty();

	// checks length of buildqueues and adjusts rate of unit production
	void CheckBuildqueues();

	//
	void CheckDefences();

	// builds all kind of buildings

//	void BuildUnit(UnitCategory category, float speed, float cost, float range, float power, float ground_eff, float air_eff, float hover_eff, float sea_eff, float submarine_eff, float stat_eff, float eff, bool urgent);

	// called when building has been finished / contruction failed
	void ConstructionFailed(float3 build_pos, int def_id);
	void ConstructionFinished();

	// builds defences around mex spot if necessary
	void DefendMex(int mex, int def_id);

	// returns a position for the unit to withdraw from close quarters combat (but try to keep enemies in weapons range)
	// returns ZeroVector if no suitable pos found (or no enemies close enough)
	void GetFallBackPos(float3 *pos, int unit_id, float max_weapon_range) const;

	void CheckFallBack(int unit_id, int def_id);


	// tries to call support vs air (returns true if succesful)
	void DefendUnitVS(int unit, unsigned int enemy_movement_type, float3 *enemy_pos, int importance);


	// adds a unit to the correct wishlist
	bool AddUnitToBuildqueue(int def_id, int number, bool urgent);

	// returns buildque for a certain factory
	list<int>* GetBuildqueueOfFactory(int def_id);


	float3 GetRallyPoint(unsigned int unit_movement_type, int continent_id, int min_dist, int max_dist);
	float3 GetUnitBuildsite(int builder, int unit);

	int unitProductionRate;

	// ressource management
	// tells ai, how many times additional metal/energy has been requested
	float futureRequestedMetal;
	float futureRequestedEnergy;
	float futureAvailableMetal;
	float futureAvailableEnergy;
	float futureStoredMetal;
	float futureStoredEnergy;
	float averageMetalSurplus;
	float averageEnergySurplus;
	int disabledMMakers;


	// urgency of construction of building of the different categories
	float urgency[METAL_MAKER+1];

	// sector where next def vs category needs to be built (0 if none)

	// debug
	void GiveOrder(Command *c, int unit, const char *owner);

private:
	// accelerates game startup
	void AddStartFactory();

	// custom relations
	float static sector_threat(AAISector *);

	bool static least_dangerous(AAISector *left, AAISector *right);
	bool static suitable_for_power_plant(AAISector *left, AAISector *right);
	bool static suitable_for_ground_factory(AAISector *left, AAISector *right);
	bool static suitable_for_sea_factory(AAISector *left, AAISector *right);
	bool static defend_vs_ground(AAISector *left, AAISector *right);
	bool static defend_vs_air(AAISector *left, AAISector *right);
	bool static defend_vs_hover(AAISector *left, AAISector *right);
	bool static defend_vs_sea(AAISector *left, AAISector *right);
	bool static defend_vs_submarine(AAISector *left, AAISector *right);
	bool static suitable_for_ground_rallypoint(AAISector *left, AAISector *right);
	bool static suitable_for_sea_rallypoint(AAISector *left, AAISector *right);
	bool static suitable_for_all_rallypoint(AAISector *left, AAISector *right);

	// cache to speed things up a bit
	float static learned;
	float static current;
	// buildques for the factories
	vector<list<int> > buildques;
	// number of factories (both mobile and sationary)

	int numOfFactories;

	// tries to build a defence building vs category in the specified sector
	// returns BUILDORDER_SUCCESFUL if succesful
	BuildOrderStatus BuildStationaryDefenceVS(UnitCategory category, AAISector *dest);

	// returns true if successfully assisting construction
	bool AssistConstructionOfCategory(UnitCategory category, int importance = 5);

	// returns the the total ground offensive power of all units



	float GetTotalGroundPower();

	// returns the the total air defence power of all units
	float GetTotalAirPower();

	// chooses a starting sector close to specified sector
	void ChooseDifferentStartingSector(int x, int y);

	// returns closest (taking into account movement speed) group with units of specified unit type that may reach the location

	AAIGroup* GetClosestGroupForDefence(UnitType group_type, float3 *pos, int continent, int importance);
	float3 GetRallyPointCloseTo(UnitCategory category, unsigned int unit_movement_type, int continent_id, float3 pos, int min_dist, int max_dist);
	float3 GetBuildsite(int builder, int building, UnitCategory category);
	void InitBuildques();

	void stopUnit(int unit);
	void ConstructBuildingAt(int building, int builder, float3 position);
	bool IsBusy(int unit);

	float GetEnergyUrgency();

	float GetMetalUrgency();
	float GetEnergyStorageUrgency();
	float GetMetalStorageUrgency();

	bool BuildFactory();
	bool BuildDefences();
	bool BuildRadar();
	bool BuildJammer();
	bool BuildExtractor();
	bool BuildMetalMaker();
	bool BuildStorage();
	bool BuildPowerPlant();
	bool BuildArty();
	bool BuildAirBase();

	float averageMetalUsage;
	float averageEnergyUsage;
	int counter;
	float metalSurplus[8];
	float energySurplus[8];
	AAISector *next_defence;
	UnitCategory def_category;

	int issued_orders;

	AAI *ai;

	// stores which buildque belongs to what kind of factory
	vector<int> factory_table;
};

#endif

