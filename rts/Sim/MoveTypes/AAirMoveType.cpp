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

CR_BIND_DERIVED_INTERFACE(AAirMoveType, AMoveType)

CR_REG_METADATA(AAirMoveType, (
	CR_MEMBER(aircraftState),
	CR_MEMBER(padStatus),

	CR_MEMBER(oldGoalPos),
	CR_MEMBER(reservedLandingPos),

	CR_MEMBER(wantedHeight),
	CR_MEMBER(orgWantedHeight),

	CR_MEMBER(accRate),
	CR_MEMBER(decRate),
	CR_MEMBER(altitudeRate),

	CR_MEMBER(collide),
	CR_MEMBER(useSmoothMesh),
	CR_MEMBER(autoLand),

	CR_MEMBER(lastColWarning),
	CR_MEMBER(reservedPad),

	CR_MEMBER(lastColWarningType),
	CR_MEMBER(lastFuelUpdateFrame)
))

AAirMoveType::AAirMoveType(CUnit* unit):
	AMoveType(unit),
	aircraftState(AIRCRAFT_LANDED),
	padStatus(PAD_STATUS_FLYING),

	reservedLandingPos(-1.0f, -1.0f, -1.0f),

	wantedHeight(80.0f),
	orgWantedHeight(0.0f),

	accRate(1.0f),
	decRate(1.0f),
	altitudeRate(3.0f),

	collide(true),
	useSmoothMesh(false),
	autoLand(true),

	lastColWarning(NULL),
	reservedPad(NULL),

	lastColWarningType(0),
	lastFuelUpdateFrame(gs->frameNum)
{
	assert(unit != NULL);

	oldGoalPos = unit->pos;
	// same as {Ground, HoverAir}MoveType::accRate
	accRate = std::max(0.01f, unit->unitDef->maxAcc);
	decRate = std::max(0.01f, unit->unitDef->maxDec);
	altitudeRate = std::max(0.01f, unit->unitDef->verticalSpeed);

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
		if (aircraftState != AIRCRAFT_CRASHING) {
			// change state only if not crashing
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
		SetState(AIRCRAFT_TAKEOFF);
	}

	if (owner->beingBuilt) {
		// while being built, MoveType::SlowUpdate is not
		// called so UpdateFuel will not be either --> do
		// it here to prevent a drop in fuel level as soon
		// as unit finishes construction
		UpdateFuel(false);
	}

	// this return value is never used
	return (useHeading = false);
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
		CGround::GetHeightReal(owner->pos.x, owner->pos.z):
		CGround::GetHeightAboveWater(owner->pos.x, owner->pos.z);
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

	owner->SetVelocityAndSpeed(owner->speed + owner->GetDragAccelerationVec(float4(0.0f, 0.0f, 0.0f, 0.1f)));
	owner->Move(UpVector * (std::max(curHeight, minHeight) - owner->pos.y), true);
	owner->Move(owner->speed, true);
	// match the terrain normal
	owner->UpdateDirVectors(owner->IsOnGround());
	owner->UpdateMidAndAimPos();
}

void AAirMoveType::UpdateFuel(bool slowUpdate) {
	if (owner->unitDef->maxFuel <= 0.0f)
		return;

	// lastFuelUpdateFrame must be updated even when early-outing
	// otherwise fuel level will jump after a period of not using
	// any (eg. when on a pad or after being built)
	if (!slowUpdate) {
		lastFuelUpdateFrame = gs->frameNum;
		return;
	}
	if (aircraftState == AIRCRAFT_LANDED) {
		lastFuelUpdateFrame = gs->frameNum;
		return;
	}
	if (padStatus == PAD_STATUS_ARRIVED) {
		lastFuelUpdateFrame = gs->frameNum;
		return;
	}

	owner->currentFuel = std::max(0.0f, owner->currentFuel - (float(gs->frameNum - lastFuelUpdateFrame) / GAME_SPEED));
	lastFuelUpdateFrame = gs->frameNum;
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


bool AAirMoveType::CanLandOnPad(const float3& padPos) const {
	// once distance to pad becomes smaller than current braking distance, switch states
	// (but do not allow state-switch until the aircraft is heading ~directly toward pad)
	// braking distance is 0.5*a*t*t where t is v/a --> 0.5*a*((v*v)/(a*a)) --> 0.5*v*v*(1/a)
	// FIXME:
	//   apply N-frame lookahead when deciding to switch state for strafing aircraft
	//   (see comments in StrafeAirMoveType::UpdateLanding, overshooting prevention)
	//   the lookahead is roughly based on the time to descend to pad-target altitude
	const float3 flatFrontDir = (float3(owner->frontdir)).Normalize2D();

	const float brakingDistSq = Square(0.5f * owner->speed.SqLength2D() / decRate);
	const float descendDistSq = Square(maxSpeed * ((owner->pos.y - padPos.y) / altitudeRate)) * owner->unitDef->IsStrafingAirUnit();

	if (padPos.SqDistance2D(owner->pos) > std::max(brakingDistSq, descendDistSq))
		return false;

	if (owner->unitDef->IsStrafingAirUnit()) {
		// horizontal guide
		if (flatFrontDir.dot((padPos - owner->pos).SafeNormalize2D()) < 0.985f)
			return false;
		// vertical guide
		if (flatFrontDir.dot((padPos - owner->pos).SafeNormalize()) < 0.707f)
			return false;
	}

	return true;
}

bool AAirMoveType::HaveLandedOnPad(const float3& padPos) {
	const AircraftState landingState = GetLandingState();
	const float landingPadHeight = CGround::GetHeightAboveWater(padPos.x, padPos.z);

	reservedLandingPos = padPos;
	wantedHeight = padPos.y - landingPadHeight;

	const float3 padVector = (padPos - owner->pos).SafeNormalize2D();
	const float curOwnerAltitude = (owner->pos.y - landingPadHeight);
	const float minOwnerAltitude = (wantedHeight + 1.0f);

	if (aircraftState != landingState)
		SetState(landingState);
	if (aircraftState == AIRCRAFT_LANDED)
		return true;

	if (curOwnerAltitude <= minOwnerAltitude) {
		// deal with overshooting aircraft: "tractor" them in
		// once they descend down to the landing pad altitude
		// this is a no-op for HAMT planes which do not apply
		// speed updates at this point
		owner->SetVelocityAndSpeed((owner->speed + (padVector * accRate)) * XZVector);
	}

	if (Square(owner->rightdir.y) < 0.01f && owner->frontdir.dot(padVector) > 0.01f) {
		owner->frontdir = padVector;
		owner->rightdir = owner->frontdir.cross(UpVector);
		owner->updir    = owner->rightdir.cross(owner->frontdir);

		owner->SetHeadingFromDirection();
	}

	if (curOwnerAltitude > minOwnerAltitude)
		return false;

	return ((owner->pos.SqDistance2D(padPos) <= 1.0f || owner->unitDef->IsHoveringAirUnit()));
}

bool AAirMoveType::MoveToRepairPad() {
	CUnit* airBase = reservedPad->GetUnit();

	if (airBase->beingBuilt || airBase->IsStunned()) {
		// pad became inoperable after being reserved
		DependentDied(airBase);
		return false;
	} else {
		const float3& relPadPos = airBase->script->GetPiecePos(reservedPad->GetPiece());
		const float3 absPadPos = airBase->GetObjectSpacePos(relPadPos);

		switch (padStatus) {
			case PAD_STATUS_FLYING: {
				// flying toward pad
				if (aircraftState != AIRCRAFT_FLYING && aircraftState != AIRCRAFT_TAKEOFF) {
					SetState(AIRCRAFT_FLYING);
				}

				if (CanLandOnPad(goalPos = absPadPos)) {
					padStatus = PAD_STATUS_LANDING;
				}
			} break;

			case PAD_STATUS_LANDING: {
				// landing on pad
				if (HaveLandedOnPad(goalPos = absPadPos)) {
					padStatus = PAD_STATUS_ARRIVED;
				}
			} break;

			case PAD_STATUS_ARRIVED: {
				if (aircraftState != AIRCRAFT_LANDED) {
					SetState(AIRCRAFT_LANDED);
				}

				owner->SetVelocityAndSpeed(ZeroVector);
				owner->Move(absPadPos, false);
				owner->UpdateMidAndAimPos(); // needed here?
				owner->AddBuildPower(airBase, airBase->unitDef->buildSpeed / GAME_SPEED);

				owner->currentFuel += (owner->unitDef->maxFuel / (GAME_SPEED * owner->unitDef->refuelTime));
				owner->currentFuel = std::min(owner->unitDef->maxFuel, owner->currentFuel);

				if (owner->health >= owner->maxHealth - 1.0f && owner->currentFuel >= owner->unitDef->maxFuel) {
					// repaired and filled up, leave the pad
					UnreservePad(reservedPad);
				}
			} break;
		}
	}

	return true;
}
