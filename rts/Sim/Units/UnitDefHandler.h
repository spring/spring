/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITDEFHANDLER_H
#define UNITDEFHANDLER_H

#include <string>
#include <vector>

#include "Sim/Misc/CommonDefHandler.h"
#include "System/UnorderedMap.hpp"
#include "System/UnorderedSet.hpp"

class LuaTable;
struct UnitDef;
struct GuiSoundSet;

class LuaParser;
class CUnitDefHandler : CommonDefHandler
{
public:
	CUnitDefHandler(LuaParser* defsParser);

	void Init();
	void ProcessDecoys();
	void AssignTechLevels();

	bool ToggleNoCost();

	// NOTE: safe with unordered_map after ctor
	const UnitDef* GetUnitDefByName(std::string name);
	const UnitDef* GetUnitDefByID(int id);

	bool IsValidUnitDefID(const int id) const {
		/// zero is not valid!
		return (id > 0) && (id < (int)unitDefs.size());
	}

	unsigned int GetUnitDefImage(const UnitDef* unitDef);
	void SetUnitDefImage(const UnitDef* unitDef,
	                     const std::string& texName);
	void SetUnitDefImage(const UnitDef* unitDef,
	                     unsigned int texID, int sizex, int sizey);

	int PushNewUnitDef(const std::string& unitName, const LuaTable& udTable);

	spring::unordered_map<int, spring::unordered_set<int> > decoyMap;
	spring::unordered_set<int> startUnitIDs;

//protected: //FIXME UnitDef::*ExplGens,buildingDecalType,trackType are initialized in UnitDrawer.cpp
	std::vector<UnitDef> unitDefs;
	spring::unordered_map<std::string, int> unitDefIDsByName;

protected:
	void UnitDefLoadSounds(UnitDef*, const LuaTable&);
	void LoadSounds(const LuaTable&, GuiSoundSet&, const std::string& soundName);
	void LoadSound(GuiSoundSet&, const std::string& fileName, const float volume);

	void CleanBuildOptions();

	void FindStartUnits();

	void AssignTechLevel(UnitDef& ud, int level);

private:
	spring::unordered_map<std::string, std::string> decoyNameMap;

	bool noCost;
};

extern CUnitDefHandler* unitDefHandler;

#endif /* UNITDEFHANDLER_H */
