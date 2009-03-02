#ifndef UNITHANDLER_H
#define UNITHANDLER_H
// UnitHandler.h: interface for the CUnitHandler class.
//
//////////////////////////////////////////////////////////////////////

class CUnit;
#include <set>
#include <vector>
#include <list>
#include <stack>
#include <string>

#include "UnitDef.h"
#include "UnitSet.h"
#include "CommandAI/Command.h"

class CBuilderCAI;
class CFeature;
class CLoadSaveInterface;

class CUnitHandler
{
public:
	CR_DECLARE(CUnitHandler)

	void Update();
	void DeleteUnit(CUnit* unit);
	void DeleteUnitNow(CUnit* unit);
	int AddUnit(CUnit* unit);
	CUnitHandler(bool serializing=false);
	void Serialize(creg::ISerializer& s);
	void PostLoad();
	virtual ~CUnitHandler();
	void UpdateWind(float x, float z, float strength);

	//return values for the following is
	//0 blocked
	//1 mobile unit in the way
	//2 free (or if feature is != 0 then with a blocking feature that can be reclaimed)
	int  TestUnitBuildSquare(const BuildInfo& buildInfo,CFeature *&feature, int allyteam); //test if a unit can be built at specified position
	int  ShowUnitBuildSquare(const BuildInfo& buildInfo);	//test if a unit can be built at specified position and show on the ground where it's to rough
	int  ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command> &cv);
	int  TestBuildSquare(const float3& pos, const UnitDef *unitdef,CFeature *&feature, int allyteam); //test a single mapsquare for build possibility

	//return true if a unit of type unitID can be build, false otherwise
	bool CanBuildUnit(const UnitDef* unitdef, int team);

	void AddBuilderCAI(CBuilderCAI*);
	void RemoveBuilderCAI(CBuilderCAI*);
	float GetBuildHeight(float3 pos, const UnitDef* unitdef);

	void LoadSaveUnits(CLoadSaveInterface* file, bool loading);
	Command GetBuildCommand(float3 pos, float3 dir);

	std::vector< vector<CUnitSet> > unitsByDefs; // units sorted by team and unitDef

	std::list<CUnit*> activeUnits;				//used to get all active units
	std::vector<int> freeIDs;
	CUnit* units[MAX_UNITS];							//used to get units from IDs (0 if not created)

	std::vector<CUnit*> toBeRemoved;			//units that will be removed at start of next update

	std::set<CUnit*> toBeAdded;			//rendering units that will be added at start of next draw
	std::list<CUnit*> renderUnits;				//units being rendered

	std::list<CUnit*>::iterator slowUpdateIterator;

	std::list<CBuilderCAI*> builderCAIs;

	float waterDamage;

	int maxUnits;			//max units per team

	float maxUnitRadius; // largest radius seen so far

	int lastDamageWarning;
	int lastCmdDamageWarning;

	bool limitDgun;
	float dgunRadius;

	bool diminishingMetalMakers;
	float metalMakerIncome;
	float metalMakerEfficiency;

	bool morphUnitToFeature;
};

extern CUnitHandler* uh;

#endif /* UNITHANDLER_H */
