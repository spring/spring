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
	
	// Make a list of all the units that mach the given categoryMask
	list<UnitType*> getGlobalUnitList(unsigned categoryMask, int side = -1);

	void UpdateChokePointArray();

	//Gets the category for a particular unit
	int GetCategory (const UnitDef* unitdef);
	int GetCategory (int unit);
	
	//Gets the UnitType for a particular unit
	UnitType* GetUnitType(const UnitDef* unitdef);
	//Gets the UnitType for a particular unit
	UnitType* GetUnitType(int unit);

	//returns the UnitDef of the best possible Unit of a given category
	const UnitDef* GetUnitByScore(int builder, int category);
	
	//returns the UnitDef of the best possible Unit of a given categoryMask
	const UnitDef* GetUnitByScore(int builder, list<UnitType*> &unitList, unsigned needMask);
	
	/**  TODO!!!! -  the global normalization data is not made yet
	An exprimental function only, might be used for factories first
	All the extra data will be ignored for now, but a unit will get score from all categories that is listed in needMask.
	If needMask is 0 then the units own categoryMask is used.
	(A score of 0 will mean that the unit is too costly or useless)
	*/
	float GetFullScore(const UnitDef* unit, unsigned needMask = 0, float buildSpeedEstimate = 100, int maxTime = 120, float budgetE = -1, float budgetM = -1);


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
	
	float GetNonNormalizationGlobalScore(const UnitType* unitType, unsigned needMask);
	void MakeGlobalNormalizationScore();
	
	// a comperator function for MakeGlobalNormalizationScore
	bool static pairCMP(const std::pair<float,UnitType*> &a, const std::pair<float,UnitType*> &b);


	/**
	The new category lists:   *** NOT USED YET ***
	There are 32 of them, one for each bit index.
	An unit can be in more than one list at the same time.
	The lists are sorted on the normalization score.
	Normalization score is a score that tells how good a unit is in each category (it ignores economics and situation).
	(For a radar the range is the only factor in the score, and so on...)
	In order to use this data you must first find the best unit that can be built in a category
	Then the extra score of a unit will be 'bestUnitScoreInCat/testUnitScoreInCat * currentCatNeed' = a number in range (0...1) * currentCatNeed
	If an bitIndex is empty then there are no usefull normalization score for that category
	
	Usage: 
		float score = scoreLists[bitIndex][i].first
		const UnitDef *def = scoreLists[bitIndex][i].second
	*/
	vector<vector<pair<float,UnitType*> > > scoreLists;

	int updateChokePointArrayTime;

	// start units of each side (e.g. commander)
	vector<int> startUnits;
	FILE *file;
	AIClasses* ai;
};




#endif /* UNITTABLE_H */
