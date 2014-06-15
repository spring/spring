/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GroundMoveType.h"
#include "MoveDefHandler.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/Players/Player.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "MoveMath/MoveMath.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Path/IPathController.hpp"
#include "Sim/Path/IPathManager.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/MobileCAI.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/FastMath.h"
#include "System/myMath.h"
#include "System/TimeProfiler.h"
#include "System/type2.h"
#include "System/Sound/SoundChannels.h"
#include "System/Sync/HsiehHash.h"
#include "System/Sync/SyncTracer.h"

#if 1
#include "Rendering/IPathDrawer.h"
#define DEBUG_DRAWING_ENABLED ((gs->cheatEnabled || gu->spectatingFullView) && pathDrawer->IsEnabled())
#else
#define DEBUG_DRAWING_ENABLED false
#endif

#define LOG_SECTION_GMT "GroundMoveType"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_GMT)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_GMT

// speeds near (MAX_UNIT_SPEED * 1e1) elmos / frame can be caused by explosion impulses
// CUnitHandler removes units with speeds > MAX_UNIT_SPEED as soon as they exit the map,
// so the assertion can be less strict
#define ASSERT_SANE_OWNER_SPEED(v) assert(v.SqLength() < (MAX_UNIT_SPEED * MAX_UNIT_SPEED * 1e2));

// magic number to reduce damage taken from collisions
// between a very heavy and a very light CSolidObject
#define COLLISION_DAMAGE_MULT         0.02f

#define MAX_IDLING_SLOWUPDATES           16
#define IGNORE_OBSTACLES                  0
#define WAIT_FOR_PATH                     1
#define PLAY_SOUNDS                       1

#define UNIT_CMD_QUE_SIZE(u) (u->commandAI->commandQue.size())
#define UNIT_HAS_MOVE_CMD(u) (u->commandAI->commandQue.empty() || u->commandAI->commandQue[0].GetID() == CMD_MOVE)

#define FOOTPRINT_RADIUS(xs, zs, s) ((math::sqrt((xs * xs + zs * zs)) * 0.5f * SQUARE_SIZE) * s)


CR_BIND_DERIVED(CGroundMoveType, AMoveType, (NULL));
CR_REG_METADATA(CGroundMoveType, (
	CR_IGNORED(pathController),

	CR_MEMBER(currWayPoint),
	CR_MEMBER(nextWayPoint),

	CR_MEMBER(waypointDir),
	CR_MEMBER(flatFrontDir),
	CR_MEMBER(lastAvoidanceDir),
	CR_MEMBER(mainHeadingPos),
	CR_MEMBER(skidRotVector),

	CR_MEMBER(turnRate),
	CR_MEMBER(turnSpeed),
	CR_MEMBER(turnAccel),
	CR_MEMBER(accRate),
	CR_MEMBER(decRate),
	CR_MEMBER(myGravity),

	CR_MEMBER(maxReverseSpeed),
	CR_MEMBER(wantedSpeed),
	CR_MEMBER(currentSpeed),
	CR_MEMBER(deltaSpeed),

	CR_MEMBER(atGoal),
	CR_MEMBER(atEndOfPath),

	CR_MEMBER(currWayPointDist),
	CR_MEMBER(prevWayPointDist),
	CR_MEMBER(goalRadius),

	CR_MEMBER(numIdlingUpdates),
	CR_MEMBER(numIdlingSlowUpdates),
	CR_MEMBER(wantedHeading),

	CR_MEMBER(pathID),

	CR_MEMBER(nextObstacleAvoidanceFrame),
	CR_MEMBER(lastPathRequestFrame),

	CR_MEMBER(reversing),
	CR_MEMBER(idling),
	CR_MEMBER(canReverse),
	CR_MEMBER(useMainHeading),
	CR_MEMBER(useRawMovement),

	CR_MEMBER(skidRotSpeed),
	CR_MEMBER(skidRotAccel),

	CR_POSTLOAD(PostLoad)
));



CGroundMoveType::CGroundMoveType(CUnit* owner):
	AMoveType(owner),
	pathController((owner != NULL)? IPathController::GetInstance(owner): NULL),

	currWayPoint(ZeroVector),
	nextWayPoint(ZeroVector),

	flatFrontDir(FwdVector),
	lastAvoidanceDir(ZeroVector),
	mainHeadingPos(ZeroVector),
	skidRotVector(UpVector),

	turnRate(0.1f),
	turnSpeed(0.0f),
	turnAccel(0.0f),

	accRate(0.01f),
	decRate(0.01f),
	myGravity(0.0f),

	maxReverseSpeed(0.0f),
	wantedSpeed(0.0f),
	currentSpeed(0.0f),
	deltaSpeed(0.0f),

	atGoal(false),
	atEndOfPath(false),

	currWayPointDist(0.0f),
	prevWayPointDist(0.0f),
	goalRadius(0.0f),

	reversing(false),
	idling(false),
	canReverse((owner != NULL) && (owner->unitDef->rSpeed > 0.0f)),
	useMainHeading(false),
	useRawMovement(false),

	skidRotSpeed(0.0f),
	skidRotAccel(0.0f),

	pathID(0),

	nextObstacleAvoidanceFrame(0),
	lastPathRequestFrame(0),

	numIdlingUpdates(0),
	numIdlingSlowUpdates(0),

	wantedHeading(0)
{
	if (owner == NULL)
		return;

	assert(owner->unitDef != NULL);
	assert(owner->moveDef != NULL);

	// maxSpeed is set in AMoveType's ctor
	maxReverseSpeed = owner->unitDef->rSpeed / GAME_SPEED;

	turnRate = owner->unitDef->turnRate;
	turnAccel = turnRate * mix(0.333f, 0.033f, owner->moveDef->speedModClass == MoveDef::Ship);

	accRate = std::max(0.01f, owner->unitDef->maxAcc);
	decRate = std::max(0.01f, owner->unitDef->maxDec);

	// unit-gravity must always be negative
	myGravity = mix(-math::fabs(owner->unitDef->myGravity), mapInfo->map.gravity, owner->unitDef->myGravity == 0.0f);
}

CGroundMoveType::~CGroundMoveType()
{
	if (pathID != 0) {
		pathManager->DeletePath(pathID);
	}

	IPathController::FreeInstance(pathController);
}

void CGroundMoveType::PostLoad()
{
	pathController = IPathController::GetInstance(owner);

	// HACK: re-initialize path after load
	if (pathID != 0) {
		pathID = pathManager->RequestPath(owner, owner->moveDef, owner->pos, goalPos, goalRadius, true);
	}
}

bool CGroundMoveType::OwnerMoved(const short oldHeading, const float3& posDif, const float3& cmpEps) {
	if (posDif.equals(ZeroVector, cmpEps)) {
		// note: the float3::== test is not exact, so even if this
		// evaluates to true the unit might still have an epsilon
		// speed vector --> nullify it to prevent apparent visual
		// micro-stuttering (speed is used to extrapolate drawPos)
		owner->SetVelocityAndSpeed(ZeroVector);

		// negative y-coordinates indicate temporary waypoints that
		// only exist while we are still waiting for the pathfinder
		// (so we want to avoid being considered "idle", since that
		// will cause our path to be re-requested and again give us
		// a temporary waypoint, etc.)
		// NOTE: this is only relevant for QTPFS (at present)
		// if the unit is just turning in-place over several frames
		// (eg. to maneuver around an obstacle), do not consider it
		// as "idling"
		idling = true;
		idling &= (currWayPoint.y != -1.0f && nextWayPoint.y != -1.0f);
		idling &= (std::abs(owner->heading - oldHeading) < turnRate);

		return false;
	}

	// note: HandleObjectCollisions() may have negated the position set
	// by UpdateOwnerPos() (so that owner->pos is again equal to oldPos)
	// note: the idling-check can only succeed if we are oriented in the
	// direction of our waypoint, which compensates for the fact distance
	// decreases much less quickly when moving orthogonal to <waypointDir>
	oldPos = owner->pos;

	const float3 ffd = flatFrontDir * posDif.SqLength() * 0.5f;
	const float3 wpd = waypointDir * ((int(!reversing) * 2) - 1);

	// too many false negatives: speed is unreliable if stuck behind an obstacle
	//   idling = (Square(owner->speed.w) < (accRate * accRate));
	//   idling &= (Square(currWayPointDist - prevWayPointDist) <= (accRate * accRate));
	// too many false positives: waypoint-distance delta and speed vary too much
	//   idling = (Square(currWayPointDist - prevWayPointDist) < Square(owner->speed.w));
	// too many false positives: many slow units cannot even manage 1 elmo/frame
	//   idling = (Square(currWayPointDist - prevWayPointDist) < 1.0f);
	idling = true;
	idling &= (math::fabs(posDif.y) < math::fabs(cmpEps.y * owner->pos.y));
	idling &= (Square(currWayPointDist - prevWayPointDist) < ffd.dot(wpd));
	return true;
}

bool CGroundMoveType::Update()
{
	ASSERT_SYNCED(owner->pos);

	// do nothing at all if we are inside a transport
	if (owner->GetTransporter() != NULL)
		return false;

	owner->UpdatePhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING, owner->IsSkidding() || OnSlope(1.0f));

	if (owner->IsSkidding()) {
		UpdateSkid();
		return false;
	}

	ASSERT_SYNCED(owner->pos);

	// set drop height when we start to drop
	if (owner->IsFalling()) {
		UpdateControlledDrop();
		return false;
	}

	ASSERT_SYNCED(owner->pos);

	const short heading = owner->heading;

	// these must be executed even when stunned (so
	// units do not get buried by restoring terrain)
	UpdateOwnerSpeedAndHeading();
	UpdateOwnerPos(owner->speed, GetNewSpeedVector(deltaSpeed, myGravity));
	AdjustPosToWaterLine();
	HandleObjectCollisions();

	ASSERT_SANE_OWNER_SPEED(owner->speed);

	// <dif> is normally equal to owner->speed (if no collisions)
	// we need more precision (less tolerance) in the y-dimension
	// for all-terrain units that are slowed down a lot on cliffs
	return (OwnerMoved(heading, owner->pos - oldPos, float3(float3::CMP_EPS, float3::CMP_EPS * 1e-2f, float3::CMP_EPS)));
}

void CGroundMoveType::UpdateOwnerSpeedAndHeading()
{
	if (owner->IsStunned() || owner->beingBuilt) {
		ChangeSpeed(0.0f, false);
		return;
	}

	// either follow user control input or pathfinder
	// waypoints; change speed and heading as required
	if (owner->UnderFirstPersonControl()) {
		UpdateDirectControl();
	} else {
		FollowPath();
	}
}

void CGroundMoveType::SlowUpdate()
{
	if (owner->GetTransporter() != NULL) {
		if (progressState == Active) {
			StopEngine(false);
		}
	} else {
		if (progressState == Active) {
			if (pathID != 0) {
				if (idling) {
					numIdlingSlowUpdates = std::min(MAX_IDLING_SLOWUPDATES, int(numIdlingSlowUpdates + 1));
				} else {
					numIdlingSlowUpdates = std::max(0, int(numIdlingSlowUpdates - 1));
				}

				if (numIdlingUpdates > (SHORTINT_MAXVALUE / turnRate)) {
					// case A: we have a path but are not moving
					LOG_L(L_DEBUG,
							"SlowUpdate: unit %i has pathID %i but %i ETA failures",
							owner->id, pathID, numIdlingUpdates);

					if (numIdlingSlowUpdates < MAX_IDLING_SLOWUPDATES) {
						ReRequestPath(false, true);
					} else {
						// unit probably ended up on a non-traversable
						// square, or got stuck in a non-moving crowd
						Fail(false);
					}
				}
			} else {
				// case B: we want to be moving but don't have a path
				LOG_L(L_DEBUG, "SlowUpdate: unit %i has no path", owner->id);
				ReRequestPath(false, true);
			}
		}

		if (!owner->IsFlying()) {
			// move us into the map, and update <oldPos>
			// to prevent any extreme changes in <speed>
			if (!owner->pos.IsInBounds()) {
				owner->Move(oldPos = owner->pos.cClampInBounds(), false);
			}
		}
	}

	AMoveType::SlowUpdate();
}


void CGroundMoveType::StartMovingRaw(const float3 moveGoalPos, float moveGoalRadius) {
	goalPos = moveGoalPos * XZVector;
	goalRadius = moveGoalRadius;

	currWayPoint = goalPos;
	nextWayPoint = goalPos;

	atGoal = ((moveGoalPos * XZVector) == (owner->pos * XZVector));
	atEndOfPath = false;

	useMainHeading = false;
	useRawMovement = true;
	progressState = Active;

	numIdlingUpdates = 0;
	numIdlingSlowUpdates = 0;

	currWayPointDist = 0.0f;
	prevWayPointDist = 0.0f;
}

void CGroundMoveType::StartMoving(float3 moveGoalPos, float moveGoalRadius) {
#ifdef TRACE_SYNC
	tracefile << "[" << __FUNCTION__ << "] ";
	tracefile << owner->pos.x << " " << owner->pos.y << " " << owner->pos.z << " " << owner->id << "\n";
#endif

	// set the new goal
	goalPos = moveGoalPos * XZVector;
	goalRadius = moveGoalRadius;

	atGoal = ((moveGoalPos * XZVector) == (owner->pos * XZVector));
	atEndOfPath = false;

	useMainHeading = false;
	useRawMovement = false;
	progressState = Active;

	numIdlingUpdates = 0;
	numIdlingSlowUpdates = 0;

	currWayPointDist = 0.0f;
	prevWayPointDist = 0.0f;

	LOG_L(L_DEBUG, "StartMoving: starting engine for unit %i", owner->id);

	if (atGoal)
		return;

	// silently free previous path if unit already had one
	//
	// units passing intermediate waypoints will TYPICALLY not cause any
	// script->{Start,Stop}Moving calls now (even when turnInPlace=true)
	// unless they come to a full stop first
	ReRequestPath(false, true);

	#if (PLAY_SOUNDS == 1)
	if (owner->team == gu->myTeam) {
		Channels::General.PlayRandomSample(owner->unitDef->sounds.activate, owner);
	}
	#endif
}

void CGroundMoveType::StopMoving(bool callScript, bool hardStop) {
#ifdef TRACE_SYNC
	tracefile << "[" << __FUNCTION__ << "] ";
	tracefile << owner->pos.x << " " << owner->pos.y << " " << owner->pos.z << " " << owner->id << "\n";
#endif

	LOG_L(L_DEBUG, "StopMoving: stopping engine for unit %i", owner->id);

	if (!atGoal) {
		currWayPoint = Here();
		goalPos = currWayPoint;
	}

	// this gets called under a variety of conditions (see MobileCAI)
	// the most common case is a CMD_STOP being issued which means no
	// StartMoving-->StartEngine will follow
	StopEngine(callScript, hardStop);

	useMainHeading = false;
	// only a new StartMoving call can reset this
	// useRawMovement = false;
	progressState = Done;
}



bool CGroundMoveType::FollowPath()
{
	bool wantReverse = false;

	if (WantToStop()) {
		ChangeSpeed(0.0f, false);
		SetMainHeading();
	} else {
		ASSERT_SYNCED(currWayPoint);
		ASSERT_SYNCED(nextWayPoint);
		ASSERT_SYNCED(owner->pos);

		prevWayPointDist = currWayPointDist;
		currWayPointDist = owner->pos.distance2D(currWayPoint);

		{
			// NOTE:
			//   uses owner->pos instead of currWayPoint (ie. not the same as atEndOfPath)
			//
			//   if our first command is a build-order, then goalRadius is set to our build-range
			//   and we cannot increase tolerance safely (otherwise the unit might stop when still
			//   outside its range and fail to start construction)
			const float curGoalDistSq = (owner->pos - goalPos).SqLength2D();
			const float minGoalDistSq = (UNIT_HAS_MOVE_CMD(owner))?
				Square(goalRadius * (numIdlingSlowUpdates + 1)):
				Square(goalRadius                             );

			atGoal |= (curGoalDistSq <= minGoalDistSq);
		}

		if (!atGoal) {
			if (!idling) {
				numIdlingUpdates = std::max(0, int(numIdlingUpdates - 1));
			} else {
				numIdlingUpdates = std::min(SHORTINT_MAXVALUE, int(numIdlingUpdates + 1));
			}
		}

		// atEndOfPath never becomes true when useRawMovement
		if (!atEndOfPath && !useRawMovement) {
			GetNextWayPoint();
		} else {
			if (atGoal) {
				Arrived(false);
			}
		}

		if (!atGoal) {
			// set direction to waypoint AFTER requesting it
			waypointDir = ((currWayPoint - owner->pos) * XZVector).SafeNormalize();
		}

		ASSERT_SYNCED(waypointDir);

		if (waypointDir.dot(flatFrontDir) < 0.0f) {
			wantReverse = WantReverse(waypointDir);
		}

		// apply obstacle avoidance (steering)
		const float3 rawWantedDir = waypointDir * Sign(int(!wantReverse));
		const float3& modWantedDir = GetObstacleAvoidanceDir(rawWantedDir);

		// ASSERT_SYNCED(modWantedDir);

		ChangeHeading(GetHeadingFromVector(modWantedDir.x, modWantedDir.z));
		ChangeSpeed(maxWantedSpeed * (1 - atGoal), wantReverse);
	}

	pathManager->UpdatePath(owner, pathID);
	return wantReverse;
}

void CGroundMoveType::ChangeSpeed(float newWantedSpeed, bool wantReverse, bool fpsMode)
{
	wantedSpeed = newWantedSpeed;

	// round low speeds to zero
	if (wantedSpeed <= 0.0f && currentSpeed < 0.01f) {
		currentSpeed = 0.0f;
		deltaSpeed = 0.0f;
		return;
	}

	// first calculate the "unrestricted" speed and acceleration
	float targetSpeed = mix(maxSpeed, maxReverseSpeed, wantReverse);

	#if (WAIT_FOR_PATH == 1)
	// don't move until we have an actual path, trying to hide queuing
	// lag is too dangerous since units can blindly drive into objects,
	// cliffs, etc. (requires the QTPFS idle-check in Update)
	if (currWayPoint.y == -1.0f && nextWayPoint.y == -1.0f) {
		targetSpeed = 0.0f;
	} else
	#endif
	{
		if (wantedSpeed > 0.0f) {
			const UnitDef* ud = owner->unitDef;
			const MoveDef* md = owner->moveDef;

			// the pathfinders do NOT check the entire footprint to determine
			// passability wrt. terrain (only wrt. structures), so we look at
			// the center square ONLY for our current speedmod
			const float groundSpeedMod = CMoveMath::GetPosSpeedMod(*md, owner->pos, flatFrontDir);

			const float curGoalDistSq = (owner->pos - goalPos).SqLength2D();
			const float minGoalDistSq = Square(BrakingDistance(currentSpeed, mix(decRate, accRate, reversing)));

			const float3& waypointDifFwd = waypointDir;
			const float3  waypointDifRev = -waypointDifFwd;

			const float3& waypointDif = reversing? waypointDifRev: waypointDifFwd;
			const short turnDeltaHeading = owner->heading - GetHeadingFromVector(waypointDif.x, waypointDif.z);

			// NOTE: <= 2 because every CMD_MOVE has a trailing CMD_SET_WANTED_MAX_SPEED
			const bool startBraking = (UNIT_CMD_QUE_SIZE(owner) <= 2 && curGoalDistSq <= minGoalDistSq && !fpsMode);

			if (!fpsMode && turnDeltaHeading != 0) {
				// only auto-adjust speed for turns when not in FPS mode
				const float reqTurnAngle = math::fabs(180.0f * short(owner->heading - wantedHeading) / SHORTINT_MAXVALUE);
				const float maxTurnAngle = (turnRate / SPRING_CIRCLE_DIVS) * 360.0f;

				float turnLinearSpeed = mix(maxSpeed, maxReverseSpeed, reversing);

				if (reqTurnAngle != 0.0f) {
					turnLinearSpeed *= std::min(std::max(0.1f, maxTurnAngle / reqTurnAngle), 1.0f);
				}

				if (waypointDir.SqLength() > 0.1f) {
					if (!ud->turnInPlace) {
						targetSpeed = std::max(ud->turnInPlaceSpeedLimit, turnLinearSpeed);
					} else {
						if (reqTurnAngle > ud->turnInPlaceAngleLimit) {
							targetSpeed = turnLinearSpeed;
						}
					}
				}

				if (atEndOfPath) {
					// at this point, Update() will no longer call GetNextWayPoint()
					// and we must slow down to prevent entering an infinite circle
					targetSpeed = std::min(targetSpeed, (currWayPointDist * PI) / (SPRING_CIRCLE_DIVS / turnRate));
				}
			}

			// now apply the terrain and command restrictions
			// NOTE:
			//   if wantedSpeed > targetSpeed, the unit will
			//   not accelerate to speed > targetSpeed unless
			//   its actual max{Reverse}Speed is also changed
			//
			//   raise wantedSpeed iff the terrain-modifier is
			//   larger than 1 (so units still get their speed
			//   bonus correctly), otherwise leave it untouched
			//
			//   disallow changing speed (except to zero) without
			//   a path if not in FPS mode (FIXME: legacy PFS can
			//   return path when none should exist, mantis3720)
			wantedSpeed *= std::max(groundSpeedMod, 1.0f);
			targetSpeed *= groundSpeedMod;
			targetSpeed *= (1 - startBraking);
			targetSpeed *= ((1 - WantToStop()) || fpsMode);
			targetSpeed = std::min(targetSpeed, wantedSpeed);
		} else {
			targetSpeed = 0.0f;
		}
	}

	deltaSpeed = pathController->GetDeltaSpeed(
		pathID,
		targetSpeed,
		currentSpeed,
		accRate,
		decRate,
		wantReverse,
		reversing
	);
}

/*
 * Changes the heading of the owner.
 * FIXME near-duplicate of HoverAirMoveType::UpdateHeading
 */
void CGroundMoveType::ChangeHeading(short newHeading) {
	if (owner->IsFlying()) return;
	if (owner->GetTransporter() != NULL) return;

	#if 0
	owner->heading += pathController->GetDeltaHeading(pathID, (wantedHeading = newHeading), owner->heading, turnRate);
	#else
	// model rotational inertia (more realistic for ships)
	owner->heading += pathController->GetDeltaHeading(pathID, (wantedHeading = newHeading), owner->heading, turnRate, turnAccel, BrakingDistance(turnSpeed, turnAccel), &turnSpeed);
	#endif

	owner->UpdateDirVectors(!owner->upright);
	owner->UpdateMidAndAimPos();

	flatFrontDir = owner->frontdir;
	flatFrontDir = (flatFrontDir * XZVector).Normalize();
}




bool CGroundMoveType::CanApplyImpulse(const float3& impulse)
{
	// NOTE: ships must be able to receive impulse too (for collision handling)
	if (owner->beingBuilt)
		return false;
	// will be applied to transporter instead
	if (owner->GetTransporter() != NULL)
		return false;
	if (impulse.SqLength() <= 0.01f)
		return false;

	useHeading = false;

	skidRotSpeed = 0.0f;
	skidRotAccel = 0.0f;

	float3 newSpeed = owner->speed + impulse;
	float3 skidDir = owner->frontdir;

	// NOTE:
	//   we no longer delay the skidding-state until owner has "accumulated" an
	//   arbitrary hardcoded amount of impulse (possibly across several frames),
	//   but enter it on any vector that causes speed to become misaligned with
	//   frontdir
	// TODO (95.0+):
	//   there should probably be a configurable minimum-impulse below which the
	//   unit does not react at all but also does NOT "store" the impulse like a
	//   small-charge capacitor
	//
	const bool startSkidding = StartSkidding(newSpeed, skidDir);
	const bool startFlying = StartFlying(newSpeed, CGround::GetNormal(owner->pos.x, owner->pos.z));

	if (newSpeed.SqLength2D() >= 0.01f) {
		skidDir = newSpeed.Normalize2D();
	}

	skidRotVector = skidDir.cross(UpVector) * startSkidding;
	skidRotAccel = ((gs->randFloat() - 0.5f) * 0.04f) * startFlying;

	owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING * (startSkidding | startFlying));
	owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING * startFlying);

	// indicate we want to react to the impulse
	return true;
}

void CGroundMoveType::UpdateSkid()
{
	ASSERT_SYNCED(owner->midPos);

	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	const UnitDef* ud = owner->unitDef;
	const float groundHeight = GetGroundHeight(pos);

	owner->SetVelocity(spd + owner->GetDragAccelerationVec(float4(mapInfo->atmosphere.fluidDensity, mapInfo->water.fluidDensity, 1.0f, 0.01f)));

	if (owner->IsFlying()) {
		const float impactSpeed = pos.IsInBounds()?
			-spd.dot(CGround::GetNormal(pos.x, pos.z)):
			-spd.dot(UpVector);
		const float impactDamageMult = impactSpeed * owner->mass * COLLISION_DAMAGE_MULT;
		const bool doColliderDamage = (modInfo.allowUnitCollisionDamage && impactSpeed > ud->minCollisionSpeed && ud->minCollisionSpeed >= 0.0f);

		if (groundHeight > pos.y) {
			// ground impact, stop flying
			owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
			owner->Move(UpVector * (groundHeight - pos.y), true);

			// deal ground impact damage
			// TODO:
			//   bouncing behaves too much like a rubber-ball,
			//   most impact energy needs to go into the ground
			if (doColliderDamage) {
				owner->DoDamage(DamageArray(impactDamageMult), ZeroVector, NULL, -CSolidObject::DAMAGE_COLLISION_GROUND, -1);
			}

			skidRotSpeed = 0.0f;
			// skidRotAccel = 0.0f;
		} else {
			owner->SetVelocity(spd + (UpVector * mapInfo->map.gravity));
		}
	} else {
		// *assume* this means the unit is still on the ground
		// (Lua gadgetry can interfere with our "physics" logic)
		float skidRotSpd = 0.0f;

		const bool onSlope = OnSlope(0.0f);
		const float speedReduction = 0.35f;

		if (!onSlope && StopSkidding(spd, owner->frontdir)) {
			useHeading = true;

			skidRotSpd = math::floor(skidRotSpeed + skidRotAccel + 0.5f);
			skidRotAccel = (skidRotSpd - skidRotSpeed) * 0.5f;
			skidRotAccel *= (PI / 180.0f);

			owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING);
			// update wanted-heading after coming to a stop
			ChangeHeading(owner->heading);
		} else {
			// number of frames until rotational speed would drop to 0
			const float speedScale = owner->SetSpeed(spd);
			const float remTime = std::max(1.0f, speedScale / speedReduction);

			if (onSlope) {
				const float3 normalVector = CGround::GetNormal(pos.x, pos.z);
				const float3 normalForce = normalVector * normalVector.dot(UpVector * mapInfo->map.gravity);
				const float3 newForce = UpVector * mapInfo->map.gravity - normalForce;

				owner->SetVelocity(spd + newForce);
				owner->SetVelocity(spd * (1.0f - (0.1f * normalVector.y)));
			} else {
				// RHS is clamped to 0..1
				owner->SetVelocity(spd * (1.0f - std::min(1.0f, speedReduction / speedScale)));
			}

			skidRotSpd = math::floor(skidRotSpeed + skidRotAccel * (remTime - 1.0f) + 0.5f);
			skidRotAccel = (skidRotSpd - skidRotSpeed) / remTime;
			skidRotAccel *= (PI / 180.0f);

			if (math::floor(skidRotSpeed) != math::floor(skidRotSpeed + skidRotAccel)) {
				skidRotSpeed = 0.0f;
				skidRotAccel = 0.0f;
			}
		}

		if ((groundHeight - pos.y) < (spd.y + mapInfo->map.gravity)) {
			owner->SetVelocity(spd + (UpVector * mapInfo->map.gravity));

			// flying requires skidding and relies on CalcSkidRot
			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING);

			useHeading = false;
		} else if ((groundHeight - pos.y) > spd.y) {
			// LHS is always negative, so this becomes true when the
			// unit is falling back down and will impact the ground
			// in one frame
			const float3& normal = (pos.IsInBounds())? CGround::GetNormal(pos.x, pos.z): UpVector;
			const float dot = spd.dot(normal);

			if (dot > 0.0f) {
				// not possible without lateral movement
				owner->SetVelocity(spd * 0.95f);
			} else {
				owner->SetVelocity(spd + (normal * (math::fabs(spd.dot(normal)) + 0.1f)));
				owner->SetVelocity(spd * 0.8f);
			}
		}
	}

	// finally update speed.w
	owner->SetSpeed(spd);
	// translate before rotate, match terrain normal if not in air
	owner->Move(spd, true);
	owner->UpdateDirVectors(!owner->upright);

	if (owner->IsSkidding()) {
		CalcSkidRot();
		CheckCollisionSkid();
	} else {
		// do this here since ::Update returns early if it calls us
		HandleObjectCollisions();
	}

	// always update <oldPos> here so that <speed> does not make
	// extreme jumps when the unit transitions from skidding back
	// to non-skidding
	oldPos = owner->pos;

	ASSERT_SANE_OWNER_SPEED(spd);
	ASSERT_SYNCED(owner->midPos);
}

void CGroundMoveType::UpdateControlledDrop()
{
	const float3& pos = owner->pos;
	const float4& spd = owner->speed;
	const float3  acc = UpVector * std::min(mapInfo->map.gravity * owner->fallSpeed, 0.0f);
	const float    gh = GetGroundHeight(pos);

	owner->SetVelocity(spd + acc);
	owner->SetVelocity(spd + owner->GetDragAccelerationVec(float4(mapInfo->atmosphere.fluidDensity, mapInfo->water.fluidDensity, 1.0f, 0.1f)));
	owner->SetSpeed(spd);
	owner->Move(spd, true);

	if (gh > pos.y) {
		// ground impact, stop parachute animation
		owner->Move(UpVector * (gh - pos.y), true);
		owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_FALLING);
		owner->script->Landed();
	}
}

void CGroundMoveType::CheckCollisionSkid()
{
	CUnit* collider = owner;

	// NOTE:
	//   the QuadField::Get* functions check o->midPos,
	//   but the quad(s) that objects are stored in are
	//   derived from o->pos (!)
	const float3& pos = collider->pos;

	const UnitDef* colliderUD = collider->unitDef;
	const vector<CUnit*>& nearUnits = quadField->GetUnitsExact(pos, collider->radius);
	const vector<CFeature*>& nearFeatures = quadField->GetFeaturesExact(pos, collider->radius);

	vector<CUnit*>::const_iterator ui;
	vector<CFeature*>::const_iterator fi;

	for (ui = nearUnits.begin(); ui != nearUnits.end(); ++ui) {
		CUnit* collidee = *ui;

		if (!collidee->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
			continue;

		const UnitDef* collideeUD = collider->unitDef;

		const float sqDist = (pos - collidee->pos).SqLength();
		const float totRad = collider->radius + collidee->radius;

		if ((sqDist >= totRad * totRad) || (sqDist <= 0.01f))
			continue;

		const float3 dif = (pos - collidee->pos).SafeNormalize();

		if (collidee->unitDef->IsImmobileUnit()) {
			const float impactSpeed = -collider->speed.dot(dif);
			const float impactDamageMult = std::min(impactSpeed * collider->mass * COLLISION_DAMAGE_MULT, MAX_UNIT_SPEED);

			const bool doColliderDamage = (modInfo.allowUnitCollisionDamage && impactSpeed > colliderUD->minCollisionSpeed && colliderUD->minCollisionSpeed >= 0.0f);
			const bool doCollideeDamage = (modInfo.allowUnitCollisionDamage && impactSpeed > collideeUD->minCollisionSpeed && collideeUD->minCollisionSpeed >= 0.0f);

			if (impactSpeed <= 0.0f)
				continue;

			// damage the collider, no added impulse
			if (doColliderDamage) {
				collider->DoDamage(DamageArray(impactDamageMult), ZeroVector, NULL, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);
			}
			// damage the (static) collidee based on collider's mass, no added impulse
			if (doCollideeDamage) {
				collidee->DoDamage(DamageArray(impactDamageMult), ZeroVector, NULL, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);
			}

			collider->Move(dif * impactSpeed, true);
			collider->SetVelocity(collider->speed + ((dif * impactSpeed) * 1.8f));
		} else {
			assert(collider->mass > 0.0f && collidee->mass > 0.0f);

			// don't conserve momentum (impact speed is halved, so impulses are too)
			// --> collisions are neither truly elastic nor truly inelastic to prevent
			// the simulation from blowing up from impulses applied to tight groups of
			// units
			const float impactSpeed = (collidee->speed - collider->speed).dot(dif) * 0.5f;
			const float colliderRelMass = (collider->mass / (collider->mass + collidee->mass));
			const float colliderRelImpactSpeed = impactSpeed * (1.0f - colliderRelMass);
			const float collideeRelImpactSpeed = impactSpeed * (       colliderRelMass);

			const float  colliderImpactDmgMult = std::min(colliderRelImpactSpeed * collider->mass * COLLISION_DAMAGE_MULT, MAX_UNIT_SPEED);
			const float  collideeImpactDmgMult = std::min(collideeRelImpactSpeed * collider->mass * COLLISION_DAMAGE_MULT, MAX_UNIT_SPEED);
			const float3 colliderImpactImpulse = dif * colliderRelImpactSpeed;
			const float3 collideeImpactImpulse = dif * collideeRelImpactSpeed;

			const bool doColliderDamage = (modInfo.allowUnitCollisionDamage && impactSpeed > colliderUD->minCollisionSpeed && colliderUD->minCollisionSpeed >= 0.0f);
			const bool doCollideeDamage = (modInfo.allowUnitCollisionDamage && impactSpeed > collideeUD->minCollisionSpeed && collideeUD->minCollisionSpeed >= 0.0f);

			if (impactSpeed <= 0.0f)
				continue;

			// damage the collider
			if (doColliderDamage) {
				collider->DoDamage(DamageArray(colliderImpactDmgMult), dif * colliderImpactDmgMult, NULL, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);
			}
			// damage the collidee
			if (doCollideeDamage) {
				collidee->DoDamage(DamageArray(collideeImpactDmgMult), dif * -collideeImpactDmgMult, NULL, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);
			}

			collider->Move( colliderImpactImpulse, true);
			collidee->Move(-collideeImpactImpulse, true);
			collider->SetVelocity        (collider->speed + colliderImpactImpulse);
			collidee->SetVelocityAndSpeed(collidee->speed - collideeImpactImpulse);
		}
	}

	for (fi = nearFeatures.begin(); fi != nearFeatures.end(); ++fi) {
		CFeature* collidee = *fi;

		if (!collidee->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
			continue;

		const float sqDist = (pos - collidee->pos).SqLength();
		const float totRad = collider->radius + collidee->radius;

		if ((sqDist >= totRad * totRad) || (sqDist <= 0.01f))
			continue;

		const float3 dif = (pos - collidee->pos).SafeNormalize();
		const float impactSpeed = -collider->speed.dot(dif);
		const float impactDamageMult = std::min(impactSpeed * collider->mass * COLLISION_DAMAGE_MULT, MAX_UNIT_SPEED);
		const float3 impactImpulse = dif * impactSpeed;
		const bool doColliderDamage = (modInfo.allowUnitCollisionDamage && impactSpeed > colliderUD->minCollisionSpeed && colliderUD->minCollisionSpeed >= 0.0f);

		if (impactSpeed <= 0.0f)
			continue;

		// damage the collider, no added impulse (!)
		if (doColliderDamage) {
			collider->DoDamage(DamageArray(impactDamageMult), ZeroVector, NULL, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);
		}

		// damage the collidee feature based on collider's mass
		collidee->DoDamage(DamageArray(impactDamageMult), -impactImpulse, NULL, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);

		collider->Move(impactImpulse, true);
		collider->SetVelocity(collider->speed + (impactImpulse * 1.8f));
	}

	// finally update speed.w
	collider->SetSpeed(collider->speed);

	ASSERT_SANE_OWNER_SPEED(collider->speed);
}

void CGroundMoveType::CalcSkidRot()
{
	skidRotSpeed += skidRotAccel;
	skidRotSpeed *= 0.999f;
	skidRotAccel *= 0.95f;

	const float angle = (skidRotSpeed / GAME_SPEED) * (PI * 2.0f);
	const float cosp = math::cos(angle);
	const float sinp = math::sin(angle);

	float3 f1 = skidRotVector * skidRotVector.dot(owner->frontdir);
	float3 f2 = owner->frontdir - f1;

	float3 r1 = skidRotVector * skidRotVector.dot(owner->rightdir);
	float3 r2 = owner->rightdir - r1;

	float3 u1 = skidRotVector * skidRotVector.dot(owner->updir);
	float3 u2 = owner->updir - u1;

	f2 = f2 * cosp + f2.cross(skidRotVector) * sinp;
	r2 = r2 * cosp + r2.cross(skidRotVector) * sinp;
	u2 = u2 * cosp + u2.cross(skidRotVector) * sinp;

	owner->frontdir = f1 + f2;
	owner->rightdir = r1 + r2;
	owner->updir    = u1 + u2;

	owner->UpdateMidAndAimPos();
}




/*
 * Dynamic obstacle avoidance, helps the unit to
 * follow the path even when it's not perfect.
 */
float3 CGroundMoveType::GetObstacleAvoidanceDir(const float3& desiredDir) {
	#if (IGNORE_OBSTACLES == 1)
	return desiredDir;
	#endif

	// obstacle-avoidance only needs to run if the unit wants to move
	if (WantToStop())
		return ZeroVector;

	// Speed-optimizer. Reduces the times this system is run.
	if (gs->frameNum < nextObstacleAvoidanceFrame)
		return lastAvoidanceDir;

	float3 avoidanceVec = ZeroVector;
	float3 avoidanceDir = desiredDir;

	lastAvoidanceDir = desiredDir;
	nextObstacleAvoidanceFrame = gs->frameNum + 1;

	CUnit* avoider = owner;
	// const UnitDef* avoiderUD = avoider->unitDef;
	const MoveDef* avoiderMD = avoider->moveDef;

	// degenerate case: if facing anti-parallel to desired direction,
	// do not actively avoid obstacles since that can interfere with
	// normal waypoint steering (if the final avoidanceDir demands a
	// turn in the opposite direction of desiredDir)
	if (avoider->frontdir.dot(desiredDir) < 0.0f)
		return lastAvoidanceDir;

	static const float AVOIDER_DIR_WEIGHT = 1.0f;
	static const float DESIRED_DIR_WEIGHT = 0.5f;
	static const float MAX_AVOIDEE_COSINE = math::cosf(120.0f * (PI / 180.0f));
	static const float LAST_DIR_MIX_ALPHA = 0.7f;

	// now we do the obstacle avoidance proper
	// avoider always uses its never-rotated MoveDef footprint
	// note: should increase radius for smaller turnAccel values
	const float avoidanceRadius = std::max(currentSpeed, 1.0f) * (avoider->radius * 2.0f);
	const float avoiderRadius = FOOTPRINT_RADIUS(avoiderMD->xsize, avoiderMD->zsize, 1.0f);

	const vector<CSolidObject*>& objects = quadField->GetSolidsExact(avoider->pos, avoidanceRadius, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS);

	for (vector<CSolidObject*>::const_iterator oi = objects.begin(); oi != objects.end(); ++oi) {
		const CSolidObject* avoidee = *oi;
		const MoveDef* avoideeMD = avoidee->moveDef;
		const UnitDef* avoideeUD = dynamic_cast<const UnitDef*>(avoidee->objectDef);

		// cases in which there is no need to avoid this obstacle
		if (avoidee == owner)
			continue;
		// do not avoid statics (it interferes too much with PFS)
		if (avoideeMD == NULL)
			continue;
		// ignore aircraft (or flying ground units)
		if (avoidee->IsInAir() || avoidee->IsFlying())
			continue;
		if (CMoveMath::IsNonBlocking(*avoiderMD, avoidee, avoider))
			continue;
		if (!CMoveMath::CrushResistant(*avoiderMD, avoidee))
			continue;

		const bool avoideeMobile  = (avoideeMD != NULL);
		const bool avoideeMovable = (avoideeUD != NULL && !avoideeUD->pushResistant);

		const float3 avoideeVector = (avoider->pos + avoider->speed) - (avoidee->pos + avoidee->speed);

		// use the avoidee's MoveDef footprint as radius if it is mobile
		// use the avoidee's Unit (not UnitDef) footprint as radius otherwise
		const float avoideeRadius = avoideeMobile?
			FOOTPRINT_RADIUS(avoideeMD->xsize, avoideeMD->zsize, 1.0f):
			FOOTPRINT_RADIUS(avoidee  ->xsize, avoidee  ->zsize, 1.0f);
		const float avoidanceRadiusSum = avoiderRadius + avoideeRadius;
		const float avoidanceMassSum = avoider->mass + avoidee->mass;
		const float avoideeMassScale = avoideeMobile? (avoidee->mass / avoidanceMassSum): 1.0f;
		const float avoideeDistSq = avoideeVector.SqLength();
		const float avoideeDist   = fastmath::sqrt2(avoideeDistSq) + 0.01f;

		// do not bother steering around idling MOBILE objects
		// (since collision handling will just push them aside)
		if (avoideeMobile && avoideeMovable) {
			if (!avoiderMD->avoidMobilesOnPath || (!avoidee->IsMoving() && avoidee->allyteam == avoider->allyteam)) {
				continue;
			}
		}

		// ignore objects that are more than this many degrees off-center from us
		// NOTE:
		//   if MAX_AVOIDEE_COSINE is too small, then this condition can be true
		//   one frame and false the next (after avoider has turned) causing the
		//   avoidance vector to oscillate --> units with turnInPlace = true will
		//   slow to a crawl as a result
		if (avoider->frontdir.dot(-(avoideeVector / avoideeDist)) < MAX_AVOIDEE_COSINE)
			continue;

		if (avoideeDistSq >= Square(std::max(currentSpeed, 1.0f) * GAME_SPEED + avoidanceRadiusSum))
			continue;
		if (avoideeDistSq >= avoider->pos.SqDistance2D(goalPos))
			continue;

		// if object and unit in relative motion are closing in on one another
		// (or not yet fully apart), then the object is on the path of the unit
		// and they are not collided
		if (DEBUG_DRAWING_ENABLED) {
			if (selectedUnitsHandler.selectedUnits.find(owner) != selectedUnitsHandler.selectedUnits.end()) {
				geometricObjects->AddLine(avoider->pos + (UpVector * 20.0f), avoidee->pos + (UpVector * 20.0f), 3, 1, 4);
			}
		}

		float avoiderTurnSign = -Sign(avoidee->pos.dot(avoider->rightdir) - avoider->pos.dot(avoider->rightdir));
		float avoideeTurnSign = -Sign(avoider->pos.dot(avoidee->rightdir) - avoidee->pos.dot(avoidee->rightdir));

		// for mobile units, avoidance-response is modulated by angle
		// between avoidee's and avoider's frontdir such that maximal
		// avoidance occurs when they are anti-parallel
		const float avoidanceCosAngle = Clamp(avoider->frontdir.dot(avoidee->frontdir), -1.0f, 1.0f);
		const float avoidanceResponse = (1.0f - avoidanceCosAngle * int(avoideeMobile)) + 0.1f;
		const float avoidanceFallOff  = (1.0f - std::min(1.0f, avoideeDist / (5.0f * avoidanceRadiusSum)));

		// if parties are anti-parallel, it is always more efficient for
		// both to turn in the same local-space direction (either R/R or
		// L/L depending on relative object positions) but there exists
		// a range of orientations for which the signs are not equal
		//
		// (this is also true for the parallel situation, but there the
		// degeneracy only occurs when one of the parties is behind the
		// other and can be ignored)
		if (avoidanceCosAngle < 0.0f)
			avoiderTurnSign = std::max(avoiderTurnSign, avoideeTurnSign);

		avoidanceDir = avoider->rightdir * AVOIDER_DIR_WEIGHT * avoiderTurnSign;
		avoidanceVec += (avoidanceDir * avoidanceResponse * avoidanceFallOff * avoideeMassScale);
	}


	// use a weighted combination of the desired- and the avoidance-directions
	// also linearly smooth it using the vector calculated the previous frame
	avoidanceDir = (mix(desiredDir, avoidanceVec, DESIRED_DIR_WEIGHT)).SafeNormalize();
	avoidanceDir = (mix(avoidanceDir, lastAvoidanceDir, LAST_DIR_MIX_ALPHA)).SafeNormalize();

	if (DEBUG_DRAWING_ENABLED) {
		if (selectedUnitsHandler.selectedUnits.find(owner) != selectedUnitsHandler.selectedUnits.end()) {
			const float3 p0 = owner->pos + (    UpVector * 20.0f);
			const float3 p1 =         p0 + (avoidanceVec * 40.0f);
			const float3 p2 =         p0 + (avoidanceDir * 40.0f);

			const int avFigGroupID = geometricObjects->AddLine(p0, p1, 8.0f, 1, 4);
			const int adFigGroupID = geometricObjects->AddLine(p0, p2, 8.0f, 1, 4);

			geometricObjects->SetColor(avFigGroupID, 1, 0.3f, 0.3f, 0.6f);
			geometricObjects->SetColor(adFigGroupID, 1, 0.3f, 0.3f, 0.6f);
		}
	}

	return (lastAvoidanceDir = avoidanceDir);
}



#if 0
// Calculates an aproximation of the physical 2D-distance between given two objects.
// Old, no longer used since all separation tests are based on FOOTPRINT_RADIUS now.
float CGroundMoveType::Distance2D(CSolidObject* object1, CSolidObject* object2, float marginal)
{
	// calculate the distance in (x,z) depending
	// on the shape of the object footprints
	float dist2D = 0.0f;

	const float xs = ((object1->xsize + object2->xsize) * (SQUARE_SIZE >> 1));
	const float zs = ((object1->zsize + object2->zsize) * (SQUARE_SIZE >> 1));

	if (object1->xsize == object1->zsize || object2->xsize == object2->zsize) {
		// use xsize as a cylindrical radius.
		const float3 distVec = object1->midPos - object2->midPos;

		dist2D = distVec.Length2D() - xs + 2.0f * marginal;
	} else {
		// Pytagorean sum of the x and z distance.
		float3 distVec;

		const float xdiff = math::fabs(object1->midPos.x - object2->midPos.x);
		const float zdiff = math::fabs(object1->midPos.z - object2->midPos.z);

		distVec.x = xdiff - xs + 2.0f * marginal;
		distVec.z = zdiff - zs + 2.0f * marginal;

		if (distVec.x > 0.0f && distVec.z > 0.0f) {
			dist2D = distVec.Length2D();
		} else if (distVec.x < 0.0f && distVec.z < 0.0f) {
			dist2D = -distVec.Length2D();
		} else if (distVec.x > 0.0f) {
			dist2D = distVec.x;
		} else {
			dist2D = distVec.z;
		}
	}

	return dist2D;
}
#endif

// Creates a path to the goal.
unsigned int CGroundMoveType::GetNewPath()
{
	unsigned int newPathID = 0;

	if (useRawMovement)
		return newPathID;
	// avoid frivolous requests if called from outside StartMoving*()
	if ((owner->pos - goalPos).SqLength2D() <= Square(goalRadius))
		return newPathID;

	if ((newPathID = pathManager->RequestPath(owner, owner->moveDef, owner->pos, goalPos, goalRadius, true)) != 0) {
		atGoal = false;
		atEndOfPath = false;

		currWayPoint = pathManager->NextWayPoint(owner, newPathID, 0,   owner->pos, 1.25f * SQUARE_SIZE, true);
		nextWayPoint = pathManager->NextWayPoint(owner, newPathID, 0, currWayPoint, 1.25f * SQUARE_SIZE, true);

		pathController->SetRealGoalPosition(newPathID, goalPos);
		pathController->SetTempGoalPosition(newPathID, currWayPoint);
	} else {
		Fail(false);
	}

	return newPathID;
}

bool CGroundMoveType::ReRequestPath(bool callScript, bool forceRequest) {
	// limit frequency of repath-requests from outside SlowUpdate
	if (((gs->frameNum - lastPathRequestFrame) < (UNIT_SLOWUPDATE_RATE >> 1)) && (!forceRequest))
		return false;

	StopEngine(callScript);
	StartEngine(callScript);

	lastPathRequestFrame = gs->frameNum;
	return true;
}



bool CGroundMoveType::CanGetNextWayPoint() {
	assert(!useRawMovement);

	if (pathID == 0)
		return false;
	if (!pathController->AllowSetTempGoalPosition(pathID, nextWayPoint))
		return false;


	if (currWayPoint.y != -1.0f && nextWayPoint.y != -1.0f) {
		const float3& pos = owner->pos;
		      float3& cwp = (float3&) currWayPoint;
		      float3& nwp = (float3&) nextWayPoint;

		if (pathManager->PathUpdated(pathID)) {
			// path changed while we were following it (eg. due
			// to terrain deformation) in between two waypoints
			// but still has the same ID; in this case (which is
			// specific to QTPFS) we don't go through GetNewPath
			//
			cwp = pathManager->NextWayPoint(owner, pathID, 0, pos, 1.25f * SQUARE_SIZE, true);
			nwp = pathManager->NextWayPoint(owner, pathID, 0, cwp, 1.25f * SQUARE_SIZE, true);
		}

		if (DEBUG_DRAWING_ENABLED) {
			if (selectedUnitsHandler.selectedUnits.find(owner) != selectedUnitsHandler.selectedUnits.end()) {
				// plot the vectors to {curr, next}WayPoint
				const int cwpFigGroupID = geometricObjects->AddLine(pos + (UpVector * 20.0f), cwp + (UpVector * (pos.y + 20.0f)), 8.0f, 1, 4);
				const int nwpFigGroupID = geometricObjects->AddLine(pos + (UpVector * 20.0f), nwp + (UpVector * (pos.y + 20.0f)), 8.0f, 1, 4);

				geometricObjects->SetColor(cwpFigGroupID, 1, 0.3f, 0.3f, 0.6f);
				geometricObjects->SetColor(nwpFigGroupID, 1, 0.3f, 0.3f, 0.6f);
			}
		}

		// perform a turn-radius check: if the waypoint
		// lies outside our turning circle, don't skip
		// it (since we can steer toward this waypoint
		// and pass it without slowing down)
		// note that we take the DIAMETER of the circle
		// to prevent sine-like "snaking" trajectories
		const int dirSign = Sign(int(!reversing));
		const float turnFrames = SPRING_CIRCLE_DIVS / turnRate;
		const float turnRadius = (owner->speed.w * turnFrames) / (PI + PI);
		const float waypointDot = Clamp(waypointDir.dot(flatFrontDir * dirSign), -1.0f, 1.0f);

		#if 1
		if (currWayPointDist > (turnRadius * 2.0f)) {
			return false;
		}
		if (currWayPointDist > SQUARE_SIZE && waypointDot >= 0.995f) {
			return false;
		}
		#else
		if ((currWayPointDist > std::max(turnRadius * 2.0f, 1.0f * SQUARE_SIZE)) && (waypointDot >= 0.0f)) {
			return false;
		}
		if ((currWayPointDist > std::max(turnRadius * 1.0f, 1.0f * SQUARE_SIZE)) && (waypointDot <  0.0f)) {
			return false;
		}
		if (math::acosf(waypointDot) < ((turnRate / SPRING_CIRCLE_DIVS) * (PI + PI))) {
			return false;
		}
		#endif

		{
			// check the rectangle between pos and cwp for obstacles
			const int xmin = std::min(cwp.x / SQUARE_SIZE, pos.x / SQUARE_SIZE), xmax = std::max(cwp.x / SQUARE_SIZE, pos.x / SQUARE_SIZE);
			const int zmin = std::min(cwp.z / SQUARE_SIZE, pos.z / SQUARE_SIZE), zmax = std::max(cwp.z / SQUARE_SIZE, pos.z / SQUARE_SIZE);

			const MoveDef* ownerMD = owner->moveDef;

			for (int x = xmin; x < xmax; x++) {
				for (int z = zmin; z < zmax; z++) {
					if (ownerMD->TestMoveSquare(owner, x, z, ZeroVector, true, true, true)) {
						continue;
					}
					// if still further than SS elmos from waypoint, disallow skipping
					// note: can somehow cause units to move in circles near obstacles
					// (mantis3718) if rectangle is too generous in size
					if ((pos - cwp).SqLength() > Square(SQUARE_SIZE) && (pos - cwp).dot(flatFrontDir) >= 0.0f) {
						return false;
					}
				}
			}
		}

		{
			const float curGoalDistSq = (currWayPoint - goalPos).SqLength2D();
			const float minGoalDistSq = (UNIT_HAS_MOVE_CMD(owner))?
				Square(goalRadius * (numIdlingSlowUpdates + 1)):
				Square(goalRadius                             );

			// trigger Arrived on the next Update (but
			// only if we have non-temporary waypoints)
			// atEndOfPath |= (currWayPoint == nextWayPoint);
			atEndOfPath |= (curGoalDistSq <= minGoalDistSq);
		}

		if (atEndOfPath) {
			currWayPoint = goalPos;
			nextWayPoint = goalPos;
			return false;
		}
	}

	return true;
}

void CGroundMoveType::GetNextWayPoint()
{
	assert(!useRawMovement);

	if (CanGetNextWayPoint()) {
		pathController->SetTempGoalPosition(pathID, nextWayPoint);

		// NOTE: pathfinder implementation should ensure waypoints are not equal
		currWayPoint = nextWayPoint;
		nextWayPoint = pathManager->NextWayPoint(owner, pathID, 0, currWayPoint, 1.25f * SQUARE_SIZE, true);
	}

	if (nextWayPoint.x == -1.0f && nextWayPoint.z == -1.0f) {
		Fail(false);
	} else {
		#define CWP_BLOCK_MASK CMoveMath::SquareIsBlocked(*owner->moveDef, currWayPoint, owner)
		#define NWP_BLOCK_MASK CMoveMath::SquareIsBlocked(*owner->moveDef, nextWayPoint, owner)

		if ((CWP_BLOCK_MASK & CMoveMath::BLOCK_STRUCTURE) != 0 || (NWP_BLOCK_MASK & CMoveMath::BLOCK_STRUCTURE) != 0) {
			// this can happen if we crushed a non-blocking feature
			// and it spawned another feature which we cannot crush
			// (eg.) --> repath
			ReRequestPath(false, false);
		}

		#undef NWP_BLOCK_MASK
		#undef CWP_BLOCK_MASK
	}
}




/*
The distance the unit will move before stopping,
starting from given speed and applying maximum
brake rate.
*/
float CGroundMoveType::BrakingDistance(float speed, float rate) const
{
	const float time = speed / std::max(rate, 0.001f);
	const float dist = 0.5f * rate * time * time;
	return dist;
}

/*
Gives the position this unit will end up at with full braking
from current velocity.
*/
float3 CGroundMoveType::Here()
{
	const float dist = BrakingDistance(currentSpeed, mix(decRate, accRate, reversing));
	const int   sign = Sign(int(!reversing));

	const float3 pos2D = owner->pos * XZVector;
	const float3 dir2D = flatFrontDir * dist * sign;

	return (pos2D + dir2D);
}






void CGroundMoveType::StartEngine(bool callScript) {
	if (pathID == 0)
		pathID = GetNewPath();

	if (pathID != 0) {
		pathManager->UpdatePath(owner, pathID);

		if (callScript) {
			// makes no sense to call this unless we have a new path
			owner->script->StartMoving(reversing);
		}
	}

	nextObstacleAvoidanceFrame = gs->frameNum;
}

void CGroundMoveType::StopEngine(bool callScript, bool hardStop) {
	if (pathID != 0) {
		pathManager->DeletePath(pathID);
		pathID = 0;

		if (callScript) {
			owner->script->StopMoving();
		}
	}

	owner->SetVelocityAndSpeed(owner->speed * (1 - hardStop));

	currentSpeed *= (1 - hardStop);
	wantedSpeed = 0.0f;
}



/* Called when the unit arrives at its goal. */
void CGroundMoveType::Arrived(bool callScript)
{
	// can only "arrive" if the engine is active
	if (progressState == Active) {
		StopEngine(callScript);

		#if (PLAY_SOUNDS == 1)
		if (owner->team == gu->myTeam) {
			Channels::General.PlayRandomSample(owner->unitDef->sounds.arrived, owner);
		}
		#endif

		// and the action is done
		progressState = Done;

		// FIXME:
		//   CAI sometimes does not update its queue correctly
		//   (probably whenever we are called "before" the CAI
		//   is ready to accept that a unit is at its goal-pos)
		owner->commandAI->GiveCommand(Command(CMD_WAIT));
		owner->commandAI->GiveCommand(Command(CMD_WAIT));

		if (!owner->commandAI->HasMoreMoveCommands()) {
			// update the position-parameter of our queue's front CMD_MOVE
			// this is needed in case we Arrive()'ed non-directly (through
			// colliding with another unit that happened to share our goal)
			static_cast<CMobileCAI*>(owner->commandAI)->SetFrontMoveCommandPos(owner->pos);
		}

		LOG_L(L_DEBUG, "Arrived: unit %i arrived", owner->id);
	}
}

/*
Makes the unit fail this action.
No more trials will be done before a new goal is given.
*/
void CGroundMoveType::Fail(bool callScript)
{
	LOG_L(L_DEBUG, "Fail: unit %i failed", owner->id);

	StopEngine(callScript);

	// failure of finding a path means that
	// this action has failed to reach its goal.
	progressState = Failed;

	eventHandler.UnitMoveFailed(owner);
	eoh->UnitMoveFailed(*owner);
}




void CGroundMoveType::HandleObjectCollisions()
{
	SCOPED_TIMER("Unit::MoveType::Update::Collisions");

	CUnit* collider = owner;

	// handle collisions for even-numbered objects on even-numbered frames and vv.
	// (temporal resolution is still high enough to not compromise accuracy much?)
	// if ((collider->id & 1) == (gs->frameNum & 1)) {
	{
		const UnitDef* colliderUD = collider->unitDef;
		const MoveDef* colliderMD = collider->moveDef;

		// NOTE:
		//   use the collider's MoveDef footprint as radius since it is
		//   always mobile (its UnitDef footprint size may be different)
		//
		//   0.75 * math::sqrt(2) ~= 1, so radius is always that of a circle
		//   _maximally bounded_ by the footprint rather than a circle
		//   _minimally bounding_ the footprint (assuming square shape)
		//
		const float colliderSpeed = collider->speed.w;
		const float colliderRadius = FOOTPRINT_RADIUS(colliderMD->xsize, colliderMD->zsize, 0.75f);

		HandleUnitCollisions(collider, colliderSpeed, colliderRadius, colliderUD, colliderMD);
		HandleFeatureCollisions(collider, colliderSpeed, colliderRadius, colliderUD, colliderMD);
		HandleStaticObjectCollision(collider, collider, colliderMD, colliderRadius, 0.0f, ZeroVector, true, false, true);
	}
}

void CGroundMoveType::HandleStaticObjectCollision(
	CUnit* collider,
	CSolidObject* collidee,
	const MoveDef* colliderMD,
	const float colliderRadius,
	const float collideeRadius,
	const float3& separationVector,
	bool canRequestPath,
	bool checkYardMap,
	bool checkTerrain
) {
	if (checkTerrain && (!collider->IsMoving() || collider->IsInAir()))
		return;

	// for factories, check if collidee's position is behind us (which means we are likely exiting)
	//
	// NOTE:
	//   allow units to move _through_ idle open factories by extending the collidee's footprint such
	//   that insideYardMap is true in a larger area (otherwise pathfinder and coldet would disagree)
	//   the transition from radius- to footprint-based handling is discontinuous --> cannot mix them
	// TODO:
	//   increase cost of squares inside open factories so PFS is less likely to path through them
	//
	#if 0
	const int xext = ((collidee->xsize >> 1) + std::max(1, colliderMD->xsizeh));
	const int zext = ((collidee->zsize >> 1) + std::max(1, colliderMD->zsizeh));

	const bool insideYardMap =
		(collider->pos.x >= (collidee->pos.x - xext * SQUARE_SIZE)) &&
		(collider->pos.x <= (collidee->pos.x + xext * SQUARE_SIZE)) &&
		(collider->pos.z >= (collidee->pos.z - zext * SQUARE_SIZE)) &&
		(collider->pos.z <= (collidee->pos.z + zext * SQUARE_SIZE));
	const bool exitingYardMap =
		((collider->frontdir.dot(separationVector) > 0.0f) &&
		 (collider->   speed.dot(separationVector) > 0.0f));
	#endif

	bool wantRequestPath = false;

	if (checkYardMap || checkTerrain) {
		const int xmid = (collider->pos.x + collider->speed.x) / SQUARE_SIZE;
		const int zmid = (collider->pos.z + collider->speed.z) / SQUARE_SIZE;

		// mantis{3614,4217}
		//   we cannot nicely bounce off terrain when checking only the center square
		//   however, testing more squares means CD can (sometimes) disagree with PFS
		//   in narrow passages --> still possible, but have to ensure we allow only
		//   lateral (non-obstructing) bounces
		const int xsh = colliderMD->xsizeh * (checkYardMap || (checkTerrain * colliderMD->allowTerrainCollisions));
		const int zsh = colliderMD->zsizeh * (checkYardMap || (checkTerrain * colliderMD->allowTerrainCollisions));

		const int xmin = std::min(-1, -xsh), xmax = std::max(1, xsh);
		const int zmin = std::min(-1, -zsh), zmax = std::max(1, zsh);

		float3 strafeVec;
		float3 bounceVec;

		float3 sqrSumPosition; // .y is always 0
		float2 sqrPenDistance; // .x = sum, .y = count

		if (DEBUG_DRAWING_ENABLED) {
			geometricObjects->AddLine(collider->pos + (UpVector * 25.0f), collider->pos + (UpVector * 100.0f), 3, 1, 4);
		}

		// check for blocked squares inside collider's MoveDef footprint zone
		// interpret each square as a "collidee" and sum up separation vectors
		//
		// NOTE:
		//   assumes the collider's footprint is still always axis-aligned
		// NOTE:
		//   the pathfinders only care about the CENTER square for terrain!
		//   this means paths can come closer to impassable terrain than is
		//   allowed by collision detection (more probable if edges between
		//   passable and impassable areas are hard instead of gradients or
		//   if a unit is not affected by slopes) --> can be solved through
		//   smoothing the cost-function, eg. blurring heightmap before PFS
		//   sees it
		for (int z = zmin; z <= zmax; z++) {
			for (int x = xmin; x <= xmax; x++) {
				const int xabs = xmid + x;
				const int zabs = zmid + z;

				if (checkTerrain) {
					if (CMoveMath::GetPosSpeedMod(*colliderMD, xabs, zabs) > 0.01f)
						continue;
				} else {
					if ((CMoveMath::SquareIsBlocked(*colliderMD, xabs, zabs, collider) & CMoveMath::BLOCK_STRUCTURE) == 0)
						continue;
				}

				const float3 squarePos = float3(xabs * SQUARE_SIZE + (SQUARE_SIZE >> 1), collider->pos.y, zabs * SQUARE_SIZE + (SQUARE_SIZE >> 1));
				const float3 squareVec = collider->pos - squarePos;

				// ignore squares behind us (relative to velocity vector)
				if (squareVec.dot(collider->speed) > 0.0f)
					continue;

				// RHS magic constant is the radius of a square (sqrt(2*(SQUARE_SIZE>>1)*(SQUARE_SIZE>>1)))
				const float  squareColRadiusSum = colliderRadius + 5.656854249492381f;
				const float   squareSepDistance = squareVec.Length2D() + 0.1f;
				const float   squarePenDistance = std::min(squareSepDistance - squareColRadiusSum, 0.0f);
				// const float  squareColSlideSign = -Sign(squarePos.dot(collider->rightdir) - (collider->pos).dot(collider->rightdir));

				// this tends to cancel out too much on average
				// strafeVec += (collider->rightdir * sqColSlideSign);
				bounceVec += (collider->rightdir * (collider->rightdir.dot(squareVec / squareSepDistance)));

				sqrPenDistance += float2(squarePenDistance, 1.0f);
				sqrSumPosition += (squarePos * XZVector);
			}
		}

		if (sqrPenDistance.y > 0.0f) {
			sqrSumPosition.x /= sqrPenDistance.y;
			sqrSumPosition.z /= sqrPenDistance.y;
			sqrPenDistance.x /= sqrPenDistance.y;

			const float strafeSign = -Sign(sqrSumPosition.dot(collider->rightdir) - (collider->pos).dot(collider->rightdir));
			const float strafeScale = std::min(std::max(currentSpeed*0.0f, maxSpeedDef), std::max(0.1f, -sqrPenDistance.x * 0.5f));
			const float bounceScale = std::min(std::max(currentSpeed*0.0f, maxSpeedDef), std::max(0.1f, -sqrPenDistance.x * 0.5f));

			// in FPS mode, normalize {strafe,bounce}Scale and multiply by maxSpeedDef
			// (otherwise it would be possible to slide along map edges at above-normal
			// speeds, etc.)
			const float fpsStrafeScale = (strafeScale / (strafeScale + bounceScale)) * maxSpeedDef;
			const float fpsBounceScale = (bounceScale / (strafeScale + bounceScale)) * maxSpeedDef;

			strafeVec = collider->rightdir * strafeSign;
			strafeVec = strafeVec.SafeNormalize2D() * mix(strafeScale, fpsStrafeScale, owner->UnderFirstPersonControl());
			bounceVec = bounceVec.SafeNormalize2D() * mix(bounceScale, fpsBounceScale, owner->UnderFirstPersonControl());

			// if checkTerrain is true, test only the center square
			if (colliderMD->TestMoveSquare(collider, collider->pos + strafeVec + bounceVec, ZeroVector, checkTerrain, checkYardMap, checkTerrain)) {
				collider->Move(strafeVec + bounceVec, true);
			} else {
				collider->Move(oldPos - collider->pos, wantRequestPath = true);
			}
		}

		// note:
		//   in many cases this does not mean we should request a new path
		//   (and it can be counter-productive to do so since we might not
		//   even *get* one)
		wantRequestPath = ((strafeVec + bounceVec) != ZeroVector);
	} else {
		const float  colRadiusSum = colliderRadius + collideeRadius;
		const float   sepDistance = separationVector.Length() + 0.1f;
		const float   penDistance = std::min(sepDistance - colRadiusSum, 0.0f);
		const float  colSlideSign = -Sign(collidee->pos.dot(collider->rightdir) - collider->pos.dot(collider->rightdir));

		const float strafeScale = std::min(currentSpeed, std::max(0.0f, -penDistance * 0.5f)) * (1 - checkYardMap * false);
		const float bounceScale = std::min(currentSpeed, std::max(0.0f, -penDistance       )) * (1 - checkYardMap *  true);

		const float3 strafeVec = (collider->rightdir * colSlideSign) * strafeScale;
		const float3 bounceVec = (   separationVector / sepDistance) * bounceScale;

		if (colliderMD->TestMoveSquare(collider, collider->pos + strafeVec + bounceVec, ZeroVector, true, true, true)) {
			collider->Move(strafeVec + bounceVec, true);
		} else {
			// move back to previous-frame position
			// ChangeSpeed calculates speedMod without checking squares for *structure* blockage
			// (so that a unit can free itself if it ends up within the footprint of a structure)
			// this means deltaSpeed will be non-zero if stuck on an impassable square and hence
			// the new speedvector which is constructed from deltaSpeed --> we would simply keep
			// moving forward through obstacles if not counteracted by this
			if (collider->frontdir.dot(separationVector) < 0.25f) {
				collider->Move(oldPos - collider->pos, wantRequestPath = true);
			}
		}

		// same here
		wantRequestPath = (penDistance < 0.0f);
	}

	if (canRequestPath && wantRequestPath) {
		ReRequestPath(false, false);
	}
}



void CGroundMoveType::HandleUnitCollisions(
	CUnit* collider,
	const float colliderSpeed,
	const float colliderRadius,
	const UnitDef* colliderUD,
	const MoveDef* colliderMD
) {
	const float searchRadius = std::max(colliderSpeed, 1.0f) * (colliderRadius * 1.0f);

	const std::vector<CUnit*>& nearUnits = quadField->GetUnitsExact(collider->pos, searchRadius);
	      std::vector<CUnit*>::const_iterator uit;

	// NOTE: probably too large for most units (eg. causes tree falling animations to be skipped)
	const int dirSign = Sign(int(!reversing));
	const float3 crushImpulse = collider->speed * collider->mass * dirSign;

	for (uit = nearUnits.begin(); uit != nearUnits.end(); ++uit) {
		CUnit* collidee = const_cast<CUnit*>(*uit);

		const UnitDef* collideeUD = collidee->unitDef;
		const MoveDef* collideeMD = collidee->moveDef;

		const bool colliderMobile = (colliderMD != NULL); // always true
		const bool collideeMobile = (collideeMD != NULL); // maybe true

		// use the collidee's MoveDef footprint as radius if it is mobile
		// use the collidee's Unit (not UnitDef) footprint as radius otherwise
		const float collideeSpeed = collidee->speed.w;
		const float collideeRadius = collideeMobile?
			FOOTPRINT_RADIUS(collideeMD->xsize, collideeMD->zsize, 0.75f):
			FOOTPRINT_RADIUS(collidee  ->xsize, collidee  ->zsize, 0.75f);

		const float3 separationVector   = collider->pos - collidee->pos;
		const float separationMinDistSq = (colliderRadius + collideeRadius) * (colliderRadius + collideeRadius);

		if ((separationVector.SqLength() - separationMinDistSq) > 0.01f)
			continue;

		if (collidee == collider) continue;
		if (collidee->IsSkidding()) continue;
		if (collidee->IsFlying()) continue;

		// disable collisions between collider and collidee
		// if collidee is currently inside any transporter,
		// or if collider is being transported by collidee
		if (collider->GetTransporter() == collidee) continue;
		if (collidee->GetTransporter() != NULL) continue;
		// also disable collisions if either party currently
		// has an order to load units (TODO: do we want this
		// for unloading as well?)
		if (collider->loadingTransportId == collidee->id) continue;
		if (collidee->loadingTransportId == collider->id) continue;

		// NOTE:
		//    we exclude aircraft (which have NULL moveDef's) landed
		//    on the ground, since they would just stack when pushed
		bool pushCollider = colliderMobile;
		bool pushCollidee = collideeMobile;
		bool crushCollidee = false;

		const bool alliedCollision =
			teamHandler->Ally(collider->allyteam, collidee->allyteam) &&
			teamHandler->Ally(collidee->allyteam, collider->allyteam);
		const bool collideeYields = (collider->IsMoving() && !collidee->IsMoving());
		const bool ignoreCollidee = (collideeYields && alliedCollision);

		// FIXME:
		//   allowPushingEnemyUnits is (now) useless because alliances are bi-directional
		//   ie. if !alliedCollision, pushCollider and pushCollidee BOTH become false and
		//   the collision is treated normally --> not what we want here, but the desired
		//   behavior (making each party stop and block the other) has many corner-cases
		//   this also happens when both parties are pushResistant --> make each respond
		//   to the other as a static obstacle so the tags still have some effect
		pushCollider &= (alliedCollision || modInfo.allowPushingEnemyUnits || !collider->blockEnemyPushing);
		pushCollidee &= (alliedCollision || modInfo.allowPushingEnemyUnits || !collidee->blockEnemyPushing);
		pushCollider &= (!collider->beingBuilt && !collider->UsingScriptMoveType() && !colliderUD->pushResistant);
		pushCollidee &= (!collidee->beingBuilt && !collidee->UsingScriptMoveType() && !collideeUD->pushResistant);

		crushCollidee |= (!alliedCollision || modInfo.allowCrushingAlliedUnits);
		crushCollidee &= ((colliderSpeed * collider->mass) > (collideeSpeed * collidee->mass));

		// don't push/crush either party if the collidee does not block the collider (or vv.)
		if (colliderMobile && CMoveMath::IsNonBlocking(*colliderMD, collidee, collider))
			continue;
		if (collideeMobile && CMoveMath::IsNonBlocking(*collideeMD, collider, collidee))
			continue;

		if (crushCollidee && !CMoveMath::CrushResistant(*colliderMD, collidee))
			collidee->Kill(collider, crushImpulse, true);

		if (pathController->IgnoreCollision(collider, collidee))
			continue;

		eventHandler.UnitUnitCollision(collider, collidee);

		// if collidee shares our goal position and is no longer
		// moving along its path, trigger Arrived() to kill long
		// pushing contests
		//
		// check the progress-states so collisions with units which
		// failed to reach goalPos for whatever reason do not count
		// (or those that still have orders)
		//
		// CFactory applies random jitter to otherwise equal goal
		// positions of at most TWOPI elmos, use half as threshold
		if ((collider->moveType->goalPos - collidee->moveType->goalPos).SqLength2D() < (PI * PI)) {
			if (collider->IsMoving() && collider->moveType->progressState == AMoveType::Active) {
				if (!collidee->IsMoving() && collidee->moveType->progressState == AMoveType::Done) {
					if (UNIT_CMD_QUE_SIZE(collidee) == 0) {
						atEndOfPath = true; atGoal = true;
					}
				}
			}
		}

		if ((!collideeMobile && !collideeUD->IsAirUnit()) || (!pushCollider && !pushCollidee)) {
			// building (always axis-aligned, possibly has a yardmap)
			// or semi-static collidee that should be handled as such
			// this also handles two mutually push-resistant parties!
			HandleStaticObjectCollision(
				collider,
				collidee,
				colliderMD,
				colliderRadius,
				collideeRadius,
				separationVector,
				(!atEndOfPath && !atGoal),
				collideeUD->IsFactoryUnit(),
				false);

			continue;
		}

		const float colliderRelRadius = colliderRadius / (colliderRadius + collideeRadius);
		const float collideeRelRadius = collideeRadius / (colliderRadius + collideeRadius);
		const float collisionRadiusSum = modInfo.allowUnitCollisionOverlap?
			(colliderRadius * colliderRelRadius + collideeRadius * collideeRelRadius):
			(colliderRadius                     + collideeRadius                    );

		const float  sepDistance = separationVector.Length() + 0.1f;
		const float  penDistance = std::max(collisionRadiusSum - sepDistance, 1.0f);
		const float  sepResponse = std::min(SQUARE_SIZE * 2.0f, penDistance * 0.5f);

		const float3 sepDirection   = (separationVector / sepDistance);
		const float3 colResponseVec = sepDirection * XZVector * sepResponse;

		const float
			m1 = collider->mass,
			m2 = collidee->mass,
			v1 = std::max(1.0f, colliderSpeed),
			v2 = std::max(1.0f, collideeSpeed),
			c1 = 1.0f + (1.0f - math::fabs(collider->frontdir.dot(-sepDirection))) * 5.0f,
			c2 = 1.0f + (1.0f - math::fabs(collidee->frontdir.dot( sepDirection))) * 5.0f,
			s1 = m1 * v1 * c1,
			s2 = m2 * v2 * c2,
 			r1 = s1 / (s1 + s2 + 1.0f),
 			r2 = s2 / (s1 + s2 + 1.0f);

		// far from a realistic treatment, but works
		const float colliderMassScale = Clamp(1.0f - r1, 0.01f, 0.99f) * (modInfo.allowUnitCollisionOverlap? (1.0f / colliderRelRadius): 1.0f);
		const float collideeMassScale = Clamp(1.0f - r2, 0.01f, 0.99f) * (modInfo.allowUnitCollisionOverlap? (1.0f / collideeRelRadius): 1.0f);

		// try to prevent both parties from being pushed onto non-traversable
		// squares (without resetting their position which stops them dead in
		// their tracks and undoes previous legitimate pushes made this frame)
		//
		// if pushCollider and pushCollidee are both false (eg. if each party
		// is pushResistant), treat the collision as regular and push both to
		// avoid deadlocks
		const float colliderSlideSign = Sign( separationVector.dot(collider->rightdir));
		const float collideeSlideSign = Sign(-separationVector.dot(collidee->rightdir));

		const float3 colliderPushVec  =  colResponseVec * colliderMassScale * int(!ignoreCollidee);
		const float3 collideePushVec  = -colResponseVec * collideeMassScale;
		const float3 colliderSlideVec = collider->rightdir * colliderSlideSign * (1.0f / penDistance) * r2;
		const float3 collideeSlideVec = collidee->rightdir * collideeSlideSign * (1.0f / penDistance) * r1;

		if ((pushCollider || !pushCollidee) && colliderMobile) {
			if (colliderMD->TestMoveSquare(collider, collider->pos + colliderPushVec + colliderSlideVec)) {
				collider->Move(colliderPushVec + colliderSlideVec, true);
			}
		}

		if ((pushCollidee || !pushCollider) && collideeMobile) {
			if (collideeMD->TestMoveSquare(collidee, collidee->pos + collideePushVec + collideeSlideVec)) {
				collidee->Move(collideePushVec + collideeSlideVec, true);
			}
		}
	}
}

void CGroundMoveType::HandleFeatureCollisions(
	CUnit* collider,
	const float colliderSpeed,
	const float colliderRadius,
	const UnitDef* colliderUD,
	const MoveDef* colliderMD
) {
	const float searchRadius = std::max(colliderSpeed, 1.0f) * (colliderRadius * 1.0f);

	const std::vector<CFeature*>& nearFeatures = quadField->GetFeaturesExact(collider->pos, searchRadius);
	      std::vector<CFeature*>::const_iterator fit;

	const int dirSign = Sign(int(!reversing));
	const float3 crushImpulse = collider->speed * collider->mass * dirSign;

	for (fit = nearFeatures.begin(); fit != nearFeatures.end(); ++fit) {
		CFeature* collidee = const_cast<CFeature*>(*fit);
		// const FeatureDef* collideeFD = collidee->def;

		// use the collidee's Feature (not FeatureDef) footprint as radius
		// const float collideeRadius = FOOTPRINT_RADIUS(collideeFD->xsize, collideeFD->zsize, 0.75f);
		const float collideeRadius = FOOTPRINT_RADIUS(collidee->xsize, collidee->zsize, 0.75f);
		const float collisionRadiusSum = colliderRadius + collideeRadius;

		const float3 separationVector   = collider->pos - collidee->pos;
		const float separationMinDistSq = collisionRadiusSum * collisionRadiusSum;

		if ((separationVector.SqLength() - separationMinDistSq) > 0.01f)
			continue;

		if (CMoveMath::IsNonBlocking(*colliderMD, collidee, collider))
			continue;
		if (!CMoveMath::CrushResistant(*colliderMD, collidee))
			collidee->Kill(collider, crushImpulse, true);

		if (pathController->IgnoreCollision(collider, collidee))
			continue;

		eventHandler.UnitFeatureCollision(collider, collidee);

		if (!collidee->IsMoving()) {
			HandleStaticObjectCollision(
				collider,
				collidee,
				colliderMD,
				colliderRadius,
				collideeRadius,
				separationVector,
				(!atEndOfPath && !atGoal),
				false,
				false);

			continue;
		}

		const float  sepDistance    = separationVector.Length() + 0.1f;
		const float  penDistance    = std::max(collisionRadiusSum - sepDistance, 1.0f);
		const float  sepResponse    = std::min(SQUARE_SIZE * 2.0f, penDistance * 0.5f);

		const float3 sepDirection   = (separationVector / sepDistance);
		const float3 colResponseVec = sepDirection * XZVector * sepResponse;

		// multiply the collider's mass by a large constant (so that heavy
		// features do not bounce light units away like jittering pinballs;
		// collideeMassScale ~= 0.01 suppresses large responses)
		const float
			m1 = collider->mass,
			m2 = collidee->mass * 10000.0f,
			v1 = std::max(1.0f, colliderSpeed),
			v2 = 1.0f,
			c1 = (1.0f - math::fabs( collider->frontdir.dot(-sepDirection))) * 5.0f,
			c2 = (1.0f - math::fabs(-collider->frontdir.dot( sepDirection))) * 5.0f,
			s1 = m1 * v1 * c1,
			s2 = m2 * v2 * c2,
 			r1 = s1 / (s1 + s2 + 1.0f),
 			r2 = s2 / (s1 + s2 + 1.0f);

		const float colliderMassScale = Clamp(1.0f - r1, 0.01f, 0.99f);
		const float collideeMassScale = Clamp(1.0f - r2, 0.01f, 0.99f);

		quadField->RemoveFeature(collidee);
		collider->Move( colResponseVec * colliderMassScale, true);
		collidee->Move(-colResponseVec * collideeMassScale, true);
		quadField->AddFeature(collidee);
	}
}



void CGroundMoveType::LeaveTransport()
{
	oldPos = owner->pos + UpVector * 0.001f;
}



void CGroundMoveType::KeepPointingTo(float3 pos, float distance, bool aggressive) {
	mainHeadingPos = pos;
	useMainHeading = aggressive;

	if (!useMainHeading)
		return;
	if (owner->weapons.empty())
		return;

	const CWeapon* frontWeapon = owner->weapons.front();

	if (!frontWeapon->weaponDef->waterweapon) {
		mainHeadingPos.y = std::max(mainHeadingPos.y, 0.0f);
	}

	float3 dir1 = frontWeapon->mainDir;
	float3 dir2 = mainHeadingPos - owner->pos;

	// in this case aligning is impossible
	if (dir1 == UpVector)
		return;

	dir1 = (dir1 * XZVector).SafeNormalize();
	dir2 = (dir2 * XZVector).SafeNormalize();

	if (dir2 == ZeroVector)
		return;

	const short heading =
		GetHeadingFromVector(dir2.x, dir2.z) -
		GetHeadingFromVector(dir1.x, dir1.z);

	if (owner->heading == heading)
		return;

	// NOTE:
	//   by changing the progress-state here (which seems redundant),
	//   SlowUpdate can suddenly request a new path for us even after
	//   StopMoving (which clears pathID; CAI often calls StopMoving
	//   before unit is at goalPos!)
	//   for this reason StopMoving always updates goalPos so internal
	//   GetNewPath's are no-ops (while CAI does not call StartMoving)
	if (!frontWeapon->TryTarget(mainHeadingPos, true, NULL)) {
		progressState = Active;
	}
}

void CGroundMoveType::KeepPointingTo(CUnit* unit, float distance, bool aggressive) {
	//! wrapper
	KeepPointingTo(unit->pos, distance, aggressive);
}

/**
* @brief Orients owner so that weapon[0]'s arc includes mainHeadingPos
*/
void CGroundMoveType::SetMainHeading() {
	if (!useMainHeading)
		return;
	if (owner->weapons.empty())
		return;

	const CWeapon* frontWeapon = owner->weapons.front();

	const float3 dir1 = ((       frontWeapon->mainDir) * XZVector).SafeNormalize();
	const float3 dir2 = ((mainHeadingPos - owner->pos) * XZVector).SafeNormalize();

	// ASSERT_SYNCED(dir1);
	// ASSERT_SYNCED(dir2);

	if (dir2 == ZeroVector)
		return;

	short newHeading =
		GetHeadingFromVector(dir2.x, dir2.z) -
		GetHeadingFromVector(dir1.x, dir1.z);

	ASSERT_SYNCED(newHeading);

	if (progressState == Active) {
		if (owner->heading != newHeading) {
			// start or continue turning
			ChangeHeading(newHeading);
		} else {
			// stop turning
			progressState = Done;
		}
	} else {
		if (owner->heading != newHeading) {
			if (!frontWeapon->TryTarget(mainHeadingPos, true, NULL)) {
				progressState = Active;
			}
		}
	}
}

bool CGroundMoveType::OnSlope(float minSlideTolerance) {
	const UnitDef* ud = owner->unitDef;
	const MoveDef* md = owner->moveDef;
	const float3& pos = owner->pos;

	if (ud->slideTolerance < minSlideTolerance) { return false; }
	if (ud->floatOnWater && owner->IsInWater()) { return false; }
	if (!pos.IsInBounds()) { return false; }

	// if minSlideTolerance is LEQ 0, do not multiply maxSlope by ud->slideTolerance
	// (otherwise the unit could stop on an invalid path location, and be teleported
	// back)
	const float slopeMul = mix(ud->slideTolerance, 1.0f, (minSlideTolerance <= 0.0f));
	const float curSlope = CGround::GetSlope(pos.x, pos.z);
	const float maxSlope = md->maxSlope * slopeMul;

	return (curSlope > maxSlope);
}



const float3& CGroundMoveType::GetGroundNormal(const float3& p) const
{
	if (owner->IsInWater() && !owner->IsOnGround()) {
		// ship or hovercraft; return (CGround::GetNormalAboveWater(p));
		return UpVector;
	}

	return (CGround::GetNormal(p.x, p.z));
}

float CGroundMoveType::GetGroundHeight(const float3& p) const
{
	// in [minHeight, maxHeight]
	const float gh = CGround::GetHeightReal(p.x, p.z);
	const float wh = -owner->unitDef->waterline * (gh <= 0.0f);

	if (owner->unitDef->floatOnWater) {
		// in [-waterline, maxHeight], note that waterline
		// can be much deeper than ground in shallow water
		return (std::max(gh, wh));
	}

	return gh;
}

void CGroundMoveType::AdjustPosToWaterLine()
{
	if (owner->IsFalling())
		return;
	if (owner->IsFlying())
		return;

	if (modInfo.allowGroundUnitGravity) {
		if (owner->unitDef->floatOnWater) {
			owner->Move(UpVector * (std::max(CGround::GetHeightReal(owner->pos.x, owner->pos.z), -owner->unitDef->waterline) - owner->pos.y), true);
		} else {
			owner->Move(UpVector * (std::max(CGround::GetHeightReal(owner->pos.x, owner->pos.z),               owner->pos.y) - owner->pos.y), true);
		}
	} else {
		owner->Move(UpVector * (GetGroundHeight(owner->pos) - owner->pos.y), true);
	}
}

bool CGroundMoveType::UpdateDirectControl()
{
	const CPlayer* myPlayer = gu->GetMyPlayer();
	const FPSUnitController& selfCon = myPlayer->fpsController;
	const FPSUnitController& unitCon = owner->fpsControlPlayer->fpsController;
	const bool wantReverse = (unitCon.back && !unitCon.forward);

	float turnSign = 0.0f;

	currWayPoint = owner->frontdir * XZVector * mix(100.0f, -100.0f, wantReverse);
	currWayPoint = (owner->pos + currWayPoint).cClampInBounds();

	if (unitCon.forward || unitCon.back) {
		ChangeSpeed((maxSpeed * unitCon.forward) + (maxReverseSpeed * unitCon.back), wantReverse, true);
	} else {
		// not moving forward or backward, stop
		ChangeSpeed(0.0f, false, true);
	}

	if (unitCon.left ) { ChangeHeading(owner->heading + turnRate); turnSign =  1.0f; }
	if (unitCon.right) { ChangeHeading(owner->heading - turnRate); turnSign = -1.0f; }

	if (selfCon.GetControllee() == owner) {
		// local client is controlling us
		camera->rot.y += (turnRate * turnSign * TAANG2RAD);
	}

	return wantReverse;
}

float3 CGroundMoveType::GetNewSpeedVector(const float hAcc, const float vAcc) const
{
	float3 speedVector;

	if (modInfo.allowGroundUnitGravity) {
		// NOTE:
		//   the drag terms ensure speed-vector always decays if
		//   wantedSpeed and deltaSpeed are 0 (needed because we
		//   do not call GetDragAccelerationVect while a unit is
		//   moving under its own power)
		const float dragCoeff = mix(0.99f, 0.9999f, owner->IsInAir());
		const float slipCoeff = mix(0.95f, 0.9999f, owner->IsInAir());

		// use terrain-tangent vector because it does not
		// depend on UnitDef::upright (unlike o->frontdir)
		const float3& gndNormVec = GetGroundNormal(owner->pos);
		const float3  gndTangVec = gndNormVec.cross(owner->rightdir);

		const float3 horSpeed = owner->speed * XZVector;
		const float3 verSpeed = UpVector * owner->speed.y;

		if (owner->moveDef->speedModClass != MoveDef::Hover || !modInfo.allowHoverUnitStrafing) {
			const float3 accelVec = (gndTangVec * hAcc) + (UpVector * vAcc);
			const float3 speedVec = (horSpeed + verSpeed) + accelVec;

			speedVector += (flatFrontDir * speedVec.dot(flatFrontDir)) * dragCoeff;
			speedVector += (    UpVector * speedVec.dot(    UpVector));
		} else {
			// TODO: also apply to non-hovercraft on low-gravity maps?
			speedVector += (              gndTangVec * (  std::max(0.0f,   owner->speed.dot(gndTangVec) + hAcc * 1.0f))) * dragCoeff;
			speedVector += (   horSpeed - gndTangVec * (/*std::max(0.0f,*/ owner->speed.dot(gndTangVec) - hAcc * 0.0f )) * slipCoeff;
			speedVector += (UpVector * (owner->speed + UpVector * vAcc).dot(UpVector));
		}

		// never drop below terrain while following tangent
		// (SPEED must be adjusted so that it does not keep
		// building up when the unit is on the ground or is
		// within one frame of hitting it)
		const float oldGroundHeight = GetGroundHeight(owner->pos              );
		const float newGroundHeight = GetGroundHeight(owner->pos + speedVector);

		if ((owner->pos.y + speedVector.y) <= newGroundHeight) {
			speedVector.y = std::min(newGroundHeight - owner->pos.y, math::fabs(newGroundHeight - oldGroundHeight));
		}
	} else {
		// LuaSyncedCtrl::SetUnitVelocity directly assigns
		// to owner->speed which gets overridden below, so
		// need to calculate hSpeedScale from it (not from
		// currentSpeed) directly
		const int    speedSign  = Sign(int(!reversing));
		const float  speedScale = owner->speed.w * speedSign + hAcc;

		speedVector = owner->frontdir * speedScale;
	}

	return speedVector;
}

void CGroundMoveType::UpdateOwnerPos(const float3& oldSpeedVector, const float3& newSpeedVector) {
	const float oldSpeed = math::fabs(oldSpeedVector.dot(flatFrontDir));
	const float newSpeed = math::fabs(newSpeedVector.dot(flatFrontDir));

	owner->UpdatePhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING, newSpeed > 0.01f);

	// if being built, the nanoframe might not be exactly on
	// the ground and would jitter from gravity acting on it
	// --> nanoframes can not move anyway, just return early
	// (units that become reverse-built will stop instantly)
	if (owner->beingBuilt)
		return;

	if (newSpeedVector != ZeroVector) {
		// use the simplest possible Euler integration
		owner->SetVelocityAndSpeed(newSpeedVector);
		owner->Move(owner->speed, true);

		// NOTE:
		//   does not check for structure blockage, coldet handles that
		//   entering of impassable terrain is *also* handled by coldet
		//
		//   the loop below tries to evade "corner" squares that would
		//   block us from initiating motion and is needed for when we
		//   are not *currently* moving but want to get underway to our
		//   first waypoint (HSOC coldet won't help then)
		//
 		//   allowing movement through blocked squares when pathID != 0
 		//   relies on assumption that PFS will not search if start-sqr
 		//   is blocked, so too fragile
		//
		if (!pathController->IgnoreTerrain(*owner->moveDef, owner->pos) && !owner->moveDef->TestMoveSquare(owner, owner->pos, ZeroVector, true, false, true)) {
			bool updatePos = false;

			for (unsigned int n = 1; n <= SQUARE_SIZE; n++) {
				if (!updatePos && (updatePos = owner->moveDef->TestMoveSquare(owner, owner->pos + owner->rightdir * n, ZeroVector, true, false, true))) {
					owner->Move(owner->pos + owner->rightdir * n, false); break;
				}
				if (!updatePos && (updatePos = owner->moveDef->TestMoveSquare(owner, owner->pos - owner->rightdir * n, ZeroVector, true, false, true))) {
					owner->Move(owner->pos - owner->rightdir * n, false); break;
				}
			}

			if (!updatePos) {
				owner->Move(owner->pos - newSpeedVector, false);
			}
		}

		// NOTE:
		//   this can fail when gravity is allowed (a unit catching air
		//   can easily end up on an impassable square, especially when
		//   terrain contains micro-bumps) --> more likely at lower g's
		// assert(owner->moveDef->TestMoveSquare(owner, owner->pos, ZeroVector, true, false, true));
	}

	reversing    = (newSpeedVector.dot(flatFrontDir) < 0.0f);
	currentSpeed = newSpeed;
	deltaSpeed   = 0.0f;

	if (oldSpeed <= 0.01f && newSpeed >  0.01f) { owner->script->StartMoving(reversing); }
	if (oldSpeed >  0.01f && newSpeed <= 0.01f) { owner->script->StopMoving(); }
}

bool CGroundMoveType::WantReverse(const float3& waypointDir2D) const
{
	if (!canReverse)
		return false;

	// these values are normally non-0, but LuaMoveCtrl
	// can override them and we do not want any div0's
	if (maxReverseSpeed <= 0.0f) return false;
	if (maxSpeed <= 0.0f) return true;

	if (accRate <= 0.0f) return false;
	if (decRate <= 0.0f) return false;
	if (turnRate <= 0.0f) return false;

	const float3 waypointDif  = (goalPos - owner->pos) * XZVector;                                // use final WP for ETA
	const float waypointDist  = waypointDif.Length();                                             // in elmos
	const float waypointFETA  = (waypointDist / maxSpeed);                                        // in frames (simplistic)
	const float waypointRETA  = (waypointDist / maxReverseSpeed);                                 // in frames (simplistic)
	const float waypointAngle = Clamp(waypointDir2D.dot(owner->frontdir), -1.0f, 1.0f);           // clamp to prevent NaN's
	const float turnAngleDeg  = math::acosf(waypointAngle) * (180.0f / PI);                       // in degrees
	const float fwdTurnAngle  = (turnAngleDeg / 360.0f) * SPRING_CIRCLE_DIVS;                     // in "headings"
	const float revTurnAngle  = SHORTINT_MAXVALUE - fwdTurnAngle;                                 // 180 deg - angle

	// units start accelerating before finishing the turn, so subtract something
	const float turnTimeMod      = 5.0f;
	const float fwdTurnAngleTime = std::max(0.0f, (fwdTurnAngle / turnRate) - turnTimeMod); // in frames
	const float revTurnAngleTime = std::max(0.0f, (revTurnAngle / turnRate) - turnTimeMod);

	const float apxFwdSpdAfterTurn = std::max(0.0f, currentSpeed - 0.125f * (fwdTurnAngleTime * decRate));
	const float apxRevSpdAfterTurn = std::max(0.0f, currentSpeed - 0.125f * (revTurnAngleTime * decRate));

	const float fwdDecTime = ( reversing * apxFwdSpdAfterTurn) / decRate;
	const float revDecTime = (!reversing * apxRevSpdAfterTurn) / decRate;
	const float fwdAccTime = (maxSpeed        - !reversing * apxFwdSpdAfterTurn) / accRate;
	const float revAccTime = (maxReverseSpeed -  reversing * apxRevSpdAfterTurn) / accRate;

	const float fwdETA = waypointFETA + fwdTurnAngleTime + fwdAccTime + fwdDecTime;
	const float revETA = waypointRETA + revTurnAngleTime + revDecTime + revAccTime;

	return (fwdETA > revETA);
}



bool CGroundMoveType::SetMemberValue(unsigned int memberHash, void* memberValue) {
	// try the generic members first
	if (AMoveType::SetMemberValue(memberHash, memberValue))
		return true;

	#define MEMBER_CHARPTR_HASH(memberName) HsiehHash(memberName, strlen(memberName),     0)
	#define MEMBER_LITERAL_HASH(memberName) HsiehHash(memberName, sizeof(memberName) - 1, 0)

	#define MAXREVERSESPEED_MEMBER_IDX 5

	static const unsigned int boolMemberHashes[] = {
		MEMBER_LITERAL_HASH(     "atGoal"),
		MEMBER_LITERAL_HASH("atEndOfPath"),
	};
	static const unsigned int floatMemberHashes[] = {
		MEMBER_LITERAL_HASH(       "turnRate"),
		MEMBER_LITERAL_HASH(      "turnAccel"),
		MEMBER_LITERAL_HASH(        "accRate"),
		MEMBER_LITERAL_HASH(        "decRate"),
		MEMBER_LITERAL_HASH(      "myGravity"),
		MEMBER_LITERAL_HASH("maxReverseSpeed"),
	};

	#undef MEMBER_CHARPTR_HASH
	#undef MEMBER_LITERAL_HASH


	// unordered_map etc. perform dynallocs, so KISS here
	bool* boolMemberPtrs[] = {
		&atGoal,
		&atEndOfPath,
	};
	float* floatMemberPtrs[] = {
		&turnRate,
		&turnAccel,

		&accRate,
		&decRate,

		&myGravity,
		&maxReverseSpeed,
	};

	// special cases
	if (memberHash == floatMemberHashes[MAXREVERSESPEED_MEMBER_IDX]) {
		*(floatMemberPtrs[MAXREVERSESPEED_MEMBER_IDX]) = *(reinterpret_cast<float*>(memberValue)) / GAME_SPEED;
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

