/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "lib/gml/gmlmut.h"
#include "lib/gml/gml_base.h"
#include "UnitHandler.h"
#include "Unit.h"
#include "UnitDefHandler.h"
#include "CommandAI/BuilderCAI.h"
#include "Sim/Misc/AirBaseHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveType.h"
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

CR_BIND(CUnitHandler, );
CR_REG_METADATA(CUnitHandler, (
	CR_MEMBER(activeUnits),
	CR_MEMBER(units),
	CR_MEMBER(freeUnitIndexToIdentMap),
	CR_MEMBER(freeUnitIdentToIndexMap),
	CR_MEMBER(maxUnits),
	CR_MEMBER(maxUnitRadius),
	CR_MEMBER(unitsToBeRemoved),
	CR_MEMBER(builderCAIs),
	CR_MEMBER(unitsByDefs),
	CR_POSTLOAD(PostLoad)
));



void CUnitHandler::PostLoad()
{
	// reset any synced stuff that is not saved
	activeSlowUpdateUnit = activeUnits.end();
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

	{
		std::vector<unsigned int> freeIDs(units.size());

		// id's are used as indices, so they must lie in [0, units.size() - 1]
		// (furthermore all id's are treated equally, none have special status)
		for (unsigned int id = 0; id < freeIDs.size(); id++) {
			freeIDs[id] = id;
		}

		// randomize the unit ID's so that Lua widgets can not
		// easily determine enemy unit counts from ID's alone
		// (shuffle twice for good measure)
		SyncedRNG rng;

		std::random_shuffle(freeIDs.begin(), freeIDs.end(), rng);
		std::random_shuffle(freeIDs.begin(), freeIDs.end(), rng);

		// NOTE:
		//   any randomization would be undone by using a std::set as-is
		//   instead create a bi-directional mapping from indices to ID's
		//   (where the ID's are a random permutation of the index range)
		//   such that ID's can be assigned and returned to the pool with
		//   their original index
		//
		//     freeUnitIndexToIdentMap = {<0, 13>, < 1, 27>, < 2, 54>, < 3, 1>, ...}
		//     freeUnitIdentToIndexMap = {<1,  3>, <13,  0>, <27,  1>, <54, 2>, ...}
		//
		//   (the ID --> index map is never changed at runtime!)
		for (unsigned int n = 0; n < freeIDs.size(); n++) {
			freeUnitIndexToIdentMap.insert(std::pair<unsigned int, unsigned int>(n, freeIDs[n]));
			freeUnitIdentToIndexMap.insert(std::pair<unsigned int, unsigned int>(freeIDs[n], n));
		}
	}

	activeSlowUpdateUnit = activeUnits.end();
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

	if (unit->id < 0) {
		// should be unreachable (all code that goes through
		// UnitLoader::LoadUnit --> Unit::PreInit checks the
		// unit limit first)
		assert(!freeUnitIndexToIdentMap.empty());
		assert(!freeUnitIdentToIndexMap.empty());

		// pick first available free ID if we want a random one
		unit->id = (freeUnitIndexToIdentMap.begin())->second;
	}

	assert(unit->id < units.size());
	assert(units[unit->id] == NULL);
	assert(freeUnitIndexToIdentMap.find(freeUnitIdentToIndexMap[unit->id]) != freeUnitIndexToIdentMap.end());

	freeUnitIndexToIdentMap.erase(freeUnitIdentToIndexMap[unit->id]);
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
	int delTeam = 0;
	int delType = 0;

	std::list<CUnit*>::iterator usi;

	for (usi = activeUnits.begin(); usi != activeUnits.end(); ++usi) {
		if (*usi == delUnit) {
			if (activeSlowUpdateUnit != activeUnits.end() && *usi == *activeSlowUpdateUnit) {
				++activeSlowUpdateUnit;
			}
			delTeam = delUnit->team;
			delType = delUnit->unitDef->id;

			GML_STDMUTEX_LOCK(dque); // DeleteUnitNow

			teamHandler->Team(delTeam)->RemoveUnit(delUnit, CTeam::RemoveDied);

			activeUnits.erase(usi);
			freeUnitIndexToIdentMap.insert(std::pair<unsigned int, unsigned int>(freeUnitIdentToIndexMap[delUnit->id], delUnit->id));
			unitsByDefs[delTeam][delType].erase(delUnit);

			units[delUnit->id] = NULL;

			CSolidObject::SetDeletingRefID(delUnit->id);
			delete delUnit;
			CSolidObject::SetDeletingRefID(-1);

			break;
		}
	}

#ifdef _DEBUG
	for (usi = activeUnits.begin(); usi != activeUnits.end(); /* no post-op */) {
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
	{
		GML_STDMUTEX_LOCK(runit); // Update

		if (!unitsToBeRemoved.empty()) {
			GML_RECMUTEX_LOCK(obj); // Update

			while (!unitsToBeRemoved.empty()) {
				eventHandler.DeleteSyncedObjects(); // the unit destructor may invoke eventHandler, so we need to call these for every unit to clear invaild references from the batching systems

				GML_RECMUTEX_LOCK(unit); // Update

				eventHandler.DeleteSyncedUnits();

				GML_RECMUTEX_LOCK(proj); // Update - projectile drawing may access owner() and lead to crash
				GML_RECMUTEX_LOCK(sel);  // Update - unit is removed from selectedUnits in ~CObject, which is too late.
				GML_RECMUTEX_LOCK(quad); // Update - make sure unit does not get partially deleted before before being removed from the quadfield

				CUnit* delUnit = unitsToBeRemoved.back();
				unitsToBeRemoved.pop_back();

				DeleteUnitNow(delUnit);
			}
		}

		eventHandler.UpdateUnits();
	}

	GML::UpdateTicks();

	#define VECTOR_SANITY_CHECK(v)                              \
		assert(!math::isnan(v.x) && !math::isinf(v.x)); \
		assert(!math::isnan(v.y) && !math::isinf(v.y)); \
		assert(!math::isnan(v.z) && !math::isinf(v.z));
	#define MAPPOS_SANITY_CHECK(unit)                          \
		if (unit->unitDef->IsGroundUnit()) {                   \
			assert(unit->pos.x >= -(float3::maxxpos * 16.0f)); \
			assert(unit->pos.x <=  (float3::maxxpos * 16.0f)); \
			assert(unit->pos.z >= -(float3::maxzpos * 16.0f)); \
			assert(unit->pos.z <=  (float3::maxzpos * 16.0f)); \
		}
	#define UNIT_SANITY_CHECK(unit)                 \
		VECTOR_SANITY_CHECK(unit->pos);             \
		VECTOR_SANITY_CHECK(unit->midPos);          \
		VECTOR_SANITY_CHECK(unit->relMidPos);       \
		VECTOR_SANITY_CHECK(unit->speed);           \
		VECTOR_SANITY_CHECK(unit->deathSpeed);      \
		VECTOR_SANITY_CHECK(unit->residualImpulse); \
		VECTOR_SANITY_CHECK(unit->rightdir);        \
		VECTOR_SANITY_CHECK(unit->updir);           \
		VECTOR_SANITY_CHECK(unit->frontdir);        \
		MAPPOS_SANITY_CHECK(unit);

	{
		SCOPED_TIMER("Unit::MoveType::Update");
		std::list<CUnit*>::iterator usi;
		for (usi = activeUnits.begin(); usi != activeUnits.end(); ++usi) {
			CUnit* unit = *usi;
			AMoveType* moveType = unit->moveType;

			UNIT_SANITY_CHECK(unit);

			if (moveType->Update()) {
				eventHandler.UnitMoved(unit);
			}
			if (!unit->pos.IsInBounds() && (unit->speed.SqLength() > (MAX_UNIT_SPEED * MAX_UNIT_SPEED))) {
				// this unit is not coming back, kill it now without any death
				// sequence (so deathScriptFinished becomes true immediately)
				unit->KillUnit(false, true, NULL, false);
			}

			UNIT_SANITY_CHECK(unit);
			GML::GetTicks(unit->lastUnitUpdate);
		}
	}

	{
		SCOPED_TIMER("Unit::Update");
		std::list<CUnit*>::iterator usi;
		for (usi = activeUnits.begin(); usi != activeUnits.end(); ++usi) {
			CUnit* unit = *usi;

			UNIT_SANITY_CHECK(unit);

			if (unit->deathScriptFinished) {
				// there are many ways to fiddle with "deathScriptFinished", so a unit may
				// arrive here without having been properly killed (and isDead still false),
				// which can result in MT deadlocking -- FIXME verify this
				// (KU returns early if isDead)
				unit->KillUnit(false, true, NULL);

				DeleteUnit(unit);
			} else {
				unit->Update();
			}

			UNIT_SANITY_CHECK(unit);
		}
	}

	{
		SCOPED_TIMER("Unit::SlowUpdate");

		// reset the iterator every <UNIT_SLOWUPDATE_RATE> frames
		if ((gs->frameNum & (UNIT_SLOWUPDATE_RATE - 1)) == 0) {
			activeSlowUpdateUnit = activeUnits.begin();
		}

		// stagger the SlowUpdate's
		int n = (activeUnits.size() / UNIT_SLOWUPDATE_RATE) + 1;

		for (; activeSlowUpdateUnit != activeUnits.end() && n != 0; ++activeSlowUpdateUnit) {
			CUnit* unit = *activeSlowUpdateUnit;

			UNIT_SANITY_CHECK(unit);
			unit->SlowUpdate();
			UNIT_SANITY_CHECK(unit);

			n--;
		}
	}
}



void CUnitHandler::AddBuilderCAI(CBuilderCAI* b)
{
	GML_STDMUTEX_LOCK(cai); // AddBuilderCAI

	builderCAIs.push_back(b);
}

void CUnitHandler::RemoveBuilderCAI(CBuilderCAI* b)
{
	GML_STDMUTEX_LOCK(cai); // RemoveBuilderCAI

	builderCAIs.remove(b);
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
