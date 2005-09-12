#ifndef UNITDEFHANDLER_H
#define UNITDEFHANDLER_H

#include <string>
#include <vector>
#include <map>
#include "MoveInfo.h"
#include "SunParser.h"
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
	int numUnits;
	

	CUnitDefHandler(void);
	~CUnitDefHandler(void);
	void Init();
	UnitDef *GetUnitByName(std::string name);
	UnitDef *GetUnitByID(int id);

	bool noCost;
protected:
	CSunParser soundcategory;

	void ParseUnit(std::string file, int id);

	void ParseTAUnit(std::string file, int id);

	void FindTABuildOpt();
	void LoadSound(CSunParser &sunparser, GuiSound &gsound, std::string sunname);

public:
//	void CreateBlockingLevels(UnitDef *def,std::string yardmap);

private:
	void CreateYardMap(UnitDef *def, std::string yardmap);
};

extern CUnitDefHandler* unitDefHandler;

#endif /* UNITDEFHANDLER_H */
