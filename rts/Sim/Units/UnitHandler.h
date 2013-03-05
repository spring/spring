/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITHANDLER_H
#define UNITHANDLER_H

#include <vector>

#include "UnitDef.h"
#include "UnitSet.h"
#include "CommandAI/Command.h"
#include "System/creg/STL_Map.h"
#include "System/creg/STL_List.h"

class CUnit;
class CBuilderCAI;
class CFeature;
class CLoadSaveInterface;
struct BuildInfo;


enum BuildSquareStatus {
	BUILDSQUARE_BLOCKED     = 0,
	BUILDSQUARE_OCCUPIED    = 1,
	BUILDSQUARE_RECLAIMABLE = 2,
	BUILDSQUARE_OPEN        = 3
};


class CUnitHandler
{
	CR_DECLARE_STRUCT(CUnitHandler)

public:
	CUnitHandler();
	~CUnitHandler();

	void Update();
	void DeleteUnit(CUnit* unit);
	void DeleteUnitNow(CUnit* unit);
	bool AddUnit(CUnit* unit);
	void PostLoad();

	bool CanAddUnit(int id) const {
		// do we want to be assigned a random ID and are any left in pool?
		if (id < 0)
			return (!freeUnitIndexToIdentMap.empty());
		// is this ID not already in use?
		if (id < MaxUnits())
			return (units[id] == NULL);
		// AddUnit will not make new room for us
		return false;
	}

	unsigned int MaxUnits() const { return maxUnits; }
	float MaxUnitRadius() const { return maxUnitRadius; }

	///< test if a unit can be built at specified position
	BuildSquareStatus TestUnitBuildSquare(
		const BuildInfo&,
		CFeature*&,
		int allyteam,
		bool synced,
		std::vector<float3>* canbuildpos = NULL,
		std::vector<float3>* featurepos = NULL,
		std::vector<float3>* nobuildpos = NULL,
		const std::vector<Command>* commands = NULL
	);
	/// Returns true if a unit of type unitID can be built, false otherwise
	bool CanBuildUnit(const UnitDef* unitdef, int team) const;

	void AddBuilderCAI(CBuilderCAI*);
	void RemoveBuilderCAI(CBuilderCAI*);
	float GetBuildHeight(const float3& pos, const UnitDef* unitdef, bool synced = true);

	Command GetBuildCommand(const float3& pos, const float3& dir);


	// note: negative ID's are implicitly converted
	CUnit* GetUnitUnsafe(unsigned int unitID) const { return units[unitID]; }
	CUnit* GetUnit(unsigned int unitID) const { return (unitID < MaxUnits()? units[unitID]: NULL); }


	std::vector< std::vector<CUnitSet> > unitsByDefs; ///< units sorted by team and unitDef

	std::list<CUnit*> activeUnits;                    ///< used to get all active units
	std::vector<CUnit*> units;                        ///< used to get units from IDs (0 if not created)
	std::list<CBuilderCAI*> builderCAIs;              // TODO use std::set? (with custom compare-func for sync)

private:
	void InsertActiveUnit(CUnit* unit);

	///< test a single mapsquare for build possibility
	static BuildSquareStatus TestBuildSquare(const float3& pos, const float buildHeight, const UnitDef* unitdef, const MoveDef* moveDef, CFeature *&feature, int allyteam, bool synced);

private:
	std::map<unsigned int, unsigned int> freeUnitIndexToIdentMap;
	std::map<unsigned int, unsigned int> freeUnitIdentToIndexMap;

	std::vector<CUnit*> unitsToBeRemoved;              ///< units that will be removed at start of next update
	std::list<CUnit*>::iterator activeSlowUpdateUnit;  ///< first unit of batch that will be SlowUpdate'd this frame

	///< global unit-limit (derived from the per-team limit)
	///< units.size() is equal to this and constant at runtime
	unsigned int maxUnits;

	///< largest radius of any unit added so far (some
	///< spatial query filters in GameHelper use this)
	float maxUnitRadius;
};

extern CUnitHandler* uh;

#endif /* UNITHANDLER_H */
