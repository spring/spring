/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "MoveTypeFactory.h"
#include "MoveInfo.h"
#include "StrafeAirMoveType.h"
#include "HoverAirMoveType.h"
#include "GroundMoveType.h"
#include "StaticMoveType.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

AMoveType* MoveTypeFactory::GetMoveType(CUnit* unit, const UnitDef* ud) {
	if (ud->IsGroundUnit()) {
		// mobile ground-unit
		assert(!ud->canfly);
		assert(ud->movedata != NULL);
		assert(unit->mobility == NULL);

		unit->mobility = new MoveData(ud->movedata);
		CGroundMoveType* gmt = new CGroundMoveType(unit);
		return gmt;
	}

	if (ud->IsAirUnit()) {
		// mobile air-unit
		assert(ud->canfly);
		assert(ud->movedata == NULL);

		if (!ud->builder && !ud->IsTransportUnit() && ud->IsNonHoveringAirUnit()) {
			CStrafeAirMoveType* sAMT = new CStrafeAirMoveType(unit);
			return sAMT;
		} else {
			// flying builders, transports, gunships
			CHoverAirMoveType* hAMT = new CHoverAirMoveType(unit);
			return hAMT;
		}
	}

	// unit is immobile (but not necessarily a structure)
	assert(ud->IsImmobileUnit());
	return (new CStaticMoveType(unit));
}
