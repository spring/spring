/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "MoveTypeFactory.h"
#include "MoveDefHandler.h"
#include "StrafeAirMoveType.h"
#include "HoverAirMoveType.h"
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
		assert(ud->pathType != -1U);
		assert(unit->moveDef == NULL);

		unit->moveDef = moveDefHandler->GetMoveDefByPathType(ud->pathType);

		return (new CGroundMoveType(unit));
	}

	if (ud->IsAirUnit()) {
		// mobile air-unit
		assert(ud->canfly);
		assert(ud->pathType == -1U);
		assert(unit->moveDef == NULL);

		if (ud->IsStrafingAirUnit()) {
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
