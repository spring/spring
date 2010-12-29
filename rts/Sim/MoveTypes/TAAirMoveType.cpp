/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "TAAirMoveType.h"

#include "Game/Player.h"
#include "Map/Ground.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Sim/Units/COB/UnitScript.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "System/myMath.h"
#include "System/Matrix44f.h"


CR_BIND_DERIVED(CTAAirMoveType, AAirMoveType, (NULL));

CR_REG_METADATA(CTAAirMoveType, (
	CR_MEMBER(dontCheckCol),
	CR_MEMBER(bankingAllowed),

	CR_MEMBER(orgWantedHeight),

	CR_MEMBER(circlingPos),
	CR_MEMBER(goalDistance),
	CR_MEMBER(waitCounter),
	CR_MEMBER(wantToStop),

	CR_MEMBER(wantedHeading),

	CR_MEMBER(wantedSpeed),
	CR_MEMBER(deltaSpeed),

	CR_MEMBER(currentBank),
	CR_MEMBER(currentPitch),

	CR_MEMBER(turnRate),
	CR_MEMBER(accRate),
	CR_MEMBER(decRate),
	CR_MEMBER(altitudeRate),

	CR_MEMBER(brakeDistance),
	CR_MEMBER(dontLand),
	CR_MEMBER(lastMoveRate),

	CR_MEMBER(forceHeading),
	CR_MEMBER(forceHeadingTo),

	CR_MEMBER(maxDrift),

	CR_MEMBER(randomWind),

	CR_RESERVED(32)
	));


CTAAirMoveType::CTAAirMoveType(CUnit* owner) :
	AAirMoveType(owner),
	flyState(FLY_CRUISING),
	dontCheckCol(false),
	bankingAllowed(true),
	orgWantedHeight(0.0f),
	circlingPos(ZeroVector),
	goalDistance(1),
	waitCounter(0),
	wantToStop(false),
	// we want to take off in direction of factory facing
	wantedHeading(owner? GetHeadingFromFacing(owner->buildFacing): 0),
	wantedSpeed(ZeroVector),
	deltaSpeed(ZeroVector),
	currentBank(0),
	currentPitch(0),
	turnRate(1),
	accRate(1),
	decRate(1),
	altitudeRate(3.0f),
	brakeDistance(1),
	dontLand(false),
	lastMoveRate(0),
	forceHeading(false),
	forceHeadingTo(wantedHeading),
	maxDrift(1)
{
	if (owner) {
		owner->dontUseWeapons = true;
	}
}

CTAAirMoveType::~CTAAirMoveType()
{
}

void CTAAirMoveType::SetGoal(float3 newPos, float distance)
{
	// aircraft need some marginals to avoid uber stacking
	// when lots of them are ordered to one place
	maxDrift = std::max(16.0f, distance);
	goalPos = newPos;
	oldGoalPos = newPos;
	forceHeading = false;
	wantedHeight = orgWantedHeight;
}

void CTAAirMoveType::SetState(AircraftState newState)
{
	if (newState == aircraftState) {
		return;
	}

	// Perform cob animation
	if (aircraftState == AIRCRAFT_LANDED) {
		owner->script->StartMoving();
	}
	if (newState == AIRCRAFT_LANDED) {
		owner->script->StopMoving();
	}

	if (newState == AIRCRAFT_LANDED) {
		owner->dontUseWeapons = true;
		owner->useAirLos = false;
	} else {
		owner->dontUseWeapons = false;
		owner->useAirLos = true;
	}

	aircraftState = newState;

	// Do animations
	switch (aircraftState) {
		case AIRCRAFT_LANDED:
			if (padStatus == 0) {
				// don't set us as on ground if we are on pad
				owner->physicalState = CSolidObject::OnGround;
				owner->Block();
			}
		case AIRCRAFT_LANDING:
			owner->Deactivate();
			break;
		case AIRCRAFT_HOVERING:
			wantedHeight = orgWantedHeight;
			wantedSpeed = ZeroVector;
			// fall through...
		default:
			owner->physicalState = CSolidObject::Flying;
			owner->UnBlock();
			owner->Activate();
			reservedLandingPos.x = -1;
			break;
	}

	// Cruise as default
	if (aircraftState == AIRCRAFT_FLYING || aircraftState == AIRCRAFT_HOVERING) {
		flyState = FLY_CRUISING;
	}

	owner->isMoving = (aircraftState != AIRCRAFT_LANDED);
	waitCounter = 0;
}

void CTAAirMoveType::StartMoving(float3 pos, float goalRadius)
{
	wantToStop = false;
	owner->isMoving = true;
	waitCounter = 0;
	forceHeading = false;

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
	brakeDistance = ((maxSpeed * maxSpeed) / decRate);
}

void CTAAirMoveType::StartMoving(float3 pos, float goalRadius, float speed)
{
	StartMoving(pos, goalRadius);
}

void CTAAirMoveType::KeepPointingTo(float3 pos, float distance, bool aggressive)
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

void CTAAirMoveType::ExecuteStop()
{
	wantToStop = false;

	switch (aircraftState) {
		case AIRCRAFT_TAKEOFF:
			if(!dontLand && autoLand) {
				SetState(AIRCRAFT_LANDING);
				// trick to land directly
				waitCounter = 30;
				break;
			} // let it fall through
		case AIRCRAFT_FLYING:
			if (owner->unitDef->DontLand()) {
				goalPos = owner->pos;
				SetState(AIRCRAFT_HOVERING);
			} else if (dontLand || !autoLand) {
				goalPos = owner->pos;
				wantedSpeed = ZeroVector;
			} else {
				SetState(AIRCRAFT_LANDING);
			}
			break;
		case AIRCRAFT_LANDING:
			break;
		case AIRCRAFT_LANDED:
			break;
		case AIRCRAFT_CRASHING:
			break;
		case AIRCRAFT_HOVERING:
			if (!dontLand && autoLand) {
				// land immediately
				SetState(AIRCRAFT_LANDING);
				waitCounter = 30;
			}
			break;
	}
}

void CTAAirMoveType::StopMoving()
{
	wantToStop = true;
	forceHeading = false;
	owner->isMoving = false;
	wantedHeight = orgWantedHeight;
}

void CTAAirMoveType::Idle()
{
	StopMoving();
}



void CTAAirMoveType::UpdateLanded()
{
	float3& pos = owner->pos;

	// dont place on ground if we are on a repair pad
	if (padStatus == 0) {
		if (owner->unitDef->canSubmerge) {
			pos.y = ground->GetApproximateHeight(pos.x, pos.z);
		} else {
			pos.y = ground->GetHeightAboveWater(pos.x, pos.z);
		}
	}

	owner->speed = ZeroVector;
}

void CTAAirMoveType::UpdateTakeoff()
{
	float3 &pos = owner->pos;
	wantedSpeed = ZeroVector;
	wantedHeight = orgWantedHeight;

	UpdateAirPhysics();

	float h = 0.0f;
	if (owner->unitDef->canSubmerge) {
		h = pos.y - ground->GetApproximateHeight(pos.x, pos.z);
	} else {
		h = pos.y - ground->GetHeightAboveWater(pos.x, pos.z);
	}

	if (h > orgWantedHeight * 0.8f) {
		SetState(AIRCRAFT_FLYING);
	}
}


// Move the unit around a bit..
void CTAAirMoveType::UpdateHovering()
{
	#define NOZERO(x) std::max(x, 0.0001f)

	const float driftSpeed = fabs(owner->unitDef->dlHoverFactor);
	float3 deltaVec = goalPos - owner->pos;
	float3 deltaDir = float3(deltaVec.x, 0, deltaVec.z);
	float l = NOZERO(deltaDir.Length2D());
	deltaDir *= smoothstep(0.0f, 20.0f, l) / l;

	// move towards goal position if it's not immediately
	// behind us when we have more waypoints to get to
	// *** this behavior interferes with the loading procedure of transports ***
	if (aircraftState != AIRCRAFT_LANDING && owner->commandAI->HasMoreMoveCommands() &&
		(l < 120) && (deltaDir.SqDistance(deltaVec) > 1.0f) && dynamic_cast<CTransportUnit*>(owner) == NULL) {
		deltaDir = owner->frontdir;
	}

	deltaDir -= owner->speed;
	l = deltaDir.SqLength2D();
	if (l > (maxSpeed * maxSpeed)) {
		deltaDir *= maxSpeed / NOZERO(sqrt(l));
	}
	wantedSpeed = owner->speed + deltaDir;

	// random movement (a sort of fake wind effect)
	// random drift values are in range -0.5 ... 0.5
	randomWind = float3(randomWind.x * 0.9f + (gs->randFloat() - 0.5f) * 0.5f, 0,
                        randomWind.z * 0.9f + (gs->randFloat() - 0.5f) * 0.5f);
	wantedSpeed += randomWind * driftSpeed * 0.5f;

	UpdateAirPhysics();
}


void CTAAirMoveType::UpdateFlying()
{
	float3 &pos = owner->pos;
	float3 &speed = owner->speed;

	// Direction to where we would like to be
	float3 dir = goalPos - pos;
	owner->restTime = 0;

	// don't change direction for waypoints we just flew over and missed slightly
	if (flyState != FLY_LANDING && owner->commandAI->HasMoreMoveCommands()
			&& dir != ZeroVector && dir.SqLength2D() < 10000.0f) {
		float3 ndir = dir; ndir = ndir.UnsafeANormalize();

		if (ndir.SqDistance(dir) < 1.0f) {
			dir = owner->frontdir;
		}
	}

	const float gHeight = UseSmoothMesh()?
		std::max(smoothGround->GetHeight(pos.x, pos.z), ground->GetApproximateHeight(pos.x, pos.z)):
		ground->GetHeightAboveWater(pos.x, pos.z);

	// are we there yet?
	bool closeToGoal =
		(dir.SqLength2D() < maxDrift * maxDrift) &&
		(fabs(gHeight - pos.y + wantedHeight) < maxDrift);

	if (flyState == FLY_ATTACKING) {
		closeToGoal = (dir.SqLength2D() < 400);
	}

	if (closeToGoal) {
		// pretty close
		switch (flyState) {
			case FLY_CRUISING: {
				bool trans = dynamic_cast<CTransportUnit*>(owner);
				bool noland = dontLand || !autoLand;
				// should CMD_LOAD_ONTO be here?
				bool hasLoadCmds = trans && !owner->commandAI->commandQue.empty()
						&& (owner->commandAI->commandQue.front().id == CMD_LOAD_ONTO
							|| owner->commandAI->commandQue.front().id == CMD_LOAD_UNITS);
				if (noland || (trans && ++waitCounter < 55 && hasLoadCmds)) {
					// transport aircraft need some time to detect that they can pickup
					if (trans) {
						wantedSpeed = ZeroVector;
						SetState(AIRCRAFT_HOVERING);
						if (waitCounter > 60) {
							wantedHeight = orgWantedHeight;
						}
					} else {
						wantedSpeed = ZeroVector;
						if (!owner->commandAI->HasMoreMoveCommands())
							wantToStop = true;
						SetState(AIRCRAFT_HOVERING);
					}
				} else {
					wantedHeight = orgWantedHeight;
					SetState(AIRCRAFT_LANDING);
				}
				return;
			}
			case FLY_CIRCLING:
				// break;
				waitCounter++;
				if (waitCounter > 100) {
					if (owner->unitDef->airStrafe) {
						float3 relPos = pos - circlingPos;
						if (relPos.x < 0.0001f && relPos.x > -0.0001f) {
							relPos.x = 0.0001f;
						}
						relPos.y = 0.0f;
						relPos.Normalize();
						static CMatrix44f rot(0.0f,fastmath::PI/4.0f,0.0f);
						float3 newPos = rot.Mul(relPos);

						// Make sure the point is on the circle
						newPos = newPos * goalDistance;

						// Go there in a straight line
						goalPos = circlingPos + newPos;
					}
					waitCounter = 0;
				}
				break;
			case FLY_ATTACKING:{
				if (owner->unitDef->airStrafe) {
					float3 relPos = pos - circlingPos;
					if (relPos.x < 0.0001f && relPos.x > -0.0001f) {
						relPos.x = 0.0001f;
					}
					relPos.y = 0;
					relPos.Normalize();
					CMatrix44f rot;
					if (gs->randFloat() > 0.5f) {
						rot.RotateY(0.6f + gs->randFloat() * 0.6f);
					} else {
						rot.RotateY(-(0.6f + gs->randFloat() * 0.6f));
					}
					float3 newPos = rot.Mul(relPos);
					newPos = newPos * goalDistance;

					// Go there in a straight line
					goalPos = circlingPos + newPos;
				}
				break;
			}
			case FLY_LANDING:{
				break;
			}
		}
	}

	// not there yet, so keep going
	dir.y = 0;
	float realMax = maxSpeed;
	float dist = dir.Length() + 0.1f;

	// If we are close to our goal, we should go slow enough to be able to break in time
	// new additional rule: if in attack mode or if we have more move orders then this is
	// an intermediate waypoint, don't slow down (FIXME)

	/// if (flyState != FLY_ATTACKING && dist < breakDistance && !owner->commandAI->HasMoreMoveCommands()) {
	if (flyState != FLY_ATTACKING && dist < brakeDistance) {
		realMax = dist / (speed.Length2D() + 0.01f) * decRate;
	}

	wantedSpeed = (dir / dist) * realMax;
	UpdateAirPhysics();

	// Point toward goal or forward - unless we just passed it to get to another goal
	if ((flyState == FLY_ATTACKING) || (flyState == FLY_CIRCLING)) {
		dir = circlingPos - pos;
	} else if (flyState != FLY_LANDING && (owner->commandAI->HasMoreMoveCommands() &&
		dist < 120) && (goalPos - pos).Normalize().SqDistance(dir) > 1) {
		dir = owner->frontdir;
	} else {
		dir = goalPos - pos;
	}

	if (dir.SqLength2D() > 1) {
		const int h = GetHeadingFromVector(dir.x, dir.z);
		wantedHeading = (h == 0)? wantedHeading : h;
	}
}



void CTAAirMoveType::UpdateLanding()
{
	float3& pos = owner->pos;
	float3& speed = owner->speed;

	// We want to land, and therefore cancel our speed first
	wantedSpeed = ZeroVector;

	waitCounter++;

	// Hang around for a while so queued commands have a chance to take effect
	if (waitCounter < 30) {
		UpdateAirPhysics();
		return;
	}

	if (reservedLandingPos.x < 0) {
		if (CanLandAt(pos)) {
			// found a landing spot
			reservedLandingPos = pos;
			goalPos = pos;
			owner->physicalState = CSolidObject::OnGround;
			owner->Block();
			owner->physicalState = CSolidObject::Flying;
			owner->Deactivate();
			owner->script->StopMoving();
		} else {
			if (goalPos.SqDistance2D(pos) < 900) {
				goalPos = goalPos + gs->randVector() * 300;
				goalPos.CheckInBounds();
			}
			flyState = FLY_LANDING;
			UpdateFlying();
			return;
		}
	}
	// We should wait until we actually have stopped smoothly
	if (speed.SqLength2D() > 1) {
		UpdateFlying();
		return;
	}

	// We have stopped, time to land
	const float gah = ground->GetApproximateHeight(pos.x, pos.z);
	float h = 0.0f;

	// if aircraft submergible and above water we want height of ocean floor
	if ((owner->unitDef->canSubmerge) && (gah < 0)) {
		h = pos.y - gah;
		wantedHeight = gah;
	} else {
		h = pos.y - ground->GetHeightAboveWater(pos.x, pos.z);
		wantedHeight = -2;
	}

	UpdateAirPhysics();

	if (h <= 0) {
		SetState(AIRCRAFT_LANDED);
		pos.y = gah;
	}
}



void CTAAirMoveType::UpdateHeading()
{
	if (aircraftState == AIRCRAFT_TAKEOFF && !owner->unitDef->factoryHeadingTakeoff) {
		return;
	}

	SyncedSshort& heading = owner->heading;
	short deltaHeading = wantedHeading - heading;

	if (forceHeading) {
		deltaHeading = forceHeadingTo - heading;
	}

	if (deltaHeading > 0) {
		heading += deltaHeading > (short int) turnRate ? (short int) turnRate : deltaHeading;
	} else {
		heading += deltaHeading > (short int) (-turnRate) ? deltaHeading : (short int) (-turnRate);
	}
}

void CTAAirMoveType::UpdateBanking(bool noBanking)
{
	SyncedFloat3 &frontdir = owner->frontdir;
	SyncedFloat3 &updir = owner->updir;

	if (!owner->upright) {
		float wantedPitch = 0;

		if (aircraftState == AIRCRAFT_FLYING && flyState == FLY_ATTACKING && circlingPos.y < owner->pos.y) {
			wantedPitch = (circlingPos.y - owner->pos.y) / circlingPos.distance(owner->pos);
		}

		currentPitch = currentPitch * 0.95f + wantedPitch * 0.05f;
	}

	frontdir = GetVectorFromHeading(owner->heading);
	frontdir.y = currentPitch;
	frontdir.Normalize();

	float3 rightdir(frontdir.cross(UpVector));		//temp rightdir
	rightdir.Normalize();

	float wantedBank = 0.0f;
	if (!noBanking && bankingAllowed) wantedBank = rightdir.dot(deltaSpeed)/accRate*0.5f;

	const float limit = std::min(1.0f,goalPos.SqDistance2D(owner->pos)*Square(0.15f));
	if (Square(wantedBank) > limit) {
		wantedBank =  math::sqrt(limit);
	} else if (Square(wantedBank) < -limit) {
		wantedBank = -math::sqrt(limit);
	}

	//Adjust our banking to the desired value
	if (currentBank > wantedBank) {
		currentBank -= std::min(0.03f, currentBank - wantedBank);
	} else {
		currentBank += std::min(0.03f, wantedBank - currentBank);
	}

	// Calculate a suitable upvector
	updir = rightdir.cross(frontdir);
	updir = updir * cos(currentBank) + rightdir * sin(currentBank);
	owner->rightdir = frontdir.cross(updir);
}


void CTAAirMoveType::UpdateAirPhysics()
{
	float3& pos = owner->pos;
	float3& speed = owner->speed;

	if (!((gs->frameNum + owner->id) & 3)) {
		CheckForCollision();
	}

	const float yspeed = speed.y;
	speed.y = 0.0f;

	float3 delta = wantedSpeed - speed;
	const float deltaDotSpeed = (speed != ZeroVector)? delta.dot(speed): 1.0f;

	if (deltaDotSpeed == 0.0f) {
		// we have the wanted speed
	} else if (deltaDotSpeed > 0.0f) {
		// accelerate
		const float sqdl = delta.SqLength();
		if (sqdl < Square(accRate)) {
			speed = wantedSpeed;
		} else {
			speed += delta / math::sqrt(sqdl) * accRate;
		}
	} else {
		// break
		const float sqdl = delta.SqLength();
		if (sqdl < Square(decRate)) {
			speed = wantedSpeed;
		} else {
			speed += delta / math::sqrt(sqdl) * decRate;
		}
	}

	speed.y = yspeed;
	float h;

	if (UseSmoothMesh()) {
		h = pos.y - std::max(
			smoothGround->GetHeightAboveWater(pos.x, pos.z),
			smoothGround->GetHeightAboveWater(pos.x + speed.x * 20.0f, pos.z + speed.z * 20.0f));
	} else {
		h = pos.y - std::max(
			ground->GetHeightAboveWater(pos.x, pos.z),
			ground->GetHeightAboveWater(pos.x + speed.x * 40.0f, pos.z + speed.z * 40.0f));
	}

	if (h < 4.0f) {
		speed.x *= 0.95f;
		speed.z *= 0.95f;
	}

	float wh = wantedHeight;

	if (lastColWarningType == 2) {
		const float3 dir = lastColWarning->midPos - owner->midPos;
		const float3 sdir = lastColWarning->speed - speed;

		if (speed.dot(dir + sdir * 20.0f) < 0.0f) {
			if (lastColWarning->midPos.y > owner->pos.y) {
				wh -= 30.0f;
			} else {
				wh += 50.0f;
			}
		}
	}


	float ws = 0.0f;

	if (h < wh) {
		ws = altitudeRate;
		if (speed.y > 0.0001f && (wh - h) / speed.y * accRate * 1.5f < speed.y) {
			ws = 0.0f;
		}
	} else {
		ws = -altitudeRate;
		if (speed.y < -0.0001f && (wh - h) / speed.y * accRate * 0.7f < -speed.y) {
			ws = 0.0f;
		}
	}

	if (fabs(wh - h) > 2.0f) {
		if (speed.y > ws) {
			speed.y = std::max(ws, speed.y - accRate * 1.5f);
		} else if(!owner->beingBuilt) {
			// let them accelerate upward faster if close to ground
			speed.y = std::min(ws, speed.y + accRate * (h < 20.0f? 2.0f: 0.7f));
		}
	} else {
		speed.y = speed.y * 0.95;
	}

	if (modInfo.allowAirPlanesToLeaveMap || (pos+speed).CheckInBounds()) {
		pos += speed;
	}
}


void CTAAirMoveType::UpdateMoveRate()
{
	int curRate = 0;

	// currentspeed is not used correctly for vertical movement, so compensate with this hax
	if ((aircraftState == AIRCRAFT_LANDING) || (aircraftState == AIRCRAFT_TAKEOFF)) {
		curRate = 1;
	} else {
		const float curSpeed = owner->speed.Length();

		curRate = (curSpeed / maxSpeed) * 3;
		curRate = std::max(0, std::min(curRate, 2));
	}

	if (curRate != lastMoveRate) {
		owner->script->MoveRate(curRate);
		lastMoveRate = curRate;
	}
}


void CTAAirMoveType::Update()
{
	float3& pos = owner->pos;
	float3& speed = owner->speed;

	// This is only set to false after the plane has finished constructing
	if (useHeading) {
		useHeading = false;
		SetState(AIRCRAFT_TAKEOFF);
	}

	// Allow us to stop if wanted
	if (wantToStop) {
		ExecuteStop();
	}

	float3 lastSpeed = speed;

	if (owner->stunned  || owner->beingBuilt) {
		wantedSpeed = ZeroVector;
		UpdateAirPhysics();
	} else {
		if (owner->directControl) {
			DirectControlStruct* dc = owner->directControl;
			SetState(AIRCRAFT_FLYING);

			float3 forward = dc->viewDir;
			float3 flatForward = forward;
			flatForward.y = 0;
			flatForward.Normalize();
			float3 right = forward.cross(UpVector);
			float3 nextPos = pos + speed;
			wantedSpeed = ZeroVector;

			if (dc->forward)
				wantedSpeed += flatForward;
			if (dc->back)
				wantedSpeed -= flatForward;
			if (dc->right)
				wantedSpeed += right;
			if (dc->left)
				wantedSpeed -= right;
			wantedSpeed.Normalize();
			wantedSpeed *= maxSpeed;

			if (!nextPos.CheckInBounds()) {
				speed = ZeroVector;
			}

			UpdateAirPhysics();
			wantedHeading = GetHeadingFromVector(flatForward.x, flatForward.z);
		} else {

			if (reservedPad) {
				CUnit* unit = reservedPad->GetUnit();
				const float3 relPos = unit->script->GetPiecePos(reservedPad->GetPiece());
				const float3 pos = unit->pos + unit->frontdir * relPos.z
						+ unit->updir * relPos.y + unit->rightdir * relPos.x;

				if (padStatus == 0) {
					if (aircraftState != AIRCRAFT_FLYING && aircraftState != AIRCRAFT_TAKEOFF)
						SetState(AIRCRAFT_FLYING);

					goalPos = pos;

					if (pos.SqDistance2D(owner->pos) < 400*400) {
						padStatus = 1;
					}
				} else if (padStatus == 1) {
					if (aircraftState != AIRCRAFT_FLYING) {
						SetState(AIRCRAFT_FLYING);
					}
					flyState = FLY_LANDING;

					goalPos = pos;
					reservedLandingPos = pos;
					wantedHeight = pos.y - ground->GetHeightAboveWater(pos.x, pos.z);

					if (owner->pos.SqDistance(pos) < 9 || aircraftState == AIRCRAFT_LANDED) {
						padStatus = 2;
					}
				} else {
					if (aircraftState != AIRCRAFT_LANDED)
						SetState(AIRCRAFT_LANDED);

					owner->pos = pos;
					owner->AddBuildPower(unit->unitDef->buildSpeed / 30, unit);
					owner->currentFuel = std::min(owner->unitDef->maxFuel,
							owner->currentFuel + (owner->unitDef->maxFuel
									/ (GAME_SPEED * owner->unitDef->refuelTime)));

					if (owner->health >= owner->maxHealth - 1
							&& owner->currentFuel >= owner->unitDef->maxFuel) {
						airBaseHandler->LeaveLandingPad(reservedPad);
						reservedPad = NULL;
						padStatus = 0;
						goalPos = oldGoalPos;
						SetState(AIRCRAFT_TAKEOFF);
					}
				}
			}

			// Main state handling
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
				case AIRCRAFT_CRASHING:
					break;
			}
		}
	}

	// Banking requires deltaSpeed.y = 0
	deltaSpeed = speed - lastSpeed;
	deltaSpeed.y = 0;

	// Turn and bank and move
	UpdateHeading();
	UpdateBanking(aircraftState == AIRCRAFT_HOVERING);			// updates dirs
	owner->UpdateMidPos();

	// Push other units out of the way
	if (pos != oldpos && aircraftState != AIRCRAFT_TAKEOFF && padStatus == 0) {
		oldpos = pos;

		if (!dontCheckCol && collide) {
			const vector<CUnit*> &nearUnits = qf->GetUnitsExact(pos, owner->radius + 6);

			for (vector<CUnit*>::const_iterator ui = nearUnits.begin(); ui != nearUnits.end(); ++ui) {
				if ((*ui)->transporter)
					continue;

				const float sqDist = (pos-(*ui)->pos).SqLength();
				const float totRad = owner->radius + (*ui)->radius;
				if (sqDist < totRad * totRad && sqDist != 0) {
					const float dist = sqrt(sqDist);
					float3 dif = pos - (*ui)->pos;

					if (dist > 0.0f) {
						dif /= dist;
					}

					if ((*ui)->mass >= CSolidObject::DEFAULT_MASS || (*ui)->immobile) {
						pos -= dif * (dist - totRad);
						owner->UpdateMidPos();
						owner->speed *= 0.99f;
					} else {
						const float part = owner->mass / (owner->mass + (*ui)->mass);
						pos -= dif * (dist - totRad) * (1 - part);
						owner->UpdateMidPos();
						CUnit* u = (CUnit*) (*ui);
						u->pos += dif * (dist - totRad) * (part);
						u->UpdateMidPos();
						const float colSpeed = -owner->speed.dot(dif) + u->speed.dot(dif);
						owner->speed += dif * colSpeed * (1 - part);
						u->speed -= dif * colSpeed * (part);
					}
				}
			}
		}
		if (pos.x < 0) {
			pos.x += 0.6f;
			owner->midPos.x += 0.6f;
		} else if (pos.x > float3::maxxpos) {
			pos.x -= 0.6f;
			owner->midPos.x -= 0.6f;
		}

		if (pos.z < 0) {
			pos.z += 0.6f;
			owner->midPos.z += 0.6f;
		} else if (pos.z > float3::maxzpos) {
			pos.z -= 0.6f;
			owner->midPos.z -= 0.6f;
		}
	}
}

void CTAAirMoveType::SlowUpdate()
{
	UpdateFuel();

	if (!reservedPad && aircraftState == AIRCRAFT_FLYING && owner->health < owner->maxHealth * repairBelowHealth) {
		CAirBaseHandler::LandingPad* lp = airBaseHandler->FindAirBase(owner, owner->unitDef->minAirBasePower);

		if (lp) {
			AddDeathDependence(lp);
			reservedPad = lp;
			padStatus = 0;
			oldGoalPos = goalPos;
		}
	}

	UpdateMoveRate();
	// note: NOT AAirMoveType::SlowUpdate
	AMoveType::SlowUpdate();
}

/// Returns true if indicated position is a suitable landing spot
bool CTAAirMoveType::CanLandAt(const float3& pos) const
{
	if (dontLand) { return false; }
	if (!autoLand) { return false; }
	if (!pos.IsInBounds()) { return false; }

	if ((ground->GetApproximateHeight(pos.x, pos.z) < 0.0f) && !(owner->unitDef->floater || owner->unitDef->canSubmerge)) {
		return false;
	}

	const int2 mp = owner->GetMapPos(pos);

	for (int z = mp.y; z < mp.y + owner->zsize; z++) {
		for (int x = mp.x; x < mp.x + owner->xsize; x++) {
			const CObject* o = groundBlockingObjectMap->GroundBlockedUnsafe(z * gs->mapx + x);
			if (o && o != owner) {
				return false;
			}
		}
	}

	return true;
}

void CTAAirMoveType::ForceHeading(short h)
{
	forceHeading = true;
	forceHeadingTo = h;
}

void CTAAirMoveType::SetWantedAltitude(float altitude)
{
	if (altitude == 0) {
		wantedHeight = orgWantedHeight;
	} else {
		wantedHeight = altitude;
	}
}

void CTAAirMoveType::SetDefaultAltitude(float altitude)
{
	wantedHeight =  altitude;
	orgWantedHeight = altitude;
}

void CTAAirMoveType::CheckForCollision()
{
	if (!collide) return;

	SyncedFloat3& pos = owner->midPos;
	SyncedFloat3 forward = owner->speed;
	forward.Normalize();
	float3 midTestPos = pos + forward * 121;

	const std::vector<CUnit*>& others = qf->GetUnitsExact(midTestPos, 115);
	float dist = 200.0f;

	if (lastColWarning) {
		DeleteDeathDependence(lastColWarning);
		lastColWarning = 0;
		lastColWarningType = 0;
	}

	for (std::vector<CUnit*>::const_iterator ui = others.begin(); ui != others.end(); ++ui) {
		if (*ui == owner || !(*ui)->unitDef->canfly)
			continue;

		SyncedFloat3& op = (*ui)->midPos;
		float3 dif = op - pos;
		float3 forwardDif = forward * (forward.dot(dif));

		if (forwardDif.SqLength() < dist * dist) {
			const float frontLength = forwardDif.Length();
			const float3 ortoDif = dif - forwardDif;
			// note: the radius is multiplied by two since we rely on aircraft
			// having small spheres (see unitloader)
			const float minOrtoDif = ((*ui)->radius + owner->radius) * 2 + frontLength * 0.05f + 5;

			if (ortoDif.SqLength() < minOrtoDif * minOrtoDif) {
				dist = frontLength;
				lastColWarning = (*ui);
			}
		}
	}
	if (lastColWarning) {
		lastColWarningType = 2;
		AddDeathDependence(lastColWarning);
		return;
	}
	for (std::vector<CUnit*>::const_iterator ui = others.begin(); ui != others.end(); ++ui) {
		if (*ui == owner)
			continue;
		if (((*ui)->midPos - pos).SqLength() < dist * dist) {
			lastColWarning = *ui;
		}
	}
	if (lastColWarning) {
		lastColWarningType = 1;
		AddDeathDependence(lastColWarning);
	}
	return;
}

void CTAAirMoveType::DependentDied(CObject* o)
{
	if (o == reservedPad) {
		reservedPad = NULL;
		SetState(AIRCRAFT_FLYING);
		goalPos = oldGoalPos;
	}
	if (o == lastColWarning) {
		lastColWarning = NULL;
		lastColWarningType = 0;
	}

	AMoveType::DependentDied(o);
}

void CTAAirMoveType::Takeoff()
{
	if (aircraftState == AAirMoveType::AIRCRAFT_LANDED) {
		SetState(AAirMoveType::AIRCRAFT_TAKEOFF);
	}
	if (aircraftState == AAirMoveType::AIRCRAFT_LANDING) {
		SetState(AAirMoveType::AIRCRAFT_FLYING);
	}
}

bool CTAAirMoveType::IsFighter() const
{
	return false;
}
