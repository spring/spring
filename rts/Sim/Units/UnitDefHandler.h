/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITDEFHANDLER_H
#define UNITDEFHANDLER_H

#include <string>
#include <vector>

#include "UnitDef.h"
#include "Sim/Misc/CommonDefHandler.h"
#include "System/UnorderedMap.hpp"

class LuaTable;
struct UnitDef;
struct GuiSoundSet;

class LuaParser;
class CUnitDefHandler : CommonDefHandler
{
public:
	void Init(LuaParser* defsParser);
	void Kill() {
		unitDefsVector.clear();
		unitDefIDs.clear(); // never iterated in synced code

		// reuse inner vectors when reloading; keys are never iterated
		// decoyMap.clear();

		for (auto& pair: decoyMap) {
			pair.second.clear();
		}

		decoyNameMap.clear();
	}

	bool GetNoCost() { return noCost; }
	void SetNoCost(bool value);

	// NOTE: safe with unordered_map after Init
	const UnitDef* GetUnitDefByName(std::string name);
	const UnitDef* GetUnitDefByID(int id) {
		if (!IsValidUnitDefID(id))
			return nullptr;

		return &unitDefsVector[id];
	}

	bool IsValidUnitDefID(const int id) const {
		return (id > 0) && (static_cast<size_t>(id) < unitDefsVector.size());
	}

	// id=0 is not a valid UnitDef, hence the -1
	unsigned int NumUnitDefs() const { return (unitDefsVector.size() - 1); }

	int PushNewUnitDef(const std::string& unitName, const LuaTable& udTable);

	const std::vector<UnitDef>& GetUnitDefsVec() const { return unitDefsVector; }
	const spring::unordered_map<std::string, int>& GetUnitDefIDs() const { return unitDefIDs; }
	const spring::unordered_map<int, std::vector<int> >& GetDecoyDefIDs() const { return decoyMap; }

protected:
	void UnitDefLoadSounds(UnitDef*, const LuaTable&);
	void LoadSounds(const LuaTable&, GuiSoundSet&, const std::string& soundName);

	void CleanBuildOptions();
	void ProcessDecoys();

private:
	std::vector<UnitDef> unitDefsVector;
	spring::unordered_map<std::string, int> unitDefIDs;
	spring::unordered_map<int, std::vector<int> > decoyMap;
	std::vector< std::pair<std::string, std::string> > decoyNameMap;

	bool noCost = false;
};

extern CUnitDefHandler* unitDefHandler;

#endif /* UNITDEFHANDLER_H */
