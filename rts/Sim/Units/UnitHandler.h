/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITHANDLER_H
#define UNITHANDLER_H

#include <array>
#include <vector>

#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/SimObjectIDPool.h"
#include "System/creg/STL_Map.h"

struct UnitDef;
class CUnit;
class CBuilderCAI;

class CUnitHandler
{
	CR_DECLARE_STRUCT(CUnitHandler)

public:
	CUnitHandler(): idPool(MAX_UNITS) {}

	void Init();
	void Kill();

	void DeleteScripts();

	void Update();
	bool AddUnit(CUnit* unit);

	bool CanAddUnit(int id) const {
		// do we want to be assigned a random ID and are any left in pool?
		if (id < 0)
			return (!idPool.IsEmpty());
		// is this ID not already in use *and* has it been recycled by pool?
		if (id < MaxUnits())
			return (units[id] == nullptr && idPool.HasID(id));
		// AddUnit will not make new room for us
		return false;
	}

	unsigned int NumUnitsByTeam      (int teamNum               ) const { return (unitsByDefs[teamNum][        0].size()); }
	unsigned int NumUnitsByTeamAndDef(int teamNum, int unitDefID) const { return (unitsByDefs[teamNum][unitDefID].size()); }
	unsigned int MaxUnits() const { return maxUnits; }
	unsigned int CalcMaxUnits() const;

	float MaxUnitRadius() const { return maxUnitRadius; }

	/// Returns true if a unit of type unitID can be built, false otherwise
	bool CanBuildUnit(const UnitDef* unitdef, int team) const;
	bool GarbageCollectUnit(unsigned int id);

	void AddBuilderCAI(CBuilderCAI*);
	void RemoveBuilderCAI(CBuilderCAI*);

	void ChangeUnitTeam(CUnit* unit, int oldTeamNum, int newTeamNum);

	// note: negative ID's are implicitly converted
	CUnit* GetUnitUnsafe(unsigned int id) const { return units[id]; }
	CUnit* GetUnit(unsigned int id) const { return ((id < MaxUnits())? units[id]: nullptr); }

	static CUnit* NewUnit(const UnitDef* ud);
	static void SanityCheckUnit(const CUnit* unit);

	const std::vector<CUnit*>& GetActiveUnits() const { return activeUnits; }
	      std::vector<CUnit*>& GetActiveUnits()       { return activeUnits; }

	const std::vector<CUnit*>& GetUnitsByTeam      (int teamNum               ) const { return unitsByDefs[teamNum][        0]; }
	const std::vector<CUnit*>& GetUnitsByTeamAndDef(int teamNum, int unitDefID) const { return unitsByDefs[teamNum][unitDefID]; }

	std::vector<CUnit*>& GetUnitsByTeam      (int teamNum               ) { return unitsByDefs[teamNum][        0]; }
	std::vector<CUnit*>& GetUnitsByTeamAndDef(int teamNum, int unitDefID) { return unitsByDefs[teamNum][unitDefID]; }

	const spring::unordered_map<unsigned int, CBuilderCAI*>& GetBuilderCAIs() const { return builderCAIs; }

private:
	void InsertActiveUnit(CUnit* unit);
	bool QueueDeleteUnit(CUnit* unit);
	void QueueDeleteUnits();
	void DeleteUnit(CUnit* unit);
	void DeleteUnits();
	void SlowUpdateUnits();
	void UpdateUnitMoveTypes();
	void UpdateUnitLosStates();
	void UpdateUnits();
	void UpdateUnitWeapons();

private:
	SimObjectIDPool idPool;

	std::vector<CUnit*> units;                                           ///< used to get units from IDs (0 if not created)
	std::array<std::vector<std::vector<CUnit*>>, MAX_TEAMS> unitsByDefs; ///< units sorted by team and unitDef

	std::vector<CUnit*> activeUnits;                                     ///< used to get all active units
	std::vector<CUnit*> unitsToBeRemoved;                                ///< units that will be removed at start of next update

	spring::unordered_map<unsigned int, CBuilderCAI*> builderCAIs;


	size_t activeSlowUpdateUnit = 0;  ///< first unit of batch that will be SlowUpdate'd this frame
	size_t activeUpdateUnit = 0;      ///< first unit of batch that will be SlowUpdate'd this frame


	///< global unit-limit (derived from the per-team limit)
	///< units.size() is equal to this and constant at runtime
	unsigned int maxUnits = 0;

	///< largest radius of any unit added so far (some
	///< spatial query filters in GameHelper use this)
	float maxUnitRadius = 0.0f;

	bool inUpdateCall = false;
};

extern CUnitHandler unitHandler;

#endif /* UNITHANDLER_H */
