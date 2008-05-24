#ifndef UNITDEFHANDLER_H
#define UNITDEFHANDLER_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include "Sim/MoveTypes/MoveInfo.h"
#include "TdfParser.h"
#include "UnitDef.h"

class CUnitDefHandler;
struct WeaponDef;
class LuaTable;


#define TA_UNIT     1
#define SPRING_UNIT 2


//this class takes care of all the unit definitions
class CUnitDefHandler
{
public:


	UnitDef *unitDefs;
	int numUnitDefs;
	std::map<std::string, int> unitID;
	std::map<int, std::set<int> > decoyMap;
	std::set<int> startUnitIDs;

	CUnitDefHandler(void);
	~CUnitDefHandler(void);
	void Init();
	void ProcessDecoys();
	void AssignTechLevels();
	const UnitDef* GetUnitByName(std::string name);
	const UnitDef* GetUnitByID(int id);
	unsigned int GetUnitImage(const UnitDef* unitdef, std::string buildPicName = "");
	bool SaveTechLevels(const std::string& filename, const std::string& modname);

	bool noCost;

protected:
	void ParseUnit(const LuaTable&, const std::string& name, int id);
	void ParseTAUnit(const LuaTable&, const std::string& name, int id);

	void LoadSounds(const LuaTable&, GuiSoundSet&, const std::string& soundName);
	void LoadSound(GuiSoundSet&, const std::string& fileName, float volume);

	void CleanBuildOptions();

	void FindStartUnits();

	void AssignTechLevel(UnitDef& ud, int level);

public:
//	void CreateBlockingLevels(UnitDef *def,std::string yardmap);

private:
	void CreateYardMap(UnitDef *def, std::string yardmap);
	std::map<std::string, std::string> decoyNameMap;
};

extern CUnitDefHandler* unitDefHandler;

#endif /* UNITDEFHANDLER_H */
