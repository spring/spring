/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "HoverAirMoveType.h"
#include "Game/Players/Player.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Rendering/Env/Particles/Classes/SmokeProjectile.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "System/myMath.h"
#include "System/Matrix44f.h"
#include "System/Sync/HsiehHash.h"

CR_BIND_DERIVED(CHoverAirMoveType, AAirMoveType, (nullptr))

CR_REG_METADATA(CHoverAirMoveType, (
	CR_MEMBER(flyState),
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

	CR_MEMBER(wantedHeading),
	CR_MEMBER(forceHeadingTo),

	CR_MEMBER(waitCounter),
	CR_MEMBER(lastMoveRate)
))



static bool IsUnitBusy(const CUnit* u) {
	// queued move-commands (or active build/repair/etc-commands) mean unit has to stay airborne
	const auto& cai = u->commandAI;
	return (cai->inCommand || cai->HasMoreMoveCommands(false));
}

CHoverAirMoveType::CHoverAirMoveType(CUnit* owner) :
	AAirMoveType(owner),
	flyState(FLY_CRUISING),

	bankingAllowed(true),
	airStrafe(owner != nullptr ? owner->unitDef->airStrafe : false),
	wantToStop(false),

	goalDistance(1),

	// we want to take off in direction of factory facing
	currentBank(0),
	currentPitch(0),

	turnRate(1),
	maxDrift(1.0f),
	maxTurnAngle(math::cos((owner != nullptr ? owner->unitDef->turnInPlaceAngleLimit : 0.0f) * (PI / 180.0f)) * -1.0f),

	wantedSpeed(ZeroVector),
	deltaSpeed(ZeroVector),
	circlingPos(ZeroVector),
	randomWind(ZeroVector),

	forceHeading(false),
	dontLand(false),

	wantedHeading(owner != nullptr ? GetHeadingFromFacing(owner->buildFacing) : 0),
	forceHeadingTo(wantedHeading),

	waitCounter(0),
	lastMoveRate(0)
{
	// creg
	if (owner == nullptr)
		return;

	assert(owner->unitDef != nullptr);

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
	if (aircraftState == AIRCRAFT_CRASHING && newState != AIRCRAFT_CRASHING)
		return;

	if (newState == aircraftState)
		return;


	owner->dontUseWeapons = (newState == AIRCRAFT_LANDED);
	owner->useAirLos = (newState != AIRCRAFT_LANDED);

	aircraftState = newState;

	switch (aircraftState) {
		case AIRCRAFT_CRASHING:
			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_CRASHING);
			break;

		#if 0
		case AIRCRAFT_FLYING:
			owner->Activate();
			break;
		case AIRCRAFT_LANDING:
			owner->Deactivate();
			break;
		#endif

		case AIRCRAFT_LANDED:
			// FIXME already inform commandAI in AIRCRAFT_LANDING!
			owner->commandAI->StopMove();

			owner->Deactivate();
			owner->Block();
			owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
			break;
		case AIRCRAFT_TAKEOFF:
			owner->Activate();
			owner->UnBlock();
			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
			break;
		case AIRCRAFT_HOVERING: {
			// when heading is forced by TCAI we are busy (un-)loading
			// a unit and do not want wantedHeight to be tampered with
			wantedHeight = mix(orgWantedHeight, wantedHeight, forceHeading);
			wantedSpeed = ZeroVector;
		} // fall through
		default:
			ClearLandingPos();
			break;
	}

	// Cruise as default
	// FIXME: RHS overrides FLY_ATTACKING and FLY_CIRCLING (via UpdateFlying-->ExecuteStop)
	if (aircraftState == AIRCRAFT_FLYING || aircraftState == AIRCRAFT_HOVERING)
		flyState = FLY_CRUISING;

	owner->UpdatePhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING, (aircraftState != AIRCRAFT_LANDED));
	waitCounter = 0;
}

void CHoverAirMoveType::SetAllowLanding(bool allowLanding)
{
	dontLand = !allowLanding;

	if (CanLand(false))
		return;

	if (aircraftState != AIRCRAFT_LANDED && aircraftState != AIRCRAFT_LANDING)
		return;

	// do not start hovering if still (un)loading a unit
	if (forceHeading)
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
			if (flyState != FLY_CRUISING) {
				flyState = FLY_CRUISING;
			}
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
	if ((aircraftState == AIRCRAFT_FLYING) && (flyState == FLY_CIRCLING || flyState == FLY_ATTACKING) && ((circlingPos - pos).SqLength2D() < 64) && (goalDistance == distance))
		return;

	circlingPos = pos;
	goalDistance = distance;
	goalPos = owner->pos;

	// let this handle any needed state transitions
	StartMoving(goalPos, goalDistance);

	// FIXME:
	//   the FLY_ATTACKING state is broken (unknown how long this has been
	//   the case because <aggressive> was always false up until e7ae68df)
	//   aircraft fly *right up* to their target without stopping
	//   after fixing this, the "circling" behavior is now broken
	//   (waitCounter is always 0)
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
	SetGoal(owner->pos);
	ClearLandingPos();

	switch (aircraftState) {
		case AIRCRAFT_TAKEOFF: {
			if (CanLand(IsUnitBusy(owner))) {
				SetState(AIRCRAFT_LANDING);
				// trick to land directly
				waitCounter = GAME_SPEED;
				break;
			}
		} // fall through
		case AIRCRAFT_FLYING: {

			if (CanLand(IsUnitBusy(owner))) {
				SetState(AIRCRAFT_LANDING);
			} else {
				SetState(AIRCRAFT_HOVERING);
			}
		} break;

		case AIRCRAFT_LANDING: {
			if (!CanLand(IsUnitBusy(owner)))
				SetState(AIRCRAFT_HOVERING);

		} break;
		case AIRCRAFT_LANDED: {} break;
		case AIRCRAFT_CRASHING: {} break;

		case AIRCRAFT_HOVERING: {
			if (CanLand(IsUnitBusy(owner))) {
				// land immediately, otherwise keep hovering
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
	// heading while "landed" (see MobileCAI)
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
		(pos.y - CGround::GetHeightReal(pos.x, pos.z)):
		(pos.y - CGround::GetHeightAboveWater(pos.x, pos.z));

	if (altitude > orgWantedHeight * 0.8f) {
		SetState(AIRCRAFT_FLYING);
	}
}


// Move the unit around a bit..
void CHoverAirMoveType::UpdateHovering()
{
	#if 0
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

	if (b0 && b1 && !owner->unitDef->IsTransportUnit()) {
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
	#endif

	#if 1
	randomWind.x = randomWind.x * 0.9f + (gs->randFloat() - 0.5f) * 0.5f;
	randomWind.z = randomWind.z * 0.9f + (gs->randFloat() - 0.5f) * 0.5f;

	// randomly drift (but not too far from goal-position)
	wantedSpeed = (randomWind * math::fabs(owner->unitDef->dlHoverFactor) * 0.5f);
	wantedSpeed += (smoothstep(0.0f, 20.0f * 20.0f, (goalPos - owner->pos)) * (goalPos - owner->pos));

	UpdateAirPhysics();
	#endif
}


void CHoverAirMoveType::UpdateFlying()
{
	const float3& pos = owner->pos;
	// const float4& spd = owner->speed;

	// Direction to where we would like to be
	float3 goalVec = goalPos - pos;

	// don't change direction for waypoints we just flew over and missed slightly
	if (flyState != FLY_LANDING && owner->commandAI->HasMoreMoveCommands()) {
		float3 goalDir = goalVec;

		if ((goalDir != ZeroVector) && (goalVec.dot(goalDir.UnsafeANormalize()) < 1.0f)) {
			goalVec = owner->frontdir;
		}
	}

	const float goalDistSq2D = goalVec.SqLength2D();
	const float gHeight = UseSmoothMesh()?
		std::max(smoothGround->GetHeight(pos.x, pos.z), CGround::GetApproximateHeight(pos.x, pos.z)):
		CGround::GetHeightAboveWater(pos.x, pos.z);

	const bool closeToGoal = (flyState == FLY_ATTACKING)?
		(goalDistSq2D < (             400.0f)):
		(goalDistSq2D < (maxDrift * maxDrift)) && (math::fabs(gHeight + wantedHeight - pos.y) < maxDrift);

	if (closeToGoal) {
		switch (flyState) {
			case FLY_CRUISING: {
				const auto& cmdQue = owner->commandAI->commandQue;
				const int topCmdID = cmdQue.empty() ? 0 : cmdQue.front().GetID();

				// NOTE: should CMD_LOAD_ONTO be here?
				const bool isTransporter = owner->unitDef->IsTransportUnit();
				const bool hasLoadCmds = isTransporter && (topCmdID == CMD_LOAD_ONTO || topCmdID == CMD_LOAD_UNITS);
				// [?] transport aircraft need some time to detect that they can pickup
				const bool canLoad = isTransporter && (++waitCounter < ((GAME_SPEED << 1) - 5));
				const bool isBusy = IsUnitBusy(owner);

				if (!CanLand(isBusy) || (canLoad && hasLoadCmds)) {
					wantedSpeed = ZeroVector;

					if (isTransporter) {
						if (waitCounter > (GAME_SPEED << 1)) {
							wantedHeight = orgWantedHeight;
						}

						SetState(AIRCRAFT_HOVERING);
					} else {
						if (!isBusy) {
							wantToStop = true;

							// NOTE:
							//   this is not useful, next frame UpdateFlying()
							//   will change it to _LANDING because wantToStop
							//   is now true
							SetState(AIRCRAFT_HOVERING);
						}
					}
				} else {
					wantedHeight = orgWantedHeight;
					SetState(AIRCRAFT_LANDING);
				}
			} break;

			case FLY_CIRCLING: {
				if ((++waitCounter) > ((GAME_SPEED * 3) + 10)) {
					if (airStrafe) {
						float3 relPos = pos - circlingPos;

						if (relPos.x < 0.0001f && relPos.x > -0.0001f) {
							relPos.x = 0.0001f;
						}

						static const CMatrix44f rot(0.0f, fastmath::PI / 4.0f, 0.0f);

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

					float rotY = 0.6f + gs->randFloat() * 0.6f;
					rotY *= (gs->randFloat() > 0.5f) ? 1.0f : -1.0f;
					const CMatrix44f rot(0.0f, rotY, 0.0f);

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
		if (!IsUnitBusy(owner)) {
			ExecuteStop();
		} else {
			wantedSpeed = ZeroVector;
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

	if (!HaveLandingPos()) {
		if (CanLandAt(pos)) {
			// found a landing spot
			reservedLandingPos = pos;
			goalPos = pos;
			wantedHeight = 0;
			UpdateLandingHeight();

			const float3 originalPos = pos;

			owner->Move(reservedLandingPos, false);
			owner->Block();
			owner->Move(originalPos, false);
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



	flyState = FLY_LANDING;

	const float altitude = pos.y - reservedLandingPos.y;
	const float distSq2D = reservedLandingPos.SqDistance2D(pos);

	if (distSq2D > landRadiusSq) {
		const float tmpWantedHeight = wantedHeight;
		SetGoal(reservedLandingPos);

		wantedHeight = std::min((orgWantedHeight - wantedHeight) * distSq2D / altitude + wantedHeight, orgWantedHeight);
		UpdateFlying();
		wantedHeight = tmpWantedHeight;
		return;
	}

	// We want to land, and therefore cancel our speed first
	wantedSpeed = ZeroVector;

	AAirMoveType::UpdateLanding();

	UpdateAirPhysics();
}



void CHoverAirMoveType::UpdateHeading()
{
	if (aircraftState == AIRCRAFT_TAKEOFF && !owner->unitDef->factoryHeadingTakeoff)
		return;
	// UpdateDirVectors() resets our up-vector but we
	// might have residual pitch angle from attacking
	// if (aircraftState == AIRCRAFT_LANDING)
	//     return;

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

		currentPitch = mix(currentPitch, wantedPitch, 0.05f);
	}

	// always positive
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
	if (Square(wantedBank) > bankLimit)
		wantedBank = math::sqrt(bankLimit);

	// Adjust our banking to the desired value
	if (currentBank > wantedBank) {
		currentBank -= std::min(0.03f, currentBank - wantedBank);
	} else {
		currentBank += std::min(0.03f, wantedBank - currentBank);
	}


	upDir = rightDir2D.cross(frontDir);
	upDir = upDir * math::cos(currentBank) + rightDir2D * math::sin(currentBank);
	upDir = upDir.Normalize();
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


void CHoverAirMoveType::UpdateVerticalSpeed(const float4& spd, float curRelHeight, float curVertSpeed) const
{
	float wh = wantedHeight; // wanted RELATIVE height (altitude)
	float ws = 0.0f;         // wanted vertical speed

	owner->SetVelocity((spd * XZVector) + (UpVector * curVertSpeed));

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

	ws *= (1 - owner->beingBuilt);
	// note: don't want this in case unit is built on some raised platform?
	wh *= (1 - owner->beingBuilt);

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

	// finally update w-component
	owner->SetSpeed(spd);
}


void CHoverAirMoveType::UpdateAirPhysics()
{
	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	// copy vertical speed
	const float yspeed = spd.y;

	if (((gs->frameNum + owner->id) & 3) == 0) {
		CheckForCollision();
	}

	// cancel out vertical speed, acc and dec are applied in xz-plane
	owner->SetVelocity(spd * XZVector);

	const float3 deltaSpeed = wantedSpeed - spd;
	const float deltaDotSpeed = deltaSpeed.dot(spd);
	const float deltaSpeedSq = deltaSpeed.SqLength();

	if (deltaDotSpeed >= 0.0f) {
		// accelerate
		if (deltaSpeedSq < Square(accRate)) {
			owner->SetVelocity(wantedSpeed);
		} else {
			if (deltaSpeedSq > 0.0f) {
				owner->SetVelocity(spd + (deltaSpeed / math::sqrt(deltaSpeedSq) * accRate));
			}
		}
	} else {
		// deccelerate
		if (deltaSpeedSq < Square(decRate)) {
			owner->SetVelocity(wantedSpeed);
		} else {
			if (deltaSpeedSq > 0.0f) {
				owner->SetVelocity(spd + (deltaSpeed / math::sqrt(deltaSpeedSq) * decRate));
			}
		}
	}

	// absolute and relative ground height at (pos.x, pos.z)
	// if this aircraft uses the smoothmesh, these values are
	// calculated with respect to that (for changing vertical
	// speed, but not for ground collision)
	float curAbsHeight = owner->unitDef->canSubmerge?
		CGround::GetHeightReal(pos.x, pos.z):
		CGround::GetHeightAboveWater(pos.x, pos.z);

	// always stay above the actual terrain (therefore either the value of
	// <midPos.y - radius> or pos.y must never become smaller than the real
	// ground height)
	// note: unlike StrafeAirMoveType, UpdateTakeoff and UpdateLanding call
	// UpdateAirPhysics() so we ignore terrain while we are in those states
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

	// restore original vertical speed, then compute new
	UpdateVerticalSpeed(spd, pos.y - curAbsHeight, yspeed);

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
		return (HandleCollisions(collide && !owner->beingBuilt && (aircraftState != AIRCRAFT_TAKEOFF)));
	}

	// allow us to stop if wanted (changes aircraft state)
	if (wantToStop)
		ExecuteStop();

	if (aircraftState != AIRCRAFT_CRASHING) {
		if (owner->UnderFirstPersonControl()) {
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

			if ((CGround::GetHeightAboveWater(owner->pos.x, owner->pos.z) + 5.0f + owner->radius) > owner->pos.y) {
				owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_CRASHING);
				owner->KillUnit(NULL, true, false);
			} else {
				#define SPIN_DIR(o) ((o->id & 1) * 2 - 1)
				wantedHeading = GetHeadingFromVector(owner->rightdir.x * SPIN_DIR(owner), owner->rightdir.z * SPIN_DIR(owner));
				wantedHeight = 0.0f;
				#undef SPIN_DIR
			}

			// Spawn unsynced smoke projectile
			new CSmokeProjectile(owner, owner->midPos, gu->RandVector() * 0.08f, 100 + gu->RandFloat() * 50, 5, 0.2f, 0.4f);
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

	return (HandleCollisions(collide && !owner->beingBuilt && (aircraftState != AIRCRAFT_TAKEOFF)));
}

void CHoverAirMoveType::SlowUpdate()
{
	UpdateMoveRate();
	// note: NOT AAirMoveType::SlowUpdate
	AMoveType::SlowUpdate();
}

/// Returns true if indicated position is a suitable landing spot
bool CHoverAirMoveType::CanLandAt(const float3& pos) const
{
	if (forceHeading)
		return true;
	if (!CanLand(false))
		return false;
	if (!pos.IsInBounds())
		return false;

	const UnitDef* ud = owner->unitDef;
	const float gah = CGround::GetApproximateHeight(pos.x, pos.z);

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

		// check for collisions if not being built or not taking off
		// includes an extra condition for transports, which are exempt while loading
		if (!forceHeading && checkCollisions) {
			const std::vector<CUnit*>& nearUnits = quadField->GetUnitsExact(pos, owner->radius + 6);

			for (CUnit* unit: nearUnits) {
				const bool unloadingUnit = (unit->unloadingTransportId == owner->id);
				const bool unloadingOwner = (owner->unloadingTransportId == unit->id);

				if (unloadingUnit)
					unit->unloadingTransportId = -1;

				if (unloadingOwner)
					owner->unloadingTransportId = -1;

				if (unit->id == owner->loadingTransportId || owner->id == unit->loadingTransportId ||
				    unit == owner->transporter || unit->transporter != NULL) {
					continue;
				}


				const float sqDist = (pos - unit->pos).SqLength();
				const float totRad = owner->radius + unit->radius;

				if (sqDist <= 0.1f || sqDist >= (totRad * totRad))
					continue;

				//Keep them marked as recently unloaded
				if (unloadingUnit) {
					unit->unloadingTransportId = owner->id;
					continue;
				}

				if (unloadingOwner) {
					owner->unloadingTransportId = unit->id;
					continue;
				}


				const float dist = math::sqrt(sqDist);
				const float3 dif = (pos - unit->pos).Normalize();

				if (unit->mass >= CSolidObject::DEFAULT_MASS || unit->immobile) {
					owner->Move(-dif * (dist - totRad), true);
					owner->SetVelocity(owner->speed * 0.99f);

					hitBuilding = true;
				} else {
					const float part = owner->mass / (owner->mass + unit->mass);
					const float colSpeed = -owner->speed.dot(dif) + unit->speed.dot(dif);

					owner->Move(-dif * (dist - totRad) * (1.0f - part), true);
					owner->SetVelocity(owner->speed + (dif * colSpeed * (1.0f - part)));

					if (!unit->UsingScriptMoveType()) {
						unit->SetVelocityAndSpeed(unit->speed - (dif * colSpeed * (part)));
						unit->Move(dif * (dist - totRad) * (part), true);
					}
				}
			}

			// update speed.w
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



bool CHoverAirMoveType::SetMemberValue(unsigned int memberHash, void* memberValue) {
	// try the generic members first
	if (AMoveType::SetMemberValue(memberHash, memberValue))
		return true;

	#define MEMBER_CHARPTR_HASH(memberName) HsiehHash(memberName, strlen(memberName),     0)
	#define MEMBER_LITERAL_HASH(memberName) HsiehHash(memberName, sizeof(memberName) - 1, 0)

	#define     DONTLAND_MEMBER_IDX 1
	#define WANTEDHEIGHT_MEMBER_IDX 0

	static const unsigned int boolMemberHashes[] = {
		MEMBER_LITERAL_HASH(       "collide"),
		MEMBER_LITERAL_HASH(      "dontLand"),
		MEMBER_LITERAL_HASH(     "airStrafe"),
		MEMBER_LITERAL_HASH( "useSmoothMesh"),
		MEMBER_LITERAL_HASH("bankingAllowed"),
	};
	static const unsigned int floatMemberHashes[] = {
		MEMBER_LITERAL_HASH( "wantedHeight"),
		MEMBER_LITERAL_HASH(      "accRate"),
		MEMBER_LITERAL_HASH(      "decRate"),
		MEMBER_LITERAL_HASH(     "turnRate"),
		MEMBER_LITERAL_HASH( "altitudeRate"),
		MEMBER_LITERAL_HASH(  "currentBank"),
		MEMBER_LITERAL_HASH( "currentPitch"),
		MEMBER_LITERAL_HASH(     "maxDrift"),
	};

	#undef MEMBER_CHARPTR_HASH
	#undef MEMBER_LITERAL_HASH


	// unordered_map etc. perform dynallocs, so KISS here
	bool* boolMemberPtrs[] = {
		&collide,
		&dontLand,
		&airStrafe,

		&useSmoothMesh,
		&bankingAllowed
	};
	float* floatMemberPtrs[] = {
		&wantedHeight,

		&accRate,
		&decRate,

		&turnRate,
		&altitudeRate,

		&currentBank,
		&currentPitch,

		&maxDrift
	};

	// special cases
	if (memberHash == boolMemberHashes[DONTLAND_MEMBER_IDX]) {
		SetAllowLanding(!(*(reinterpret_cast<bool*>(memberValue))));
		return true;
	}
	if (memberHash == floatMemberHashes[WANTEDHEIGHT_MEMBER_IDX]) {
		SetDefaultAltitude(*(reinterpret_cast<float*>(memberValue)));
		return true;
	}

	// note: <memberHash> should be calculated via HsiehHash
	for (unsigned int n = 0; n < sizeof(boolMemberPtrs) / sizeof(boolMemberPtrs[0]); n++) {
		if (memberHash == boolMemberHashes[n]) {
			*(boolMemberPtrs[n]) = *(reinterpret_cast<bool*>(memberValue));
			return true;
		}
	}

	for (unsigned int n = 0; n < sizeof(floatMemberPtrs) / sizeof(floatMemberPtrs[0]); n++) {
		if (memberHash == floatMemberHashes[n]) {
			*(floatMemberPtrs[n]) = *(reinterpret_cast<float*>(memberValue));
			return true;
		}
	}

	return false;
}

