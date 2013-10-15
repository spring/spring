/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "HoverAirMoveType.h"
#include "Game/Players/Player.h"
#include "Map/Ground.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Sim/Projectiles/Unsynced/SmokeProjectile.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "System/myMath.h"
#include "System/Matrix44f.h"


CR_BIND_DERIVED(CHoverAirMoveType, AAirMoveType, (NULL));

CR_REG_METADATA(CHoverAirMoveType, (
	CR_MEMBER(bankingAllowed),
	CR_MEMBER(airStrafe),
	CR_MEMBER(wantToStop),

	CR_MEMBER(goalDistance),

	CR_MEMBER(currentBank),
	CR_MEMBER(currentPitch),

	CR_MEMBER(turnRate),
	CR_MEMBER(maxDrift),
	CR_MEMBER(maxTurnAngle),

	CR_MEMBER(wantedSpeed),
	CR_MEMBER(deltaSpeed),

	CR_MEMBER(circlingPos),
	CR_MEMBER(randomWind),

	CR_MEMBER(forceHeading),
	CR_MEMBER(dontLand),
	CR_MEMBER(loadingUnits),

	CR_MEMBER(wantedHeading),
	CR_MEMBER(forceHeadingTo),

	CR_MEMBER(waitCounter),
	CR_MEMBER(lastMoveRate),

	CR_RESERVED(32)
));


CHoverAirMoveType::CHoverAirMoveType(CUnit* owner) :
	AAirMoveType(owner),
	flyState(FLY_CRUISING),

	bankingAllowed(true),
	airStrafe(owner->unitDef->airStrafe),
	wantToStop(false),

	goalDistance(1),

	// we want to take off in direction of factory facing
	currentBank(0),
	currentPitch(0),

	turnRate(1),
	maxDrift(1.0f),
	maxTurnAngle(math::cos(owner->unitDef->turnInPlaceAngleLimit * (PI / 180.0f)) * -1.0f),

	wantedSpeed(ZeroVector),
	deltaSpeed(ZeroVector),
	circlingPos(ZeroVector),
	randomWind(ZeroVector),

	forceHeading(false),
	dontLand(false),
	loadingUnits(false),

	wantedHeading(GetHeadingFromFacing(owner->buildFacing)),
	forceHeadingTo(wantedHeading),

	waitCounter(0),
	lastMoveRate(0)
{
	assert(owner != NULL);
	assert(owner->unitDef != NULL);

	turnRate = owner->unitDef->turnRate;

	wantedHeight = owner->unitDef->wantedHeight + gs->randFloat() * 5.0f;
	orgWantedHeight = wantedHeight;
	dontLand = owner->unitDef->DontLand();
	collide = owner->unitDef->collide;
	bankingAllowed = owner->unitDef->bankingAllowed;
	useSmoothMesh = owner->unitDef->useSmoothMesh;

	// prevent weapons from being updated and firing while on the ground
	owner->dontUseWeapons = true;
}


void CHoverAirMoveType::SetGoal(const float3& pos, float distance)
{
	goalPos = pos;
	oldGoalPos = pos;

	// aircraft need some marginals to avoid uber stacking
	// when lots of them are ordered to one place
	maxDrift = std::max(16.0f, distance);
	wantedHeight = orgWantedHeight;

	forceHeading = false;
}

void CHoverAirMoveType::SetState(AircraftState newState)
{
	// once in crashing, we should never change back into another state
	if (aircraftState == AIRCRAFT_CRASHING && newState != AIRCRAFT_CRASHING) {
		return;
	}

	if (newState == aircraftState) {
		return;
	}

	if (aircraftState == AIRCRAFT_LANDED) {
		assert(newState != AIRCRAFT_LANDING); // redundant SetState() call, we already landed and get command to switch into landing
	}

	if (newState == AIRCRAFT_LANDED) {
		owner->dontUseWeapons = true;
		owner->useAirLos = false;
	} else {
		owner->dontUseWeapons = false;
		owner->useAirLos = true;
	}

	aircraftState = newState;

	switch (aircraftState) {
		case AIRCRAFT_CRASHING:
			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_CRASHING);
			break;
		case AIRCRAFT_LANDED:
			// FIXME already inform commandAI in AIRCRAFT_LANDING!
			owner->commandAI->StopMove();

			owner->Block();
			owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
			break;
		case AIRCRAFT_LANDING:
			owner->Deactivate();
			break;
		case AIRCRAFT_HOVERING:
			wantedHeight = orgWantedHeight;
			wantedSpeed = ZeroVector;
			// fall through
		default:
			owner->Activate();
			owner->UnBlock();
			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);

			reservedLandingPos.x = -1.0f;
			break;
	}

	// Cruise as default
	if (aircraftState == AIRCRAFT_FLYING || aircraftState == AIRCRAFT_HOVERING) {
		flyState = FLY_CRUISING;
	}

	owner->UpdatePhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING, (aircraftState != AIRCRAFT_LANDED));
	waitCounter = 0;
}

void CHoverAirMoveType::SetAllowLanding(bool allowLanding)
{
	dontLand = !allowLanding;

	if (CanLand())
		return;

	if (aircraftState != AIRCRAFT_LANDED && aircraftState != AIRCRAFT_LANDING)
		return;

	// do not start hovering if still loading units
	if (loadingUnits)
		return;

	SetState(AIRCRAFT_HOVERING);
}

void CHoverAirMoveType::StartMoving(float3 pos, float goalRadius)
{
	forceHeading = false;
	wantToStop = false;
	waitCounter = 0;

	owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING);

	switch (aircraftState) {
		case AIRCRAFT_LANDED:
			SetState(AIRCRAFT_TAKEOFF);
			break;
		case AIRCRAFT_TAKEOFF:
			SetState(AIRCRAFT_TAKEOFF);
			break;
		case AIRCRAFT_FLYING:
			if (flyState != FLY_CRUISING)
				flyState = FLY_CRUISING;
			break;
		case AIRCRAFT_LANDING:
			SetState(AIRCRAFT_TAKEOFF);
			break;
		case AIRCRAFT_HOVERING:
			SetState(AIRCRAFT_FLYING);
			break;
		case AIRCRAFT_CRASHING:
			break;
	}

	SetGoal(pos, goalRadius);
	progressState = AMoveType::Active;
}

void CHoverAirMoveType::StartMoving(float3 pos, float goalRadius, float speed)
{
	StartMoving(pos, goalRadius);
}

void CHoverAirMoveType::KeepPointingTo(float3 pos, float distance, bool aggressive)
{
	wantToStop = false;
	forceHeading = false;
	wantedHeight = orgWantedHeight;
	// close in a little to avoid the command AI to override the pos constantly
	distance -= 15;

	// Ignore the exact same order
	if ((aircraftState == AIRCRAFT_FLYING)
			&& (flyState == FLY_CIRCLING || flyState == FLY_ATTACKING)
			&& ((circlingPos - pos).SqLength2D() < 64)
			&& (goalDistance == distance))
	{
		return;
	}

	circlingPos = pos;
	goalDistance = distance;
	goalPos = owner->pos;

	SetState(AIRCRAFT_FLYING);

	if (aggressive) {
		flyState = FLY_ATTACKING;
	} else {
		flyState = FLY_CIRCLING;
	}
}

void CHoverAirMoveType::ExecuteStop()
{
	wantToStop = false;
	wantedSpeed = ZeroVector;

	switch (aircraftState) {
		case AIRCRAFT_TAKEOFF: {
			if (CanLand()) {
				SetState(AIRCRAFT_LANDING);
				// trick to land directly
				waitCounter = GAME_SPEED;
				break;
			}
		} // fall through
		case AIRCRAFT_FLYING: {
			goalPos = owner->pos;

			if (CanLand()) {
				SetState(AIRCRAFT_LANDING);
			} else {
				SetState(AIRCRAFT_HOVERING);
			}
		} break;

		case AIRCRAFT_LANDING: {} break;
		case AIRCRAFT_LANDED: {} break;
		case AIRCRAFT_CRASHING: {} break;

		case AIRCRAFT_HOVERING: {
			if (CanLand()) {
				// land immediately
				SetState(AIRCRAFT_LANDING);
				waitCounter = GAME_SPEED;
			}
		} break;
	}
}

void CHoverAirMoveType::StopMoving(bool callScript, bool hardStop)
{
	// transports switch to landed state (via SetState which calls
	// us) during pickup but must *not* be allowed to change their
	// heading while "landed" (see TransportCAI)
	forceHeading &= (aircraftState == AIRCRAFT_LANDED);
	wantToStop = true;
	wantedHeight = orgWantedHeight;

	owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING);

	if (progressState != AMoveType::Failed)
		progressState = AMoveType::Done;
}



void CHoverAirMoveType::UpdateLanded()
{
	AAirMoveType::UpdateLanded();

	if (progressState != AMoveType::Failed)
		progressState = AMoveType::Done;
}

void CHoverAirMoveType::UpdateTakeoff()
{
	const float3& pos = owner->pos;

	wantedSpeed = ZeroVector;
	wantedHeight = orgWantedHeight;

	UpdateAirPhysics();

	const float altitude = owner->unitDef->canSubmerge?
		(pos.y - ground->GetHeightReal(pos.x, pos.z)):
		(pos.y - ground->GetHeightAboveWater(pos.x, pos.z));

	if (altitude > orgWantedHeight * 0.8f) {
		SetState(AIRCRAFT_FLYING);
	}
}


// Move the unit around a bit..
void CHoverAirMoveType::UpdateHovering()
{
	#define NOZERO(x) std::max(x, 0.0001f)

	float3 deltaVec = goalPos - owner->pos;
	float3 deltaDir = float3(deltaVec.x, 0.0f, deltaVec.z);

	const float driftSpeed = math::fabs(owner->unitDef->dlHoverFactor);
	const float goalDistance = NOZERO(deltaDir.Length2D());
	const float brakeDistance = 0.5f * owner->speed.SqLength2D() / decRate;

	// move towards goal position if it's not immediately
	// behind us when we have more waypoints to get to
	// *** this behavior interferes with the loading procedure of transports ***
	const bool b0 = (aircraftState != AIRCRAFT_LANDING && owner->commandAI->HasMoreMoveCommands());
	const bool b1 = (goalDistance < brakeDistance && goalDistance > 1.0f);

	if (b0 && b1 && dynamic_cast<CTransportUnit*>(owner) == NULL) {
		deltaDir = owner->frontdir;
	} else {
		deltaDir *= smoothstep(0.0f, 20.0f, goalDistance) / goalDistance;
		deltaDir -= owner->speed;
	}

	if (deltaDir.SqLength2D() > (maxSpeed * maxSpeed)) {
		deltaDir *= (maxSpeed / NOZERO(math::sqrt(deltaDir.SqLength2D())));
	}

	// random movement (a sort of fake wind effect)
	// random drift values are in range -0.5 ... 0.5
	randomWind.x = randomWind.x * 0.9f + (gs->randFloat() - 0.5f) * 0.5f;
	randomWind.z = randomWind.z * 0.9f + (gs->randFloat() - 0.5f) * 0.5f;

	wantedSpeed = owner->speed + deltaDir;
	wantedSpeed += (randomWind * driftSpeed * 0.5f);

	UpdateAirPhysics();
}


void CHoverAirMoveType::UpdateFlying()
{
	const float3& pos = owner->pos;
	// const float4& spd = owner->speed;

	// Direction to where we would like to be
	float3 goalVec = goalPos - pos;

	owner->restTime = 0;

	// don't change direction for waypoints we just flew over and missed slightly
	if (flyState != FLY_LANDING && owner->commandAI->HasMoreMoveCommands()) {
		float3 goalDir = goalVec;

		if ((goalDir != ZeroVector) && (goalVec.dot(goalDir.UnsafeANormalize()) < 1.0f)) {
			goalVec = owner->frontdir;
		}
	}

	const float goalDistSq2D = goalVec.SqLength2D();
	const float gHeight = UseSmoothMesh()?
		std::max(smoothGround->GetHeight(pos.x, pos.z), ground->GetApproximateHeight(pos.x, pos.z)):
		ground->GetHeightAboveWater(pos.x, pos.z);

	const bool closeToGoal = (flyState == FLY_ATTACKING)?
		(goalDistSq2D < (             400.0f)):
		(goalDistSq2D < (maxDrift * maxDrift)) && (math::fabs(gHeight + wantedHeight - pos.y) < maxDrift);

	if (closeToGoal) {
		switch (flyState) {
			case FLY_CRUISING: {
				const bool hasMoreMoveCmds = owner->commandAI->HasMoreMoveCommands();
				const bool blockLanding = (!CanLand() || hasMoreMoveCmds);

				// NOTE: should CMD_LOAD_ONTO be here?
				const bool isTransporter = (dynamic_cast<CTransportUnit*>(owner) != NULL);
				const bool hasLoadCmds = isTransporter &&
					!owner->commandAI->commandQue.empty() &&
					(owner->commandAI->commandQue.front().GetID() == CMD_LOAD_ONTO ||
					 owner->commandAI->commandQue.front().GetID() == CMD_LOAD_UNITS);
				// [?] transport aircraft need some time to detect that they can pickup
				const bool canLoad = isTransporter && (++waitCounter < ((GAME_SPEED << 1) - 5));

				if (blockLanding || (canLoad && hasLoadCmds)) {
					wantedSpeed = ZeroVector;

					if (isTransporter) {
						if (waitCounter > (GAME_SPEED << 1)) {
							wantedHeight = orgWantedHeight;
						}

						SetState(AIRCRAFT_HOVERING);
						return;
					} else {
						if (!hasMoreMoveCmds) {
							wantToStop = true;
							SetState(AIRCRAFT_HOVERING);
							return;
						}
					}
				} else {
					wantedHeight = orgWantedHeight;
					SetState(AIRCRAFT_LANDING);
					return;
				}
			} break;

			case FLY_CIRCLING: {
				// break;
				if ((++waitCounter) > ((GAME_SPEED * 3) + 10)) {
					if (airStrafe) {
						float3 relPos = pos - circlingPos;

						if (relPos.x < 0.0001f && relPos.x > -0.0001f) {
							relPos.x = 0.0001f;
						}

						static CMatrix44f rot(0.0f, fastmath::PI / 4.0f, 0.0f);

						// make sure the point is on the circle, go there in a straight line
						goalPos = circlingPos + (rot.Mul(relPos.Normalize2D()) * goalDistance);
					}
					waitCounter = 0;
				}
			} break;

			case FLY_ATTACKING: {
				if (airStrafe) {
					float3 relPos = pos - circlingPos;

					if (relPos.x < 0.0001f && relPos.x > -0.0001f) {
						relPos.x = 0.0001f;
					}

					CMatrix44f rot;

					if (gs->randFloat() > 0.5f) {
						rot.RotateY(0.6f + gs->randFloat() * 0.6f);
					} else {
						rot.RotateY(-(0.6f + gs->randFloat() * 0.6f));
					}

					// Go there in a straight line
					goalPos = circlingPos + (rot.Mul(relPos.Normalize2D()) * goalDistance);
				}
			} break;

			case FLY_LANDING: {
			} break;
		}
	}

	// not "close" to goal yet, so keep going
	// use 2D math since goal is on the ground
	// but we are not
	goalVec.y = 0.0f;

	// if we are close to our goal, we should
	// adjust speed st. we never overshoot it
	// (by respecting current brake-distance)
	const float curSpeed = owner->speed.Length2D();
	const float brakeDist = (0.5f * curSpeed * curSpeed) / decRate;
	const float goalDist = goalVec.Length() + 0.1f;
	const float goalSpeed =
		(maxSpeed          ) * (goalDist >  brakeDist) +
		(curSpeed - decRate) * (goalDist <= brakeDist);

	if (goalDist > goalSpeed) {
		// update our velocity and heading so long as goal is still
		// further away than the distance we can cover in one frame
		// we must use a variable-size "dead zone" to avoid freezing
		// in mid-air or oscillation behavior at very close distances
		// NOTE:
		//   wantedSpeed is a vector, so even aircraft with turnRate=0
		//   are coincidentally able to reach any goal by side-strafing
		wantedHeading = GetHeadingFromVector(goalVec.x, goalVec.z);
		wantedSpeed = (goalVec / goalDist) * goalSpeed;
	} else {
		// switch to hovering (if !CanLand()))
		if (flyState != FLY_ATTACKING) {
			ExecuteStop();
		}
	}

	// redundant, done in Update()
	// UpdateHeading();
	UpdateAirPhysics();

	// Point toward goal or forward - unless we just passed it to get to another goal
	if ((flyState == FLY_ATTACKING) || (flyState == FLY_CIRCLING)) {
		goalVec = circlingPos - pos;
	} else {
		const bool b0 = (flyState != FLY_LANDING && (owner->commandAI->HasMoreMoveCommands()));
		const bool b1 = (goalDist < brakeDist && goalDist > 1.0f);

		if (b0 && b1) {
			goalVec = owner->frontdir;
		} else {
			goalVec = goalPos - pos;
		}
	}

	if (goalVec.SqLength2D() > 1.0f) {
		// update heading again in case goalVec changed
		wantedHeading = GetHeadingFromVector(goalVec.x, goalVec.z);
	}
}



void CHoverAirMoveType::UpdateLanding()
{
	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	// We want to land, and therefore cancel our speed first
	wantedSpeed = ZeroVector;

	// Hang around for a while so queued commands have a chance to take effect
	if ((++waitCounter) < GAME_SPEED) {
		UpdateAirPhysics();
		return;
	}

	if (reservedLandingPos.x < 0.0f) {
		if (CanLandAt(pos)) {
			// found a landing spot
			reservedLandingPos = pos;
			goalPos = pos;

			owner->Block();
			owner->Deactivate();
			owner->script->StopMoving();
		} else {
			if (goalPos.SqDistance2D(pos) < (30.0f * 30.0f)) {
				// randomly pick another landing spot and try again
				goalPos += (gs->randVector() * 300.0f);
				goalPos.ClampInBounds();

				// exact landing pos failed, make sure finishcommand is called anyway
				progressState = AMoveType::Failed;
			}

			flyState = FLY_LANDING;
			UpdateFlying();
			return;
		}
	}

	// We should wait until we actually have stopped smoothly
	if (spd.SqLength2D() > 1.0f) {
		UpdateFlying();
		UpdateAirPhysics();
		return;
	}

	// We have stopped, time to land
	// NOTE: wantedHeight is interpreted as RELATIVE altitude
	const float gh = ground->GetHeightAboveWater(pos.x, pos.z);
	const float gah = ground->GetHeightReal(pos.x, pos.z);
	float altitude = (wantedHeight = 0.0f);

	// can we submerge and are we still above water?
	if ((owner->unitDef->canSubmerge) && (gah < 0.0f)) {
		altitude = pos.y - gah;
	} else {
		altitude = pos.y - gh;
	}

	UpdateAirPhysics();

	// collision detection does not let us get
	// closer to the ground than <radius> elmos
	// (wrt. midPos.y)
	if (altitude <= owner->radius) {
		SetState(AIRCRAFT_LANDED);
	}
}



void CHoverAirMoveType::UpdateHeading()
{
	if (aircraftState == AIRCRAFT_TAKEOFF && !owner->unitDef->factoryHeadingTakeoff)
		return;

	SyncedSshort& heading = owner->heading;
	const short deltaHeading = forceHeading?
		(forceHeadingTo - heading):
		(wantedHeading - heading);

	if (deltaHeading > 0) {
		heading += std::min(deltaHeading, short(turnRate));
	} else {
		heading += std::max(deltaHeading, short(-turnRate));
	}

	owner->UpdateDirVectors(owner->IsOnGround());
	owner->UpdateMidAndAimPos();
}

void CHoverAirMoveType::UpdateBanking(bool noBanking)
{
	if (aircraftState != AIRCRAFT_FLYING && aircraftState != AIRCRAFT_HOVERING)
		return;

	if (!owner->upright) {
		float wantedPitch = 0.0f;

		if (aircraftState == AIRCRAFT_FLYING && flyState == FLY_ATTACKING && circlingPos.y < owner->pos.y) {
			wantedPitch = (circlingPos.y - owner->pos.y) / circlingPos.distance(owner->pos);
		}

		currentPitch = currentPitch * 0.95f + wantedPitch * 0.05f;
	}

	const float bankLimit = std::min(1.0f, goalPos.SqDistance2D(owner->pos) * Square(0.15f));
	float wantedBank = 0.0f;

	SyncedFloat3& frontDir = owner->frontdir;
	SyncedFloat3& upDir = owner->updir;
	SyncedFloat3& rightDir3D = owner->rightdir;
	SyncedFloat3  rightDir2D;

	// pitching does not affect rightdir, but...
	frontDir.y = currentPitch;
	frontDir.Normalize();

	// we want a flat right-vector to calculate wantedBank
	rightDir2D = frontDir.cross(UpVector);

	if (!noBanking && bankingAllowed)
		wantedBank = rightDir2D.dot(deltaSpeed) / accRate * 0.5f;

	if (Square(wantedBank) > bankLimit) {
		wantedBank =  math::sqrt(bankLimit);
	} else if (Square(wantedBank) < -bankLimit) {
		wantedBank = -math::sqrt(bankLimit);
	}

	// Adjust our banking to the desired value
	if (currentBank > wantedBank) {
		currentBank -= std::min(0.03f, currentBank - wantedBank);
	} else {
		currentBank += std::min(0.03f, wantedBank - currentBank);
	}


	upDir = rightDir2D.cross(frontDir);
	upDir = upDir * math::cos(currentBank) + rightDir2D * math::sin(currentBank);
	rightDir3D = frontDir.cross(upDir);

	// NOTE:
	//     heading might not be fully in sync with frontDir due to the
	//     vector<-->heading mapping not being 1:1 (such that heading
	//     != GetHeadingFromVector(frontDir)), therefore this call can
	//     cause owner->heading to change --> unwanted if forceHeading
	//
	//     it is "safe" to skip because only frontDir.y is manipulated
	//     above so its xz-direction does not change, but the problem
	//     should really be fixed elsewhere
	if (!forceHeading) {
		owner->SetHeadingFromDirection();
	}

	owner->UpdateMidAndAimPos();
}


void CHoverAirMoveType::UpdateAirPhysics()
{
	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	// copy vertical speed
	const float yspeed = spd.y;

	if (!((gs->frameNum + owner->id) & 3)) {
		CheckForCollision();
	}

	owner->SetVelocity(spd * XZVector);

	const float3 deltaSpeed = wantedSpeed - spd;
	const float deltaDotSpeed = (spd != ZeroVector)? deltaSpeed.dot(spd): 1.0f;

	if (deltaDotSpeed == 0.0f) {
		// we have the wanted speed
	} else if (deltaDotSpeed > 0.0f) {
		// accelerate
		const float sqdl = deltaSpeed.SqLength();

		if (sqdl < Square(accRate)) {
			owner->SetVelocity(wantedSpeed);
		} else {
			owner->SetVelocity(spd + (deltaSpeed / math::sqrt(sqdl) * accRate));
		}
	} else {
		// deccelerate
		const float sqdl = deltaSpeed.SqLength();

		if (sqdl < Square(decRate)) {
			owner->SetVelocity(wantedSpeed);
		} else {
			owner->SetVelocity(spd + (deltaSpeed / math::sqrt(sqdl) * decRate));
		}
	}

	// absolute and relative ground height at (pos.x, pos.z)
	// if this aircraft uses the smoothmesh, these values are
	// calculated with respect to that (for changing vertical
	// speed, but not for ground collision)
	float curAbsHeight = owner->unitDef->canSubmerge?
		ground->GetHeightReal(pos.x, pos.z):
		ground->GetHeightAboveWater(pos.x, pos.z);
	float curRelHeight = 0.0f;

	float wh = wantedHeight; // wanted RELATIVE height (altitude)
	float ws = 0.0f;         // wanted vertical speed

	// always stay above the actual terrain (therefore either the value of
	// <midPos.y - radius> or pos.y must never become smaller than the real
	// ground height)
	// note: unlike StrafeAirMoveType, UpdateTakeoff calls UpdateAirPhysics
	// so we ignore terrain while we are in the takeoff state to avoid jumps
	if (modInfo.allowAircraftToHitGround) {
		const bool groundContact = (curAbsHeight > (owner->midPos.y - owner->radius));
		const bool handleContact = (aircraftState != AIRCRAFT_LANDED && aircraftState != AIRCRAFT_TAKEOFF);

		if (groundContact && handleContact) {
			owner->Move(UpVector * (curAbsHeight - (owner->midPos.y - owner->radius) + 0.01f), true);
		}
	}

	if (UseSmoothMesh()) {
		curAbsHeight = owner->unitDef->canSubmerge?
			smoothGround->GetHeight(pos.x, pos.z):
			smoothGround->GetHeightAboveWater(pos.x, pos.z);
	}

	// restore original vertical speed
	owner->SetVelocity((spd * XZVector) + (UpVector * yspeed));

	if (lastColWarningType == 2) {
		const float3 dir = lastColWarning->midPos - owner->midPos;
		const float3 sdir = lastColWarning->speed - spd;

		if (spd.dot(dir + sdir * 20.0f) < 0.0f) {
			if (lastColWarning->midPos.y > owner->pos.y) {
				wh -= 30.0f;
			} else {
				wh += 50.0f;
			}
		}
	}

	curRelHeight = pos.y - curAbsHeight;

	if (curRelHeight < wh) {
		ws = altitudeRate;

		if ((spd.y > 0.0001f) && (((wh - curRelHeight) / spd.y) * accRate * 1.5f) < spd.y) {
			ws = 0.0f;
		}
	} else {
		ws = -altitudeRate;

		if ((spd.y < -0.0001f) && (((wh - curRelHeight) / spd.y) * accRate * 0.7f) < -spd.y) {
			ws = 0.0f;
		}
	}

	if (!owner->beingBuilt) {
		if (math::fabs(wh - curRelHeight) > 2.0f) {
			if (spd.y > ws) {
				owner->SetVelocity((spd * XZVector) + (UpVector * std::max(ws, spd.y - accRate * 1.5f)));
			} else {
				// accelerate upward faster if close to ground
				owner->SetVelocity((spd * XZVector) + (UpVector * std::min(ws, spd.y + accRate * ((curRelHeight < 20.0f)? 2.0f: 0.7f))));
			}
		} else {
			owner->SetVelocity((spd * XZVector) + (UpVector * spd.y * 0.95f));
		}
	}

	owner->SetSpeed(spd);

	if (modInfo.allowAircraftToLeaveMap || (pos + spd).IsInBounds()) {
		owner->Move(spd, true);
	}
}


void CHoverAirMoveType::UpdateMoveRate()
{
	int curRate = 0;

	// currentspeed is not used correctly for vertical movement, so compensate with this hax
	if ((aircraftState == AIRCRAFT_LANDING) || (aircraftState == AIRCRAFT_TAKEOFF)) {
		curRate = 1;
	} else {
		curRate = (owner->speed.w / maxSpeed) * 3;
		curRate = std::max(0, std::min(curRate, 2));
	}

	if (curRate != lastMoveRate) {
		owner->script->MoveRate(curRate);
		lastMoveRate = curRate;
	}
}


bool CHoverAirMoveType::Update()
{
	const float3 lastPos = owner->pos;
	const float4 lastSpd = owner->speed;

	AAirMoveType::Update();

	if ((owner->IsStunned() && !owner->IsCrashing()) || owner->beingBuilt) {
		wantedSpeed = ZeroVector;

		UpdateAirPhysics();
		return (HandleCollisions(collide && !owner->beingBuilt && (padStatus == PAD_STATUS_FLYING) && (aircraftState != AIRCRAFT_TAKEOFF)));
	}

	// allow us to stop if wanted (changes aircraft state)
	if (wantToStop)
		ExecuteStop();

	if (aircraftState != AIRCRAFT_CRASHING) {
		if (owner->fpsControlPlayer != NULL) {
			SetState(AIRCRAFT_FLYING);

			const FPSUnitController& con = owner->fpsControlPlayer->fpsController;

			const float3 forward = con.viewDir;
			const float3 right = forward.cross(UpVector);
			const float3 nextPos = lastPos + owner->speed;

			float3 flatForward = forward;
			flatForward.Normalize2D();

			wantedSpeed = ZeroVector;

			if (con.forward) wantedSpeed += flatForward;
			if (con.back   ) wantedSpeed -= flatForward;
			if (con.right  ) wantedSpeed += right;
			if (con.left   ) wantedSpeed -= right;

			wantedSpeed.Normalize();
			wantedSpeed *= maxSpeed;

			if (!nextPos.IsInBounds()) {
				owner->SetVelocityAndSpeed(ZeroVector);
			}

			UpdateAirPhysics();
			wantedHeading = GetHeadingFromVector(flatForward.x, flatForward.z);
		}

		if (reservedPad != NULL) {
			MoveToRepairPad();

			if (padStatus >= PAD_STATUS_LANDING) {
				flyState = FLY_LANDING;
			}
		}
	}

	switch (aircraftState) {
		case AIRCRAFT_LANDED:
			UpdateLanded();
			break;
		case AIRCRAFT_TAKEOFF:
			UpdateTakeoff();
			break;
		case AIRCRAFT_FLYING:
			UpdateFlying();
			break;
		case AIRCRAFT_LANDING:
			UpdateLanding();
			break;
		case AIRCRAFT_HOVERING:
			UpdateHovering();
			break;
		case AIRCRAFT_CRASHING: {
			UpdateAirPhysics();

			if ((ground->GetHeightAboveWater(owner->pos.x, owner->pos.z) + 5.0f + owner->radius) > owner->pos.y) {
				owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_CRASHING);
				owner->KillUnit(NULL, true, false);
			} else {
				#define SPIN_DIR(o) ((o->id & 1) * 2 - 1)
				wantedHeading = GetHeadingFromVector(owner->rightdir.x * SPIN_DIR(owner), owner->rightdir.z * SPIN_DIR(owner));
				wantedHeight = 0.0f;
				#undef SPIN_DIR
			}

			new CSmokeProjectile(owner->midPos, gs->randVector() * 0.08f, 100 + gs->randFloat() * 50, 5, 0.2f, owner, 0.4f);
		} break;
	}

	if (lastSpd == ZeroVector && owner->speed != ZeroVector) { owner->script->StartMoving(false); }
	if (lastSpd != ZeroVector && owner->speed == ZeroVector) { owner->script->StopMoving(); }

	// Banking requires deltaSpeed.y = 0
	deltaSpeed = owner->speed - lastSpd;
	deltaSpeed.y = 0.0f;

	// Turn and bank and move; update dirs
	UpdateHeading();
	UpdateBanking(aircraftState == AIRCRAFT_HOVERING);

	return (HandleCollisions(collide && !owner->beingBuilt && (padStatus == PAD_STATUS_FLYING) && (aircraftState != AIRCRAFT_TAKEOFF)));
}

void CHoverAirMoveType::SlowUpdate()
{
	UpdateFuel();

	// HoverAirMoveType aircraft are controlled by AirCAI's,
	// but only MobileCAI's reserve pads so we need to do
	// this for ourselves
	if (reservedPad == NULL && aircraftState == AIRCRAFT_FLYING && WantsRepair()) {
		CAirBaseHandler::LandingPad* lp = airBaseHandler->FindAirBase(owner, owner->unitDef->minAirBasePower, true);

		if (lp != NULL) {
			AAirMoveType::ReservePad(lp);
		}
	}

	UpdateMoveRate();
	// note: NOT AAirMoveType::SlowUpdate
	AMoveType::SlowUpdate();
}

/// Returns true if indicated position is a suitable landing spot
bool CHoverAirMoveType::CanLandAt(const float3& pos) const
{
	if (loadingUnits)
		return true;
	if (!CanLand())
		return false;
	if (!pos.IsInBounds())
		return false;

	const UnitDef* ud = owner->unitDef;
	const float gah = ground->GetApproximateHeight(pos.x, pos.z);

	if ((gah < 0.0f) && !(ud->floatOnWater || ud->canSubmerge)) {
		return false;
	}

	const int2 mp = owner->GetMapPos(pos);

	for (int z = mp.y; z < mp.y + owner->zsize; z++) {
		for (int x = mp.x; x < mp.x + owner->xsize; x++) {
			if (groundBlockingObjectMap->GroundBlocked(x, z, owner)) {
				return false;
			}
		}
	}

	return true;
}

void CHoverAirMoveType::ForceHeading(short h)
{
	forceHeading = true;
	forceHeadingTo = h;
}

void CHoverAirMoveType::SetWantedAltitude(float altitude)
{
	if (altitude == 0) {
		wantedHeight = orgWantedHeight;
	} else {
		wantedHeight = altitude;
	}
}

void CHoverAirMoveType::SetDefaultAltitude(float altitude)
{
	wantedHeight =  altitude;
	orgWantedHeight = altitude;
}

void CHoverAirMoveType::Takeoff()
{
	if (aircraftState == AAirMoveType::AIRCRAFT_LANDED) {
		SetState(AAirMoveType::AIRCRAFT_TAKEOFF);
	}
	if (aircraftState == AAirMoveType::AIRCRAFT_LANDING) {
		SetState(AAirMoveType::AIRCRAFT_FLYING);
	}
}

void CHoverAirMoveType::Land()
{
	if (aircraftState == AAirMoveType::AIRCRAFT_HOVERING) {
		SetState(AAirMoveType::AIRCRAFT_FLYING); // switch to flying, it performs necessary checks to prepare for landing
	}
}

bool CHoverAirMoveType::HandleCollisions(bool checkCollisions)
{
	const float3& pos = owner->pos;

	if (pos != oldPos) {
		oldPos = pos;

		bool hitBuilding = false;

		// check for collisions if not on a pad, not being built, or not taking off
		// includes an extra condition for transports, which are exempt while loading
		if (!loadingUnits && checkCollisions) {
			const vector<CUnit*>& nearUnits = quadField->GetUnitsExact(pos, owner->radius + 6);

			for (vector<CUnit*>::const_iterator ui = nearUnits.begin(); ui != nearUnits.end(); ++ui) {
				CUnit* unit = *ui;

				if (unit->transporter != NULL)
					continue;

				const float sqDist = (pos - unit->pos).SqLength();
				const float totRad = owner->radius + unit->radius;

				if (sqDist <= 0.1f || sqDist >= (totRad * totRad))
					continue;

				const float dist = math::sqrt(sqDist);
				const float3 dif = (pos - unit->pos).Normalize();

				if (unit->mass >= CSolidObject::DEFAULT_MASS || unit->immobile) {
					owner->Move(-dif * (dist - totRad), true);
					owner->SetVelocity(owner->speed * 0.99f);

					hitBuilding = true;
				} else {
					const float part = owner->mass / (owner->mass + unit->mass);

					owner->Move(-dif * (dist - totRad) * (1.0f - part), true);
					unit->Move(dif * (dist - totRad) * (part), true);

					const float colSpeed = -owner->speed.dot(dif) + unit->speed.dot(dif);

					owner->SetVelocity(owner->speed + (dif * colSpeed * (1.0f - part)));
					unit->SetVelocityAndSpeed(unit->speed - (dif * colSpeed * (part)));
				}
			}

			owner->SetSpeed(owner->speed);
		}

		if (hitBuilding && owner->IsCrashing()) {
			owner->KillUnit(NULL, true, false);
			return true;
		}

		if (pos.x < 0.0f) {
			owner->Move( RgtVector * 0.6f, true);
		} else if (pos.x > float3::maxxpos) {
			owner->Move(-RgtVector * 0.6f, true);
		}

		if (pos.z < 0.0f) {
			owner->Move( FwdVector * 0.6f, true);
		} else if (pos.z > float3::maxzpos) {
			owner->Move(-FwdVector * 0.6f, true);
		}

		return true;
	}

	return false;
}
