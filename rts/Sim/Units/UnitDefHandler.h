/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITDEFHANDLER_H
#define UNITDEFHANDLER_H

#include <string>
#include <map>
#include <set>

#include "Sim/Misc/CommonDefHandler.h"

class LuaTable;
struct UnitDef;
struct GuiSoundSet;

/// Takes care of all the unit definitions
class CUnitDefHandler : CommonDefHandler
{
public:
	UnitDef* unitDefs;
	int numUnitDefs;
	std::map<std::string, int> unitDefIDsByName;
	std::map<int, std::set<int> > decoyMap;
	std::set<int> startUnitIDs;

	CUnitDefHandler(void);
	~CUnitDefHandler(void);
	void Init();
	void ProcessDecoys();
	void AssignTechLevels();

	bool ToggleNoCost();
	const UnitDef* GetUnitDefByName(std::string name);
	const UnitDef* GetUnitDefByID(int id);

	unsigned int GetUnitDefImage(const UnitDef* unitDef);
	void SetUnitDefImage(const UnitDef* unitDef,
	                     const std::string& texName);
	void SetUnitDefImage(const UnitDef* unitDef,
	                     unsigned int texID, int sizex, int sizey);

protected:
	void ParseUnitDef(const LuaTable&, const std::string& name, int id);
	void ParseUnitDefTable(const LuaTable&, const std::string& name, int id);

	void LoadSounds(const LuaTable&, GuiSoundSet&, const std::string& soundName);
	void LoadSound(GuiSoundSet&, const std::string& fileName, const float volume);

	void CleanBuildOptions();

	void FindStartUnits();

	void AssignTechLevel(UnitDef& ud, int level);

private:
	void CreateYardMap(UnitDef *def, std::string yardmap);
	std::map<std::string, std::string> decoyNameMap;

	bool noCost;
};

extern CUnitDefHandler* unitDefHandler;

#endif /* UNITDEFHANDLER_H */
