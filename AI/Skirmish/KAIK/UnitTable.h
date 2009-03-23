#ifndef UNITTABLE_H
#define UNITTABLE_H

#include <string>
#include <vector>

#include "IncCREG.h"

struct UnitDef;
struct AIClasses;
struct UnitType;

class CUnitTable {
	public:
		CR_DECLARE(CUnitTable);

		CUnitTable() {}
		CUnitTable(AIClasses* ai);
		~CUnitTable();

		void PostLoad();
		// initialize all unit lists, categories etc
		void Init();

		// not implemented
		const UnitDef* GetBestEconomyBuilding(int builder, float minUsefulness);


		// true if a builder can build a certain unit (use UnitDef.id)
		bool CanBuildUnit(int id_builder, int id_unit);

		int BuildModSideMap();
		int ReadTeamSides();
		void ReadModConfig();

		// returns side we (the AI) are on
		int GetSide(void) const;
		// returns side a given unit is on
		int GetSide(int unit) const;

		// gets the average Damage Per second a unit can cause (provided all weapons are in range)
		float GetDPS(const UnitDef* unit);
		// finds the actual DPS versus a specific enemy unit
		float GetDPSvsUnit(const UnitDef* unit, const UnitDef* victim);
		// checks the combat potential of this unit vs all active enemy units
		float GetCurrentDamageScore(const UnitDef* unit);

		void UpdateChokePointArray();

		// gets the category for a particular unit
		int GetCategory(const UnitDef* unitdef) const;
		int GetCategory(int unit) const;

		// returns the ID of the best possible Unit of a given category
		const UnitDef* GetUnitByScore(int builder, int category);

		// finds the general score of any given unit
		float GetScore(const UnitDef* unit, int category);

		// returns max range of all weapons (or 0)
		float GetMaxRange(const UnitDef* unit);
		// returns min range for all weapons (or FLT_MAX)
		float GetMinRange(const UnitDef* unit);

		// std::vector<std::vector<int> std::vector<int> > all_lists;
		std::vector<   std::vector<std::vector<int> >   > all_lists;

		std::vector<std::vector<int> > ground_factories;
		std::vector<std::vector<int> > ground_builders;
		std::vector<std::vector<int> > ground_attackers;
		std::vector<std::vector<int> > metal_extractors;
		std::vector<std::vector<int> > metal_makers;
		std::vector<std::vector<int> > ground_energy;
		std::vector<std::vector<int> > ground_defences;
		std::vector<std::vector<int> > metal_storages;
		std::vector<std::vector<int> > energy_storages;
		std::vector<std::vector<int> > nuke_silos;

		int numOfSides;							// the number of sides (races) the mod offers
		std::vector<std::string> sideNames;		// side number (0) to side string ("Arm")
		std::map<std::string, int> modSideMap;	// side string ("Arm") to side number (0)
		std::vector<int> teamSides;				// team numbers to side numbers

		std::vector<const UnitDef*> unitDefs;
		std::vector<UnitType> unitTypes;
		int numDefs;

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
