/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


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

CBuilding::CBuilding()
{
	immobile = true;
}



void CBuilding::PreInit(const UnitLoadParams& params)
{
	unitDef = params.unitDef;
	blockHeightChanges = unitDef->levelGround;

	CUnit::PreInit(params);
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
	speed = ZeroVector;
	heading = GetHeadingFromFacing(buildFacing);
	frontdir = GetVectorFromHeading(heading);

	Move3D(CGameHelper::Pos2BuildPos(BuildInfo(unitDef, newPos, buildFacing), true), false);
	UpdateMidAndAimPos();

	CUnit::ForcedMove(pos);

	unitLoader->FlattenGround(this);
}
