/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "MoveTypeFactory.h"
#include "MoveDefHandler.h"
#include "StrafeAirMoveType.h"
#include "HoverAirMoveType.h"
#include "ClassicGroundMoveType.h"
#include "GroundMoveType.h"
#include "StaticMoveType.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

AMoveType* MoveTypeFactory::GetMoveType(CUnit* unit, const UnitDef* ud) {
	if (ud->IsGroundUnit()) {
		// mobile ground-unit
		assert(!ud->canfly);
		assert(ud->moveDef != NULL);
		assert(unit->moveDef == NULL);

		unit->moveDef = new MoveDef(ud->moveDef);

		if (modInfo.useClassicGroundMoveType) {
			return (new CClassicGroundMoveType(unit));
		} else {
			return (new CGroundMoveType(unit));
		}
	}

	if (ud->IsAirUnit()) {
		// mobile air-unit
		assert(ud->canfly);
		assert(ud->moveDef == NULL);

		if (!ud->builder && !ud->IsTransportUnit() && ud->IsNonHoveringAirUnit()) {
			return (new CStrafeAirMoveType(unit));
		} else {
			// flying builders, transports, gunships
			return (new CHoverAirMoveType(unit));
		}
	}

	// unit is immobile (but not necessarily a structure)
	assert(ud->IsImmobileUnit());
	return (new CStaticMoveType(unit));
}
