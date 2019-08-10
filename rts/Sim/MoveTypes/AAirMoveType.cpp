/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AAirMoveType.h"

#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Rendering/Env/Particles/Classes/SmokeProjectile.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "System/SpringMath.h"

CR_BIND_DERIVED_INTERFACE(AAirMoveType, AMoveType)

CR_REG_METADATA(AAirMoveType, (
	CR_MEMBER(aircraftState),
	CR_MEMBER(collisionState),

	CR_MEMBER(oldGoalPos),
	CR_MEMBER(reservedLandingPos),

	CR_MEMBER(landRadiusSq),
	CR_MEMBER(wantedHeight),
	CR_MEMBER(orgWantedHeight),

	CR_MEMBER(accRate),
	CR_MEMBER(decRate),
	CR_MEMBER(altitudeRate),

	CR_MEMBER(collide),
	CR_MEMBER(autoLand),
	CR_MEMBER(dontLand),
	CR_MEMBER(useSmoothMesh),
	CR_MEMBER(canSubmerge),
	CR_MEMBER(floatOnWater),

	CR_MEMBER(lastCollidee),

	CR_MEMBER(crashExpGenID)
))



static inline float AAMTGetGroundHeightAW(float x, float z) { return CGround::GetHeightAboveWater(x, z); }
static inline float AAMTGetGroundHeight  (float x, float z) { return CGround::GetHeightReal      (x, z); }
static inline float AAMTGetSmoothGroundHeightAW(float x, float z) { return smoothGround.GetHeightAboveWater(x, z); }
static inline float AAMTGetSmoothGroundHeight  (float x, float z) { return smoothGround.GetHeight          (x, z); }
static inline float HAMTGetMaxGroundHeight(float x, float z) { return std::max(smoothGround.GetHeight(x, z), CGround::GetApproximateHeight(x, z)); }
static inline float SAMTGetMaxGroundHeight(float x, float z) { return std::max(smoothGround.GetHeight(x, z), CGround::GetHeightAboveWater(x, z)); }

static inline void AAMTEmitEngineTrail(CUnit* owner, unsigned int) {
	projMemPool.alloc<CSmokeProjectile>(owner, owner->midPos, guRNG.NextVector() * 0.08f, (100.0f + guRNG.NextFloat() * 50.0f), 5.0f, 0.2f, 0.4f);
}
static inline void AAMTEmitCustomTrail(CUnit* owner, unsigned int id) {
	explGenHandler.GenExplosion(id, owner->midPos, owner->frontdir, 1.0f, 0.0f, 1.0f, owner, nullptr);
}


AAirMoveType::GetGroundHeightFunc amtGetGroundHeightFuncs[6] = {
	AAMTGetGroundHeightAW,       // canSubmerge=0 useSmoothMesh=0
	AAMTGetGroundHeight  ,       // canSubmerge=1 useSmoothMesh=0
	AAMTGetSmoothGroundHeightAW, // canSubmerge=0 useSmoothMesh=1
	AAMTGetSmoothGroundHeight,   // canSubmerge=1 useSmoothMesh=1
	HAMTGetMaxGroundHeight,      // HoverAirMoveType::UpdateFlying
	SAMTGetMaxGroundHeight,      // StrafeAirMoveType::UpdateFlying
};

AAirMoveType::EmitCrashTrailFunc amtEmitCrashTrailFuncs[2] = {
	AAMTEmitEngineTrail,
	AAMTEmitCustomTrail,
};



AAirMoveType::AAirMoveType(CUnit* unit): AMoveType(unit)
{
	// creg
	if (unit == nullptr)
		return;

	const UnitDef* ud = owner->unitDef;

	oldGoalPos = unit->pos;

	// same as {Ground, HoverAir}MoveType::accRate
	accRate = std::max(0.01f, ud->maxAcc);
	decRate = std::max(0.01f, ud->maxDec);
	altitudeRate = std::max(0.01f, ud->verticalSpeed);
	landRadiusSq = Square(BrakingDistance(maxSpeed, decRate));

	collide = ud->collide;
	dontLand = ud->DontLand();
	useSmoothMesh = ud->useSmoothMesh;
	canSubmerge = ud->canSubmerge;
	floatOnWater = ud->floatOnWater;

	UseHeading(false);

	if (ud->GetCrashExpGenCount() > 0) {
		crashExpGenID = guRNG.NextInt(ud->GetCrashExpGenCount());
		crashExpGenID = ud->GetCrashExpGenID(crashExpGenID);
	}
}


bool AAirMoveType::UseSmoothMesh() const {
	if (!useSmoothMesh)
		return false;

	const CCommandAI* cai = owner->commandAI;
	const CCommandQueue& cq = cai->commandQue;
	const Command& fc = (cq.empty())? Command(CMD_STOP): cq.front();

	const bool closeGoalPos = (goalPos.SqDistance2D(owner->pos) < Square(landRadiusSq * 2.0f));
	const bool transportCmd = ((fc.GetID() == CMD_LOAD_UNITS) || (fc.GetID() == CMD_UNLOAD_UNIT));
	const bool forceDisable = ((transportCmd && closeGoalPos) || (aircraftState != AIRCRAFT_FLYING && aircraftState != AIRCRAFT_HOVERING));

	return !forceDisable;
}

void AAirMoveType::DependentDied(CObject* o) {
	if (o == lastCollidee) {
		lastCollidee = nullptr;
		collisionState = COLLISION_NOUNIT;
	}
}

bool AAirMoveType::Update() {
	// NOTE: useHeading is never true by default for aircraft (AAirMoveType
	// forces it to false, while only CUnit::{Attach,Detach}Unit manipulate
	// it specifically for HoverAirMoveType's)
	if (UseHeading()) {
		SetState(AIRCRAFT_TAKEOFF);
		UseHeading(false);
	}

	// prevent UnitMoved event spam
	return false;
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
	const float minHeight = amtGetGroundHeightFuncs[owner->unitDef->canSubmerge](owner->pos.x, owner->pos.z);
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
	owner->UpdateDirVectors(owner->IsOnGround(), false);
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

	wantedHeight = reservedLandingPos.y - amtGetGroundHeightFuncs[owner->unitDef->canSubmerge](reservedLandingPos.x, reservedLandingPos.z);
}


void AAirMoveType::UpdateLandingHeight(float newWantedHeight)
{
	wantedHeight = newWantedHeight;
	reservedLandingPos.y = wantedHeight + amtGetGroundHeightFuncs[owner->unitDef->canSubmerge](reservedLandingPos.x, reservedLandingPos.z);
}


void AAirMoveType::UpdateLanding()
{
	const float3& pos = owner->pos;

	const float radius = std::max(owner->radius, 10.0f);
	const float radiusSq = radius * radius;
	const float distSq = reservedLandingPos.SqDistance(pos);


	const float localAltitude = pos.y - amtGetGroundHeightFuncs[owner->unitDef->canSubmerge](owner->pos.x, owner->pos.z);

	if (distSq <= radiusSq || (distSq < landRadiusSq && localAltitude < wantedHeight + radius)) {
		SetState(AIRCRAFT_LANDED);
		owner->SetVelocityAndSpeed(UpVector * owner->speed);
	}
}


void AAirMoveType::CheckForCollision()
{
	if (!collide)
		return;

	const SyncedFloat3& pos = owner->midPos;
	const SyncedFloat3& forward = owner->frontdir;

	float dist = 200.0f;

	QuadFieldQuery qfQuery;
	quadField.GetUnitsExact(qfQuery, pos + forward * 121.0f, dist);

	if (lastCollidee != nullptr) {
		DeleteDeathDependence(lastCollidee, DEPENDENCE_LASTCOLWARN);

		lastCollidee = nullptr;
		collisionState = COLLISION_NOUNIT;
	}

	// find closest potential collidee
	for (CUnit* unit: *qfQuery.units) {
		if (unit == owner || !unit->unitDef->canfly)
			continue;

		const SyncedFloat3& op = unit->midPos;
		const float3 dif = op - pos;
		const float3 forwardDif = forward * (forward.dot(dif));

		if (forwardDif.SqLength() >= (dist * dist))
			continue;

		const float3 ortoDif = dif - forwardDif;
		const float frontLength = forwardDif.Length();
		// note: radii are multiplied by two
		const float minOrtoDif = (unit->radius + owner->radius) * 2.0f + frontLength * 0.1f + 10.0f;

		if (ortoDif.SqLength() < (minOrtoDif * minOrtoDif)) {
			dist = frontLength;
			lastCollidee = const_cast<CUnit*>(unit);
		}
	}

	if (lastCollidee != nullptr) {
		collisionState = COLLISION_DIRECT;
		AddDeathDependence(lastCollidee, DEPENDENCE_LASTCOLWARN);
		return;
	}

	for (CUnit* u: *qfQuery.units) {
		if (u == owner)
			continue;

		if ((u->midPos - pos).SqLength() > Square((owner->radius + u->radius) * 2.0f))
			continue;

		lastCollidee = u;
	}

	if (lastCollidee != nullptr) {
		collisionState = COLLISION_NEARBY;
		AddDeathDependence(lastCollidee, DEPENDENCE_LASTCOLWARN);
		return;
	}
}
