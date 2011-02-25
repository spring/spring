/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "AAirMoveType.h"
#include "MoveMath/MoveMath.h"

#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/CommandAI/CommandAI.h"

CR_BIND_DERIVED_INTERFACE(AAirMoveType, AMoveType);

CR_REG_METADATA(AAirMoveType, (
	CR_MEMBER(oldGoalPos),
	CR_MEMBER(reservedLandingPos),

	CR_MEMBER(wantedHeight),

	CR_MEMBER(collide),
	CR_MEMBER(useSmoothMesh),
	CR_MEMBER(lastColWarning),
	CR_MEMBER(lastColWarningType),

	CR_MEMBER(autoLand),
	CR_MEMBER(lastFuelUpdateFrame),

	CR_RESERVED(16)
));

AAirMoveType::AAirMoveType(CUnit* unit) :
	AMoveType(unit),
	aircraftState(AIRCRAFT_LANDED),
	oldGoalPos(owner? owner->pos : ZeroVector),
	reservedLandingPos(-1.0f, -1.0f, -1.0f),
	wantedHeight(80.0f),
	collide(true),
	useSmoothMesh(false),
	lastColWarning(NULL),
	lastColWarningType(0),
	autoLand(true),
	lastFuelUpdateFrame(0)
{
	useHeading = false;
}

AAirMoveType::~AAirMoveType()
{
	if (reservedPad) {
		airBaseHandler->LeaveLandingPad(reservedPad);
		reservedPad = NULL;
	}
}

bool AAirMoveType::UseSmoothMesh() const {
	if (useSmoothMesh) {
		const bool onTransportMission =
			!owner->commandAI->commandQue.empty() &&
			((owner->commandAI->commandQue.front().id == CMD_LOAD_UNITS) || (owner->commandAI->commandQue.front().id == CMD_UNLOAD_UNIT));
		const bool repairing = reservedPad ? padStatus >= 1 : false;
		const bool forceDisableSmooth =
			repairing ||
			aircraftState == AIRCRAFT_LANDING ||
			aircraftState == AIRCRAFT_LANDED ||
			onTransportMission;
		return !forceDisableSmooth;
	} else {
		return false;
	}
}

void AAirMoveType::ReservePad(CAirBaseHandler::LandingPad* lp) {
	oldGoalPos = goalPos;
	AMoveType::ReservePad(lp);
	Takeoff();
}

void AAirMoveType::DependentDied(CObject* o) {
	AMoveType::DependentDied(o);

	if (o == reservedPad) {
		SetState(AIRCRAFT_FLYING);
		goalPos = oldGoalPos;
	}
}

void AAirMoveType::UpdateFuel() {
	if (owner->unitDef->maxFuel > 0.0f) {
		if (aircraftState != AIRCRAFT_LANDED)
			owner->currentFuel = std::max(0.0f, owner->currentFuel - ((float)(gs->frameNum - lastFuelUpdateFrame) / GAME_SPEED));
		lastFuelUpdateFrame = gs->frameNum;
	}
}



void AAirMoveType::CheckForCollision()
{
	if (!collide)
		return;

	const SyncedFloat3& pos = owner->midPos;
	const SyncedFloat3& forward = owner->frontdir;

	const float3 midTestPos = pos + forward * 121.0f;
	const std::vector<CUnit*>& others = qf->GetUnitsExact(midTestPos, 115.0f);

	float dist = 200.0f;

	if (lastColWarning) {
		DeleteDeathDependence(lastColWarning);
		lastColWarning = NULL;
		lastColWarningType = 0;
	}

	for (std::vector<CUnit*>::const_iterator ui = others.begin(); ui != others.end(); ++ui) {
		const CUnit* unit = *ui;

		if (unit == owner || !unit->unitDef->canfly) {
			continue;
		}

		const SyncedFloat3& op = unit->midPos;
		const float3 dif = op - pos;
		const float3 forwardDif = forward * (forward.dot(dif));

		if (forwardDif.SqLength() >= (dist * dist)) {
			continue;
		}

		const float3 ortoDif = dif - forwardDif;
		const float frontLength = forwardDif.Length();
		// note: radii are multiplied by two
		const float minOrtoDif = (unit->radius + owner->radius) * 2.0f + frontLength * 0.1f + 10;

		if (ortoDif.SqLength() < (minOrtoDif * minOrtoDif)) {
			dist = frontLength;
			lastColWarning = const_cast<CUnit*>(unit);
		}
	}

	if (lastColWarning != NULL) {
		lastColWarningType = 2;
		AddDeathDependence(lastColWarning);
		return;
	}

	for (std::vector<CUnit*>::const_iterator ui = others.begin(); ui != others.end(); ++ui) {
		if (*ui == owner)
			continue;
		if (((*ui)->midPos - pos).SqLength() < (dist * dist)) {
			lastColWarning = *ui;
		}
	}

	if (lastColWarning != NULL) {
		lastColWarningType = 1;
		AddDeathDependence(lastColWarning);
	}
}
