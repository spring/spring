/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "UnitHandler.h"
#include "Unit.h"
#include "UnitDefHandler.h"
#include "CommandAI/BuilderCAI.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Weapons/Weapon.h"
#include "System/EventHandler.h"
#include "System/EventBatchHandler.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/myMath.h"
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
	CR_IGNORED(activeSlowUpdateUnit),
	CR_IGNORED(activeSlowUpdateWeapon),
	CR_MEMBER(maxUnits),
	CR_MEMBER(maxUnitRadius),
	CR_POSTLOAD(PostLoad)
))



void CUnitHandler::PostLoad()
{
	// reset any synced stuff that is not saved
	activeSlowUpdateUnit = activeUnits.end();
	activeSlowUpdateWeapon = activeUnits.end();
}


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
	unitsByDefs.resize(teamHandler->ActiveTeams(), std::vector<CUnitSet>(unitDefHandler->unitDefs.size()));

	// id's are used as indices, so they must lie in [0, units.size() - 1]
	// (furthermore all id's are treated equally, none have special status)
	idPool.Expand(0, units.size());

	activeSlowUpdateUnit = activeUnits.end();
	activeSlowUpdateWeapon = activeUnits.end();
	airBaseHandler = new CAirBaseHandler();
}


CUnitHandler::~CUnitHandler()
{
	for (std::list<CUnit*>::iterator usi = activeUnits.begin(); usi != activeUnits.end(); ++usi) {
		// ~CUnit dereferences featureHandler which is destroyed already
		(*usi)->delayedWreckLevel = -1;
		delete (*usi);
	}

	delete airBaseHandler;
}

void CUnitHandler::InsertActiveUnit(CUnit* unit)
{
	std::list<CUnit*>::iterator ui = activeUnits.begin();

	if (ui != activeUnits.end()) {
		// randomize this to make the slow-update order random (good if one
		// builds say many buildings at once and then many mobile ones etc)
		const unsigned int insertionPos = gs->randFloat() * activeUnits.size();

		for (unsigned int n = 0; n < insertionPos; ++n) {
			++ui;
		}
	}

	idPool.AssignID(unit);

	assert(unit->id < units.size());
	assert(units[unit->id] == NULL);

	activeUnits.insert(ui, unit);
	units[unit->id] = unit;
}


bool CUnitHandler::AddUnit(CUnit* unit)
{
	// LoadUnit should make sure this is true
	assert(CanAddUnit(unit->id));

	InsertActiveUnit(unit);

	teamHandler->Team(unit->team)->AddUnit(unit, CTeam::AddBuilt);
	unitsByDefs[unit->team][unit->unitDef->id].insert(unit);

	maxUnitRadius = std::max(unit->radius, maxUnitRadius);
	return true;
}

void CUnitHandler::DeleteUnit(CUnit* unit)
{
	unitsToBeRemoved.push_back(unit);
	(eventBatchHandler->GetUnitCreatedDestroyedBatch()).dequeue_synced(unit);
}


void CUnitHandler::DeleteUnitNow(CUnit* delUnit)
{
	if (activeSlowUpdateUnit != activeUnits.end() && delUnit == *activeSlowUpdateUnit) {
		++activeSlowUpdateUnit;
		++activeSlowUpdateWeapon;
	}

	for (auto usi = activeUnits.begin(); usi != activeUnits.end(); ++usi) {
		if (*usi != delUnit)
			continue;

		int delTeam = delUnit->team;
		int delType = delUnit->unitDef->id;

		teamHandler->Team(delTeam)->RemoveUnit(delUnit, CTeam::RemoveDied);

		activeUnits.erase(usi);
		unitsByDefs[delTeam][delType].erase(delUnit);
		idPool.FreeID(delUnit->id, true);

		units[delUnit->id] = NULL;

		CSolidObject::SetDeletingRefID(delUnit->id);
		delete delUnit;
		CSolidObject::SetDeletingRefID(-1);

		break;
	}

#ifdef _DEBUG
	for (auto usi = activeUnits.begin(); usi != activeUnits.end(); /* no post-op */) {
		if (*usi == delUnit) {
			LOG_L(L_ERROR, "Duplicated unit found in active units on erase");
			usi = activeUnits.erase(usi);
		} else {
			++usi;
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

	{
		if (!unitsToBeRemoved.empty()) {
			while (!unitsToBeRemoved.empty()) {
				eventHandler.DeleteSyncedObjects(); // the unit destructor may invoke eventHandler, so we need to call these for every unit to clear invaild references from the batching systems
				eventHandler.DeleteSyncedUnits();

				CUnit* delUnit = unitsToBeRemoved.back();
				unitsToBeRemoved.pop_back();

				DeleteUnitNow(delUnit);
			}
		}
	}

	{
		SCOPED_TIMER("Unit::MoveType::Update");

		for (CUnit* unit: activeUnits) {
			AMoveType* moveType = unit->moveType;

			UNIT_SANITY_CHECK(unit);

			if (moveType->Update()) {
				eventHandler.UnitMoved(unit);
			}
			if (!unit->pos.IsInBounds() && (Square(unit->speed.w) > (MAX_UNIT_SPEED * MAX_UNIT_SPEED))) {
				// this unit is not coming back, kill it now without any death
				// sequence (so deathScriptFinished becomes true immediately)
				unit->KillUnit(NULL, false, true, false);
			}

			UNIT_SANITY_CHECK(unit);
		}
	}

	{
		// Delete dead units
		for (CUnit* unit: activeUnits) {
			if (!unit->deathScriptFinished)
				continue;

			// there are many ways to fiddle with "deathScriptFinished", so a unit may
			// arrive here without having been properly killed (and isDead still false),
			// which can result in MT deadlocking -- FIXME verify this
			// (KU returns early if isDead)
			unit->KillUnit(NULL, false, true);
			DeleteUnit(unit);
		}
	}

	{
		SCOPED_TIMER("Unit::UpdatePieceMatrices");

		for (CUnit* unit: activeUnits) {
			// UnitScript only applies piece-space transforms so
			// we apply the forward kinematics update separately
			// (only if we have any dirty pieces)
			unit->localModel->UpdatePieceMatrices();
		}
	}

	{
		SCOPED_TIMER("Unit::SlowUpdate");

		// reset the iterator every <UNIT_SLOWUPDATE_RATE> frames
		if ((gs->frameNum % UNIT_SLOWUPDATE_RATE) == 0) {
			activeSlowUpdateUnit = activeUnits.begin();
		}

		// stagger the SlowUpdate's
		unsigned int n = (activeUnits.size() / UNIT_SLOWUPDATE_RATE) + 1;

		for (; activeSlowUpdateUnit != activeUnits.end() && n != 0; ++activeSlowUpdateUnit) {
			CUnit* unit = *activeSlowUpdateUnit;

			UNIT_SANITY_CHECK(unit);
			unit->SlowUpdate();
			UNIT_SANITY_CHECK(unit);

			n--;
		}
	}

	{
		SCOPED_TIMER("Unit::Weapon::SlowUpdate");

		// reset the iterator every <UNIT_SLOWUPDATE_RATE> frames
		if ((gs->frameNum % UNIT_SLOWUPDATE_RATE) == 0) {
			activeSlowUpdateWeapon = activeUnits.begin();
		}

		// stagger the SlowUpdate's
		unsigned int n = (activeUnits.size() / UNIT_SLOWUPDATE_RATE) + 1;

		for (; activeSlowUpdateWeapon != activeUnits.end() && n != 0; ++activeSlowUpdateWeapon) {
			CUnit* unit = *activeSlowUpdateWeapon;
			unit->SlowUpdateWeapons();
			n--;
		}
	}

	{
		SCOPED_TIMER("Unit::Update");

		for (CUnit* unit: activeUnits) {
			UNIT_SANITY_CHECK(unit);
			unit->Update();
			UNIT_SANITY_CHECK(unit);
		}
	}

	{
		SCOPED_TIMER("Unit::Weapon::Update");

		for (CUnit* unit: activeUnits) {
			if (!unit->beingBuilt && !unit->IsStunned() && !unit->dontUseWeapons && !unit->isDead) {
				for (CWeapon* w: unit->weapons) {
					w->Update();
				}
			}
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
	assert(b->owner != NULL);
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
