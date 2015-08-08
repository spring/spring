/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "UnitHandler.h"
#include "Unit.h"
#include "UnitDefHandler.h"
#include "CommandAI/BuilderCAI.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveType.h"
#include "Sim/Weapons/Weapon.h"
#include "System/EventHandler.h"
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
	CR_MEMBER(maxUnits),
	CR_MEMBER(maxUnitRadius),
	CR_POSTLOAD(PostLoad)
))



void CUnitHandler::PostLoad()
{
	// reset any synced stuff that is not saved
	activeSlowUpdateUnit = 0;
	activeUpdateUnit = 0;
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

	activeSlowUpdateUnit = 0;
	activeUpdateUnit = 0;
	airBaseHandler = new CAirBaseHandler();
}


CUnitHandler::~CUnitHandler()
{
	for (CUnit* u: activeUnits) {
		// ~CUnit dereferences featureHandler which is destroyed already
		u->delayedWreckLevel = -1;
		delete u;
	}

	delete airBaseHandler;
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
	unitsByDefs[unit->team][unit->unitDef->id].insert(unit);

	maxUnitRadius = std::max(unit->radius, maxUnitRadius);
	return true;
}

void CUnitHandler::DeleteUnit(CUnit* unit)
{
	unitsToBeRemoved.push_back(unit);
}


void CUnitHandler::DeleteUnitNow(CUnit* delUnit)
{
	//we want to call RenderUnitDestroyed while the unit is still valid
	eventHandler.RenderUnitDestroyed(delUnit);
	for (int i = 0; i < activeUnits.size();++i) {
		if (activeUnits[i] != delUnit)
			continue;

			
		int delTeam = delUnit->team;
		int delType = delUnit->unitDef->id;

		teamHandler->Team(delTeam)->RemoveUnit(delUnit, CTeam::RemoveDied);
		activeUnits.erase(activeUnits.begin() + i);
		
		if (activeSlowUpdateUnit > i) {
			--activeSlowUpdateUnit;
		}
		
		assert(activeUnits.size() == i || activeUnits[i] != delUnit);
		
		unitsByDefs[delTeam][delType].erase(delUnit);
		idPool.FreeID(delUnit->id, true);

		units[delUnit->id] = NULL;

		CSolidObject::SetDeletingRefID(delUnit->id);
		delete delUnit;
		CSolidObject::SetDeletingRefID(-1);
		
		break;
	}

#ifdef _DEBUG
	for (int i = 0; i < activeUnits.size();) {
		if (activeUnits[i] == delUnit) {
			LOG_L(L_ERROR, "Duplicated unit found in active units on erase");
			activeUnits.erase(activeUnits.begin() + i);
		} else {
			++i;
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
				CUnit* delUnit = unitsToBeRemoved.back();
				unitsToBeRemoved.pop_back();
				DeleteUnitNow(delUnit);
			}
		}
	}

	{
		SCOPED_TIMER("Unit::MoveType::Update");
		
		for (activeUpdateUnit = 0; activeUpdateUnit < activeUnits.size();++activeUpdateUnit) {
			CUnit *unit = activeUnits[activeUpdateUnit];
			AMoveType* moveType = unit->moveType;

			UNIT_SANITY_CHECK(unit);

			if (moveType->Update()) {
				eventHandler.UnitMoved(unit);
			}
			if (!unit->pos.IsInBounds() && (unit->speed.w > MAX_UNIT_SPEED)) {
				// this unit is not coming back, kill it now without any death
				// sequence (so deathScriptFinished becomes true immediately)
				unit->KillUnit(NULL, false, true, false);
			}

			UNIT_SANITY_CHECK(unit);
			assert(activeUnits[activeUpdateUnit] == unit);
		}
	}

	{
		// Delete dead units
		for (activeUpdateUnit = 0; activeUpdateUnit < activeUnits.size();++activeUpdateUnit) {
			CUnit *unit = activeUnits[activeUpdateUnit];
			if (!unit->deathScriptFinished)
				continue;

			// there are many ways to fiddle with "deathScriptFinished", so a unit may
			// arrive here without having been properly killed (and isDead still false),
			// which can result in MT deadlocking -- FIXME verify this
			// (KU returns early if isDead)
			unit->KillUnit(NULL, false, true);
			DeleteUnit(unit);
			assert(activeUnits[activeUpdateUnit] == unit);
		}
	}

	{
		SCOPED_TIMER("Unit::UpdatePieceMatrices");
		//Shouldn't insert new units
		for (CUnit* unit: activeUnits) {
			// UnitScript only applies piece-space transforms so
			// we apply the forward kinematics update separately
			// (only if we have any dirty pieces)
			unit->localModel->UpdatePieceMatrices();
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
			UNIT_SANITY_CHECK(unit);

			n--;
		}
	}

	{
		SCOPED_TIMER("Unit::Update");

		for (activeUpdateUnit = 0; activeUpdateUnit < activeUnits.size();++activeUpdateUnit) {
			CUnit *unit = activeUnits[activeUpdateUnit];
			UNIT_SANITY_CHECK(unit);
			unit->Update();
			UNIT_SANITY_CHECK(unit);
			assert(activeUnits[activeUpdateUnit] == unit);
		}
	}

	{
		SCOPED_TIMER("Unit::Weapon::Update");

		for (activeUpdateUnit = 0; activeUpdateUnit < activeUnits.size();++activeUpdateUnit) {
			CUnit *unit = activeUnits[activeUpdateUnit];
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
