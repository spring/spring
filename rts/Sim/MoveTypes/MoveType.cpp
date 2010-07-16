/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "MoveType.h"
#include "MoveMath/MoveMath.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include <assert.h>

CR_BIND_DERIVED_INTERFACE(AMoveType, CObject);
CR_REG_METADATA(AMoveType, (
		CR_MEMBER(forceTurn),
		CR_MEMBER(forceTurnTo),
		CR_MEMBER(owner),
		CR_MEMBER(goalPos),
		CR_MEMBER(oldPos),
		CR_MEMBER(oldSlowUpdatePos),
		CR_MEMBER(maxSpeed),
		CR_MEMBER(maxWantedSpeed),
		CR_MEMBER(reservedPad),
		CR_MEMBER(padStatus),
		CR_MEMBER(repairBelowHealth),
		CR_MEMBER(useHeading),
		CR_ENUM_MEMBER(progressState),
		CR_RESERVED(32)
		));

AMoveType::AMoveType(CUnit* owner):
	forceTurn(0),
	forceTurnTo(0),
	owner(owner),
	goalPos(owner ? owner->pos : float3(0.0f, 0.0f, 0.0f)),
	oldPos(owner? owner->pos: float3(0.0f, 0.0f, 0.0f)),
	oldSlowUpdatePos(oldPos),
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
	//! note static buildings don't have any unitDef->moveData, that's why CStaticMoveType uses its own method
	owner->pos.y = owner->unitDef->movedata->moveMath->yLevel(owner->pos.x, owner->pos.z);
	owner->midPos.y = owner->pos.y + owner->relMidPos.y;
};

void AMoveType::LeaveTransport(void)
{
}

void AMoveType::KeepPointingTo(CUnit* unit, float distance, bool aggressive)
{
	KeepPointingTo(float3(unit->pos), distance, aggressive);
}

void AMoveType::SetGoal(const float3& pos)
{
	goalPos = pos;
}

void AMoveType::DependentDied(CObject* o)
{
	if (o == reservedPad) {
		reservedPad = 0;
	}
}

void AMoveType::ReservePad(CAirBaseHandler::LandingPad* lp)
{
	AddDeathDependence(lp);
	reservedPad = lp;
	padStatus = 0;
	SetGoal(lp->GetUnit()->pos);
}
