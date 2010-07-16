/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "StaticMoveType.h"
#include "Map/Ground.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"

CR_BIND_DERIVED(CStaticMoveType, AMoveType, (NULL));
CR_REG_METADATA(CStaticMoveType,
		(
		CR_RESERVED(63)
		));

void CStaticMoveType::SlowUpdate()
{
	//FIXME intercept the heightmapUpdate event and update buildings y-pos only on-demand!
	owner->pos.y = ground->GetHeight2(owner->pos.x, owner->pos.z);
	if (owner->floatOnWater && owner->pos.y < -owner->unitDef->waterline) {
		owner->pos.y = -owner->unitDef->waterline;
	}
	owner->midPos.y = owner->pos.y + owner->relMidPos.y;
};
