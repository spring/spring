#ifndef UNITTABLE_H
#define UNITTABLE_H
/*pragma once removed*/
#include "GlobalAI.h"


class CUnitTable
{
public: 
	CUnitTable(AIClasses* ai);
	virtual ~CUnitTable();

	// Initialize all unit lists, categories etc
	void Init();
	
	// Temp stuff:
	void Init_nr2();
	const UnitDef* GetBestEconomyBuilding(int builder, float minUsefullnes);
	// Temp end


	// returns true, if a builder can build a certain unit (use UnitDef.id)
	bool CanBuildUnit(int id_builder, int id_unit);

	// returns side of a unit
	int GetSide(int unit);

	//Gets the average Damage Per second a unit can cause (provided all weapons are in range)
	float GetDPS(const UnitDef* unit);

	//Finds the actual dps versus a specific enemy unit
	float GetDPSvsUnit(const UnitDef* unit,const UnitDef* victim);

	//checks the combat potential of this unit vs all active enemy units
	float GetCurrentDamageScore(const UnitDef* unit);

	void UpdateChokePointArray();

	//Gets the category for a particular unit
	int GetCategory (const UnitDef* unitdef);
	int GetCategory (int unit);

	//returns the ID of the best possible Unit of a given category
	const UnitDef* GetUnitByScore(int builder, int category);

	//Finds the general score of any given unit
	float GetScore(const UnitDef* unit);

	// returns max range of all weapons (or 0)
	float GetMaxRange(const UnitDef* unit);
	// returns min range for all weapons (or FLT_MAX)
	float GetMinRange(const UnitDef* unit);

	vector<vector<int>*>	all_lists;
	vector<int> *ground_factories;
	vector<int> *ground_builders;
	vector<int> *ground_attackers;
	vector<int> *metal_extractors;
	vector<int> *metal_makers;
	vector<int> *ground_energy;
	vector<int> *ground_defences;
	vector<int> *metal_storages;
	vector<int> *energy_storages;


	// number of sides 
	int numOfSides;
	vector<string> sideNames;

	// all the unit defs
	const UnitDef **unitList;
	UnitType *unittypearray;
	int numOfUnits;

private:
	// for internal use
	void CalcBuildTree(int unit);
	void DebugPrint();

	// number of unit definitions
	

	// start units of each side (e.g. commander)
	vector<int> startUnits;
	FILE *file;
	AIClasses* ai;
};




#endif /* UNITTABLE_H */
