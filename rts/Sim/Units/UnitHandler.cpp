/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "UnitHandler.h"
#include "Unit.h"
#include "UnitDefHandler.h"
#include "UnitMemPool.h"
#include "UnitTypes/Builder.h"
#include "UnitTypes/ExtractorBuilding.h"
#include "UnitTypes/Factory.h"

#include "CommandAI/BuilderCAI.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Weapons/Weapon.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/myMath.h"
#include "System/TimeProfiler.h"
#include "System/Sync/SyncTracer.h"
#include "System/creg/STL_Deque.h"
#include "System/creg/STL_List.h"
#include "System/creg/STL_Set.h"


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



UnitMemPool unitMemPool;

CUnitHandler* unitHandler = nullptr;


void CUnitHandler::SanityCheckUnit(const CUnit* unit)
{
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

CUnit* CUnitHandler::NewUnit(const UnitDef* ud)
{
	// special static builder structures that can always be given
	// move orders (which are passed on to all mobile buildees)
	if (ud->IsFactoryUnit())
		return (unitMemPool.alloc<CFactory>());

	// all other types of non-structure "builders", including hubs and
	// nano-towers (the latter should not have any build-options at all,
	// whereas the former should be unable to build any mobile units)
	if (ud->IsMobileBuilderUnit() || ud->IsStaticBuilderUnit())
		return (unitMemPool.alloc<CBuilder>());

	// static non-builder structures
	if (ud->IsBuildingUnit()) {
		if (ud->IsExtractorUnit())
			return (unitMemPool.alloc<CExtractorBuilding>());

		return (unitMemPool.alloc<CBuilding>());
	}

	// regular mobile unit
	return (unitMemPool.alloc<CUnit>());
}



CUnitHandler::CUnitHandler() {
	static_assert(sizeof(CBuilder) >= sizeof(CUnit             ), "");
	static_assert(sizeof(CBuilder) >= sizeof(CBuilding         ), "");
	static_assert(sizeof(CBuilder) >= sizeof(CExtractorBuilding), "");
	static_assert(sizeof(CBuilder) >= sizeof(CFactory          ), "");

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

	units.resize(maxUnits, nullptr);
	unitsByDefs.resize(teamHandler->ActiveTeams(), std::vector<std::vector<CUnit*>>(unitDefHandler->unitDefs.size()));

	unitMemPool.reserve(128);
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
		unitMemPool.free(u);
	}

	// do not clear in ctor because creg-loaded objects would be wiped out
	unitMemPool.clear();
}


void CUnitHandler::DeleteScripts()
{
	// predelete scripts since they sometimes reference (pieces
	// of) models, which are already gone before KillSimulation
	for (CUnit* u: activeUnits) {
		u->DeleteScript();
	}
}


void CUnitHandler::InsertActiveUnit(CUnit* unit)
{
	idPool.AssignID(unit);

	assert(unit->id < units.size());
	assert(units[unit->id] == nullptr);

	#if 0
	// randomized insertion is supposed to break up peak loads
	// during the (staggered) SlowUpdate step, but also causes
	// more jumping around in memory for regular Updates
	// in larger games (where it would matter most) the order
	// of insertion is essentially guaranteed to be random by
	// interleaved player actions anyway, and if needed could
	// also be achieved by periodic shuffling
	const unsigned int insertionPos = gsRNG.NextInt(activeUnits.size());

	assert(insertionPos < activeUnits.size());
	activeUnits.insert(activeUnits.begin() + insertionPos, unit);

	// do not (slow)update the same unit twice if the new one
	// gets inserted behind our current iterator position and
	// right-shifts the rest
	activeSlowUpdateUnit += (insertionPos <= activeSlowUpdateUnit);
	activeUpdateUnit += (insertionPos <= activeUpdateUnit);

	#else
	activeUnits.push_back(unit);
	#endif

	units[unit->id] = unit;
}


bool CUnitHandler::AddUnit(CUnit* unit)
{
	// LoadUnit should make sure this is true
	assert(CanAddUnit(unit->id));

	InsertActiveUnit(unit);

	teamHandler->Team(unit->team)->AddUnit(unit, CTeam::AddBuilt);
	spring::VectorInsertUnique(unitsByDefs[unit->team][unit->unitDef->id], unit, false);

	maxUnitRadius = std::max(unit->radius, maxUnitRadius);
	return true;
}


bool CUnitHandler::GarbageCollectUnit(unsigned int id)
{
	if (inUpdateCall)
		return false;

	// ensure that QDU can actually collect this unit
	if (!units[id]->deathScriptFinished)
		return false;

	QueueDeleteUnits();
	DeleteUnits();

	return (idPool.RecycleID(id));
}


void CUnitHandler::QueueDeleteUnits()
{
	// gather up dead units
	for (activeUpdateUnit = 0; activeUpdateUnit < activeUnits.size(); ++activeUpdateUnit) {
		CUnit* unit = activeUnits[activeUpdateUnit];

		if (!unit->deathScriptFinished)
			continue;

		// there are many ways to fiddle with "deathScriptFinished", so a unit may
		// arrive here not having been properly killed while isDead is still false
		// make sure we always call Killed; no-op if isDead was already set to true
		unit->KillUnit(nullptr, false, true, true);
		unitsToBeRemoved.push_back(unit);

		assert(activeUnits[activeUpdateUnit] == unit);
	}
}


void CUnitHandler::DeleteUnits()
{
	while (!unitsToBeRemoved.empty()) {
		DeleteUnit(unitsToBeRemoved.back());
		unitsToBeRemoved.pop_back();
	}
}

void CUnitHandler::DeleteUnit(CUnit* delUnit)
{
	assert(delUnit->isDead);
	// we want to call RenderUnitDestroyed while the unit is still valid
	eventHandler.RenderUnitDestroyed(delUnit);

	const auto it = std::find(activeUnits.begin(), activeUnits.end(), delUnit);

	if (it == activeUnits.end()) {
		assert(false);
		return;
	}

	const int delUnitTeam = delUnit->team;
	const int delUnitType = delUnit->unitDef->id;

	teamHandler->Team(delUnitTeam)->RemoveUnit(delUnit, CTeam::RemoveDied);

	if (activeSlowUpdateUnit > std::distance(activeUnits.begin(), it))
		--activeSlowUpdateUnit;

	activeUnits.erase(it);

	spring::VectorErase(unitsByDefs[delUnitTeam][delUnitType], delUnit);
	idPool.FreeID(delUnit->id, true);

	units[delUnit->id] = nullptr;

	CSolidObject::SetDeletingRefID(delUnit->id);
	unitMemPool.free(delUnit);
	CSolidObject::SetDeletingRefID(-1);
}


void CUnitHandler::UpdateUnitMoveTypes()
{
	SCOPED_TIMER("Sim::Unit::MoveType");

	for (activeUpdateUnit = 0; activeUpdateUnit < activeUnits.size(); ++activeUpdateUnit) {
		CUnit* unit = activeUnits[activeUpdateUnit];
		AMoveType* moveType = unit->moveType;

		SanityCheckUnit(unit);

		if (moveType->Update())
			eventHandler.UnitMoved(unit);

		// this unit is not coming back, kill it now without any death
		// sequence (so deathScriptFinished becomes true immediately)
		if (!unit->pos.IsInBounds() && (unit->speed.w > MAX_UNIT_SPEED))
			unit->KillUnit(nullptr, false, true, false);

		SanityCheckUnit(unit);
		assert(activeUnits[activeUpdateUnit] == unit);
	}
}

void CUnitHandler::UpdateUnitLosStates()
{
	SCOPED_TIMER("Sim::Unit::UpdateLosStatus");

	for (CUnit* unit: activeUnits) {
		for (int at = 0; at < teamHandler->ActiveAllyTeams(); ++at) {
			unit->UpdateLosStatus(at);
		}
	}
}


void CUnitHandler::SlowUpdateUnits()
{
	SCOPED_TIMER("Sim::Unit::SlowUpdate");
	assert(activeSlowUpdateUnit >= 0);

	// reset the iterator every <UNIT_SLOWUPDATE_RATE> frames
	if ((gs->frameNum % UNIT_SLOWUPDATE_RATE) == 0)
		activeSlowUpdateUnit = 0;

	// stagger the SlowUpdate's
	for (size_t n = (activeUnits.size() / UNIT_SLOWUPDATE_RATE) + 1; (activeSlowUpdateUnit < activeUnits.size() && n != 0); ++activeSlowUpdateUnit) {
		CUnit* unit = activeUnits[activeSlowUpdateUnit];

		SanityCheckUnit(unit);
		unit->SlowUpdate();
		unit->SlowUpdateWeapons();
		unit->SlowUpdateLocalModel();
		SanityCheckUnit(unit);

		n--;
	}
}

void CUnitHandler::UpdateUnits()
{
	SCOPED_TIMER("Sim::Unit::Update");

	for (activeUpdateUnit = 0; activeUpdateUnit < activeUnits.size(); ++activeUpdateUnit) {
		CUnit* unit = activeUnits[activeUpdateUnit];
		SanityCheckUnit(unit);
		unit->Update();
		// unsynced; done on-demand when drawing unit
		// unit->UpdateLocalModel();
		SanityCheckUnit(unit);
		assert(activeUnits[activeUpdateUnit] == unit);
	}
}

void CUnitHandler::UpdateUnitWeapons()
{
	SCOPED_TIMER("Sim::Unit::Weapon");

	for (activeUpdateUnit = 0; activeUpdateUnit < activeUnits.size(); ++activeUpdateUnit) {
		CUnit* unit = activeUnits[activeUpdateUnit];

		if (!unit->CanUpdateWeapons())
			continue;

		for (CWeapon* w: unit->weapons) {
			w->Update();
		}

		assert(activeUnits[activeUpdateUnit] == unit);
	}
}


void CUnitHandler::Update()
{
	inUpdateCall = true;

	DeleteUnits();
	UpdateUnitMoveTypes();
	QueueDeleteUnits();
	UpdateUnitLosStates();
	SlowUpdateUnits();
	UpdateUnits();
	UpdateUnitWeapons();

	inUpdateCall = false;
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
	if (teamHandler->Team(team)->AtUnitLimit())
		return false;

	if (unitsByDefs[team][unitdef->id].size() >= unitdef->maxThisUnit)
		return false;

	return true;
}

