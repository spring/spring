#ifndef UNITHANDLER_H
#define UNITHANDLER_H
// UnitHandler.h: interface for the CUnitHandler class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

class CUnit;
#include <set>
#include <vector>
#include <list>
#include <stack>
#include <string>

struct UnitDef;
class CBuilderCAI;
class CFeature;
class CLoadSaveInterface;

const int MAX_UNITS=5000;

class CUnitHandler  
{
public:
	int CreateChecksum();
	void Update();
	void DeleteUnit(CUnit* unit);
	int AddUnit(CUnit* unit);
	CUnitHandler();
	virtual ~CUnitHandler();
	void PushNewWind(float x, float z, float strength);

	//return values for the following is
	//0 blocked
	//1 mobile unit in the way
	//2 free (or if feature is != 0 then with a blocking feature that can be reclaimed)
	int  TestUnitBuildSquare(const float3& pos, const UnitDef *unitdef,CFeature *&feature);	//test if a unit can be built at specified position
	int  TestUnitBuildSquare(const float3& pos, std::string unit,CFeature *&feature);	//test if a unit can be built at specified position
	int  ShowUnitBuildSquare(const float3& pos, const UnitDef *unitdef);	//test if a unit can be built at specified position and show on the ground where it's to rough
	int  TestBuildSquare(const float3& pos, const UnitDef *unitdef,CFeature *&feature);	//test a singel mapsquare for build possibility

	void AddBuilderCAI(CBuilderCAI*);
	void RemoveBuilderCAI(CBuilderCAI*);
	float GetBuildHeight(float3 pos, const UnitDef* unitdef);

	void LoadSaveUnits(CLoadSaveInterface* file, bool loading);

	std::list<CUnit*> activeUnits;				//used to get all active units
	std::deque<int> freeIDs;
	CUnit* units[MAX_UNITS];							//used to get units from IDs (0 if not created)
	int overrideId;

	std::stack<CUnit*> toBeRemoved;			//units that will be removed at start of next update

	std::list<CUnit*>::iterator slowUpdateIterator;

	std::set<CBuilderCAI*> builderCAIs;

	float waterDamage;

	int maxUnits;			//max units per team

	int lastDamageWarning;
	int lastCmdDamageWarning;

	bool CanCloseYard(CUnit* unit);

	bool limitDgun;
	float dgunRadius;

	bool diminishingMetalMakers;
	float metalMakerIncome;
	float metalMakerEfficiency;
};

extern CUnitHandler* uh;

#endif /* UNITHANDLER_H */
