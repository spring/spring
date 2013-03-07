/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AAirMoveType.h"
#include "MoveMath/MoveMath.h"

#include "Map/Ground.h"
#include "Map/MapInfo.h"
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
	CR_MEMBER(orgWantedHeight),

	CR_MEMBER(collide),
	CR_MEMBER(useSmoothMesh),
	CR_MEMBER(autoLand),

	CR_MEMBER(lastColWarning),
	CR_MEMBER(reservedPad),

	CR_MEMBER(lastColWarningType),
	CR_MEMBER(lastFuelUpdateFrame),

	CR_RESERVED(16)
));

AAirMoveType::AAirMoveType(CUnit* unit):
	AMoveType(unit),
	aircraftState(AIRCRAFT_LANDED),
	padStatus(PAD_STATUS_FLYING),

	oldGoalPos(owner? owner->pos : ZeroVector),
	reservedLandingPos(-1.0f, -1.0f, -1.0f),

	wantedHeight(80.0f),
	orgWantedHeight(0.0f),

	collide(true),
	useSmoothMesh(false),
	autoLand(true),

	lastColWarning(NULL),
	reservedPad(NULL),

	lastColWarningType(0),
	lastFuelUpdateFrame(gs->frameNum)
{
	useHeading = false;
}

AAirMoveType::~AAirMoveType()
{
	// NOTE:
	//   this calls Takeoff and (indirectly) SetState,
	//   so neither of these must be pure virtuals (!)
	UnreservePad(reservedPad);
}

bool AAirMoveType::UseSmoothMesh() const {
	if (!useSmoothMesh)
		return false;

	const bool onTransportMission =
		!owner->commandAI->commandQue.empty() &&
		((owner->commandAI->commandQue.front().GetID() == CMD_LOAD_UNITS) || (owner->commandAI->commandQue.front().GetID() == CMD_UNLOAD_UNIT));
	const bool repairing = reservedPad ? padStatus >= PAD_STATUS_LANDING : false;
	const bool forceDisableSmooth = repairing || onTransportMission || (aircraftState != AIRCRAFT_FLYING);
	return !forceDisableSmooth;
}


void AAirMoveType::ReservePad(CAirBaseHandler::LandingPad* lp) {
	oldGoalPos = goalPos;
	orgWantedHeight = wantedHeight;

	assert(reservedPad == NULL);

	AddDeathDependence(lp, DEPENDENCE_LANDINGPAD);
	SetGoal(lp->GetUnit()->pos);

	reservedPad = lp;
	padStatus = PAD_STATUS_FLYING;

	Takeoff();
}

void AAirMoveType::UnreservePad(CAirBaseHandler::LandingPad* lp)
{
	if (lp == NULL)
		return;

	assert(reservedPad == lp);

	DeleteDeathDependence(reservedPad, DEPENDENCE_LANDINGPAD);
	airBaseHandler->LeaveLandingPad(reservedPad);

	reservedPad = NULL;
	padStatus = PAD_STATUS_FLYING;

	goalPos = oldGoalPos;
	wantedHeight = orgWantedHeight;
	SetState(AIRCRAFT_TAKEOFF);
}

void AAirMoveType::DependentDied(CObject* o) {
	if (o == lastColWarning) {
		lastColWarning = NULL;
		lastColWarningType = 0;
	}

	if (o == reservedPad) {
		if (aircraftState!=AIRCRAFT_CRASHING) { //don't change state when crashing
			SetState(AIRCRAFT_TAKEOFF);

			goalPos = oldGoalPos;
			wantedHeight = orgWantedHeight;
			padStatus = PAD_STATUS_FLYING;
		}
		reservedPad = NULL;
	}
}

bool AAirMoveType::Update() {
	// NOTE: useHeading is never true by default for aircraft (AAirMoveType
	// forces it to false, TransportUnit::{Attach,Detach}Unit manipulate it
	// specifically for HoverAirMoveType's)
	if (useHeading) {
		useHeading = false;
		SetState(AIRCRAFT_TAKEOFF);
	}

	// this return value is never used
	return false;
}

void AAirMoveType::UpdateLanded()
{
	// while an aircraft is being built we do not adjust its
	// position, because the builder might be a tall platform
	// we also do nothing if the aircraft is preparing to land
	// or has already landed on a repair-pad
	if (owner->beingBuilt)
		return;
	if (padStatus != PAD_STATUS_FLYING)
		return;

	// when an aircraft transitions to the landed state it
	// is on the ground, but the terrain can be raised (or
	// lowered) later and we do not want to end up hovering
	// in mid-air or sink below it
	// let gravity do the job instead of teleporting
	const float minHeight = owner->unitDef->canSubmerge?
		ground->GetHeightReal(owner->pos.x, owner->pos.z):
		ground->GetHeightAboveWater(owner->pos.x, owner->pos.z);
	const float curHeight = owner->pos.y;

	if (curHeight > minHeight) {
		if (curHeight > 0.0f) {
			owner->speed.y += mapInfo->map.gravity;
		} else {
			owner->speed.y = mapInfo->map.gravity;
		}
	} else {
		owner->speed.y = 0.0f;
	}

	owner->speed.x = 0.0f;
	owner->speed.z = 0.0f;

	owner->Move1D(std::max(curHeight, minHeight), 1, false);
	owner->Move3D(owner->speed, true);
	// match the terrain normal
	owner->UpdateDirVectors(true);
	owner->UpdateMidAndAimPos();
}

void AAirMoveType::UpdateFuel() {
	if (owner->unitDef->maxFuel > 0.0f) {
		if (aircraftState != AIRCRAFT_LANDED)
			owner->currentFuel = std::max(0.0f, owner->currentFuel - (float(gs->frameNum - lastFuelUpdateFrame) / GAME_SPEED));

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
	const std::vector<CUnit*>& others = quadField->GetUnitsExact(midTestPos, 115.0f);

	float dist = 200.0f;

	if (lastColWarning) {
		DeleteDeathDependence(lastColWarning, DEPENDENCE_LASTCOLWARN);
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
		AddDeathDependence(lastColWarning, DEPENDENCE_LASTCOLWARN);
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
		AddDeathDependence(lastColWarning, DEPENDENCE_LASTCOLWARN);
	}
}



bool AAirMoveType::MoveToRepairPad() {
	CUnit* airBase = reservedPad->GetUnit();

	if (airBase->beingBuilt || airBase->IsStunned()) {
		// pad became inoperable after being reserved
		DependentDied(airBase);
		return false;
	} else {
		const float3& relPadPos = airBase->script->GetPiecePos(reservedPad->GetPiece());
		const float3 absPadPos = airBase->pos +
			(airBase->frontdir * relPadPos.z) +
			(airBase->updir    * relPadPos.y) +
			(airBase->rightdir * relPadPos.x);

		if (padStatus == PAD_STATUS_FLYING) {
			if (aircraftState != AIRCRAFT_FLYING && aircraftState != AIRCRAFT_TAKEOFF) {
				SetState(AIRCRAFT_FLYING);
			}

			goalPos = absPadPos;

			if (absPadPos.SqDistance2D(owner->pos) < (400.0f * 400.0f)) {
				padStatus = PAD_STATUS_LANDING;
			}
		} else if (padStatus == PAD_STATUS_LANDING) {
			// landing on pad
			const AircraftState landingState = GetLandingState();
			if (aircraftState != landingState)
				SetState(landingState);

			goalPos = absPadPos;
			reservedLandingPos = absPadPos;
			wantedHeight = absPadPos.y - ground->GetHeightAboveWater(absPadPos.x, absPadPos.z);

			const float curPadDistanceSq = owner->midPos.SqDistance(absPadPos);
			const float minPadDistanceSq = owner->radius * owner->radius;

			if (curPadDistanceSq < minPadDistanceSq || aircraftState == AIRCRAFT_LANDED) {
				padStatus = PAD_STATUS_ARRIVED;
				owner->speed = ZeroVector;
			}
		} else {
			// taking off from pad
			if (aircraftState != AIRCRAFT_LANDED) {
				SetState(AIRCRAFT_LANDED);
			}

			owner->pos = absPadPos;

			owner->UpdateMidAndAimPos(); // needed here?
			owner->AddBuildPower(airBase->unitDef->buildSpeed / GAME_SPEED, airBase);

			owner->currentFuel += (owner->unitDef->maxFuel / (GAME_SPEED * owner->unitDef->refuelTime));
			owner->currentFuel = std::min(owner->unitDef->maxFuel, owner->currentFuel);

			if (owner->health >= owner->maxHealth - 1.0f && owner->currentFuel >= owner->unitDef->maxFuel) {
				// repaired and filled up, leave the pad
				UnreservePad(reservedPad);
			}
		}
	}

	return true;
}
