#include "StdAfx.h"
#include "MoveType.h"
#include "Map/Ground.h"
#include "Sim/Units/UnitDef.h"
#include "mmgr.h"

CR_BIND_DERIVED(CMoveType, CObject, (NULL));

CR_REG_METADATA(CMoveType, (
		CR_MEMBER(forceTurn),
		CR_MEMBER(forceTurnTo),
		CR_MEMBER(owner),
		CR_MEMBER(maxSpeed),
		CR_MEMBER(maxWantedSpeed),
		CR_MEMBER(useHeading),
		CR_ENUM_MEMBER(progressState)));


CMoveType::CMoveType(CUnit* owner)
: owner(owner),
	forceTurn(0),
	forceTurnTo(0),
	maxSpeed(0.2f),
	maxWantedSpeed(0.2f),
	useHeading(true),
	progressState(Done)
{
}

CMoveType::~CMoveType(void)
{
}

void CMoveType::SetMaxSpeed(float speed)
{
	assert(speed > 0);
	maxSpeed=speed;
}

void CMoveType::SetWantedMaxSpeed(float speed)
{
	if(speed > maxSpeed)
		maxWantedSpeed = maxSpeed;
	else if(speed < 0.001f)
		maxWantedSpeed = 0;
	else
		maxWantedSpeed = speed;
}

void CMoveType::ImpulseAdded(void)
{
}

void CMoveType::SlowUpdate()
{
	owner->pos.y=ground->GetHeight2(owner->pos.x,owner->pos.z);
	if(owner->floatOnWater && owner->pos.y<0)
		owner->pos.y = -owner->unitDef->waterline;
	owner->midPos.y=owner->pos.y+owner->relMidPos.y;
};

void CMoveType::LeaveTransport(void)
{
}

void CMoveType::KeepPointingTo(CUnit* unit, float distance, bool aggressive)
{
	KeepPointingTo(float3(unit->pos), distance, aggressive);
};
