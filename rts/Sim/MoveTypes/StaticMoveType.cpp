/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "StaticMoveType.h"
#include "Map/Ground.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

CR_BIND_DERIVED(CStaticMoveType, AMoveType, (nullptr))
CR_REG_METADATA(CStaticMoveType, )

void CStaticMoveType::SlowUpdate()
{
	// buildings and pseudo-static units can be transported
	if (owner->GetTransporter() != nullptr)
		return;

	// NOTE:
	//   static buildings don't have any MoveDef instance, hence we need
	//   to get the ground height instead of calling CMoveMath::yLevel()
	// FIXME: intercept heightmapUpdate events and update buildings y-pos only on-demand!
	if (owner->FloatOnWater() && owner->IsInWater()) {
		owner->Move(UpVector * (-waterline - owner->pos.y), true);
	} else {
		owner->Move(UpVector * (CGround::GetHeightReal(owner->pos.x, owner->pos.z) - owner->pos.y), true);
	}
}
