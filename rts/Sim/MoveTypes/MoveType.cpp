#include "StdAfx.h"
#include "MoveType.h"
#include "Sim/Map/Ground.h"
#include "Sim/Units/UnitDef.h"
#include "mmgr.h"

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


void CMoveType::SetWantedMaxSpeed(float speed) {
	if(speed > maxSpeed)
		maxWantedSpeed = maxSpeed;
	else if(speed < 0.001)
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
