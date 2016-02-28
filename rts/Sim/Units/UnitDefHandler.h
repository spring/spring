/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITDEFHANDLER_H
#define UNITDEFHANDLER_H

#include <string>
#include <map>
#include <set>
#include <vector>

#include "Sim/Misc/CommonDefHandler.h"

class LuaTable;
struct UnitDef;
struct GuiSoundSet;

class LuaParser;
class CUnitDefHandler : CommonDefHandler
{
public:
	CUnitDefHandler(LuaParser* defsParser);
	~CUnitDefHandler();

	void Init();
	void ProcessDecoys();
	void AssignTechLevels();

	bool ToggleNoCost();
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

	std::map<int, std::set<int> > decoyMap;
	std::set<int> startUnitIDs;

//protected: //FIXME UnitDef::*ExplGens,buildingDecalType,trackType are initialized in UnitDrawer.cpp
	std::vector<UnitDef> unitDefs;
	std::map<std::string, int> unitDefIDsByName;

protected:
	void UnitDefLoadSounds(UnitDef*, const LuaTable&);
	void LoadSounds(const LuaTable&, GuiSoundSet&, const std::string& soundName);
	void LoadSound(GuiSoundSet&, const std::string& fileName, const float volume);

	void CleanBuildOptions();

	void FindStartUnits();

	void AssignTechLevel(UnitDef& ud, int level);

private:
	std::map<std::string, std::string> decoyNameMap;

	bool noCost;
};

extern CUnitDefHandler* unitDefHandler;

#endif /* UNITDEFHANDLER_H */
