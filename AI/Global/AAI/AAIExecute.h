#pragma once

#include "AAI.h"
#include "aidef.h"
#include "AAIGroup.h"
#include "AAIBuilder.h"


class AAI;
class AAIBuildTable;
class AAIBrain;
class AAIMap;
class AAIUnitTable;

class AAIExecute
{
public:
	AAIExecute(AAI* ai, AAIBrain *brain);
	~AAIExecute(void);

	void ConstructBuildingAt(int building, int builder, float3 position); 

	void moveUnitTo(int unit, float3 *position);  

	void stopUnit(int unit);

	bool IsBusy(int unit);

	void AddUnitToGroup(int unit_id, int def_id, UnitCategory category);

	void UpdateRecon(); 

	// returns a position to retreat unit of certain type
	float3 GetSafePos(int def_id);
	float3 GetSafePos(bool land, bool water);

	// updates average ressource usage
	void UpdateRessources();

	// checks if ressources are sufficient and orders construction of new buildings
	void CheckRessources();

	float GetEnergyUrgency();
	float GetMetalUrgency();
	float GetEnergyStorageUrgency();
	float GetMetalStorageUrgency();

	// upgrades
	void CheckMexUpgrade();
	void CheckRadarUpgrade();
	void CheckJammerUpgrade();

	// checks which building is most important to be constructed
	void CheckConstruction();
	void CheckFactories();
	void CheckAirBase();
	// checks radar and jammers
	void CheckRecon(); 
	void CheckJammer();
	// checks length of buildques and adjusts rate of unit production
	void CheckBuildques();
	void CheckDefences();
	void CheckStationaryArty();
	void CheckUnits();

	// builds all kind of buildings
	bool BuildFactory();
	bool BuildDefences();
	void BuildUnit(UnitCategory category, float speed, float cost, float range, float ground_eff, float air_eff, float hover_eff, float sea_eff, float submarine_eff, float stat_eff, bool urgent);
	bool BuildRecon();
	bool BuildJammer();
	bool BuildExtractor();
	bool BuildMetalMaker();
	bool BuildStorage();
	bool BuildPowerPlant();
	bool BuildArty();
	bool BuildAirBase();

	// called when building has been finished / contruction failed
	void ConstructionFailed(int unit_id, float3 build_pos, int def_id);
	void ConstructionFinished();

	// builds defences around mex spot if necessary
	void DefendMex(int mex, int def_id);

	// tries to build a defence building vs category in the specified sector
	// returns BUILDORDER_SUCCESFUL if succesful
	BuildOrderStatus BuildStationaryDefenceVS(UnitCategory category, AAISector *dest);

	// tries to call support vs air (returns true if succesful)
	void DefendUnitVS(int unit, const UnitDef *def, UnitCategory category, float3 enemy_pos, int importance);

	// returns true if succesfully assisting power plant construction
	bool AssistConstructionOfCategory(UnitCategory category, int importance = 5);

	// adds a unit to the correct wishlist
	bool AddUnitToBuildque(int def_id, int number = 1, bool urgent = false);

	// returns buildque for a certain factory
	list<int>* GetBuildqueOfFactory(int def_id);

	// returns the the total ground offensive power of all units
	float GetTotalGroundPower();

	// returns the the total air defence power of all units
	float GetTotalAirPower();

	// 
	AAIGroup* GetClosestGroupOfCategory(UnitCategory category, UnitType type, float3 pos, int importance); 

	float3 GetRallyPoint(UnitCategory category);

	AAIMetalSpot* FindBaseMetalSpot(float3 builder_pos);
	AAIMetalSpot* FindExternalMetalSpot(int land_mex, int water_mex);

	float3 GetBuildsite(int builder, int building, UnitCategory category);

	void InitBuildques();
	
	// accelerates game startup
	void AddStartFactory();

	// custom relations
	bool static least_dangerous(AAISector *left, AAISector *right);
	bool static suitable_for_power_plant(AAISector *left, AAISector *right);
	bool static suitable_for_ground_factory(AAISector *left, AAISector *right);
	bool static suitable_for_sea_factory(AAISector *left, AAISector *right);
	bool static suitable_for_arty(AAISector *left, AAISector *right);
	bool static defend_vs_ground(AAISector *left, AAISector *right);
	bool static defend_vs_air(AAISector *left, AAISector *right);
	bool static defend_vs_hover(AAISector *left, AAISector *right);
	bool static defend_vs_sea(AAISector *left, AAISector *right);

	// cache to speed things up a bit
	float static learned;
	float static current;
	
	// buildques for the factories
	list<int> *buildques;
	int numOfFactories;
	int unitProductionRate;			

	// ressource management
	// tells ai, how many times additional metal/energy has been requested
	float futureRequestedMetal;
	float futureRequestedEnergy;
	float futureAvailableMetal;
	float futureAvailableEnergy;
	float futureStoredMetal;
	float futureStoredEnergy;
	float averageMetalUsage;
	float averageEnergyUsage;
	float averageMetalSurplus;
	float averageEnergySurplus;
	int disabledMMakers; 

	int counter; 
	float metalSurplus[8];
	float energySurplus[8];

	// urgency of construction of building of the different categories
	float urgency[METAL_MAKER+2];

private:
	AAI *ai;
	IAICallback *cb;
	AAIBuildTable *bt;
	AAIBrain *brain;
	AAIMap *map;
	AAIUnitTable *ut;

	// stores which buildque belongs to what kind of factory
	int *factory_table;
	
};
