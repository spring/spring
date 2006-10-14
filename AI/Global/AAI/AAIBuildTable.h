#pragma once

class AAI;

#include "aidef.h"


class AAIBuildTable
{
public:
	AAIBuildTable(IAICallback *cb, AAI* ai);
	~AAIBuildTable(void);

	// call before you want to use the buildtable
	// loads everything from a cache file or creates a new one
	void Init();

	// precaches speed/cost/buildtime/range stats
	void PrecacheStats();

	// returns true, if a builder can build a certain unit (use UnitDef.id)
	bool CanBuildUnit(int id_builder, int id_unit);

	// returns side of a unit
	int GetSide(int unit);

	// returns side of a certian unittype (use UnitDef->id)
	int GetSideByID(int unit_id);

	// return unit type (for groups)
	UnitType GetUnitType(int def_id);

	// returns true, if unitid is in the list
	bool MemberOf(int unit_id, list<int> unit_list);

	// ******************************************************************************************************
	// the following functions are used to determine units that suit a certain purpose
	// if water == true, only water based units/buildings will be returned
	// randomness == 1 means no randomness at all; never set randomnes to zero -> crash 
	// ******************************************************************************************************
	// returns power plant
	int GetPowerPlant(int side, float cost, float urgency, float max_power, float current_energy, bool water, bool geo, bool canBuild);

	// returns a extractor from the list based on certain factors
	int GetMex(int side, float cost, float effiency, bool armed, bool water, bool canBuild);
	// returns mex with the biggest yardmap
	int GetBiggestMex();

	// return defence buildings to counter a certain category
	int GetDefenceBuilding(int side, float efficiency, float combat_power, float ground_eff, float air_eff, float hover_eff, float sea_eff, float submarine_eff, float urgency, float range, int randomness, bool water, bool canBuild);

	// returns a metal maker
	int GetMetalMaker(int side, float cost, float efficiency, float metal, float urgency, bool water, bool canBuild);

	// returns a storage
	int GetStorage(int side, float cost, float metal, float energy, float urgency, bool water, bool canBuild);

	// return repair pad
	int GetAirBase(int side, float cost, bool water, bool canBuild);

	// returns a ground unit according to the following criteria 
	int GetGroundAssault(int side, float gr_eff, float air_eff, float hover_eff, float sea_eff, float stat_eff, float speed, float range, float cost, int randomness, bool canBuild);
	
	int GetHoverAssault(int side, float gr_eff, float air_eff, float hover_eff, float sea_eff, float stat_eff, float speed, float range, float cost, int randomness, bool canBuild);
	
	// returns an air unit according to the following criteria 
	int GetAirAssault(int side, float gr_eff, float air_eff, float hover_eff, float sea_eff, float stat_eff, float speed, float range, float cost, int randomness, bool canBuild);

	int GetSeaAssault(int side, float gr_eff, float air_eff, float hover_eff, float sea_eff, float submarine_eff, float stat_eff, float speed, float range, float cost, int randomness, bool canBuild);

	int GetSubmarineAssault(int side, float sea_eff, float submarine_eff, float stat_eff, float speed, float range, float cost, int randomness, bool canBuild);

	// returns a random unit from the list
	int GetRandomUnit(list<int> unit_list);

	int GetRandomDefence(int side, UnitCategory category);

	int GetStationaryArty(int side, float cost, float range, float efficiency, bool water, bool canBuild);

	// returns a scout
	int GetScout(int side, float speed, float los, float cost, UnitCategory category, int randomness, bool canBuild);
	int GetScout(int side, float speed, float los, float cost, UnitCategory category1, UnitCategory category2, int randomness, bool canBuild);

	int GetRadar(int side, float cost, float range, bool water, bool canBuild);

	int GetJammer(int side, float cost, float range, bool water, bool canBuild);

	// returns a possible builder
	int GetBuilder(int unit_id);

	int GetBuilder(int unit_id, UnitMoveType moveType);

	// checks which factory is needed for a specific unit and orders it to be built
	void BuildFactoryFor(int unit_id);
	void BuildBuilderFor(int building_id);

	// tries to build another builder for a certain building
	void CheckAddBuilder(int building_id);
	
	// tries to build another builder for a certain building
	void AddAssitantBuilder(bool water, bool floater, bool canBuild);

	float GetFactoryRating(int id);
	float GetBuilderRating(int id);

	// updates unit table
	void UpdateTable(const UnitDef* def_killer, int killer, const UnitDef *def_killed, int killed);

	// updates max and average eff. values of the different categories
	void UpdateMinMaxAvgEfficiency(int side);

	// returns max range of all weapons
	float GetMaxRange(int unit_id);

	// returns max damage of all weapons
	float GetMaxDamage(int unit_id);

	// returns true, if unit is arty
	bool IsArty(int id);

	// returns true, if unit is a scout
	bool IsScout(int id);

	bool IsMissileLauncher(int def_id);

	bool IsDeflectionShieldEmitter(int def_id);

	// return a units eff. against a certain category
	float GetEfficiencyAgainst(int unit_def_id, UnitCategory category);

	// returns true if unit is starting unit
	bool IsStartingUnit(int def_id);
	bool IsBuilder(UnitCategory category);
	bool IsScout(UnitCategory category);

	// returns id of assault category
	int GetIDOfAssaultCategory(UnitCategory category);
	UnitCategory GetAssaultCategoryOfID(int id);
		
	// number of sides 
	int numOfSides;
	string *sideNames;

	bool initialized;

	//
	// these data are shared by several instances of aai
	// 

	// usefulness of unit category of side 
	static float ***mod_usefulness;

	// how many aai instances have been initialized
	static int aai_instances; 

	// all the unit defs
	static const UnitDef **unitList;
	
	// cached values of average costs and buildtime
	static float *avg_cost[SEA_BUILDER+1]; 
	static float *avg_buildtime[SEA_BUILDER+1];
	static float *avg_value[SEA_BUILDER+1];	// used for different things, range of weapons, radar range, mex efficiency
	static float *max_cost[SEA_BUILDER+1]; 
	static float *max_buildtime[SEA_BUILDER+1];
	static float *max_value[SEA_BUILDER+1];
	static float *min_cost[SEA_BUILDER+1]; 
	static float *min_buildtime[SEA_BUILDER+1];
	static float *min_value[SEA_BUILDER+1];

	static float **avg_speed;
	static float **min_speed;
	static float **max_speed;
	static float **group_speed;

	// units of the different categories
	static list<int> *units_of_category[SEA_BUILDER+1];

	// AAI unit defs (static things like id, side, etc.)
	static UnitTypeStatic *units_static;

	// storage for def. building selection
	static double **def_power;
	static double *max_pplant_eff;

	//
	//	non static variales
	//

	// AAI unit defs with aai-instance specific information (number of requested, active units, etc.)
	UnitTypeDynamic *units_dynamic;


	// number of unit definitions
	int numOfUnits;

	// start units of each side (e.g. commander)
	int *startUnits;

	// for internal use
	const char* GetCategoryString(int def_id);
	const char* GetCategoryString2(UnitCategory category);

	// all assault unit categories
	list<UnitCategory> assault_categories;

	// number of assault cat + arty & stat defences
	int combat_categories;

	// cached combat efficiencies
	float **avg_eff;
	float **max_eff;
	float **min_eff;
	float **total_eff;

	void SaveBuildTable();

private:	
	// for internal use
	void CalcBuildTree(int unit);
	bool LoadBuildTable();
	

	void DebugPrint();

	IAICallback *cb;
	AAI * ai;
		
	FILE *file;
};
