/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITHANDLER_H
#define UNITHANDLER_H

#include <set>
#include <vector>
#include <list>
#include <stack>
#include <string>

#include "UnitDef.h"
#include "UnitSet.h"
#include "CommandAI/Command.h"

class CUnit;
class CBuilderCAI;
class CFeature;
class CLoadSaveInterface;

class CUnitHandler
{
	CR_DECLARE(CUnitHandler)

public:
	void Update();
	void DeleteUnit(CUnit* unit);
	void DeleteUnitNow(CUnit* unit);
	int AddUnit(CUnit* unit);
	CUnitHandler(bool serializing=false);
	void Serialize(creg::ISerializer& s);
	void PostLoad();
	virtual ~CUnitHandler();

	///< test if a unit can be built at specified position
	///<   return values for the following is
	///<   0 blocked
	///<   1 mobile unit in the way
	///<   2 free (or if feature is != 0 then with a blocking feature that can be reclaimed)
	int TestUnitBuildSquare(
		const BuildInfo&,
		CFeature*&,
		int,
		std::vector<float3>* canbuildpos = NULL,
		std::vector<float3>* featurepos = NULL,
		std::vector<float3>* nobuildpos = NULL,
		const std::vector<Command>* commands = NULL
	);
	///< test a single mapsquare for build possibility
	int TestBuildSquare(const float3& pos, const UnitDef *unitdef,CFeature *&feature, int allyteam);

	/// Returns true if a unit of type unitID can be built, false otherwise
	bool CanBuildUnit(const UnitDef* unitdef, int team);

	void AddBuilderCAI(CBuilderCAI*);
	void RemoveBuilderCAI(CBuilderCAI*);
	float GetBuildHeight(float3 pos, const UnitDef* unitdef);

	void LoadSaveUnits(CLoadSaveInterface* file, bool loading);
	Command GetBuildCommand(float3 pos, float3 dir);

	int MaxUnitsPerTeam() const
	{
		return unitsPerTeam;
	};

	size_t MaxUnits() const
	{
		return units.size();
	};


	// note: negative ID's are implicitly converted
	CUnit* GetUnitUnsafe(unsigned int unitID) const { return units[unitID]; }
	CUnit* GetUnit(unsigned int unitID) const { return (unitID < MaxUnits()? units[unitID]: NULL); }


	std::vector< std::vector<CUnitSet> > unitsByDefs; ///< units sorted by team and unitDef

	std::list<CUnit*> activeUnits;                    ///< used to get all active units
	std::vector<int> freeIDs;
	std::vector<CUnit*> units;                        ///< used to get units from IDs (0 if not created)

	std::vector<CUnit*> toBeRemoved;                  ///< units that will be removed at start of next update

	std::list<CUnit*>::iterator slowUpdateIterator;

	std::list<CBuilderCAI*> builderCAIs;

	float waterDamage;

	float maxUnitRadius; ///< largest radius seen so far

	int lastDamageWarning;
	int lastCmdDamageWarning;

	bool limitDgun;
	float dgunRadius;

	bool morphUnitToFeature;

private:
	int unitsPerTeam;
};

extern CUnitHandler* uh;

#endif /* UNITHANDLER_H */
