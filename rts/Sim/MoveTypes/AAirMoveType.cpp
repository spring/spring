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

	CR_MEMBER(oldGoalPos),
	CR_MEMBER(reservedLandingPos),

	CR_MEMBER(landRadiusSq),
	CR_MEMBER(wantedHeight),
	CR_MEMBER(orgWantedHeight),

	CR_MEMBER(accRate),
	CR_MEMBER(decRate),
	CR_MEMBER(altitudeRate),

	CR_MEMBER(collide),
	CR_MEMBER(useSmoothMesh),
	CR_MEMBER(autoLand),

	CR_MEMBER(lastColWarning),

	CR_MEMBER(lastColWarningType)
))

AAirMoveType::AAirMoveType(CUnit* unit):
	AMoveType(unit),
	aircraftState(AIRCRAFT_LANDED),

	reservedLandingPos(-1.0f, -1.0f, -1.0f),

	landRadiusSq(0.0f),
	wantedHeight(80.0f),
	orgWantedHeight(0.0f),

	accRate(1.0f),
	decRate(1.0f),
	altitudeRate(3.0f),

	collide(true),
	useSmoothMesh(false),
	autoLand(true),

	lastColWarning(nullptr),

	lastColWarningType(0)
{
	// creg
	if (unit == nullptr)
		return;

	assert(owner->unitDef != nullptr);

	oldGoalPos = unit->pos;
	// same as {Ground, HoverAir}MoveType::accRate
	accRate = std::max(0.01f, unit->unitDef->maxAcc);
	decRate = std::max(0.01f, unit->unitDef->maxDec);
	altitudeRate = std::max(0.01f, unit->unitDef->verticalSpeed);
	landRadiusSq = Square(BrakingDistance(maxSpeed, decRate));

	useHeading = false;
}

AAirMoveType::~AAirMoveType()
{ }

bool AAirMoveType::UseSmoothMesh() const {
	if (!useSmoothMesh)
		return false;

	const bool onTransportMission =
		!owner->commandAI->commandQue.empty() &&
		((owner->commandAI->commandQue.front().GetID() == CMD_LOAD_UNITS) || (owner->commandAI->commandQue.front().GetID() == CMD_UNLOAD_UNIT));
	const bool forceDisableSmooth = onTransportMission || (aircraftState != AIRCRAFT_FLYING);
	return !forceDisableSmooth;
}

void AAirMoveType::DependentDied(CObject* o) {
	if (o == lastColWarning) {
		lastColWarning = NULL;
		lastColWarningType = 0;
	}
}

bool AAirMoveType::Update() {
	// NOTE: useHeading is never true by default for aircraft (AAirMoveType
	// forces it to false, TransportUnit::{Attach,Detach}Unit manipulate it
	// specifically for HoverAirMoveType's)
	if (useHeading) {
		SetState(AIRCRAFT_TAKEOFF);
	}

	// this return value is never used
	return (useHeading = false);
}

void AAirMoveType::UpdateLanded()
{
	// while an aircraft is being built we do not adjust its
	// position, because the builder might be a tall platform
	if (owner->beingBuilt)
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

void AAirMoveType::LandAt(float3 pos, float distanceSq)
{
	if (distanceSq < 0.0f)
		distanceSq = Square(BrakingDistance(maxSpeed, decRate));

	if (aircraftState != AIRCRAFT_LANDING)
		SetState(AIRCRAFT_LANDING);

	landRadiusSq = std::max(distanceSq, Square(std::max(owner->radius, 10.0f)));
	reservedLandingPos = pos;
	const float3 originalPos = owner->pos;
	owner->Move(reservedLandingPos, false);
	owner->Block();
	owner->Move(originalPos, false);

	const float gh = CGround::GetHeightReal(reservedLandingPos.x, reservedLandingPos.z);
	wantedHeight = reservedLandingPos.y - (owner->unitDef->canSubmerge ? gh : std::max(0.0f, gh));
}


void AAirMoveType::UpdateLandingHeight()
{
	const float gh = CGround::GetHeightReal(reservedLandingPos.x, reservedLandingPos.z);
	reservedLandingPos.y = wantedHeight + (owner->unitDef->canSubmerge ? gh : std::max(0.0f, gh));
}


void AAirMoveType::UpdateLanding()
{
	const float3& pos = owner->pos;

	const float radius = std::max(owner->radius, 10.0f);
	const float radiusSq = radius * radius;
	const float distSq = reservedLandingPos.SqDistance(pos);


	const float localAltitude = pos.y - (owner->unitDef->canSubmerge ?
		CGround::GetHeightReal(owner->pos.x, owner->pos.z):
		CGround::GetHeightAboveWater(owner->pos.x, owner->pos.z));

	if (distSq <= radiusSq || (distSq < landRadiusSq && localAltitude < wantedHeight + radius)) {
		SetState(AIRCRAFT_LANDED);
		owner->SetVelocityAndSpeed(UpVector * owner->speed);
	}
}

void AAirMoveType::SetWantedAltitude(float altitude)
{
	if (altitude == 0.0f) {
		wantedHeight = orgWantedHeight;
	} else {
		wantedHeight = altitude;
	}
}

void AAirMoveType::SetDefaultAltitude(float altitude)
{
	wantedHeight = altitude;
	orgWantedHeight = altitude;
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

	for (CUnit* unit: others) {
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

	for (CUnit* u: others) {
		if (u == owner)
			continue;
		if ((u->midPos - pos).SqLength() < (dist * dist)) {
			lastColWarning = u;
		}
	}

	if (lastColWarning != NULL) {
		lastColWarningType = 1;
		AddDeathDependence(lastColWarning, DEPENDENCE_LASTCOLWARN);
	}
}
