/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "MoveTypeFactory.h"
#include "MoveDefHandler.h"
#include "StrafeAirMoveType.h"
#include "HoverAirMoveType.h"
#include "GroundMoveType.h"
#include "StaticMoveType.h"
#include "ScriptMoveType.h"

#include "Sim/Misc/SimObjectMemPool.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

// CGroundMoveType is (currently) the largest derived move-type
static SimObjectMemPool<sizeof(CGroundMoveType)> memPool;

void MoveTypeFactory::InitStatic() {
	memPool.clear();
	memPool.reserve(128);

	static_assert(sizeof(CGroundMoveType) >= sizeof(CStrafeAirMoveType), "");
	static_assert(sizeof(CGroundMoveType) >= sizeof(CHoverAirMoveType ), "");
	static_assert(sizeof(CGroundMoveType) >= sizeof(CStaticMoveType   ), "");
	static_assert(sizeof(CGroundMoveType) >= sizeof(CScriptMoveType   ), "");
}

AMoveType* MoveTypeFactory::GetMoveType(CUnit* unit, const UnitDef* ud) {
	if (ud->IsGroundUnit()) {
		// mobile ground-unit
		assert(!ud->canfly);
		assert(ud->pathType != -1U);
		assert(unit->moveDef == nullptr);

		unit->moveDef = moveDefHandler->GetMoveDefByPathType(ud->pathType);

		return (memPool.alloc<CGroundMoveType>(unit));
	}

	if (ud->IsAirUnit()) {
		// mobile air-unit
		assert(ud->canfly);
		assert(ud->pathType == -1U);
		assert(unit->moveDef == nullptr);

		if (ud->IsStrafingAirUnit())
			return (memPool.alloc<CStrafeAirMoveType>(unit));

		// flying builders, transports, gunships
		return (memPool.alloc<CHoverAirMoveType>(unit));
	}

	// unit is immobile (but not necessarily a structure)
	assert(ud->IsImmobileUnit());
	return (memPool.alloc<CStaticMoveType>(unit));
}

AMoveType* MoveTypeFactory::GetScriptMoveType(CUnit* unit) {
	return (memPool.alloc<CScriptMoveType>(unit));
}


bool MoveTypeFactory::FreeMoveType(AMoveType*& moveType) {
	if (moveType == nullptr)
		return false;

	memPool.free(moveType);
	return true;
}

