/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "StaticMoveType.h"
#include "Map/Ground.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

CR_BIND_DERIVED(CStaticMoveType, AMoveType, (NULL));
CR_REG_METADATA(CStaticMoveType, (
	CR_RESERVED(63)
));

void CStaticMoveType::SlowUpdate()
{
	// buildings and pseudo-static units can be transported
	if (owner->GetTransporter() != NULL)
		return;

	// NOTE:
	//     static buildings don't have any unitDef->moveDef, hence we need
	//     to get the ground height instead of calling CMoveMath::yLevel()
	// FIXME: intercept heightmapUpdate events and update buildings y-pos only on-demand!
	const UnitDef* ud = owner->unitDef;

	if (ud->floatOnWater && (owner->pos.y <= 0.0f)) {
		owner->Move(-UpVector * ud->waterline, false);
	} else {
		owner->Move(UpVector * ground->GetHeightReal(owner->pos.x, owner->pos.z), false);
	}
}
