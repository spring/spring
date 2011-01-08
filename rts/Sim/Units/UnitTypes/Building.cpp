/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "Building.h"
#include "Game/GameHelper.h"
#include "Map/ReadMap.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitLoader.h"
#include "System/myMath.h"

CR_BIND_DERIVED(CBuilding, CUnit, );

CR_REG_METADATA(CBuilding, (
	CR_RESERVED(8),
	CR_POSTLOAD(PostLoad)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBuilding::CBuilding(): buildingDecal(0)
{
	immobile = true;
}



void CBuilding::PreInit(const UnitDef* def, int team, int facing, const float3& position, bool build)
{
	if (def->levelGround) {
		blockHeightChanges = true;
	}

	CUnit::PreInit(def, team, facing, position, build);
}

void CBuilding::PostInit(const CUnit* builder)
{
	if (unitDef->cantBeTransported)
		mass = CSolidObject::DEFAULT_MASS;
	physicalState = OnGround;

	CUnit::PostInit(builder);
}


void CBuilding::ForcedMove(const float3& newPos, int facing) {
	buildFacing = facing;
	pos = helper->Pos2BuildPos(BuildInfo(unitDef, newPos, buildFacing));
	speed = ZeroVector;
	heading = GetHeadingFromFacing(buildFacing);
	frontdir = GetVectorFromHeading(heading);
	CUnit::ForcedMove(pos);
	unitLoader->FlattenGround(this);
}
