#include "StdAfx.h"
#include "mmgr.h"

#include "MoveType.h"
#include "Map/Ground.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include <assert.h>

CR_BIND_DERIVED_INTERFACE(AMoveType, CObject);
CR_REG_METADATA(AMoveType, (
		CR_MEMBER(forceTurn),
		CR_MEMBER(forceTurnTo),
		CR_MEMBER(owner),
		CR_MEMBER(goalPos),
		CR_MEMBER(maxSpeed),
		CR_MEMBER(maxWantedSpeed),
		CR_MEMBER(reservedPad),
		CR_MEMBER(padStatus),
		CR_MEMBER(repairBelowHealth),
		CR_MEMBER(useHeading),
		CR_ENUM_MEMBER(progressState),
		CR_RESERVED(32)
		));

CR_BIND_DERIVED(CMoveType, AMoveType, (NULL));
CR_REG_METADATA(CMoveType,
		(
		CR_RESERVED(63)
		));

AMoveType::AMoveType(CUnit* owner):
	forceTurn(0),
	forceTurnTo(0),
	owner(owner),
	goalPos(owner ? owner->pos : float3(0.0f, 0.0f, 0.0f)),
	maxSpeed(0.2f),
	maxWantedSpeed(0.2f),
	reservedPad(0),
	padStatus(0),
	repairBelowHealth(0.3f),
	useHeading(true),
	progressState(Done)
{
}

AMoveType::~AMoveType(void)
{
}

void AMoveType::SetMaxSpeed(float speed)
{
	assert(speed > 0);
	maxSpeed = speed;
}

void AMoveType::SetWantedMaxSpeed(float speed)
{
	if(speed > maxSpeed) {
		maxWantedSpeed = maxSpeed;
	} else if(speed < 0.001f) {
		maxWantedSpeed = 0;
	} else {
		maxWantedSpeed = speed;
	}
}

void AMoveType::ImpulseAdded(void)
{
}

void AMoveType::SlowUpdate()
{
	owner->pos.y = ground->GetHeight2(owner->pos.x, owner->pos.z);
	if (owner->floatOnWater && owner->pos.y < -owner->unitDef->waterline) {
		owner->pos.y = -owner->unitDef->waterline;
	}
	owner->midPos.y = owner->pos.y + owner->relMidPos.y;
};

void AMoveType::LeaveTransport(void)
{
}

void AMoveType::KeepPointingTo(CUnit* unit, float distance, bool aggressive)
{
	KeepPointingTo(float3(unit->pos), distance, aggressive);
}

void AMoveType::SetGoal(float3 pos)
{
	goalPos = pos;
}

void AMoveType::DependentDied(CObject* o)
{
	if(o == reservedPad){
		reservedPad=0;
	}
}

void AMoveType::ReservePad(CAirBaseHandler::LandingPad* lp)
{
	AddDeathDependence(lp);
	reservedPad = lp;
	padStatus = 0;
	SetGoal(lp->GetUnit()->pos);
}
