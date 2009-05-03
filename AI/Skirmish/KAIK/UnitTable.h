#ifndef KAIK_UNITTABLE_HDR
#define KAIK_UNITTABLE_HDR

#include <string>
#include <vector>

#include "Defines.h"

struct UnitDef;
struct AIClasses;
struct UnitType;
struct MoveData;

struct CategoryData {
	CR_DECLARE_STRUCT(CategoryData);

	bool CanBuild(UnitDefCategory c) const {
		switch (c) {
			case CAT_GROUND_FACTORY: { return (!groundFactories.empty()); } break;
			case CAT_GROUND_BUILDER: { return (!groundBuilders.empty()); } break;
			case CAT_GROUND_ATTACKER: { return (!groundAttackers.empty()); } break;
			case CAT_METAL_EXTRACTOR: { return (!metalExtractors.empty()); } break;
			case CAT_METAL_MAKER: { return (!metalMakers.empty()); } break;
			case CAT_METAL_STORAGE: { return (!metalStorages.empty()); } break;
			case CAT_ENERGY_STORAGE: { return (!energyStorages.empty()); } break;
			case CAT_GROUND_ENERGY: { return (!groundEnergy.empty()); } break;
			case CAT_GROUND_DEFENSE: { return (!groundDefenses.empty()); } break;
			case CAT_NUKE_SILO: { return (!nukeSilos.empty()); } break;
			default: { return false; } break;
		}
	}

	// maps UnitCategory to UnitDefCategory
	std::vector<int>& GetDefsForUnitCat(UnitCategory c) {
		switch (c) {
			case CAT_COMM: { return groundFactories; } break; // ??
			case CAT_ENERGY: { return groundEnergy; } break;
			case CAT_MEX: { return metalExtractors; } break;
			case CAT_MMAKER: { return metalMakers; } break;
			case CAT_BUILDER: { return groundBuilders; } break;
			case CAT_ESTOR: { return energyStorages; } break;
			case CAT_MSTOR: { return metalStorages; } break;
			case CAT_FACTORY: { return groundFactories; } break;
			case CAT_DEFENCE: { return groundDefenses; } break;
			case CAT_G_ATTACK: { return groundAttackers; } break;
			case CAT_NUKE: { return nukeSilos; } break;
			default: { return dummy; } break;
		}
	}

	std::vector<int>& GetDefsForUnitDefCat(UnitDefCategory c) {
		switch (c) {
			case CAT_GROUND_FACTORY: { return groundFactories; } break;
			case CAT_GROUND_BUILDER: { return groundBuilders; } break;
			case CAT_GROUND_ATTACKER: { return groundAttackers; } break;
			case CAT_METAL_EXTRACTOR: { return metalExtractors; } break;
			case CAT_METAL_MAKER: { return metalMakers; } break;
			case CAT_METAL_STORAGE: { return metalStorages; } break;
			case CAT_ENERGY_STORAGE: { return energyStorages; } break;
			case CAT_GROUND_ENERGY: { return groundEnergy; } break;
			case CAT_GROUND_DEFENSE: { return groundDefenses; } break;
			case CAT_NUKE_SILO: { return nukeSilos; } break;
			default: { return dummy; } break;
		}
	}

	std::vector<int> groundFactories;
	std::vector<int> groundBuilders;
	std::vector<int> groundAttackers;
	std::vector<int> metalExtractors;
	std::vector<int> metalMakers;
	std::vector<int> metalStorages;
	std::vector<int> energyStorages;
	std::vector<int> groundEnergy;
	std::vector<int> groundDefenses;
	std::vector<int> nukeSilos;
	std::vector<int> dummy;
};

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

		// gets the average Damage Per second a unit can cause (provided all weapons are in range)
		float GetDPS(const UnitDef* unit);
		// finds the actual DPS versus a specific enemy unit
		float GetDPSvsUnit(const UnitDef* unit, const UnitDef* victim);
		// checks the combat potential of this unit vs all active enemy units
		float GetCurrentDamageScore(const UnitDef* unit);

		void UpdateChokePointArray();

		// gets the category for a particular unit
		UnitCategory GetCategory(const UnitDef*) const;
		UnitCategory GetCategory(int) const;

		// returns the ID of the best possible Unit of a given category
		const UnitDef* GetUnitByScore(int, UnitCategory);

		// finds the general score of any given unit
		float GetScore(const UnitDef*, UnitCategory);

		// returns max range of all weapons (or 0)
		float GetMaxRange(const UnitDef*);
		// returns min range for all weapons (or FLT_MAX)
		float GetMinRange(const UnitDef*);

		CategoryData categoryData;
		std::map<int, MoveData*> moveDefs;

		std::vector<const UnitDef*> unitDefs;
		std::vector<UnitType> unitTypes;
		int numDefs;

		// KLOOTNOTE: unused for now
		int minTechLevel;
		int maxTechLevel;

	private:
		/// int BuildModSideMap();
		/// int ReadTeamSides();
		void ReadModConfig();
		void DebugPrint();

		/// int GetSide(void) const;
		/// int GetSide(int) const;
		/// int GetSide(const UnitDef*) const;

		std::string GetDbgLogName() const;
		std::string GetModCfgName() const;

		/// start units of each side (e.g. commander)
		/// std::vector<int> startUnits;

		/// std::vector<std::string> sideNames;		// side number (0) to side string ("Arm")
		/// std::map<std::string, int> modSideMap;	// side string ("Arm") to side number (0)
		/// std::vector<int> teamSides;				// team numbers to side numbers

		AIClasses* ai;
};

#endif
