/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "UnitHandler.h"
#include "Unit.h"
#include "UnitDefHandler.h"
#include "CommandAI/BuilderCAI.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Weapons/Weapon.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/TimeProfiler.h"
#include "System/Util.h"
#include "System/Sync/SyncTracer.h"
#include "System/creg/STL_Deque.h"
#include "System/creg/STL_List.h"
#include "System/creg/STL_Set.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUnitHandler* unitHandler = NULL;

CR_BIND(CUnitHandler, )
CR_REG_METADATA(CUnitHandler, (
	CR_MEMBER(units),
	CR_MEMBER(unitsByDefs),
	CR_MEMBER(activeUnits),
	CR_MEMBER(builderCAIs),
	CR_MEMBER(idPool),
	CR_MEMBER(unitsToBeRemoved),
	CR_MEMBER(activeSlowUpdateUnit),
	CR_MEMBER(activeUpdateUnit),
	CR_MEMBER(maxUnits),
	CR_MEMBER(maxUnitRadius)
))


CUnitHandler::CUnitHandler()
:
	maxUnits(0),
	maxUnitRadius(0.0f)
{
	// set the global (runtime-constant) unit-limit as the sum
	// of  all team unit-limits, which is *always* <= MAX_UNITS
	// (note that this also counts the Gaia team)
	//
	// teams can not be created at runtime, but they can die and
	// in that case the per-team limit is recalculated for every
	// other team in the respective allyteam
	for (unsigned int n = 0; n < teamHandler->ActiveTeams(); n++) {
		maxUnits += teamHandler->Team(n)->GetMaxUnits();
	}

	units.resize(maxUnits, NULL);
	unitsByDefs.resize(teamHandler->ActiveTeams(), std::vector<std::vector<CUnit*>>(unitDefHandler->unitDefs.size()));

	// id's are used as indices, so they must lie in [0, units.size() - 1]
	// (furthermore all id's are treated equally, none have special status)
	idPool.Expand(0, units.size());

	activeSlowUpdateUnit = 0;
	activeUpdateUnit = 0;
}


CUnitHandler::~CUnitHandler()
{
	for (CUnit* u: activeUnits) {
		// ~CUnit dereferences featureHandler which is destroyed already
		u->delayedWreckLevel = -1;
		delete u;
	}
}


void CUnitHandler::DeleteScripts()
{
	// Predelete scripts since they sometimes call models
	// which are already gone by KillSimulation.
	for (CUnit* u: activeUnits) {
		u->DeleteScript();
	}
}


void CUnitHandler::InsertActiveUnit(CUnit* unit)
{
	const unsigned int insertionPos = gs->randFloat() * activeUnits.size();

	idPool.AssignID(unit);

	assert(unit->id < units.size());
	assert(units[unit->id] == NULL);

	assert(insertionPos >= 0 && insertionPos <= activeUnits.size());
	activeUnits.insert(activeUnits.begin() + insertionPos, unit);
	if (insertionPos <= activeSlowUpdateUnit) {
		++activeSlowUpdateUnit;
	}
	if (insertionPos <= activeUpdateUnit) {
		++activeUpdateUnit;
	}
	units[unit->id] = unit;
}


bool CUnitHandler::AddUnit(CUnit* unit)
{
	// LoadUnit should make sure this is true
	assert(CanAddUnit(unit->id));

	InsertActiveUnit(unit);

	teamHandler->Team(unit->team)->AddUnit(unit, CTeam::AddBuilt);
	VectorInsertUnique(unitsByDefs[unit->team][unit->unitDef->id], unit, false);

	maxUnitRadius = std::max(unit->radius, maxUnitRadius);
	return true;
}


void CUnitHandler::DeleteUnit(CUnit* unit)
{
	unitsToBeRemoved.push_back(unit);
}

void CUnitHandler::DeleteUnitsNow()
{
	if (unitsToBeRemoved.empty())
		return;

	while (!unitsToBeRemoved.empty()) {
		DeleteUnitNow(unitsToBeRemoved.back());
		unitsToBeRemoved.pop_back();
	}
}

void CUnitHandler::DeleteUnitNow(CUnit* delUnit)
{
	// we want to call RenderUnitDestroyed while the unit is still valid
	eventHandler.RenderUnitDestroyed(delUnit);

	const auto it = std::find(activeUnits.begin(), activeUnits.end(), delUnit);
	assert(it != activeUnits.end());
	{
		const int delTeam = delUnit->team;
		const int delType = delUnit->unitDef->id;

		teamHandler->Team(delTeam)->RemoveUnit(delUnit, CTeam::RemoveDied);

		if (activeSlowUpdateUnit > std::distance(activeUnits.begin(), it)) {
			--activeSlowUpdateUnit;
		}

		activeUnits.erase(it);
		VectorErase(unitsByDefs[delTeam][delType], delUnit);
		idPool.FreeID(delUnit->id, true);
		units[delUnit->id] = nullptr;

		CSolidObject::SetDeletingRefID(delUnit->id);
		delete delUnit;
		CSolidObject::SetDeletingRefID(-1);
	}

#ifdef _DEBUG
	for (CUnit* u: activeUnits) {
		if (u == delUnit) {
			LOG_L(L_ERROR, "Duplicated unit found in active units on erase");
		}
	}
#endif
}


void CUnitHandler::Update()
{
	auto UNIT_SANITY_CHECK = [](const CUnit* unit) {
		unit->pos.AssertNaNs();
		unit->midPos.AssertNaNs();
		unit->relMidPos.AssertNaNs();
		unit->speed.AssertNaNs();
		unit->deathSpeed.AssertNaNs();
		unit->rightdir.AssertNaNs();
		unit->updir.AssertNaNs();
		unit->frontdir.AssertNaNs();
		if (unit->unitDef->IsGroundUnit()) {
			assert(unit->pos.x >= -(float3::maxxpos * 16.0f));
			assert(unit->pos.x <=  (float3::maxxpos * 16.0f));
			assert(unit->pos.z >= -(float3::maxzpos * 16.0f));
			assert(unit->pos.z <=  (float3::maxzpos * 16.0f));
		}
	};

	DeleteUnitsNow();

	{
		SCOPED_TIMER("Unit::MoveType::Update");

		for (activeUpdateUnit = 0; activeUpdateUnit < activeUnits.size();++activeUpdateUnit) {
			CUnit* unit = activeUnits[activeUpdateUnit];
			AMoveType* moveType = unit->moveType;

			UNIT_SANITY_CHECK(unit);

			if (moveType->Update()) {
				eventHandler.UnitMoved(unit);
			}
			if (!unit->pos.IsInBounds() && (unit->speed.w > MAX_UNIT_SPEED)) {
				// this unit is not coming back, kill it now without any death
				// sequence (so deathScriptFinished becomes true immediately)
				unit->KillUnit(nullptr, false, true, false);
			}

			UNIT_SANITY_CHECK(unit);
			assert(activeUnits[activeUpdateUnit] == unit);
		}
	}

	{
		// Delete dead units
		for (activeUpdateUnit = 0; activeUpdateUnit < activeUnits.size();++activeUpdateUnit) {
			CUnit* unit = activeUnits[activeUpdateUnit];

			if (!unit->deathScriptFinished)
				continue;

			// there are many ways to fiddle with "deathScriptFinished", so a unit
			// may arrive here not having been properly killed (with isDead still
			// false)
			// make sure we always call Killed; no-op if isDead is already true
			unit->KillUnit(nullptr, false, true, true);
			DeleteUnit(unit);

			assert(activeUnits[activeUpdateUnit] == unit);
		}
	}

	{
		SCOPED_TIMER("Unit::UpdateLosStatus");
		for (CUnit* unit: activeUnits) {
			for (int at = 0; at < teamHandler->ActiveAllyTeams(); ++at) {
				unit->UpdateLosStatus(at);
			}
		}
	}

	{
		SCOPED_TIMER("Unit::SlowUpdate");
		assert(activeSlowUpdateUnit >= 0);
		// reset the iterator every <UNIT_SLOWUPDATE_RATE> frames
		if ((gs->frameNum % UNIT_SLOWUPDATE_RATE) == 0) {
			activeSlowUpdateUnit = 0;
		}

		// stagger the SlowUpdate's
		unsigned int n = (activeUnits.size() / UNIT_SLOWUPDATE_RATE) + 1;

		for (; activeSlowUpdateUnit < activeUnits.size() && n != 0; ++activeSlowUpdateUnit) {
			CUnit* unit = activeUnits[activeSlowUpdateUnit];

			UNIT_SANITY_CHECK(unit);
			unit->SlowUpdate();
			unit->SlowUpdateWeapons();
			unit->localModel.UpdateBoundingVolume();
			UNIT_SANITY_CHECK(unit);

			n--;
		}
	}

	{
		SCOPED_TIMER("Unit::Update");

		for (activeUpdateUnit = 0; activeUpdateUnit < activeUnits.size();++activeUpdateUnit) {
			CUnit* unit = activeUnits[activeUpdateUnit];
			UNIT_SANITY_CHECK(unit);
			unit->Update();
			UNIT_SANITY_CHECK(unit);
			assert(activeUnits[activeUpdateUnit] == unit);
		}
	}

	{
		SCOPED_TIMER("Unit::Weapon::Update");

		for (activeUpdateUnit = 0; activeUpdateUnit < activeUnits.size();++activeUpdateUnit) {
			CUnit* unit = activeUnits[activeUpdateUnit];
			if (unit->CanUpdateWeapons()) {
				for (CWeapon* w: unit->weapons) {
					w->Update();
				}
			}
			assert(activeUnits[activeUpdateUnit] == unit);
		}
	}
}



void CUnitHandler::AddBuilderCAI(CBuilderCAI* b)
{
	// called from CBuilderCAI --> owner is already valid
	builderCAIs[b->owner->id] = b;
}

void CUnitHandler::RemoveBuilderCAI(CBuilderCAI* b)
{
	// called from ~CUnit --> owner is still valid
	assert(b->owner != nullptr);
	builderCAIs.erase(b->owner->id);
}



bool CUnitHandler::CanBuildUnit(const UnitDef* unitdef, int team) const
{
	if (teamHandler->Team(team)->AtUnitLimit()) {
		return false;
	}
	if (unitsByDefs[team][unitdef->id].size() >= unitdef->maxThisUnit) {
		return false;
	}

	return true;
}
