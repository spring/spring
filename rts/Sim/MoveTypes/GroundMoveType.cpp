/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/mmgr.h"

#include "GroundMoveType.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Player.h"
#include "Game/SelectedUnits.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "MoveMath/MoveMath.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/FastMath.h"
#include "System/myMath.h"
#include "System/Vec2.h"
#include "System/Sound/SoundChannels.h"
#include "System/Sync/SyncTracer.h"


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

#define RAD2DEG (180.0f / PI)
#define MIN_WAYPOINT_DISTANCE (SQUARE_SIZE)
#define MAX_IDLING_SLOWUPDATES 16
#define DEBUG_OUTPUT 0
#define PLAY_SOUNDS 1


CR_BIND_DERIVED(CGroundMoveType, AMoveType, (NULL));

CR_REG_METADATA(CGroundMoveType, (
	CR_MEMBER(turnRate),
	CR_MEMBER(accRate),
	CR_MEMBER(decRate),

	CR_MEMBER(maxReverseSpeed),
	CR_MEMBER(wantedSpeed),
	CR_MEMBER(currentSpeed),
	CR_MEMBER(requestedSpeed),
	CR_MEMBER(deltaSpeed),

	CR_MEMBER(pathId),
	CR_MEMBER(goalRadius),

	CR_MEMBER(currWayPoint),
	CR_MEMBER(nextWayPoint),
	CR_MEMBER(atGoal),
	CR_MEMBER(haveFinalWaypoint),
	CR_MEMBER(currWayPointDist),
	CR_MEMBER(prevWayPointDist),

	CR_MEMBER(pathRequestDelay),

	CR_MEMBER(numIdlingUpdates),
	CR_MEMBER(numIdlingSlowUpdates),
	CR_MEMBER(wantedHeading),

	CR_MEMBER(moveSquareX),
	CR_MEMBER(moveSquareY),

	CR_MEMBER(nextObstacleAvoidanceUpdate),

	CR_MEMBER(skidding),
	CR_MEMBER(flying),
	CR_MEMBER(reversing),
	CR_MEMBER(idling),
	CR_MEMBER(canReverse),
	CR_MEMBER(useMainHeading),

	CR_MEMBER(waypointDir),
	CR_MEMBER(flatFrontDir),
	CR_MEMBER(lastAvoidanceDir),
	CR_MEMBER(mainHeadingPos),

	CR_MEMBER(skidRotVector),
	CR_MEMBER(skidRotSpeed),
	CR_MEMBER(skidRotAccel),
	CR_ENUM_MEMBER(oldPhysState),

	CR_RESERVED(64),
	CR_POSTLOAD(PostLoad)
));



std::vector<int2> CGroundMoveType::lineTable[LINETABLE_SIZE][LINETABLE_SIZE];

CGroundMoveType::CGroundMoveType(CUnit* owner):
	AMoveType(owner),

	turnRate(0.1f),
	accRate(0.01f),
	decRate(0.01f),
	maxReverseSpeed(0.0f),
	wantedSpeed(0.0f),
	currentSpeed(0.0f),
	requestedSpeed(0.0f),
	deltaSpeed(0.0f),

	pathId(0),
	goalRadius(0),

	currWayPoint(ZeroVector),
	nextWayPoint(ZeroVector),
	atGoal(false),
	haveFinalWaypoint(false),
	currWayPointDist(0.0f),
	prevWayPointDist(0.0f),

	skidding(false),
	flying(false),
	reversing(false),
	idling(false),
	canReverse(owner->unitDef->rSpeed > 0.0f),
	useMainHeading(false),

	skidRotVector(UpVector),
	skidRotSpeed(0.0f),
	skidRotAccel(0.0f),

	oldPhysState(CSolidObject::OnGround),

	flatFrontDir(0.0f, 0.0, 1.0f),
	lastAvoidanceDir(ZeroVector),
	mainHeadingPos(ZeroVector),

	nextObstacleAvoidanceUpdate(0),
	pathRequestDelay(0),

	numIdlingUpdates(0),
	numIdlingSlowUpdates(0),

	wantedHeading(0)
{
	assert(owner != NULL);
	assert(owner->unitDef != NULL);

	moveSquareX = owner->pos.x / MIN_WAYPOINT_DISTANCE;
	moveSquareY = owner->pos.z / MIN_WAYPOINT_DISTANCE;

	maxReverseSpeed = owner->unitDef->rSpeed / GAME_SPEED;

	turnRate = owner->unitDef->turnRate;
	accRate = std::max(0.01f, owner->unitDef->maxAcc);
	decRate = std::max(0.01f, owner->unitDef->maxDec);
}

CGroundMoveType::~CGroundMoveType()
{
	if (pathId != 0) {
		pathManager->DeletePath(pathId);
	}
}

void CGroundMoveType::PostLoad()
{
	// HACK: re-initialize path after load
	if (pathId != 0) {
		pathId = pathManager->RequestPath(owner->mobility, owner->pos, goalPos, goalRadius, owner);
	}
}

bool CGroundMoveType::Update()
{
	ASSERT_SYNCED(owner->pos);

	if (owner->GetTransporter() != NULL) {
		return false;
	}

	if (OnSlope(1.0f)) {
		skidding = true;
	}
	if (skidding) {
		UpdateSkid();
		HandleObjectCollisions();
		return false;
	}

	ASSERT_SYNCED(owner->pos);

	// set drop height when we start to drop
	if (owner->falling) {
		UpdateControlledDrop();
		return false;
	}

	ASSERT_SYNCED(owner->pos);

	bool hasMoved = false;
	bool wantReverse = false;

	if (owner->stunned || owner->beingBuilt) {
		owner->script->StopMoving();
		SetDeltaSpeed(0.0f, false);
	} else {
		if (owner->fpsControlPlayer != NULL) {
			wantReverse = UpdateDirectControl();
		} else {
			wantReverse = FollowPath();
		}
	}

	// these must be executed even when stunned (so
	// units do not get buried by restoring terrain)
	UpdateOwnerPos(wantReverse);
	AdjustPosToWaterLine();
	HandleObjectCollisions();

	ASSERT_SANE_OWNER_SPEED(owner->speed);

	if (owner->pos == oldPos) {
		// note: the float3::== test is not exact, so even if this
		// evaluates to true the unit might still have an epsilon
		// speed vector --> nullify it to prevent apparent visual
		// micro-stuttering (speed is used to extrapolate drawPos)
		owner->speed = ZeroVector;

		idling = true;
		hasMoved = false;
	} else {
		TestNewTerrainSquare();

		// note: HandleObjectCollisions() may have negated the position set
		// by UpdateOwnerPos() (so that owner->pos is again equal to oldPos)
		oldPos = owner->pos;

		// too many false negatives: speed is unreliable if stuck behind an obstacle
		//   idling = (owner->speed.SqLength() < (accRate * accRate));
		// too many false positives: waypoint-distance delta and speed vary too much
		//   idling = (Square(currWayPointDist - prevWayPointDist) < owner->speed.SqLength());
		// too many false positives: many slow units cannot even manage 1 elmo/frame
		//   idling = (Square(currWayPointDist - prevWayPointDist) < 1.0f);

		idling = (Square(currWayPointDist - prevWayPointDist) <= (owner->speed.SqLength() * 0.5f));
		hasMoved = true;
	}

	return hasMoved;
}

void CGroundMoveType::SlowUpdate()
{
	if (owner->GetTransporter() != NULL) {
		if (progressState == Active)
			StopEngine();
		return;
	}

	if (progressState == Active) {
		if (pathId != 0) {
			if (idling) {
				numIdlingSlowUpdates = std::min(MAX_IDLING_SLOWUPDATES, int(numIdlingSlowUpdates + 1));
			} else {
				numIdlingSlowUpdates = std::max(0, int(numIdlingSlowUpdates - 1));
			}

			if (numIdlingUpdates > (SHORTINT_MAXVALUE / turnRate)) {
				// case A: we have a path but are not moving
				// FIXME: triggers during 180-degree turns
				LOG_L(L_DEBUG,
						"SlowUpdate: unit %i has pathID %i but %i ETA failures",
						owner->id, pathId, numIdlingUpdates);

				if (numIdlingSlowUpdates < MAX_IDLING_SLOWUPDATES) {
					StopEngine();
					StartEngine();
				} else {
					// unit probably ended up on a non-traversable
					// square, or got stuck in a non-moving crowd
					Fail();
				}
			}
		} else {
			if (gs->frameNum > pathRequestDelay) {
				// case B: we want to be moving but don't have a path
				LOG_L(L_DEBUG, "SlowUpdate: unit %i has no path", owner->id);

				StopEngine();
				StartEngine();
			}
		}
	}

	if (!flying) {
		// move us into the map, and update <oldPos>
		// to prevent any extreme changes in <speed>
		if (!owner->pos.IsInBounds()) {
			owner->pos.ClampInBounds();
			oldPos = owner->pos;
		}
	}

	AMoveType::SlowUpdate();
}


/*
Sets unit to start moving against given position with max speed.
*/
void CGroundMoveType::StartMoving(float3 pos, float goalRadius) {
	StartMoving(pos, goalRadius, (reversing? maxReverseSpeed: maxSpeed));
}


/*
Sets owner unit to start moving against given position with requested speed.
*/
void CGroundMoveType::StartMoving(float3 moveGoalPos, float _goalRadius, float speed)
{
#ifdef TRACE_SYNC
	tracefile << "[" << __FUNCTION__ << "] ";
	tracefile << owner->pos.x << " " << owner->pos.y << " " << owner->pos.z << " " << owner->id << "\n";
#endif

	if (progressState == Active) {
		StopEngine();
	}

	// set the new goal
	goalPos = moveGoalPos;
	goalRadius = _goalRadius;
	requestedSpeed = speed;
	atGoal = false;
	useMainHeading = false;
	progressState = Active;

	numIdlingUpdates = 0;
	numIdlingSlowUpdates = 0;

	currWayPointDist = 0.0f;
	prevWayPointDist = 0.0f;

	LOG_L(L_DEBUG, "StartMoving: starting engine for unit %i", owner->id);

	StartEngine();

	#if (PLAY_SOUNDS == 1)
	if (owner->team == gu->myTeam) {
		Channels::General.PlayRandomSample(owner->unitDef->sounds.activate, owner);
	}
	#endif
}

void CGroundMoveType::StopMoving() {
#ifdef TRACE_SYNC
	tracefile << "[" << __FUNCTION__ << "] ";
	tracefile << owner->pos.x << " " << owner->pos.y << " " << owner->pos.z << " " << owner->id << "\n";
#endif

	LOG_L(L_DEBUG, "StopMoving: stopping engine for unit %i", owner->id);

	StopEngine();

	useMainHeading = false;
	progressState = Done;
}



bool CGroundMoveType::FollowPath()
{
	bool wantReverse = false;

	if (pathId == 0) {
		SetDeltaSpeed(0.0f, false);
		SetMainHeading();
	} else {
		ASSERT_SYNCED(currWayPoint);
		ASSERT_SYNCED(nextWayPoint);
		ASSERT_SYNCED(owner->pos);

		prevWayPointDist = currWayPointDist;
		currWayPointDist = owner->pos.distance2D(currWayPoint);
		atGoal = ((owner->pos - goalPos).SqLength2D() < Square(MIN_WAYPOINT_DISTANCE));

		if (!atGoal) {
			if (!idling) {
				numIdlingUpdates = std::max(0, int(numIdlingUpdates - 1));
			} else {
				// note: the unit could just be turning in-place
				// over several frames (eg. to maneuver around an
				// obstacle), which unlike actual immobilization
				// should be ignored
				numIdlingUpdates = std::min(SHORTINT_MAXVALUE, int(numIdlingUpdates + 1));
			}
		}

		if (!haveFinalWaypoint) {
			GetNextWayPoint();
		} else {
			if (atGoal) {
				Arrived();
			}
		}

		// set direction to waypoint AFTER requesting it
		waypointDir = currWayPoint - owner->pos;
		waypointDir.y = 0.0f;
		waypointDir.SafeNormalize();

		ASSERT_SYNCED(waypointDir);

		const float3 wpDirInv = -waypointDir;
	//	const float3 wpPosTmp = owner->pos + wpDirInv;
		const bool   wpBehind = (waypointDir.dot(flatFrontDir) < 0.0f);

		if (wpBehind) {
			wantReverse = WantReverse(waypointDir);
		}

		// apply obstacle avoidance (steering)
		const float3 avoidVec = ObstacleAvoidance(waypointDir);

		ASSERT_SYNCED(avoidVec);

		if (avoidVec != ZeroVector) {
			if (wantReverse) {
				ChangeHeading(GetHeadingFromVector(wpDirInv.x, wpDirInv.z));
			} else {
				// should be waypointDir + avoidanceDir
				ChangeHeading(GetHeadingFromVector(avoidVec.x, avoidVec.z));
			}
		} else {
			SetMainHeading();
		}

		SetDeltaSpeed(requestedSpeed, wantReverse);
	}

	pathManager->UpdatePath(owner, pathId);
	return wantReverse;
}

void CGroundMoveType::SetDeltaSpeed(float newWantedSpeed, bool wantReverse, bool fpsMode)
{
	wantedSpeed = newWantedSpeed;

	// round low speeds to zero
	if (wantedSpeed == 0.0f && currentSpeed < 0.01f) {
		currentSpeed = 0.0f;
		deltaSpeed = 0.0f;
		return;
	}

	// wanted speed and acceleration
	float targetSpeed = wantReverse? maxReverseSpeed: maxSpeed;

	if (wantedSpeed > 0.0f) {
		const UnitDef* ud = owner->unitDef;
		const float groundMod = ud->movedata->moveMath->GetPosSpeedMod(*ud->movedata, owner->pos, flatFrontDir);

		const float3 goalDifFwd = currWayPoint - owner->pos;
		const float3 goalDifRev = -goalDifFwd;

		const float3 goalDif = reversing? goalDifRev: goalDifFwd;
		const short turnDeltaHeading = owner->heading - GetHeadingFromVector(goalDif.x, goalDif.z);

		// NOTE: <= 2 because every CMD_MOVE has a trailing CMD_SET_WANTED_MAX_SPEED
		const bool startBreaking =
			(owner->commandAI->commandQue.size() <= 2) &&
			((owner->pos - goalPos).SqLength() <= Square(BreakingDistance(currentSpeed)));

		if (!fpsMode && turnDeltaHeading != 0) {
			// only auto-adjust speed for turns when not in FPS mode
			const float reqTurnAngle = math::fabs(180.0f * (owner->heading - wantedHeading) / SHORTINT_MAXVALUE);
			const float maxTurnAngle = (turnRate / SPRING_CIRCLE_DIVS) * 360.0f;

			float reducedSpeed = (reversing)? maxReverseSpeed: maxSpeed;

			if (reqTurnAngle != 0.0f) {
				reducedSpeed *= (maxTurnAngle / reqTurnAngle);
			}

			if (haveFinalWaypoint && !atGoal) {
				// at this point, Update() will no longer call GetNextWayPoint()
				// and we must slow down to prevent entering an infinite circle
				targetSpeed = fastmath::apxsqrt(currWayPointDist * (reversing? accRate: decRate));
			}

			if (!ud->turnInPlace) {
				if (waypointDir.SqLength() > 0.1f) {
					if (reqTurnAngle > maxTurnAngle) {
						targetSpeed = std::max(ud->turnInPlaceSpeedLimit, reducedSpeed);
					}
				}
			} else {
				targetSpeed = reducedSpeed;
			}
		}

		targetSpeed *= groundMod;
		targetSpeed *= ((startBreaking)? 0.0f: 1.0f);
		targetSpeed = std::min(targetSpeed, wantedSpeed);
	} else {
		targetSpeed = 0.0f;
	}


	const int targetSpeedSign = int(!wantReverse) * 2 - 1;
	const int currentSpeedSign = int(!reversing) * 2 - 1;

	const float rawSpeedDiff = (targetSpeed * targetSpeedSign) - (currentSpeed * currentSpeedSign);
	const float absSpeedDiff = math::fabs(rawSpeedDiff);
	// need to clamp, game-supplied values can be much larger than |speedDiff|
	const float modAccRate = std::min(absSpeedDiff, accRate);
	const float modDecRate = std::min(absSpeedDiff, decRate);

	if (reversing) {
		// speed-sign in UpdateOwnerPos is negative
		//   --> to go faster in reverse gear, we need to add +decRate
		//   --> to go slower in reverse gear, we need to add -accRate
		deltaSpeed = (rawSpeedDiff < 0.0f)?  modDecRate: -modAccRate;
	} else {
		// speed-sign in UpdateOwnerPos is positive
		//   --> to go faster in forward gear, we need to add +accRate
		//   --> to go slower in forward gear, we need to add -decRate
		deltaSpeed = (rawSpeedDiff < 0.0f)? -modDecRate:  modAccRate;
	}
}

/*
 * Changes the heading of the owner.
 * FIXME near-duplicate of HoverAirMoveType::UpdateHeading
 */
void CGroundMoveType::ChangeHeading(short newHeading) {
	if (flying) return;
	if (owner->GetTransporter() != NULL) return;

	wantedHeading = newHeading;
	SyncedSshort& heading = owner->heading;
	const short deltaHeading = wantedHeading - heading;

	ASSERT_SYNCED(deltaHeading);
	ASSERT_SYNCED(turnRate);

	if (deltaHeading > 0) {
		heading += std::min(deltaHeading, short(turnRate));
	} else {
		heading += std::max(deltaHeading, short(-turnRate));
	}

	owner->UpdateDirVectors(!owner->upright && maxSpeed > 0.0f);
	owner->UpdateMidPos();

	flatFrontDir = owner->frontdir;
	flatFrontDir.y = 0.0f;
	flatFrontDir.Normalize();
}




void CGroundMoveType::ImpulseAdded(const float3&)
{
	// NOTE: ships must be able to receive impulse too (for collision handling)
	if (owner->beingBuilt)
		return;

	float3& impulse = owner->residualImpulse;
	float3& speed = owner->speed;

	if (skidding) {
		speed += impulse;
		impulse = ZeroVector;
	}

	const float3& groundNormal = ground->GetNormal(owner->pos.x, owner->pos.z);
	const float groundImpulseScale = impulse.dot(groundNormal);

	if (groundImpulseScale < 0.0f)
		impulse -= (groundNormal * groundImpulseScale);

	if (impulse.SqLength() <= 9.0f && groundImpulseScale <= 0.3f)
		return;

	skidding = true;
	useHeading = false;

	speed += impulse;
	impulse = ZeroVector;

	skidRotSpeed = 0.0f;
	skidRotAccel = 0.0f;

	float3 skidDir = owner->frontdir;

	if (speed.SqLength2D() >= 0.01f) {
		skidDir = speed;
		skidDir.y = 0.0f;
		skidDir.Normalize();
	}

	skidRotVector = skidDir.cross(UpVector);

	oldPhysState = owner->physicalState;
	owner->physicalState = CSolidObject::Flying;

	if (speed.dot(groundNormal) > 0.2f) {
		skidRotAccel = (gs->randFloat() - 0.5f) * 0.04f;
		flying = true;
	}

	ASSERT_SANE_OWNER_SPEED(speed);
}

void CGroundMoveType::UpdateSkid()
{
	ASSERT_SYNCED(owner->midPos);

	const float3& pos = owner->pos;
	      float3& speed  = owner->speed;

	const UnitDef* ud = owner->unitDef;
	const float groundHeight = GetGroundHeight(pos);

	if (flying) {
		// water drag
		if (pos.y < 0.0f)
			speed *= 0.95f;

		const float impactSpeed = pos.IsInBounds()?
			-speed.dot(ground->GetNormal(pos.x, pos.z)):
			-speed.dot(UpVector);
		const float impactDamageMult = impactSpeed * owner->mass * 0.02f;
		const bool doColliderDamage = (modInfo.allowUnitCollisionDamage && impactSpeed > ud->minCollisionSpeed && ud->minCollisionSpeed >= 0.0f);

		if (groundHeight > pos.y) {
			// ground impact, stop flying
			flying = false;

			owner->Move1D(groundHeight, 1, false);

			// deal ground impact damage
			// TODO:
			//     bouncing behaves too much like a rubber-ball,
			//     most impact energy needs to go into the ground
			if (doColliderDamage) {
				owner->DoDamage(DamageArray(impactDamageMult), NULL, ZeroVector);
			}

			skidRotSpeed = 0.0f;
			// skidRotAccel = 0.0f;
		} else {
			speed.y += mapInfo->map.gravity;
		}
	} else {
		float speedf = speed.Length();
		float skidRotSpd = 0.0f;

		const bool onSlope = OnSlope(-1.0f);
		const float speedReduction = 0.35f;

		if (speedf < speedReduction && !onSlope) {
			// stop skidding
			speed = ZeroVector;

			skidding = false;
			useHeading = true;

			owner->physicalState = oldPhysState;

			skidRotSpd = math::floor(skidRotSpeed + skidRotAccel + 0.5f);
			skidRotAccel = (skidRotSpd - skidRotSpeed) * 0.5f;
			skidRotAccel *= (PI / 180.0f);

			ChangeHeading(owner->heading);
		} else {
			if (onSlope) {
				const float3 normal = ground->GetNormal(pos.x, pos.z);
				const float3 normalForce = normal * normal.dot(UpVector * mapInfo->map.gravity);
				const float3 newForce = UpVector * mapInfo->map.gravity - normalForce;

				speed += newForce;
				speedf = speed.Length();
				speed *= (1.0f - (0.1f * normal.y));
			} else {
				speed *= (1.0f - std::min(1.0f, speedReduction / speedf)); // clamped 0..1
			}

			// number of frames until rotational speed would drop to 0
			const float remTime = std::max(1.0f, speedf / speedReduction);

			skidRotSpd = math::floor(skidRotSpeed + skidRotAccel * (remTime - 1.0f) + 0.5f);
			skidRotAccel = (skidRotSpd - skidRotSpeed) / remTime;
			skidRotAccel *= (PI / 180.0f);

			if (math::floor(skidRotSpeed) != math::floor(skidRotSpeed + skidRotAccel)) {
				skidRotSpeed = 0.0f;
				skidRotAccel = 0.0f;
			}
		}

		if ((groundHeight - pos.y) < (speed.y + mapInfo->map.gravity)) {
			speed.y += mapInfo->map.gravity;

			flying = true;
			skidding = true; // flying requires skidding
			useHeading = false; // and relies on CalcSkidRot
		} else if ((groundHeight - pos.y) > speed.y) {
			const float3& normal = (pos.IsInBounds())? ground->GetNormal(pos.x, pos.z): UpVector;
			const float dot = speed.dot(normal);

			if (dot > 0.0f) {
				speed *= 0.95f;
			} else {
				speed += (normal * (math::fabs(speed.dot(normal)) + 0.1f)) * 1.9f;
				speed *= 0.8f;
			}
		}
	}

	// translate before rotate
	owner->Move3D(speed, true);

	// NOTE: only needed to match terrain normal
	if ((pos.y - groundHeight) <= SQUARE_SIZE)
		owner->UpdateDirVectors(true);

	if (skidding) {
		CalcSkidRot();
		CheckCollisionSkid();
	}

	// always update <oldPos> here so that <speed> does not make
	// extreme jumps when the unit transitions from skidding back
	// to non-skidding
	oldPos = owner->pos;

	ASSERT_SANE_OWNER_SPEED(speed);
	ASSERT_SYNCED(owner->midPos);
}

void CGroundMoveType::UpdateControlledDrop()
{
	if (!owner->falling)
		return;

	const float3& pos = owner->pos;
	      float3& speed = owner->speed;

	speed.y += (mapInfo->map.gravity * owner->fallSpeed);
	speed.y = std::min(speed.y, 0.0f);

	owner->Move3D(speed, true);

	// water drag
	if (pos.y < 0.0f)
		speed *= 0.90;

	const float wh = GetGroundHeight(pos);

	if (wh > pos.y) {
		// ground impact
		owner->falling = false;
		owner->Move1D(wh, 1, false);
		owner->script->Landed(); //stop parachute animation
	}
}

void CGroundMoveType::CheckCollisionSkid()
{
	CUnit* collider = owner;

	// NOTE:
	//     the QuadField::Get* functions check o->midPos,
	//     but the quad(s) that objects are stored in are
	//     derived from o->pos (!)
	const float3& pos = collider->pos;
	const UnitDef* colliderUD = collider->unitDef;
	const vector<CUnit*>& nearUnits = qf->GetUnitsExact(pos, collider->radius);
	const vector<CFeature*>& nearFeatures = qf->GetFeaturesExact(pos, collider->radius);

	// magic number to reduce damage taken from collisions
	// between a very heavy and a very light CSolidObject
	static const float MASS_MULT = 0.02f;

	vector<CUnit*>::const_iterator ui;
	vector<CFeature*>::const_iterator fi;

	for (ui = nearUnits.begin(); ui != nearUnits.end(); ++ui) {
		CUnit* collidee = *ui;
		const UnitDef* collideeUD = collider->unitDef;

		const float sqDist = (pos - collidee->pos).SqLength();
		const float totRad = collider->radius + collidee->radius;

		if ((sqDist >= totRad * totRad) || (sqDist <= 0.01f)) {
			continue;
		}

		// stop units from reaching escape velocity
		const float3 dif = (pos - collidee->pos).SafeNormalize();

		if (collidee->mobility == NULL) {
			const float impactSpeed = -collider->speed.dot(dif);
			const float impactDamageMult = std::min(impactSpeed * collider->mass * MASS_MULT, MAX_UNIT_SPEED);

			const bool doColliderDamage = (modInfo.allowUnitCollisionDamage && impactSpeed > colliderUD->minCollisionSpeed && colliderUD->minCollisionSpeed >= 0.0f);
			const bool doCollideeDamage = (modInfo.allowUnitCollisionDamage && impactSpeed > collideeUD->minCollisionSpeed && collideeUD->minCollisionSpeed >= 0.0f);

			if (impactSpeed <= 0.0f)
				continue;

			collider->Move3D(dif * impactSpeed, true);
			collider->speed += ((dif * impactSpeed) * 1.8f);

			// damage the collider, no added impulse
			if (doColliderDamage) {
				collider->DoDamage(DamageArray(impactDamageMult), NULL, ZeroVector);
			}
			// damage the (static) collidee based on collider's mass, no added impulse
			if (doCollideeDamage) {
				collidee->DoDamage(DamageArray(impactDamageMult), NULL, ZeroVector);
			}
		} else {
			// don't conserve momentum
			assert(collider->mass > 0.0f && collidee->mass > 0.0f);

			const float impactSpeed = (collidee->speed - collider->speed).dot(dif) * 0.5f;
			const float colliderRelMass = (collider->mass / (collider->mass + collidee->mass));
			const float colliderRelImpactSpeed = impactSpeed * (1.0f - colliderRelMass);
			const float collideeRelImpactSpeed = impactSpeed * (       colliderRelMass); 

			const float colliderImpactDmgMult  = std::min(colliderRelImpactSpeed * collider->mass * MASS_MULT, MAX_UNIT_SPEED);
			const float collideeImpactDmgMult  = std::min(collideeRelImpactSpeed * collider->mass * MASS_MULT, MAX_UNIT_SPEED);
			const float3 colliderImpactImpulse = dif * colliderRelImpactSpeed;
			const float3 collideeImpactImpulse = dif * collideeRelImpactSpeed;

			const bool doColliderDamage = (modInfo.allowUnitCollisionDamage && impactSpeed > colliderUD->minCollisionSpeed && colliderUD->minCollisionSpeed >= 0.0f);
			const bool doCollideeDamage = (modInfo.allowUnitCollisionDamage && impactSpeed > collideeUD->minCollisionSpeed && collideeUD->minCollisionSpeed >= 0.0f);

			if (impactSpeed <= 0.0f)
				continue;

			collider->Move3D(colliderImpactImpulse, true);
			collidee->Move3D(-collideeImpactImpulse, true);

			// damage the collider
			if (doColliderDamage) {
				collider->DoDamage(DamageArray(colliderImpactDmgMult), NULL, dif * colliderImpactDmgMult);
			}
			// damage the collidee
			if (doCollideeDamage) {
				collidee->DoDamage(DamageArray(collideeImpactDmgMult), NULL, dif * -collideeImpactDmgMult);
			}

			collider->speed += colliderImpactImpulse;
			collider->speed *= 0.9f;
			collidee->speed -= collideeImpactImpulse;
			collidee->speed *= 0.9f;
		}
	}

	for (fi = nearFeatures.begin(); fi != nearFeatures.end(); ++fi) {
		CFeature* f = *fi;

		if (!f->blocking)
			continue;

		const float sqDist = (pos - f->pos).SqLength();
		const float totRad = collider->radius + f->radius;

		if ((sqDist >= totRad * totRad) || (sqDist <= 0.01f))
			continue;

		const float3 dif = (pos - f->pos).SafeNormalize();
		const float impactSpeed = -collider->speed.dot(dif);
		const float impactDamageMult = std::min(impactSpeed * collider->mass * MASS_MULT, MAX_UNIT_SPEED);
		const float3 impactImpulse = dif * impactSpeed;
		const bool doColliderDamage = (modInfo.allowUnitCollisionDamage && impactSpeed > colliderUD->minCollisionSpeed && colliderUD->minCollisionSpeed >= 0.0f);

		if (impactSpeed <= 0.0f)
			continue;

		collider->Move3D(impactImpulse, true);
		collider->speed += (impactImpulse * 1.8f);

		// damage the collider, no added impulse (!) 
		if (doColliderDamage) {
			collider->DoDamage(DamageArray(impactDamageMult), NULL, ZeroVector);
		}

		// damage the collidee feature based on collider's mass
		f->DoDamage(DamageArray(impactDamageMult), -impactImpulse);
	}

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

	owner->UpdateMidPos();
}




/*
 * Dynamic obstacle avoidance, helps the unit to
 * follow the path even when it's not perfect.
 */
float3 CGroundMoveType::ObstacleAvoidance(const float3& desiredDir) {
	#ifdef IGNORE_OBSTACLES
	return desiredDir;
	#endif

	// multiplier for how strongly an object should be avoided
	static const float AVOIDANCE_STRENGTH = 8000.0f;

	// Obstacle-avoidance-system only needs to be run if the unit wants to move
	if (pathId == 0)
		return ZeroVector;

	float3 avoidanceVec = ZeroVector;
	float3 avoidanceDir = desiredDir;

	// Speed-optimizer. Reduces the times this system is run.
	if (gs->frameNum < nextObstacleAvoidanceUpdate)
		return lastAvoidanceDir;

	lastAvoidanceDir = desiredDir;
	nextObstacleAvoidanceUpdate = gs->frameNum + 4;

	// now we do the obstacle avoidance proper
	const float avoidanceRadius = std::max(currentSpeed, 1.0f) * (owner->radius * 2.0f);
	const float3 rightOfPath = desiredDir.cross(UpVector);

	float avoidLeft = 0.0f;
	float avoidRight = 0.0f;

	MoveData* moveData = owner->mobility;
	CMoveMath* moveMath = moveData->moveMath;
	moveData->tempOwner = owner;

	const vector<CSolidObject*>& nearbyObjects = qf->GetSolidsExact(owner->pos, avoidanceRadius);

	for (vector<CSolidObject*>::const_iterator oi = nearbyObjects.begin(); oi != nearbyObjects.end(); ++oi) {
		CSolidObject* object = *oi;

		// cases in which there is no need to avoid this obstacle
		if (object == owner)
			continue;
		if (moveMath->IsNonBlocking(*moveData, object))
			continue;
		if (!moveMath->CrushResistant(*moveData, object))
			continue;
		// ignore objects that are slightly behind us
		if ((desiredDir.dot(object->pos - owner->pos) + 0.25f) <= 0.0f)
			continue;

		const float3 objectToUnit = (owner->pos - object->pos - object->speed * GAME_SPEED);
		const float objectDistSq = objectToUnit.SqLength();

		// NOTE: use the footprint radii instead?
		const float radiusSum = owner->radius + object->radius;
		const float massSum = owner->mass + object->mass;
		const bool objectMobile = (object->mobility != NULL);
		const float objectMassScale = (objectMobile)? (object->mass / massSum): 1.0f;

		if (objectDistSq >= Square(currentSpeed * GAME_SPEED + radiusSum))
			continue;
		if (objectDistSq >= Square(owner->pos.distance2D(goalPos)))
			continue;

		// note: positive angle cosines mean object is to our right
		const float objectDistToAvoidDirCenter = objectToUnit.dot(rightOfPath);

		if (objectToUnit.dot(avoidanceDir) >= radiusSum)
			continue;
		if (math::fabs(objectDistToAvoidDirCenter) >= radiusSum)
			continue;

		// do not bother steering around idling objects
		// (collision handling will push them aside, or
		// us in case of "allied" features)
		if (!object->isMoving && object->allyteam == owner->allyteam)
			continue;

		// if object and unit in relative motion are closing in on one another
		// (or not yet fully apart), then the object is on the path of the unit
		// and they are not collided
		if (objectMobile || (Distance2D(owner, object, SQUARE_SIZE) >= 0.0f)) {
			#if (DEBUG_OUTPUT == 1)
			{
				GML_RECMUTEX_LOCK(sel); // ObstacleAvoidance

				if (selectedUnits.selectedUnits.find(owner) != selectedUnits.selectedUnits.end()) {
					geometricObjects->AddLine(owner->pos + UpVector * 20, object->pos + UpVector * 20, 3, 1, 4);
				}
			}
			#endif

			const float iSqrtObjDist = math::isqrt2(objectDistSq);
			const float avoidScale = (AVOIDANCE_STRENGTH * iSqrtObjDist * iSqrtObjDist * iSqrtObjDist) * objectMassScale;

			// avoid collision by turning either left or right
			// using the direction thats needs most adjustment
			if (objectDistToAvoidDirCenter > 0.0f) {
				avoidRight += ((radiusSum - objectDistToAvoidDirCenter) * avoidScale);
			} else {
				avoidLeft += ((radiusSum + objectDistToAvoidDirCenter) * avoidScale);
			}
		}
	}

	moveData->tempOwner = NULL;


	// Sum up avoidance.
	avoidanceVec = (desiredDir.cross(UpVector) * (avoidRight - avoidLeft));

	#if (DEBUG_OUTPUT == 1)
	{
		GML_RECMUTEX_LOCK(sel); // ObstacleAvoidance

		if (selectedUnits.selectedUnits.find(owner) != selectedUnits.selectedUnits.end()) {
			int a = geometricObjects->AddLine(owner->pos + UpVector * 20, owner->pos + UpVector * 20 + avoidanceVec * 40, 7, 1, 4);
			geometricObjects->SetColor(a, 1, 0.3f, 0.3f, 0.6f);

			a = geometricObjects->AddLine(owner->pos + UpVector * 20, owner->pos + UpVector * 20 + desiredDir * 40, 7, 1, 4);
			geometricObjects->SetColor(a, 0.3f, 0.3f, 1, 0.6f);
		}
	}
	#endif


	// Return the resulting recommended velocity.
	avoidanceDir = (desiredDir + avoidanceVec).SafeNormalize();

#ifdef TRACE_SYNC
	tracefile << "[" << __FUNCTION__ << "] ";
	tracefile << "avoidanceVec = <" << avoidanceVec.x << " " << avoidanceVec.y << " " << avoidanceVec.z << ">\n";
#endif

	return (lastAvoidanceDir = avoidanceDir);
}



// Calculates an aproximation of the physical 2D-distance between given two objects.
float CGroundMoveType::Distance2D(CSolidObject* object1, CSolidObject* object2, float marginal)
{
	// calculate the distance in (x,z) depending
	// on the shape of the object footprints
	float dist2D;
	if (object1->xsize == object1->zsize || object2->xsize == object2->zsize) {
		// use xsize as a cylindrical radius.
		float3 distVec = (object1->midPos - object2->midPos);
		dist2D = distVec.Length2D() - (object1->xsize + object2->xsize) * SQUARE_SIZE / 2 + 2 * marginal;
	} else {
		// Pytagorean sum of the x and z distance.
		float3 distVec;
		float xdiff = math::fabs(object1->midPos.x - object2->midPos.x);
		float zdiff = math::fabs(object1->midPos.z - object2->midPos.z);

		distVec.x = xdiff - (object1->xsize + object2->xsize) * SQUARE_SIZE / 2 + 2 * marginal;
		distVec.z = zdiff - (object1->zsize + object2->zsize) * SQUARE_SIZE / 2 + 2 * marginal;

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

// Creates a path to the goal.
void CGroundMoveType::GetNewPath()
{
	assert(pathId == 0);
	pathId = pathManager->RequestPath(owner->mobility, owner->pos, goalPos, goalRadius, owner);

	// if new path received, can't be at waypoint
	if (pathId != 0) {
		atGoal = false;
		haveFinalWaypoint = false;

		currWayPoint = pathManager->NextWayPoint(pathId, owner->pos, 1.25f * SQUARE_SIZE, 0, owner->id);
		nextWayPoint = pathManager->NextWayPoint(pathId, currWayPoint, 1.25f * SQUARE_SIZE, 0, owner->id);
	} else {
		Fail();
	}

	// limit frequency of (case B) path-requests from SlowUpdate's
	pathRequestDelay = gs->frameNum + (UNIT_SLOWUPDATE_RATE << 1);
}


/*
Sets waypoint to next in path.
*/
void CGroundMoveType::GetNextWayPoint()
{
	if (pathId == 0) {
		return;
	}

	{
		#if (DEBUG_OUTPUT == 1)
		// plot the vector to currWayPoint
		const int figGroupID =
			geometricObjects->AddLine(owner->pos + UpVector * 20, currWayPoint + UpVector * 20, 8.0f, 1, 4);

		geometricObjects->SetColor(figGroupID, 1, 0.3f, 0.3f, 0.6f);
		#endif

		// perform a turn-radius check: if the waypoint
		// lies outside our turning circle, don't skip
		// it (since we can steer toward this waypoint
		// and pass it without slowing down)
		// note that we take the DIAMETER of the circle
		// to prevent sine-like "snaking" trajectories
		const float turnFrames = SPRING_CIRCLE_DIVS / turnRate;
		const float turnRadius = (owner->speed.Length() * turnFrames) / (PI + PI);

		if (currWayPointDist > (turnRadius * 2.0f)) {
			return;
		}
		if (currWayPointDist > MIN_WAYPOINT_DISTANCE && waypointDir.dot(flatFrontDir) >= 0.995f) {
			return;
		}
	}


	if (currWayPoint.SqDistance2D(goalPos) < Square(MIN_WAYPOINT_DISTANCE)) {
		// trigger Arrived on the next Update
		haveFinalWaypoint = true;
	}

	haveFinalWaypoint = haveFinalWaypoint || (nextWayPoint.x == -1.0f);

	if (haveFinalWaypoint) {
		currWayPoint = goalPos;
		nextWayPoint = goalPos;
		return;
	}

	// TODO-QTPFS:
	//     regularly check if a terrain-change modified our path by comparing
	//     NextWayPoint(pathId, owner->pos, 1.25f * SQUARE_SIZE, 0, owner->id)
	//     and waypoint, eg. every 15 frames
	//     if we do not and a terrain-change area happened to contain <waypoint>
	//     (the immediate next destination), then the unit will keep heading for
	//     it and might possibly get stuck
	//
	if ((nextWayPoint - currWayPoint).SqLength2D() > 0.01f) {
		currWayPoint = nextWayPoint;
		nextWayPoint = pathManager->NextWayPoint(pathId, currWayPoint, 1.25f * SQUARE_SIZE, 0, owner->id);

		if (currWayPoint.SqDistance2D(goalPos) < Square(MIN_WAYPOINT_DISTANCE)) {
			currWayPoint = goalPos;
		}
	}
}




/*
The distance the unit will move at max breaking before stopping,
starting from given speed.
*/
float CGroundMoveType::BreakingDistance(float speed) const
{
	const float rate = reversing? accRate: decRate;
	const float time = speed / std::max(rate, 0.001f);
	const float dist = 0.5f * rate * time * time;
	return dist;
}

/*
Gives the position this unit will end up at with full breaking
from current velocity.
*/
float3 CGroundMoveType::Here()
{
	const float dist = BreakingDistance(currentSpeed);
	const int   sign = int(!reversing) * 2 - 1;

	return (owner->pos + (owner->frontdir * dist * sign));
}






void CGroundMoveType::StartEngine() {
	// ran only if the unit has no path and is not already at goal
	if (pathId == 0 && !atGoal) {
		GetNewPath();

		// activate "engine" only if a path was found
		if (pathId != 0) {
			pathManager->UpdatePath(owner, pathId);

			owner->isMoving = true;
			owner->script->StartMoving();
		}
	}

	nextObstacleAvoidanceUpdate = gs->frameNum;
}

void CGroundMoveType::StopEngine() {

	if (pathId != 0) {
		pathManager->DeletePath(pathId);
		pathId = 0;

		if (!atGoal) {
			currWayPoint = Here();
		}

		// Stop animation.
		owner->script->StopMoving();

		LOG_L(L_DEBUG, "StopEngine: engine stopped for unit %i", owner->id);
	}

	owner->isMoving = false;
	wantedSpeed = 0.0f;
}



/* Called when the unit arrives at its goal. */
void CGroundMoveType::Arrived()
{
	// can only "arrive" if the engine is active
	if (progressState == Active) {
		// we have reached our goal
		StopEngine();

		#if (PLAY_SOUNDS == 1)
		if (owner->team == gu->myTeam) {
			Channels::General.PlayRandomSample(owner->unitDef->sounds.arrived, owner);
		}
		#endif

		// and the action is done
		progressState = Done;
		owner->commandAI->SlowUpdate();

		LOG_L(L_DEBUG, "Arrived: unit %i arrived", owner->id);
	}
}

/*
Makes the unit fail this action.
No more trials will be done before a new goal is given.
*/
void CGroundMoveType::Fail()
{
	LOG_L(L_DEBUG, "Fail: unit %i failed", owner->id);

	StopEngine();

	// failure of finding a path means that
	// this action has failed to reach its goal.
	progressState = Failed;

	eventHandler.UnitMoveFailed(owner);
	eoh->UnitMoveFailed(*owner);
}




// FIXME move this to a global space to optimize the check (atm unit collision checks are done twice for the collider & collidee!)
//
// allow some degree of inter-penetration (1 - 0.75)
// between objects to avoid sudden extreme responses
//
// FIXME: this is bad for immobile obstacles because
// their corners might stick out through the radius
#define FOOTPRINT_RADIUS(xs, zs) ((math::sqrt((xs * xs + zs * zs)) * 0.5f * SQUARE_SIZE) * 0.75f)

void CGroundMoveType::HandleObjectCollisions()
{
	static const float3 sepDirMask = float3(1.0f, 0.0f, 1.0f);

	CUnit* collider = owner;
	collider->mobility->tempOwner = collider;

	{
		const UnitDef*   colliderUD = collider->unitDef;
		const MoveData*  colliderMD = collider->mobility;
		const CMoveMath* colliderMM = colliderMD->moveMath;

		const float colliderSpeed = collider->speed.Length();
		const float colliderRadius = (colliderMD != NULL)?
			FOOTPRINT_RADIUS(colliderMD->xsize, colliderMD->zsize):
			FOOTPRINT_RADIUS(colliderUD->xsize, colliderUD->zsize);

		HandleUnitCollisions(collider, collider->pos, oldPos, colliderSpeed, colliderRadius, sepDirMask, colliderUD, colliderMD, colliderMM);
		HandleFeatureCollisions(collider, collider->pos, oldPos, colliderSpeed, colliderRadius, sepDirMask, colliderUD, colliderMD, colliderMM);
	}

	collider->mobility->tempOwner = NULL;
	collider->Block();
}

void CGroundMoveType::HandleUnitCollisions(
	CUnit* collider,
	const float3& colliderCurPos,
	const float3& colliderOldPos,
	const float colliderSpeed,
	const float colliderRadius,
	const float3& sepDirMask,
	const UnitDef* colliderUD,
	const MoveData* colliderMD,
	const CMoveMath* colliderMM
) {
	const float searchRadius = std::max(colliderSpeed, 1.0f) * (colliderRadius * 2.0f);

	const std::vector<CUnit*>& nearUnits = qf->GetUnitsExact(colliderCurPos, searchRadius);
	      std::vector<CUnit*>::const_iterator uit;

	// NOTE: probably too large for most units (eg. causes tree falling animations to be skipped)
	const float3 crushImpulse = collider->speed * ((reversing)? -collider->mass: collider->mass);

	for (uit = nearUnits.begin(); uit != nearUnits.end(); ++uit) {
		CUnit* collidee = const_cast<CUnit*>(*uit);

		if (collidee == collider) { continue; }
		if (collidee->moveType->IsSkidding()) { continue; }
		if (collidee->moveType->IsFlying()) { continue; }

		const UnitDef*   collideeUD = collidee->unitDef;
		const MoveData*  collideeMD = collidee->mobility;
		const CMoveMath* collideeMM = (collideeMD != NULL)? collideeMD->moveMath: NULL;

		const float3& collideeCurPos = collidee->pos;
		const float3& collideeOldPos = collidee->moveType->oldPos;

		const bool colliderMobile = (collider->mobility != NULL);
		const bool collideeMobile = (collideeMD != NULL);

		const float collideeSpeed = collidee->speed.Length();
		const float collideeRadius = collideeMobile?
			FOOTPRINT_RADIUS(collideeMD->xsize, collideeMD->zsize):
			FOOTPRINT_RADIUS(collidee  ->xsize, collidee  ->zsize);

		bool pushCollider = colliderMobile;
		bool pushCollidee = (collideeMobile || collideeUD->canfly);
		bool crushCollidee = false;

		const float3 separationVector = colliderCurPos - collideeCurPos;
		const float separationMinDist = (colliderRadius + collideeRadius) * (colliderRadius + collideeRadius);

		if ((separationVector.SqLength() - separationMinDist) > 0.01f) { continue; }
		if (collidee->usingScriptMoveType && !collidee->inAir) { pushCollidee = false; }
		if (collideeUD->pushResistant) { pushCollidee = false; }

		// if not an allied collision, neither party is allowed to be pushed (bi-directional)
		// if an allied collision, only the collidee is allowed to be crushed (uni-directional)
		const bool alliedCollision =
			teamHandler->Ally(collider->allyteam, collidee->allyteam) &&
			teamHandler->Ally(collidee->allyteam, collider->allyteam);

		pushCollider &= (alliedCollision || modInfo.allowPushingEnemyUnits || (collider->inAir && !colliderUD->IsAirUnit()));
		pushCollidee &= (alliedCollision || modInfo.allowPushingEnemyUnits || (collidee->inAir && !collideeUD->IsAirUnit()));
		crushCollidee |= (!alliedCollision || modInfo.allowCrushingAlliedUnits);
		crushCollidee &= (collider->speed != ZeroVector);

		// don't push/crush either party if the collidee does not block the collider
		if (colliderMM->IsNonBlocking(*colliderMD, collidee)) { continue; }
		if (crushCollidee && !colliderMM->CrushResistant(*colliderMD, collidee)) { collidee->Kill(crushImpulse, true); }

		eventHandler.UnitUnitCollision(collider, collidee);

		const float  sepDistance    = (separationVector.Length() + 0.01f);
		const float  penDistance    = std::max((colliderRadius + collideeRadius) - sepDistance, 1.0f);
		const float  sepResponse    = std::min(SQUARE_SIZE * 2.0f, penDistance * 0.5f);

		const float3 sepDirection   = (separationVector / sepDistance);
		const float3 colResponseVec = sepDirection * sepDirMask * sepResponse;

		const float
			m1 = collider->mass,
			m2 = collidee->mass,
			v1 = std::max(1.0f, colliderSpeed), // TODO: precalculate
			v2 = std::max(1.0f, collideeSpeed), // TODO: precalculate
			c1 = 1.0f + (1.0f - math::fabs(collider->frontdir.dot(-sepDirection))) * 5.0f,
			c2 = 1.0f + (1.0f - math::fabs(collidee->frontdir.dot( sepDirection))) * 5.0f,
			s1 = m1 * v1 * c1,
			s2 = m2 * v2 * c2;

		// far from a realistic treatment, but works
		const float collisionMassSum  = s1 + s2 + 1.0f;
		      float colliderMassScale = std::max(0.01f, std::min(0.99f, 1.0f - (s1 / collisionMassSum)));
		      float collideeMassScale = std::max(0.01f, std::min(0.99f, 1.0f - (s2 / collisionMassSum)));

		if (!collideeMobile) {
			const float3 colliderNxtPos = colliderCurPos + collider->speed;
			const CMoveMath::BlockType colliderCurPosBits = colliderMM->IsBlocked(*colliderMD, colliderCurPos);
			const CMoveMath::BlockType colliderNxtPosBits = colliderMM->IsBlocked(*colliderMD, colliderNxtPos);

			if ((colliderCurPosBits & CMoveMath::BLOCK_STRUCTURE) == 0)
				continue;
			if (colliderNxtPos == colliderCurPos)
				continue;

			if ((colliderNxtPosBits & CMoveMath::BLOCK_STRUCTURE) != 0) {
				// applied every frame objects are colliding, so be careful
				collider->AddImpulse(sepDirection * sepDirMask);

				currentSpeed = 0.0f;
				deltaSpeed = 0.0f;

				if ((gs->frameNum > pathRequestDelay) && ((-sepDirection).dot(owner->frontdir) >= 0.5f)) {
					// repath iff obstacle is within 60-degree cone; we do this
					// because the GNWP lookahead (for non-TIP units) can cause
					// corners to be cut across statically blocked squares
					StartMoving(goalPos, goalRadius, 0.0f);
				}
			}
		}

		const float3 colliderNewPos = colliderCurPos + (colResponseVec * colliderMassScale);
		const float3 collideeNewPos = collideeCurPos - (colResponseVec * collideeMassScale);

		// try to prevent both parties from being pushed onto non-traversable squares
		if (                  (colliderMM->IsBlocked(*colliderMD, colliderNewPos) & CMoveMath::BLOCK_STRUCTURE) != 0) { colliderMassScale = 0.0f; }
		if (collideeMobile && (collideeMM->IsBlocked(*collideeMD, collideeNewPos) & CMoveMath::BLOCK_STRUCTURE) != 0) { collideeMassScale = 0.0f; }
		if (                  colliderMM->GetPosSpeedMod(*colliderMD, colliderNewPos) <= 0.01f) { colliderMassScale = 0.0f; }
		if (collideeMobile && collideeMM->GetPosSpeedMod(*collideeMD, collideeNewPos) <= 0.01f) { collideeMassScale = 0.0f; }

		// ignore pushing contributions from idling collidee's
		if (collider->isMoving && !collidee->isMoving && alliedCollision) {
			colliderMassScale *= ((collideeMobile)? 0.0f: 1.0f);
		}

		     if (  pushCollider) { collider->Move3D( colResponseVec * colliderMassScale, true); }
		else if (colliderMobile) { collider->Move3D(colliderOldPos, false); }
		     if (  pushCollidee) { collidee->Move3D(-colResponseVec * collideeMassScale, true); }
		else if (collideeMobile) { collidee->Move3D(collideeOldPos, false); }

		#if 0
		if (!((gs->frameNum + collider->id) & 31) && !colliderCAI->unimportantMove) {
			// if we do not have an internal move order, tell units around us to bugger off
			// note: this causes too much chaos among the ranks when groups get large
			helper->BuggerOff(colliderCurPos + collider->frontdir * colliderRadius, colliderRadius, true, false, collider->team, collider);
		}
		#endif
	}
}

void CGroundMoveType::HandleFeatureCollisions(
	CUnit* collider,
	const float3& colliderCurPos,
	const float3& colliderOldPos,
	const float colliderSpeed,
	const float colliderRadius,
	const float3& sepDirMask,
	const UnitDef* colliderUD,
	const MoveData* colliderMD,
	const CMoveMath* colliderMM
) {
	const float searchRadius = std::max(colliderSpeed, 1.0f) * (colliderRadius * 2.0f);

	const std::vector<CFeature*>& nearFeatures = qf->GetFeaturesExact(colliderCurPos, searchRadius);
	      std::vector<CFeature*>::const_iterator fit;

	const float3 crushImpulse = collider->speed * ((reversing)? -collider->mass: collider->mass);

	for (fit = nearFeatures.begin(); fit != nearFeatures.end(); ++fit) {
		CFeature* collidee = const_cast<CFeature*>(*fit);

	//	const FeatureDef* collideeFD = collidee->def;
		const float3& collideeCurPos = collidee->pos;

	//	const float collideeRadius = FOOTPRINT_RADIUS(collideeFD->xsize, collideeFD->zsize);
		const float collideeRadius = FOOTPRINT_RADIUS(collidee  ->xsize, collidee  ->zsize);

		const float3 separationVector = colliderCurPos - collideeCurPos;
		const float separationMinDist = (colliderRadius + collideeRadius) * (colliderRadius + collideeRadius);

		if ((separationVector.SqLength() - separationMinDist) > 0.01f) { continue; }

		if (colliderMM->IsNonBlocking(*colliderMD, collidee)) { continue; }
		if (!colliderMM->CrushResistant(*colliderMD, collidee)) { collidee->Kill(crushImpulse, true); }

		eventHandler.UnitFeatureCollision(collider, collidee);

		const float  sepDistance    = (separationVector.Length() + 0.01f);
		const float  penDistance    = std::max((colliderRadius + collideeRadius) - sepDistance, 1.0f);
		const float  sepResponse    = std::min(SQUARE_SIZE * 2.0f, penDistance * 0.5f);

		const float3 sepDirection   = (separationVector / sepDistance);
		const float3 colResponseVec = sepDirection * sepDirMask * sepResponse;

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
			s2 = m2 * v2 * c2;

		const float collisionMassSum  = s1 + s2 + 1.0f;
		const float colliderMassScale = std::max(0.01f, std::min(0.99f, 1.0f - (s1 / collisionMassSum)));
	//	const float collideeMassScale = std::max(0.01f, std::min(0.99f, 1.0f - (s2 / collisionMassSum)));

		if (collidee->reachedFinalPos) {
			const float3 colliderNxtPos = colliderCurPos + collider->speed;
			const CMoveMath::BlockType colliderCurPosBits = colliderMM->IsBlocked(*colliderMD, colliderCurPos);
			const CMoveMath::BlockType colliderNxtPosBits = colliderMM->IsBlocked(*colliderMD, colliderNxtPos);

			if ((colliderCurPosBits & CMoveMath::BLOCK_STRUCTURE) == 0)
				continue;
			if (colliderNxtPos == colliderCurPos)
				continue;

			if ((colliderNxtPosBits & CMoveMath::BLOCK_STRUCTURE) != 0) {
				// applied every frame objects are colliding, so be careful
				collider->AddImpulse(sepDirection * sepDirMask);

				currentSpeed = 0.0f;
				deltaSpeed = 0.0f;

				if ((gs->frameNum > pathRequestDelay) && ((-sepDirection).dot(owner->frontdir) >= 0.5f)) {
					StartMoving(goalPos, goalRadius, 0.0f);
				}
			}
		}

		collider->Move3D(colResponseVec * colliderMassScale, true);
	}
}

#undef FOOTPRINT_RADIUS




void CGroundMoveType::CreateLineTable()
{
	// for every <xt, zt> pair, computes a set of regularly spaced
	// grid sample-points (int2 offsets) along the line from <start>
	// to <to>; <to> ranges from [x=-4.5, z=-4.5] to [x=+5.5, z=+5.5]
	//
	// TestNewTerrainSquare() and ObstacleAvoidance() check whether
	// squares are blocked at these offsets to get a fast estimate
	// of terrain passability
	for (int yt = 0; yt < LINETABLE_SIZE; ++yt) {
		for (int xt = 0; xt < LINETABLE_SIZE; ++xt) {
			// center-point of grid-center cell
			const float3 start(0.5f, 0.0f, 0.5f);
			// center-point of target cell
			const float3 to((xt - (LINETABLE_SIZE / 2)) + 0.5f, 0.0f, (yt - (LINETABLE_SIZE / 2)) + 0.5f);

			const float dx = to.x - start.x;
			const float dz = to.z - start.z;
			float xp = start.x;
			float zp = start.z;

			if (floor(start.x) == floor(to.x)) {
				if (dz > 0.0f) {
					for (int a = 1; a <= floor(to.z); ++a)
						lineTable[yt][xt].push_back(int2(0, a));
				} else {
					for (int a = -1; a >= floor(to.z); --a)
						lineTable[yt][xt].push_back(int2(0, a));
				}
			} else if (floor(start.z) == floor(to.z)) {
				if (dx > 0.0f) {
					for (int a = 1; a <= floor(to.x); ++a)
						lineTable[yt][xt].push_back(int2(a, 0));
				} else {
					for (int a = -1; a >= floor(to.x); --a)
						lineTable[yt][xt].push_back(int2(a, 0));
				}
			} else {
				float xn, zn;
				bool keepgoing = true;

				while (keepgoing) {
					if (dx > 0.0f) {
						xn = (floor(xp) + 1.0f - xp) / dx;
					} else {
						xn = (floor(xp)        - xp) / dx;
					}
					if (dz > 0.0f) {
						zn = (floor(zp) + 1.0f - zp) / dz;
					} else {
						zn = (floor(zp)        - zp) / dz;
					}

					if (xn < zn) {
						xp += (xn + 0.0001f) * dx;
						zp += (xn + 0.0001f) * dz;
					} else {
						xp += (zn + 0.0001f) * dx;
						zp += (zn + 0.0001f) * dz;
					}

					keepgoing =
						math::fabs(xp - start.x) <= math::fabs(to.x - start.x) &&
						math::fabs(zp - start.z) <= math::fabs(to.z - start.z);
					int2 pt(int(floor(xp)), int(floor(zp)));

					static const int MIN_IDX = -int(LINETABLE_SIZE / 2);
					static const int MAX_IDX = -MIN_IDX;

					if (MIN_IDX > pt.x || pt.x > MAX_IDX) continue;
					if (MIN_IDX > pt.y || pt.y > MAX_IDX) continue;

					lineTable[yt][xt].push_back(pt);
				}
			}
		}
	}
}

void CGroundMoveType::DeleteLineTable()
{
	for (int yt = 0; yt < LINETABLE_SIZE; ++yt) {
		for (int xt = 0; xt < LINETABLE_SIZE; ++xt) {
			lineTable[yt][xt].clear();
		}
	}
}

void CGroundMoveType::TestNewTerrainSquare()
{
	// first make sure we don't go into any terrain we cant get out of
	int newMoveSquareX = owner->pos.x / (MIN_WAYPOINT_DISTANCE);
	int newMoveSquareY = owner->pos.z / (MIN_WAYPOINT_DISTANCE);

	float3 newpos = owner->pos;

	if (newMoveSquareX != moveSquareX || newMoveSquareY != moveSquareY) {
		const CMoveMath* movemath = owner->unitDef->movedata->moveMath;
		const MoveData& md = *(owner->unitDef->movedata);
		const float cmod = movemath->GetPosSpeedMod(md, moveSquareX, moveSquareY);

		if (math::fabs(owner->frontdir.x) < math::fabs(owner->frontdir.z)) {
			if (newMoveSquareX > moveSquareX) {
				const float nmod = movemath->GetPosSpeedMod(md, newMoveSquareX, newMoveSquareY);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.x = moveSquareX * MIN_WAYPOINT_DISTANCE + (MIN_WAYPOINT_DISTANCE - 0.01f);
					newMoveSquareX = moveSquareX;
				}
			} else if (newMoveSquareX < moveSquareX) {
				const float nmod = movemath->GetPosSpeedMod(md, newMoveSquareX, newMoveSquareY);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.x = moveSquareX * MIN_WAYPOINT_DISTANCE + 0.01f;
					newMoveSquareX = moveSquareX;
				}
			}
			if (newMoveSquareY > moveSquareY) {
				const float nmod = movemath->GetPosSpeedMod(md, newMoveSquareX, newMoveSquareY);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.z = moveSquareY * MIN_WAYPOINT_DISTANCE + (MIN_WAYPOINT_DISTANCE - 0.01f);
					newMoveSquareY = moveSquareY;
				}
			} else if (newMoveSquareY < moveSquareY) {
				const float nmod = movemath->GetPosSpeedMod(md, newMoveSquareX, newMoveSquareY);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.z = moveSquareY * MIN_WAYPOINT_DISTANCE + 0.01f;
					newMoveSquareY = moveSquareY;
				}
			}
		} else {
			if (newMoveSquareY > moveSquareY) {
				const float nmod = movemath->GetPosSpeedMod(md, newMoveSquareX, newMoveSquareY);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.z = moveSquareY * MIN_WAYPOINT_DISTANCE + (MIN_WAYPOINT_DISTANCE - 0.01f);
					newMoveSquareY = moveSquareY;
				}
			} else if (newMoveSquareY < moveSquareY) {
				const float nmod = movemath->GetPosSpeedMod(md, newMoveSquareX, newMoveSquareY);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.z = moveSquareY * MIN_WAYPOINT_DISTANCE + 0.01f;
					newMoveSquareY = moveSquareY;
				}
			}

			if (newMoveSquareX > moveSquareX) {
				const float nmod = movemath->GetPosSpeedMod(md, newMoveSquareX, newMoveSquareY);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.x = moveSquareX * MIN_WAYPOINT_DISTANCE + (MIN_WAYPOINT_DISTANCE - 0.01f);
					newMoveSquareX = moveSquareX;
				}
			} else if (newMoveSquareX < moveSquareX) {
				const float nmod = movemath->GetPosSpeedMod(md, newMoveSquareX, newMoveSquareY);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.x = moveSquareX * MIN_WAYPOINT_DISTANCE + 0.01f;
					newMoveSquareX = moveSquareX;
				}
			}
		}
		// do not teleport units. if the unit is too far away from old position,
		// reset the pathfinder instead of teleporting it.
		if (newpos.SqDistance2D(owner->pos) > (MIN_WAYPOINT_DISTANCE * MIN_WAYPOINT_DISTANCE)) {
			newMoveSquareX = (int) owner->pos.x / (MIN_WAYPOINT_DISTANCE);
			newMoveSquareY = (int) owner->pos.z / (MIN_WAYPOINT_DISTANCE);
		} else {
			owner->Move3D(newpos, false);
		}

		if (newMoveSquareX != moveSquareX || newMoveSquareY != moveSquareY) {
			moveSquareX = newMoveSquareX;
			moveSquareY = newMoveSquareY;

			// if we have moved, check if we can get to the next waypoint
			int nwsx = (int) nextWayPoint.x / (MIN_WAYPOINT_DISTANCE) - moveSquareX;
			int nwsy = (int) nextWayPoint.z / (MIN_WAYPOINT_DISTANCE) - moveSquareY;
			int numIter = 0;

			static const unsigned int blockBits =
				CMoveMath::BLOCK_STRUCTURE |
				CMoveMath::BLOCK_MOBILE |
				CMoveMath::BLOCK_MOBILE_BUSY;

			while ((nwsx * nwsx + nwsy * nwsy) < LINETABLE_SIZE && !haveFinalWaypoint && pathId != 0) {
				const int ltx = nwsx + LINETABLE_SIZE / 2;
				const int lty = nwsy + LINETABLE_SIZE / 2;
				bool wpOk = true;

				if (ltx >= 0 && ltx < LINETABLE_SIZE && lty >= 0 && lty < LINETABLE_SIZE) {
					for (std::vector<int2>::iterator li = lineTable[lty][ltx].begin(); li != lineTable[lty][ltx].end(); ++li) {
						const int x = (moveSquareX + li->x);
						const int y = (moveSquareY + li->y);

						if ((movemath->IsBlocked(md, x, y) & blockBits) ||
							movemath->GetPosSpeedMod(md, x, y) <= 0.01f) {
							wpOk = false;
							break;
						}
					}
				}

				if (!wpOk || numIter > 6) {
					break;
				}

				GetNextWayPoint();

				nwsx = (int) nextWayPoint.x / (MIN_WAYPOINT_DISTANCE) - moveSquareX;
				nwsy = (int) nextWayPoint.z / (MIN_WAYPOINT_DISTANCE) - moveSquareY;
				++numIter;
			}
		}
	}
}

void CGroundMoveType::LeaveTransport()
{
	oldPos = owner->pos + UpVector * 0.001f;
}



void CGroundMoveType::KeepPointingTo(float3 pos, float distance, bool aggressive) {
	mainHeadingPos = pos;
	useMainHeading = aggressive;

	if (!useMainHeading) return;
	if (owner->weapons.empty()) return;

	CWeapon* frontWeapon = owner->weapons.front();

	if (!frontWeapon->weaponDef->waterweapon && mainHeadingPos.y <= 1.0f) {
		mainHeadingPos.y = 1.0f;
	}

	float3 dir1 = frontWeapon->mainDir;
	float3 dir2 = mainHeadingPos - owner->pos;

	dir1.y = 0.0f;
	dir1.Normalize();
	dir2.y = 0.0f;
	dir2.SafeNormalize();

	if (dir2 == ZeroVector)
		return;

	short heading =
		GetHeadingFromVector(dir2.x, dir2.z) -
		GetHeadingFromVector(dir1.x, dir1.z);

	if (owner->heading == heading)
		return;

	if (!frontWeapon->TryTarget(mainHeadingPos, true, 0)) {
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
	if (!useMainHeading) return;
	if (owner->weapons.empty()) return;

	CWeapon* frontWeapon = owner->weapons.front();

	float3 dir1 = frontWeapon->mainDir;
	float3 dir2 = mainHeadingPos - owner->pos;

	dir1.y = 0.0f;
	dir1.Normalize();
	dir2.y = 0.0f;
	dir2.SafeNormalize();

	ASSERT_SYNCED(dir1);
	ASSERT_SYNCED(dir2);

	if (dir2 == ZeroVector)
		return;

	short heading =
		GetHeadingFromVector(dir2.x, dir2.z) -
		GetHeadingFromVector(dir1.x, dir1.z);

	ASSERT_SYNCED(heading);

	if (progressState == Active) {
		if (owner->heading == heading) {
			// stop turning
			owner->script->StopMoving();
			progressState = Done;
		} else {
			ChangeHeading(heading);
		}
	} else {
		if (owner->heading != heading) {
			if (!frontWeapon->TryTarget(mainHeadingPos, true, 0)) {
				// start moving
				progressState = Active;
				owner->script->StartMoving();
				ChangeHeading(heading);
			}
		}
	}
}

void CGroundMoveType::SetMaxSpeed(float speed)
{
	maxSpeed        = std::min(speed, maxSpeedDef);
	maxReverseSpeed = std::min(speed, maxReverseSpeed);

	requestedSpeed = speed;
}

bool CGroundMoveType::OnSlope(float minSlideTolerance) {
	const UnitDef* ud = owner->unitDef;
	const float3& pos = owner->pos;

	if (ud->slideTolerance < minSlideTolerance) { return false; }
	if (owner->unitDef->floatOnWater && owner->inWater) { return false; }
	if (!pos.IsInBounds()) { return false; }

	// if minSlideTolerance is zero, do not multiply maxSlope by ud->slideTolerance
	// (otherwise the unit could stop on an invalid path location, and be teleported
	// back)
	const float gSlope = ground->GetSlope(pos.x, pos.z);
	const float uSlope = ud->movedata->maxSlope * ((minSlideTolerance <= 0.0f)? 1.0f: ud->slideTolerance);

	return (gSlope > uSlope);
}



float CGroundMoveType::GetGroundHeight(const float3& p) const
{
	float h = 0.0f;

	if (owner->unitDef->floatOnWater) {
		// in [0, maxHeight]
		h = ground->GetHeightAboveWater(p.x, p.z);
	} else {
		// in [minHeight, maxHeight]
		h = ground->GetHeightReal(p.x, p.z);
	}

	return h;
}

void CGroundMoveType::AdjustPosToWaterLine()
{
	if (!(owner->falling || flying)) {
		float groundHeight = GetGroundHeight(owner->pos);

		if (owner->unitDef->floatOnWater && owner->inWater && groundHeight <= 0.0f) {
			groundHeight = -owner->unitDef->waterline;
		}

		owner->Move1D(groundHeight, 1, false);

		/*
		const UnitDef* ud = owner->unitDef;
		const MoveData* md = ud->movedata;
		const CMoveMath* mm = md->moveMath;

		y = mm->yLevel(owner->pos.x, owner->pos.z);

		if (owner->unitDef->floatOnWater && owner->inWater) {
			y -= owner->unitDef->waterline;
		}

		owner->Move1D(y, 1, false);
		*/
	}
}

bool CGroundMoveType::UpdateDirectControl()
{
	const CPlayer* myPlayer = gu->GetMyPlayer();
	const FPSUnitController& selfCon = myPlayer->fpsController;
	const FPSUnitController& unitCon = owner->fpsControlPlayer->fpsController;
	const bool wantReverse = (unitCon.back && !unitCon.forward);
	float turnSign = 0.0f;

	currWayPoint = owner->pos;
	currWayPoint += wantReverse ? -owner->frontdir * 100 : owner->frontdir * 100;
	currWayPoint.ClampInBounds();

	if (unitCon.forward) {
		SetDeltaSpeed(maxSpeed, wantReverse, true);

		owner->isMoving = true;
		owner->script->StartMoving();
	} else if (unitCon.back) {
		SetDeltaSpeed(maxReverseSpeed, wantReverse, true);

		owner->isMoving = true;
		owner->script->StartMoving();
	} else {
		// not moving forward or backward, stop
		SetDeltaSpeed(0.0f, false, true);

		owner->isMoving = false;
		owner->script->StopMoving();
	}

	if (unitCon.left ) { ChangeHeading(owner->heading + turnRate); turnSign =  1.0f; }
	if (unitCon.right) { ChangeHeading(owner->heading - turnRate); turnSign = -1.0f; }

	if (selfCon.GetControllee() == owner) {
		camera->rot.y += (turnRate * turnSign * TAANG2RAD);
	}

	return wantReverse;
}

void CGroundMoveType::UpdateOwnerPos(bool wantReverse)
{
	if (wantedSpeed > 0.0f || currentSpeed != 0.0f) {
		if (wantReverse) {
			if (!reversing) {
				reversing = (currentSpeed <= accRate);
			}
		} else {
			if (reversing) {
				reversing = (currentSpeed > accRate);
			}
		}

		const UnitDef* ud = owner->unitDef;
		const MoveData* md = ud->movedata;
		const CMoveMath* mm = md->moveMath;

		const int    speedSign = int(!reversing) * 2 - 1;
		const float  speedScale = std::min(currentSpeed + deltaSpeed, (reversing? maxReverseSpeed: maxSpeed));
		const float3 speedVector = owner->frontdir * speedScale * speedSign;

		owner->speed = speedVector;

		if (mm->GetPosSpeedMod(*md, owner->pos + owner->speed) <= 0.01f) {
			// never move onto an impassable square (units
			// can still tunnel across them at high enough
			// speeds however)
			owner->speed = ZeroVector;
		} else {
			// use the simplest possible Euler integration
			owner->Move3D(owner->speed, true);
		}

		currentSpeed = (owner->speed != ZeroVector)? speedScale: 0.0f;
		deltaSpeed = 0.0f;

		assert(math::fabs(currentSpeed) < 1e6f);
	}

	if (!wantReverse && currentSpeed == 0.0f) {
		reversing = false;
	}
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

	const float3 waypointDif  = goalPos - owner->pos;                                           // use final WP for ETA
	const float waypointDist  = waypointDif.Length();                                           // in elmos
	const float waypointFETA  = (waypointDist / maxSpeed);                                      // in frames (simplistic)
	const float waypointRETA  = (waypointDist / maxReverseSpeed);                               // in frames (simplistic)
	const float waypointDirDP = waypointDir2D.dot(owner->frontdir);
	const float waypointAngle = Clamp(waypointDirDP, -1.0f, 1.0f);                              // prevent NaN's
	const float turnAngleDeg  = math::acosf(waypointAngle) * RAD2DEG;                           // in degrees
	const float turnAngleSpr  = (turnAngleDeg / 360.0f) * SPRING_CIRCLE_DIVS;                   // in "headings"
	const float revAngleSpr   = SHORTINT_MAXVALUE - turnAngleSpr;                               // 180 deg - angle

	// units start accelerating before finishing the turn, so subtract something
	const float turnTimeMod   = 5.0f;
	const float turnAngleTime = std::max(0.0f, (turnAngleSpr / turnRate) - turnTimeMod); // in frames
	const float revAngleTime  = std::max(0.0f, (revAngleSpr  / turnRate) - turnTimeMod);

	const float apxSpeedAfterTurn  = std::max(0.f, currentSpeed - 0.125f * (turnAngleTime * decRate));
	const float apxRevSpdAfterTurn = std::max(0.f, currentSpeed - 0.125f * (revAngleTime  * decRate));
	const float decTime       = ( reversing * apxSpeedAfterTurn)  / decRate;
	const float revDecTime    = (!reversing * apxRevSpdAfterTurn) / decRate;
	const float accTime       = (maxSpeed        - !reversing * apxSpeedAfterTurn)  / accRate;
	const float revAccTime    = (maxReverseSpeed -  reversing * apxRevSpdAfterTurn) / accRate;
	const float revAccDecTime = revDecTime + revAccTime;

	const float fwdETA = waypointFETA + turnAngleTime + accTime + decTime;
	const float revETA = waypointRETA + revAngleTime + revAccDecTime;

	return (fwdETA > revETA);
}
