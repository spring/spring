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

#include "UnitDef.h"
#include "Game/command.h"

class CBuilderCAI;
class CFeature;
class CLoadSaveInterface;


struct CChecksum {
	CChecksum() : x(0), y(0), z(0), m(0), e(0) {}
	int toInt() const                          { return x ^ y ^ z ^ m ^ e; }
	bool operator<(const CChecksum& c) const   { return toInt() < c.toInt(); }
	operator bool() const                      { return toInt() != 0; }
	bool operator==(const CChecksum& c) const  { return x == c.x && y == c.y && z == c.z && m == c.m && e == c.e; }
	bool operator!=(const CChecksum& c) const  { return !(*this == c); }
	CChecksum& operator=(int a)                { x = y = z = m = e = a; return *this; }
	char* diff(char* buf, const CChecksum& c);
	int x, y, z, m, e; // midPos.x, midPos.y, midPos.z, metal, energy
};


class CUnitHandler  
{
public:
	CChecksum CreateChecksum();
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
	int  TestUnitBuildSquare(const BuildInfo& buildInfo,CFeature *&feature);	//test if a unit can be built at specified position
	int  ShowUnitBuildSquare(const BuildInfo& buildInfo);	//test if a unit can be built at specified position and show on the ground where it's to rough
	int  ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command> &cv);
	int  TestBuildSquare(const float3& pos, const UnitDef *unitdef,CFeature *&feature);	//test a single mapsquare for build possibility

	void AddBuilderCAI(CBuilderCAI*);
	void RemoveBuilderCAI(CBuilderCAI*);
	float GetBuildHeight(float3 pos, const UnitDef* unitdef);

	void LoadSaveUnits(CLoadSaveInterface* file, bool loading);
	Command GetBuildCommand(float3 pos, float3 dir);

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
