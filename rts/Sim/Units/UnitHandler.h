/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITHANDLER_H
#define UNITHANDLER_H

#include <vector>

#include "UnitDef.h"
#include "Sim/Misc/SimObjectIDPool.h"
#include "System/creg/STL_Map.h"

class CUnit;
class CBuilderCAI;

class CUnitHandler
{
	CR_DECLARE_STRUCT(CUnitHandler)

public:
	CUnitHandler();
	~CUnitHandler();

	void DeleteScripts();

	void Update();
	bool AddUnit(CUnit* unit);

	bool CanAddUnit(int id) const {
		// do we want to be assigned a random ID and are any left in pool?
		if (id < 0)
			return (!idPool.IsEmpty());
		// is this ID not already in use?
		if (id < MaxUnits())
			return (units[id] == nullptr);
		// AddUnit will not make new room for us
		return false;
	}

	unsigned int MaxUnits() const { return maxUnits; }
	float MaxUnitRadius() const { return maxUnitRadius; }

	/// Returns true if a unit of type unitID can be built, false otherwise
	bool CanBuildUnit(const UnitDef* unitdef, int team) const;

	void AddBuilderCAI(CBuilderCAI*);
	void RemoveBuilderCAI(CBuilderCAI*);

	// note: negative ID's are implicitly converted
	CUnit* GetUnitUnsafe(unsigned int unitID) const { return units[unitID]; }
	CUnit* GetUnit(unsigned int unitID) const { return (unitID < MaxUnits()? units[unitID]: nullptr); }

	const std::unordered_map<unsigned int, CBuilderCAI*>& GetBuilderCAIs() const { return builderCAIs; }

public:
	// FIXME
	std::vector<CUnit*> units;                        ///< used to get units from IDs (0 if not created)
	std::vector<std::vector<std::vector<CUnit*>>> unitsByDefs; ///< units sorted by team and unitDef
	std::vector<CUnit*> activeUnits;                  ///< used to get all active units

private:
	void DeleteUnit(CUnit* unit);
	void DeleteUnitNow(CUnit* unit);
	void DeleteUnitsNow();
	void InsertActiveUnit(CUnit* unit);

private:
	SimObjectIDPool idPool;

	std::vector<CUnit*> unitsToBeRemoved;              ///< units that will be removed at start of next update
	std::unordered_map<unsigned int, CBuilderCAI*> builderCAIs;

	size_t activeSlowUpdateUnit;  ///< first unit of batch that will be SlowUpdate'd this frame
	size_t activeUpdateUnit;  ///< first unit of batch that will be SlowUpdate'd this frame

	///< global unit-limit (derived from the per-team limit)
	///< units.size() is equal to this and constant at runtime
	unsigned int maxUnits;

	///< largest radius of any unit added so far (some
	///< spatial query filters in GameHelper use this)
	float maxUnitRadius;
};

extern CUnitHandler* unitHandler;

#endif /* UNITHANDLER_H */
