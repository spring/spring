#ifndef UNITTABLE_H
#define UNITTABLE_H


#include "GlobalAI.h"

using std::vector;

class CUnitTable {
	public:
		/// CR_DECLARE(CUnitTable);

		CUnitTable(AIClasses* ai);
		~CUnitTable();

		// initialize all unit lists, categories etc
		void Init();

		// not implemented
		const UnitDef* GetBestEconomyBuilding(int builder, float minUsefulness);


		// true if a builder can build a certain unit (use UnitDef.id)
		bool CanBuildUnit(int id_builder, int id_unit);

		int BuildModSideMap();
		int ReadTeamSides();
		void ReadModConfig();

		// returns side of a unit
		int GetSide(int unit);
		// gets the average Damage Per second a unit can cause (provided all weapons are in range)
		float GetDPS(const UnitDef* unit);
		// finds the actual DPS versus a specific enemy unit
		float GetDPSvsUnit(const UnitDef* unit, const UnitDef* victim);
		// checks the combat potential of this unit vs all active enemy units
		float GetCurrentDamageScore(const UnitDef* unit);

		void UpdateChokePointArray();

		// gets the category for a particular unit
		int GetCategory(const UnitDef* unitdef);
		int GetCategory(int unit);

		// returns the ID of the best possible Unit of a given category
		const UnitDef* GetUnitByScore(int builder, int category);

		// finds the general score of any given unit
		float GetScore(const UnitDef* unit, int category);

		// returns max range of all weapons (or 0)
		float GetMaxRange(const UnitDef* unit);
		// returns min range for all weapons (or FLT_MAX)
		float GetMinRange(const UnitDef* unit);

		vector<vector<int>*> all_lists;
		vector<int>* ground_factories;
		vector<int>* ground_builders;
		vector<int>* ground_attackers;
		vector<int>* metal_extractors;
		vector<int>* metal_makers;
		vector<int>* ground_energy;
		vector<int>* ground_defences;
		vector<int>* metal_storages;
		vector<int>* energy_storages;
		vector<int>* nuke_silos;

		// number of sides
		int numOfSides;
		std::vector<string> sideNames;			// side number (0) to side string ("Arm")
		std::map<string, int> modSideMap;		// side string ("Arm") to side number (0)
		std::vector<int> teamSides;				// team numbers to side numbers

		// all the unit defs
		const UnitDef** unitList;
		UnitType* unitTypes;
		int numOfUnits;

		// KLOOTNOTE: unused for now
		int minTechLevel;
		int maxTechLevel;

	private:
		// for internal use
		void CalcBuildTree(int unit, int rootSide);
		void DebugPrint();

		// start units of each side (e.g. commander)
		std::vector<int> startUnits;

		FILE* file;
		AIClasses* ai;
};


#endif
