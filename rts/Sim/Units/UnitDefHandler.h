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

#define TA_UNIT 1
#define SPRING_UNIT 2



//this class takes care of all the unit defenitions
class CUnitDefHandler
{
public:


	UnitDef *unitDefs;
	std::map<std::string, int> unitID;
	std::set<int> commanderIDs;
	int numUnits;
	

	CUnitDefHandler(void);
	~CUnitDefHandler(void);
	void Init();
	void AssignTechLevels();
	UnitDef *GetUnitByName(std::string name);
	UnitDef *GetUnitByID(int id);
	unsigned int GetUnitImage(UnitDef *unitdef);
	bool SaveTechLevels(const std::string& filename, const std::string& modname);

	bool noCost;

protected:
	TdfParser soundcategory;
	
	void ParseUnit(std::string file, int id);

	void ParseTAUnit(std::string file, int id);

	void FindTABuildOpt();

	void AssignTechLevel(UnitDef& ud, int level);
	
	void LoadSound(TdfParser &sunparser, GuiSound &gsound, std::string sunname);

public:
//	void CreateBlockingLevels(UnitDef *def,std::string yardmap);

private:
	void CreateYardMap(UnitDef *def, std::string yardmap);
};

extern CUnitDefHandler* unitDefHandler;

#endif /* UNITDEFHANDLER_H */
