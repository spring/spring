/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "MoveType.h"
#include "Map/Ground.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include <cassert>

CR_BIND_DERIVED_INTERFACE(AMoveType, CObject);
CR_REG_METADATA(AMoveType, (
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
	assert(speed > 0.0f);
	maxSpeed = speed;
}

void AMoveType::SetWantedMaxSpeed(float speed)
{
	if (speed > maxSpeed) {
		maxWantedSpeed = maxSpeed;
	} else if (speed < 0.001f) {
		maxWantedSpeed = 0;
	} else {
		maxWantedSpeed = speed;
	}
}

void AMoveType::SlowUpdate()
{
	if (owner->pos != oldSlowUpdatePos) {
		oldSlowUpdatePos = owner->pos;

		const int newMapSquare = ground->GetSquare(owner->pos);
		const float losHeight = owner->losHeight;
		const bool isAirMoveType = !owner->usingScriptMoveType && owner->unitDef->canfly;

		if (newMapSquare != owner->mapSquare) {
			owner->mapSquare = newMapSquare;

			if (isAirMoveType) {
				// temporarily set LOS-height to current altitude for aircraft
				owner->losHeight = (owner->pos.y - ground->GetApproximateHeight(owner->pos.x, owner->pos.z)) + 5.0f;
			}

			loshandler->MoveUnit(owner, false);
			radarhandler->MoveUnit(owner);

			if (isAirMoveType) {
				owner->losHeight = losHeight;
			}
		}

		qf->MovedUnit(owner);
	}
}

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
		reservedPad = NULL;
	}
}

void AMoveType::ReservePad(CAirBaseHandler::LandingPad* lp)
{
	AddDeathDependence(lp);
	SetGoal(lp->GetUnit()->pos);

	reservedPad = lp;
	padStatus = 0;
}

bool AMoveType::WantsRepair() const { return (owner->health      < (repairBelowHealth * owner->maxHealth)); }
bool AMoveType::WantsRefuel() const { return (owner->currentFuel < (repairBelowHealth * owner->unitDef->maxFuel)); }
