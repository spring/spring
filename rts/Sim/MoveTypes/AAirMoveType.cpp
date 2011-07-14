/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "AAirMoveType.h"
#include "MoveMath/MoveMath.h"

#include "Map/Ground.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/Scripts/UnitScript.h"

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
	autoLand(true),
	lastColWarning(NULL),
	lastColWarningType(0),
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
			((owner->commandAI->commandQue.front().GetID() == CMD_LOAD_UNITS) || (owner->commandAI->commandQue.front().GetID() == CMD_UNLOAD_UNIT));
		const bool repairing = reservedPad ? padStatus >= 1 : false;
		const bool forceDisableSmooth = repairing || onTransportMission || (aircraftState != AIRCRAFT_FLYING);
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

bool AAirMoveType::Update() {
	// NOTE: useHeading is never true by default for aircraft (AAirMoveType
	// forces it to false, TransportUnit::{Attach,Detach}Unit manipulate it
	// specifically for TAAirMoveType's)
	if (useHeading) {
		useHeading = false;
		SetState(AIRCRAFT_TAKEOFF);
	}

	return false; // this return value is never used
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



bool AAirMoveType::MoveToRepairPad() {
	CUnit* airBase = reservedPad->GetUnit();

	if (airBase->beingBuilt || airBase->stunned) {
		// pad became inoperable after being reserved
		DependentDied(airBase);
		return false;
	} else {
		const float3& relPadPos = airBase->script->GetPiecePos(reservedPad->GetPiece());
		const float3 absPadPos = airBase->pos +
			(airBase->frontdir * relPadPos.z) +
			(airBase->updir    * relPadPos.y) +
			(airBase->rightdir * relPadPos.x);

		if (padStatus == 0) {
			// approaching pad
			if (aircraftState != AIRCRAFT_FLYING && aircraftState != AIRCRAFT_TAKEOFF) {
				SetState(AIRCRAFT_FLYING);
			}

			goalPos = absPadPos;

			if (absPadPos.SqDistance2D(owner->pos) < (400.0f * 400.0f)) {
				padStatus = 1;
			}
		} else if (padStatus == 1) {
			// landing on pad
			if (aircraftState != AIRCRAFT_FLYING) {
				SetState(AIRCRAFT_FLYING);
			}

			goalPos = absPadPos;
			reservedLandingPos = absPadPos;
			wantedHeight = absPadPos.y - ground->GetHeightAboveWater(absPadPos.x, absPadPos.z);

			if (owner->pos.SqDistance(absPadPos) < 9.0f || aircraftState == AIRCRAFT_LANDED) {
				padStatus = 2;
			}
		} else {
			// taking off from pad
			if (aircraftState != AIRCRAFT_LANDED) {
				SetState(AIRCRAFT_LANDED);
			}

			owner->pos = absPadPos;
			owner->AddBuildPower(airBase->unitDef->buildSpeed / GAME_SPEED, airBase);
			owner->currentFuel += (owner->unitDef->maxFuel / (GAME_SPEED * owner->unitDef->refuelTime));
			owner->currentFuel = std::min(owner->unitDef->maxFuel, owner->currentFuel);

			if (owner->health >= owner->maxHealth - 1.0f && owner->currentFuel >= owner->unitDef->maxFuel) {
				// repaired and filled up, leave the pad
				airBaseHandler->LeaveLandingPad(reservedPad);
				reservedPad = NULL;
				padStatus = 0;
				goalPos = oldGoalPos;
				SetState(AIRCRAFT_TAKEOFF);
			}
		}
	}

	return true;
}
