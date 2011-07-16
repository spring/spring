/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
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
#include "System/LogOutput.h"
#include "System/FastMath.h"
#include "System/myMath.h"
#include "System/Vec2.h"
#include "System/Sound/SoundChannels.h"
#include "System/Sync/SyncTracer.h"

#define MIN_WAYPOINT_DISTANCE (SQUARE_SIZE << 1)
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
	CR_MEMBER(deltaHeading),

	CR_MEMBER(pathId),
	CR_MEMBER(goalRadius),

	CR_MEMBER(waypoint),
	CR_MEMBER(nextWaypoint),
	CR_MEMBER(atGoal),
	CR_MEMBER(haveFinalWaypoint),
	CR_MEMBER(currWayPointDist),
	CR_MEMBER(prevWayPointDist),

	CR_MEMBER(pathRequestDelay),

	CR_MEMBER(numIdlingUpdates),
	CR_MEMBER(numIdlingSlowUpdates),

	CR_MEMBER(moveSquareX),
	CR_MEMBER(moveSquareY),

	CR_MEMBER(nextDeltaSpeedUpdate),
	CR_MEMBER(nextObstacleAvoidanceUpdate),

	CR_MEMBER(skidding),
	CR_MEMBER(flying),
	CR_MEMBER(reversing),
	CR_MEMBER(idling),
	CR_MEMBER(canReverse),
	CR_MEMBER(useMainHeading),

	CR_MEMBER(waypointDir),
	CR_MEMBER(flatFrontDir),

	CR_MEMBER(skidRotVector),
	CR_MEMBER(skidRotSpeed),
	CR_MEMBER(skidRotSpeed2),
	CR_MEMBER(skidRotPos2),
	CR_ENUM_MEMBER(oldPhysState),

	CR_MEMBER(mainHeadingPos),

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
	deltaHeading(0),

	pathId(0),
	goalRadius(0),

	waypoint(ZeroVector),
	nextWaypoint(ZeroVector),
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
	skidRotSpeed2(0.0f),
	skidRotPos2(0.0f),
	oldPhysState(CSolidObject::OnGround),

	flatFrontDir(0.0f, 0.0, 1.0f),
	mainHeadingPos(ZeroVector),

	nextDeltaSpeedUpdate(0),
	nextObstacleAvoidanceUpdate(0),

	pathRequestDelay(0),

	numIdlingUpdates(0),
	numIdlingSlowUpdates(0)
{
	if (owner) {
		moveSquareX = owner->pos.x / MIN_WAYPOINT_DISTANCE;
		moveSquareY = owner->pos.z / MIN_WAYPOINT_DISTANCE;
	} else {
		moveSquareX = 0;
		moveSquareY = 0;
	}
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
	ASSERT_SYNCED_FLOAT3(owner->pos);

	if (owner->transporter != NULL) {
		return false;
	}

	if (OnSlope() && (!owner->floatOnWater || !owner->inWater)) {
		skidding = true;
	}
	if (skidding) {
		UpdateSkid();
		return false;
	}

	ASSERT_SYNCED_FLOAT3(owner->pos);

	// set drop height when we start to drop
	if (owner->falling) {
		UpdateControlledDrop();
		return false;
	}

	ASSERT_SYNCED_FLOAT3(owner->pos);

	bool hasMoved = false;
	bool wantReverse = false;

	if (owner->stunned || owner->beingBuilt) {
		owner->script->StopMoving();
		SetDeltaSpeed(0.0f, false);
	} else {
		if (owner->fpsControlPlayer != NULL) {
			wantReverse = UpdateDirectControl();
			ChangeHeading(owner->heading + deltaHeading);
		} else {
			if (pathId == 0) {
				SetDeltaSpeed(0.0f, false);
				SetMainHeading();
			} else {
				// TODO: Stop the unit from moving as a reaction on collision/explosion physics.
				ASSERT_SYNCED_FLOAT3(waypoint);
				ASSERT_SYNCED_FLOAT3(owner->pos);

				prevWayPointDist = currWayPointDist;
				currWayPointDist = owner->pos.distance2D(waypoint);
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

				// Set direction to waypoint.
				waypointDir = waypoint - owner->pos;
				waypointDir.y = 0.0f;
				waypointDir.SafeNormalize();

				ASSERT_SYNCED_FLOAT3(waypointDir);

				const float3 wpDirInv = -waypointDir;
				const float3 wpPosTmp = owner->pos + wpDirInv;
				const bool   wpBehind = (waypointDir.dot(flatFrontDir) < 0.0f);

				if (!haveFinalWaypoint) {
					GetNextWaypoint();
				} else {
					if (atGoal) {
						Arrived();
					}
				}

				if (wpBehind) {
					wantReverse = WantReverse(waypointDir);
				}

				// apply obstacle avoidance (steering)
				const float3 avoidVec = ObstacleAvoidance(waypointDir);

				ASSERT_SYNCED_FLOAT3(avoidVec);

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
		}
	}

	// these must be executed even when stunned (so
	// units do not get buried by restoring terrain)
	UpdateOwnerPos(wantReverse);
	AdjustPosToWaterLine();

	if (owner->pos != oldPos) {
		TestNewTerrainSquare();
		HandleObjectCollisions();

		// note: HandleObjectCollisions() may have negated the position set
		// by UpdateOwnerPos() (so that owner->pos is again equal to oldPos)
		owner->speed = owner->pos - oldPos;
		owner->UpdateMidPos();

		// too many false negatives: speed is unreliable if stuck behind an obstacle
		//   idling = (owner->speed.SqLength() < (accRate * accRate));
		// too many false positives: waypoint-distance delta and speed vary too much
		//   idling = (Square(currWayPointDist - prevWayPointDist) < owner->speed.SqLength());
		// too many false positives: many slow units cannot even manage 1 elmo/frame
		//   idling = (Square(currWayPointDist - prevWayPointDist) < 1.0f);

		idling = (Square(currWayPointDist - prevWayPointDist) <= (owner->speed.SqLength() * 0.5f));
		oldPos = owner->pos;
		hasMoved = true;
	} else {
		owner->speed = ZeroVector;
		idling = true;
	}

	return hasMoved;
}

void CGroundMoveType::SlowUpdate()
{
	if (owner->transporter) {
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
				#if (DEBUG_OUTPUT == 1)
				logOutput.Print("[CGMT::SU] unit %i has pathID %i but %i ETA failures", owner->id, pathId, numIdlingUpdates);
				#endif

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
				#if (DEBUG_OUTPUT == 1)
				logOutput.Print("[CGMT::SU] unit %i has no path", owner->id);
				#endif

				StopEngine();
				StartEngine();
			}
		}
	}


	if (!flying) {
		// just kindly move it into the map again instead of deleting
		owner->pos.CheckInBounds();
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

	#if (DEBUG_OUTPUT == 1)
	logOutput.Print("[CGMT::StartMoving] starting engine for unit %i", owner->id);
	#endif

	StartEngine();

	#if (PLAY_SOUNDS == 1)
	if (owner->team == gu->myTeam) {
		const int soundIdx = owner->unitDef->sounds.activate.getRandomIdx();
		if (soundIdx >= 0) {
			Channels::UnitReply.PlaySample(
				owner->unitDef->sounds.activate.getID(soundIdx), owner,
				owner->unitDef->sounds.activate.getVolume(soundIdx));
		}
	}
	#endif
}

void CGroundMoveType::StopMoving() {
#ifdef TRACE_SYNC
	tracefile << "[" << __FUNCTION__ << "] ";
	tracefile << owner->pos.x << " " << owner->pos.y << " " << owner->pos.z << " " << owner->id << "\n";
#endif

	#if (DEBUG_OUTPUT == 1)
	logOutput.Print("[CGMT::StopMoving] stopping engine for unit %i", owner->id);
	#endif

	StopEngine();

	useMainHeading = false;
	progressState = Done;
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
	float wSpeed = reversing? maxReverseSpeed: maxSpeed;

	if (wantedSpeed > 0.0f) {
		const UnitDef* ud = owner->unitDef;
		const float groundMod = ud->movedata->moveMath->GetPosSpeedMod(*ud->movedata, owner->pos, flatFrontDir);

		wSpeed *= groundMod;

		const float3 goalDifFwd = waypoint - owner->pos;
		const float3 goalDifRev = -goalDifFwd;
		const float3 goalPosTmp = owner->pos + goalDifRev;

		const float3 goalDif = reversing? goalDifRev: goalDifFwd;
		const short turnDeltaHeading = owner->heading - GetHeadingFromVector(goalDif.x, goalDif.z);

		if (!fpsMode && turnDeltaHeading != 0) {
			// only auto-adjust speed for turns when not in FPS mode
			const bool startBreaking = (haveFinalWaypoint && !atGoal);

			const float reqTurnAngle = reversing?
				(streflop::acosf(waypointDir.dot(-flatFrontDir)) * (180.0f / PI)):
				(streflop::acosf(waypointDir.dot( flatFrontDir)) * (180.0f / PI));
			const float maxTurnAngle = (turnRate / SPRING_CIRCLE_DIVS) * 360.0f;
			const float reducedSpeed = reversing?
				(maxReverseSpeed * (maxTurnAngle / reqTurnAngle)):
				(maxSpeed * (maxTurnAngle / reqTurnAngle));

			if (startBreaking) {
				// at this point, Update() will no longer call GetNextWaypoint()
				// and we must slow down to prevent entering an infinite circle
				wSpeed = std::min(wSpeed, fastmath::apxsqrt(currWayPointDist * decRate));
			}

			if (!ud->turnInPlace) {
				if (waypointDir.SqLength() > 0.1f) {
					if (reqTurnAngle > maxTurnAngle) {
						wSpeed = std::min(wSpeed, std::max(ud->turnInPlaceSpeedLimit, reducedSpeed));
					}
				}
			} else {
				wSpeed = reducedSpeed;
			}
		}

		wSpeed = std::min(wSpeed, wantedSpeed);
	} else {
		wSpeed = 0.0f;
	}


	float speedDif = wSpeed - currentSpeed;

	// make the forward/reverse transitions more fluid
	// (iff we are already moving in one direction and
	// want to go the opposite)
	if ( wantReverse && !reversing) { speedDif = -currentSpeed; }
	if (!wantReverse &&  reversing) { speedDif = -currentSpeed; }

	// limit speed change according to acceleration
	if (math::fabs(speedDif) < 0.05f) {
		// we are already going (mostly) how fast we want to go
		deltaSpeed = speedDif * 0.125f;
		nextDeltaSpeedUpdate = gs->frameNum + 8;
	} else if (speedDif > 0.0f) {
		// we want to accelerate
		if (speedDif < accRate) {
			deltaSpeed = speedDif;
			nextDeltaSpeedUpdate = gs->frameNum;
		} else {
			deltaSpeed = accRate;
			nextDeltaSpeedUpdate = (gs->frameNum + std::min(8.0f, speedDif / accRate));
		}
	} else {
		// we want to decelerate
		if (speedDif > -10.0f * decRate) {
			deltaSpeed = speedDif;
			nextDeltaSpeedUpdate = gs->frameNum + 1;
		} else {
			deltaSpeed = -10.0f * decRate;
			nextDeltaSpeedUpdate = (gs->frameNum + std::min(8.0f, speedDif / deltaSpeed));
		}
	}

#ifdef TRACE_SYNC
	tracefile << "[" << __FUNCTION__ << "] ";
	tracefile << owner->pos.x << " " << owner->pos.y << " " << owner->pos.z << " " << deltaSpeed << " " /*<< wSpeed*/ << " " << owner->id << "\n";
#endif
}

/*
Changes the heading of the owner.
*/
void CGroundMoveType::ChangeHeading(short wantedHeading) {
#ifdef TRACE_SYNC
	short _oldheading = owner->heading;
#endif
	SyncedSshort& heading = owner->heading;

	deltaHeading = wantedHeading - heading;

	ASSERT_SYNCED_PRIMITIVE(deltaHeading);
	ASSERT_SYNCED_PRIMITIVE(turnRate);
	ASSERT_SYNCED_PRIMITIVE((short)turnRate);

	short sTurnRate = short(turnRate);

	if (deltaHeading > 0) {
		short tmp = (deltaHeading < sTurnRate)? deltaHeading: sTurnRate;
		ASSERT_SYNCED_PRIMITIVE(tmp);
		heading += tmp;
	} else {
		short tmp = (deltaHeading > -sTurnRate)? deltaHeading: -sTurnRate;
		ASSERT_SYNCED_PRIMITIVE(tmp);
		heading += tmp;
	}

#ifdef TRACE_SYNC
	tracefile << "[" << __FUNCTION__ << "] ";
	tracefile << "unit " << owner->id << " changed heading to " << heading << " from " << _oldheading << " (wantedHeading: " << wantedHeading << ")\n";
#endif

	owner->SetDirectionFromHeading();

	flatFrontDir = owner->frontdir;
	flatFrontDir.y = 0.0f;
	flatFrontDir.Normalize();
}




void CGroundMoveType::ImpulseAdded(const float3&)
{
	if (owner->beingBuilt || owner->unitDef->movedata->moveType == MoveData::Ship_Move)
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

	if (impulse.SqLength() > 9.0f || groundImpulseScale > 0.3f) {
		skidding = true;
		speed += impulse;
		impulse = ZeroVector;

		// FIXME: impulse should not cause a _random_ rotational component
		skidRotSpeed += (gs->randFloat() - 0.5f) * 1500;
		skidRotSpeed2 = 0.0f;
		skidRotPos2 = 0.0f;

		float3 skidDir;

		if (speed.SqLength2D() >= 0.01f) {
			skidDir = speed;
			skidDir.y = 0.0f;
			skidDir.Normalize();
		} else {
			skidDir = owner->frontdir;
		}

		skidRotVector = skidDir.cross(UpVector);

		oldPhysState = owner->physicalState;
		owner->physicalState = CSolidObject::Flying;
		owner->moveType->useHeading = false;

		if (speed.dot(groundNormal) > 0.2f) {
			flying = true;
			skidRotSpeed2 = (gs->randFloat() - 0.5f) * 0.04f;
		}
	}
}

void CGroundMoveType::UpdateSkid()
{
	ASSERT_SYNCED_FLOAT3(owner->midPos);

	float3& speed  = owner->speed;
	float3& pos    = owner->pos;
	float3  midPos = owner->midPos;

	const UnitDef* ud = owner->unitDef;

	if (flying) {
		speed.y += mapInfo->map.gravity;
		if (midPos.y < 0.0f)
			speed *= 0.95f;

		const float wh = GetGroundHeight(midPos);

		if (wh > (midPos.y - owner->relMidPos.y)) {
			flying = false;
			skidRotSpeed += (gs->randFloat() - 0.5f) * 1500; //*=0.5f+gs->randFloat();
			midPos.y = wh + owner->relMidPos.y - speed.y * 0.5f;

			const float impactSpeed = midPos.IsInBounds()?
				-speed.dot(ground->GetNormal(midPos.x, midPos.z)):
				-speed.dot(UpVector);

			if (impactSpeed > ud->minCollisionSpeed && ud->minCollisionSpeed >= 0.0f) {
				owner->DoDamage(DamageArray(impactSpeed * owner->mass * 0.2f), 0, ZeroVector);
			}
		}
	} else {
		float speedf = speed.Length();
		const float speedReduction = 0.35f;

		// does not use OnSlope() because then it could stop on an invalid path
		// location, and be teleported back.
		const bool onSlope =
			midPos.IsInBounds() &&
			(ground->GetSlope(midPos.x, midPos.z) > ud->movedata->maxSlope) &&
			(!owner->floatOnWater || !owner->inWater);

		if (speedf < speedReduction && !onSlope) {
			// stop skidding
			currentSpeed = 0.0f;
			speed = ZeroVector;
			skidding = false;
			skidRotSpeed = 0.0f;

			owner->physicalState = oldPhysState;
			owner->moveType->useHeading = true;

			const float rp = floor(skidRotPos2 + skidRotSpeed2 + 0.5f);
			skidRotSpeed2 = (rp - skidRotPos2) * 0.5f;

			ChangeHeading(owner->heading);
		} else {
			if (onSlope) {
				const float3 normal = ground->GetNormal(midPos.x, midPos.z);
				const float3 normalForce = normal * normal.dot(UpVector * mapInfo->map.gravity);
				const float3 newForce = UpVector * mapInfo->map.gravity - normalForce;

				speed += newForce;
				speedf = speed.Length();
				speed *= 1.0f - (0.1f * normal.y);
			} else {
				speed *= (speedf - speedReduction) / speedf;
			}

			const float remTime = speedf / speedReduction - 1.0f;
			const float rp = floor(skidRotPos2 + skidRotSpeed2 * remTime + 0.5f);

			skidRotSpeed2 = (remTime + 1.0f == 0.0f ) ? 0.0f : (rp - skidRotPos2) / (remTime + 1.0f);

			if (floor(skidRotPos2) != floor(skidRotPos2 + skidRotSpeed2)) {
				skidRotPos2 = 0.0f;
				skidRotSpeed2 = 0.0f;
			}
		}

		const float wh = GetGroundHeight(pos);

		if ((wh - pos.y) < (speed.y + mapInfo->map.gravity)) {
			speed.y += mapInfo->map.gravity;
			skidding = true; // flying requires skidding
			flying = true;
		} else if ((wh - pos.y) > speed.y) {
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
	CalcSkidRot();
	owner->MoveMidPos(speed);

	CheckCollisionSkid();
	ASSERT_SYNCED_FLOAT3(owner->midPos);
}

void CGroundMoveType::UpdateControlledDrop()
{
	float3& speed = owner->speed;
	SyncedFloat3& midPos = owner->midPos;

	if (owner->falling) {
		speed.y += (mapInfo->map.gravity * owner->fallSpeed);
		speed.y = std::min(speed.y, 0.0f);

		owner->MoveMidPos(speed);

		if (midPos.y < 0.0f)
			speed *= 0.90;

		const float wh = GetGroundHeight(midPos);

		if (wh > (midPos.y - owner->relMidPos.y)) {
			owner->falling = false;
			midPos.y = wh + owner->relMidPos.y - speed.y * 0.8;
			owner->script->Landed(); //stop parachute animation
		}
	}
}

void CGroundMoveType::CheckCollisionSkid()
{
	SyncedFloat3& midPos = owner->midPos;

	const UnitDef* ownerUD = owner->unitDef;
	const vector<CUnit*>& nearUnits = qf->GetUnitsExact(midPos, owner->radius);
	const vector<CFeature*>& nearFeatures = qf->GetFeaturesExact(midPos, owner->radius);

	vector<CUnit*>::const_iterator ui;
	vector<CFeature*>::const_iterator fi;

	for (ui = nearUnits.begin(); ui != nearUnits.end(); ++ui) {
		CUnit* u = (*ui);
		const UnitDef* unitUD = owner->unitDef;

		const float sqDist = (midPos - u->midPos).SqLength();
		const float totRad = owner->radius + u->radius;

		if ((sqDist >= totRad * totRad) || (sqDist <= 0.01f)) {
			continue;
		}

		// stop units from reaching escape velocity
		const float3 dif = (midPos - u->midPos).SafeNormalize();

		if (u->mobility == NULL) {
			const float impactSpeed = -owner->speed.dot(dif);

			if (impactSpeed > 0.0f) {
				owner->MoveMidPos(dif * impactSpeed);
				owner->speed += ((dif * impactSpeed) * 1.8f);

				// damage the collider
				if (impactSpeed > ownerUD->minCollisionSpeed && ownerUD->minCollisionSpeed >= 0) {
					owner->DoDamage(DamageArray(impactSpeed * owner->mass * 0.2f), 0, ZeroVector);
				}
				// damage the (static) collidee
				if (impactSpeed > unitUD->minCollisionSpeed && unitUD->minCollisionSpeed >= 0) {
					u->DoDamage(DamageArray(impactSpeed * owner->mass * 0.2f), 0, ZeroVector);
				}
			}
		} else {
			// don't conserve momentum
			const float part = (owner->mass / (owner->mass + u->mass));
			const float impactSpeed = (u->speed - owner->speed).dot(dif) * 0.5f;

			if (impactSpeed > 0.0f) {
				owner->MoveMidPos(dif * (impactSpeed * (1 - part) * 2));
				owner->speed += dif * (impactSpeed * (1 - part) * 2);

				u->MoveMidPos(dif * (impactSpeed * part * -2));
				u->speed -= dif * (impactSpeed * part * 2);

				if (CGroundMoveType* mt = dynamic_cast<CGroundMoveType*>(u->moveType)) {
					mt->skidding = true;
				}

				// damage the collider
				if (impactSpeed > ownerUD->minCollisionSpeed && ownerUD->minCollisionSpeed >= 0.0f) {
					owner->DoDamage(
						DamageArray(impactSpeed * owner->mass * 0.2f * (1 - part)),
						0, dif * impactSpeed * (owner->mass * (1 - part)));
				}
				// damage the collidee
				if (impactSpeed > unitUD->minCollisionSpeed && unitUD->minCollisionSpeed >= 0.0f) {
					u->DoDamage(
						DamageArray(impactSpeed * owner->mass * 0.2f * part),
						0, dif * -impactSpeed * (u->mass * part));
				}

				owner->speed *= 0.9f;
				u->speed *= 0.9f;
			}
		}
	}

	for (fi = nearFeatures.begin(); fi != nearFeatures.end(); ++fi) {
		CFeature* f = *fi;

		if (!f->blocking)
			continue;

		const float sqDist = (midPos - f->midPos).SqLength();
		const float totRad = owner->radius + f->radius;

		if ((sqDist >= totRad * totRad) || (sqDist <= 0.01f)) {
			continue;
		}

		const float3 dif = (midPos - f->midPos).SafeNormalize();
		const float impactSpeed = -owner->speed.dot(dif);

		if (impactSpeed > 0.0f) {
			owner->MoveMidPos(dif * impactSpeed);
			owner->speed += ((dif * impactSpeed) * 1.8f);

			if (impactSpeed > ownerUD->minCollisionSpeed && ownerUD->minCollisionSpeed >= 0) {
				owner->DoDamage(DamageArray(impactSpeed * owner->mass * 0.2f), 0, ZeroVector);
			}
			f->DoDamage(DamageArray(impactSpeed * owner->mass * 0.2f), -dif * impactSpeed);
		}
	}
}

void CGroundMoveType::CalcSkidRot()
{
	owner->heading += (short int) skidRotSpeed;
	owner->SetDirectionFromHeading();

	skidRotPos2 += skidRotSpeed2;

	const float cosp = cos(skidRotPos2 * PI * 2.0f);
	const float sinp = sin(skidRotPos2 * PI * 2.0f);

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
}




/*
 * Dynamic obstacle avoidance, helps the unit to
 * follow the path even when it's not perfect.
 */
float3 CGroundMoveType::ObstacleAvoidance(const float3& desiredDir) {
	// multiplier for how strongly an object should be avoided
	static const float AVOIDANCE_STRENGTH = 2.0f;

	// NOTE: based on the requirement that all objects have symetrical footprints.
	// If this is false, then radius has to be calculated in a different way!

	// Obstacle-avoidance-system only needs to be run if the unit wants to move
	if (pathId != 0) {
		float3 avoidanceVec = ZeroVector;
		float3 avoidanceDir = desiredDir;

		// Speed-optimizer. Reduces the times this system is run.
		if (gs->frameNum >= nextObstacleAvoidanceUpdate) {
			nextObstacleAvoidanceUpdate = gs->frameNum + 4;

			// first check if the current waypoint is reachable
			const int wsx = waypoint.x / (MIN_WAYPOINT_DISTANCE);
			const int wsy = waypoint.z / (MIN_WAYPOINT_DISTANCE);
			const int ltx = wsx - moveSquareX + (LINETABLE_SIZE / 2);
			const int lty = wsy - moveSquareY + (LINETABLE_SIZE / 2);

			static const unsigned int blockBits =
				CMoveMath::BLOCK_STRUCTURE |
				CMoveMath::BLOCK_MOBILE_BUSY;
			const MoveData& udMoveData = *owner->unitDef->movedata;

			if (ltx >= 0 && ltx < LINETABLE_SIZE && lty >= 0 && lty < LINETABLE_SIZE) {
				for (std::vector<int2>::iterator li = lineTable[lty][ltx].begin(); li != lineTable[lty][ltx].end(); ++li) {
					const int x = (moveSquareX + li->x) * 2;
					const int y = (moveSquareY + li->y) * 2;

					if ((udMoveData.moveMath->IsBlocked(udMoveData, x, y) & blockBits) ||
					    (udMoveData.moveMath->GetPosSpeedMod(udMoveData, x, y) <= 0.01f)) {

						#if (DEBUG_OUTPUT == 1)
						logOutput.Print("[CGMT::OA] path blocked for unit %i", owner->id);
						#endif
						break;
					}
				}
			}

			// now we do the obstacle avoidance proper
			const float currentDistanceToGoal = owner->pos.distance2D(goalPos);
			const float currentDistanceToGoalSq = currentDistanceToGoal * currentDistanceToGoal;
			const float3 rightOfPath = desiredDir.cross(float3(0.0f, 1.0f, 0.0f));
			const float speedf = owner->speed.Length2D();

			float avoidLeft = 0.0f;
			float avoidRight = 0.0f;
			float3 rightOfAvoid = rightOfPath;


			MoveData* moveData = owner->mobility;
			CMoveMath* moveMath = moveData->moveMath;
			moveData->tempOwner = owner;

			const vector<CSolidObject*> &nearbyObjects = qf->GetSolidsExact(owner->pos, speedf * 35 + 30 + owner->xsize / 2);
			vector<CSolidObject*> objectsOnPath;

			for (vector<CSolidObject*>::const_iterator oi = nearbyObjects.begin(); oi != nearbyObjects.end(); ++oi) {
				CSolidObject* o = *oi;

				if (moveMath->IsNonBlocking(*moveData, o)) {
					// no need to avoid this obstacle
					continue;
				}

				// basic blocking-check (test if the obstacle cannot be overrun)
				if (o != owner && moveMath->CrushResistant(*moveData, o) && desiredDir.dot(o->pos - owner->pos) > 0) {
					float3 objectToUnit = (owner->pos - o->pos - o->speed * 30);
					float distanceToObjectSq = objectToUnit.SqLength();
					float radiusSum = (owner->xsize + o->xsize) * SQUARE_SIZE / 2;
					float distanceLimit = speedf * 35 + 10 + radiusSum;
					float distanceLimitSq = distanceLimit * distanceLimit;

					// if object is close enough
					if (distanceToObjectSq < distanceLimitSq && distanceToObjectSq < currentDistanceToGoalSq
							&& distanceToObjectSq > 0.0001f) {
						// Don't divide by zero. (TODO: figure out why this can
						// actually happen.) Positive value means "to the right".
						float objectDistToAvoidDirCenter = objectToUnit.dot(rightOfAvoid);

						// If object and unit in relative motion are closing in on one another
						// (or not yet fully apart), then the object is on the path of the unit
						// and they are not collided.
						if (objectToUnit.dot(avoidanceDir) < radiusSum &&
							math::fabs(objectDistToAvoidDirCenter) < radiusSum &&
							(o->mobility || Distance2D(owner, o) >= 0)) {

							// Avoid collision by turning the heading to left or right.
							// Using the object thats needs the most adjustment.

							#if (DEBUG_OUTPUT == 1)
							GML_RECMUTEX_LOCK(sel); // ObstacleAvoidance

							if (selectedUnits.selectedUnits.find(owner) != selectedUnits.selectedUnits.end())
								geometricObjects->AddLine(owner->pos + UpVector * 20, o->pos + UpVector * 20, 3, 1, 4);
							#endif

							if (objectDistToAvoidDirCenter > 0.0f) {
								avoidRight +=
									(radiusSum - objectDistToAvoidDirCenter) *
									AVOIDANCE_STRENGTH * fastmath::isqrt2(distanceToObjectSq);
								avoidanceDir += (rightOfAvoid * avoidRight);
								avoidanceDir.Normalize();
								rightOfAvoid = avoidanceDir.cross(float3(0.0f, 1.0f, 0.0f));
							} else {
								avoidLeft +=
									(radiusSum - math::fabs(objectDistToAvoidDirCenter)) *
									AVOIDANCE_STRENGTH * fastmath::isqrt2(distanceToObjectSq);
								avoidanceDir -= (rightOfAvoid * avoidLeft);
								avoidanceDir.Normalize();
								rightOfAvoid = avoidanceDir.cross(float3(0.0f, 1.0f, 0.0f));
							}
							objectsOnPath.push_back(o);
						}
					}

				}
			}

			moveData->tempOwner = NULL;


			// Sum up avoidance.
			avoidanceVec = (desiredDir.cross(UpVector) * (avoidRight - avoidLeft));

			#if (DEBUG_OUTPUT == 1)
			GML_RECMUTEX_LOCK(sel); //ObstacleAvoidance

			if (selectedUnits.selectedUnits.find(owner) != selectedUnits.selectedUnits.end()) {
				int a = geometricObjects->AddLine(owner->pos + UpVector * 20, owner->pos + UpVector * 20 + avoidanceVec * 40, 7, 1, 4);
				geometricObjects->SetColor(a, 1, 0.3f, 0.3f, 0.6f);

				a = geometricObjects->AddLine(owner->pos + UpVector * 20, owner->pos + UpVector * 20 + desiredDir * 40, 7, 1, 4);
				geometricObjects->SetColor(a, 0.3f, 0.3f, 1, 0.6f);
			}
			#endif
		}

		// Return the resulting recommended velocity.
		avoidanceDir = (desiredDir + avoidanceVec).Normalize();

#ifdef TRACE_SYNC
		tracefile << "[" << __FUNCTION__ << "] ";
		tracefile << "avoidanceVec = <" << avoidanceVec.x << " " << avoidanceVec.y << " " << avoidanceVec.z << ">\n";
#endif

		return avoidanceDir;
	} else {
		return ZeroVector;
	}
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
	pathManager->DeletePath(pathId);
	pathId = pathManager->RequestPath(owner->mobility, owner->pos, goalPos, goalRadius, owner);

	// if new path received, can't be at waypoint
	if (pathId != 0) {
		atGoal = false;
		haveFinalWaypoint = false;

		waypoint = owner->pos;
		nextWaypoint = pathManager->NextWaypoint(pathId, waypoint, 1.25f * SQUARE_SIZE, 0, owner->id);
	} else {
		Fail();
	}

	// limit frequency of (case B) path-requests from SlowUpdate's
	pathRequestDelay = gs->frameNum + (UNIT_SLOWUPDATE_RATE << 1);
}


/*
Sets waypoint to next in path.
*/
void CGroundMoveType::GetNextWaypoint()
{

	if (pathId == 0) {
		return;
	}

	{
		#if (DEBUG_OUTPUT == 1)
		// plot the vector to the waypoint
		const int figGroupID =
			geometricObjects->AddLine(owner->pos + UpVector * 20, waypoint + UpVector * 20, 8.0f, 1, 4);
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

	if (nextWaypoint.x != -1.0f && (nextWaypoint - waypoint).SqLength2D() > 0.1f) {
		waypoint = nextWaypoint;
		nextWaypoint = pathManager->NextWaypoint(pathId, waypoint, 1.25f * SQUARE_SIZE, 0, owner->id);

		if (waypoint.SqDistance2D(goalPos) < Square(MIN_WAYPOINT_DISTANCE)) {
			waypoint = goalPos;
		}
	} else {
		// trigger Arrived on the next Update
		haveFinalWaypoint = true;

		waypoint = goalPos;
		nextWaypoint = goalPos;
	}
}




/*
The distance the unit will move at max breaking before stopping,
starting from given speed.
*/
float CGroundMoveType::BreakingDistance(float speed) const
{
	return math::fabs(speed * speed / std::max(decRate, 0.01f));
}

/*
Gives the position this unit will end up at with full breaking
from current velocity.
*/
float3 CGroundMoveType::Here()
{
	float3 motionDir = owner->speed;
	if (motionDir.SqLength2D() < float3::NORMALIZE_EPS) {
		return owner->midPos;
	} else {
		motionDir.Normalize();
		return owner->midPos + (motionDir * BreakingDistance(owner->speed.Length2D()));
	}
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
			waypoint = Here();
		}

		// Stop animation.
		owner->script->StopMoving();

		#if (DEBUG_OUTPUT == 1)
		logOutput.Print("[CGMT::StopEngine] engine stopped for unit %i", owner->id);
		#endif
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
			const int soundIdx = owner->unitDef->sounds.arrived.getRandomIdx();
			if (soundIdx >= 0) {
				Channels::UnitReply.PlaySample(
					owner->unitDef->sounds.arrived.getID(soundIdx), owner,
					owner->unitDef->sounds.arrived.getVolume(soundIdx));
			}
		}
		#endif

		// and the action is done
		progressState = Done;
		owner->commandAI->SlowUpdate();

		#if (DEBUG_OUTPUT == 1)
		logOutput.Print("[CGMT::Arrived] unit %i arrived", owner->id);
		#endif
	}
}

/*
Makes the unit fail this action.
No more trials will be done before a new goal is given.
*/
void CGroundMoveType::Fail()
{
	#if (DEBUG_OUTPUT == 1)
	logOutput.Print("[CGMT::Fail] unit %i failed", owner->id);
	#endif

	StopEngine();

	// failure of finding a path means that
	// this action has failed to reach its goal.
	progressState = Failed;

	eventHandler.UnitMoveFailed(owner);
	eoh->UnitMoveFailed(*owner);
}



void CGroundMoveType::HandleObjectCollisions()
{
	//FIXME move this to a global space to optimize the check (atm unit collision checks are done twice for the collider & collidee!)

	CUnit* collider = owner;
	collider->mobility->tempOwner = collider;

	{
		#define FOOTPRINT_RADIUS(xs, zs) (math::sqrt((xs * xs + zs * zs)) * 0.5f * SQUARE_SIZE)

		const UnitDef*   colliderUD = collider->unitDef;
		const MoveData*  colliderMD = collider->mobility;
		const CMoveMath* colliderMM = colliderMD->moveMath;

		const float3& colliderCurPos = collider->pos;
		const float3& colliderOldPos = collider->moveType->oldPos;
		const float colliderSpeed = collider->speed.Length();
		const float colliderRadius = (colliderMD != NULL)?
			FOOTPRINT_RADIUS(colliderMD->xsize, colliderMD->zsize):
			FOOTPRINT_RADIUS(colliderUD->xsize, colliderUD->zsize);

		const std::vector<CUnit*>& nearUnits = qf->GetUnitsExact(colliderCurPos, colliderRadius * 2.0f);
		const std::vector<CFeature*>& nearFeatures = qf->GetFeaturesExact(colliderCurPos, colliderRadius * 2.0f);

		std::vector<CUnit*>::const_iterator uit;
		std::vector<CFeature*>::const_iterator fit;

		for (uit = nearUnits.begin(); uit != nearUnits.end(); ++uit) {
			CUnit* collidee = const_cast<CUnit*>(*uit);

			if (collidee == collider) {
				continue;
			}

			const UnitDef*   collideeUD = collidee->unitDef;
			const MoveData*  collideeMD = collidee->mobility;
			const CMoveMath* collideeMM = (collideeMD != NULL)? collideeMD->moveMath: NULL;

			const float3& collideeCurPos = collidee->pos;
			const float3& collideeOldPos = collidee->moveType->oldPos;
 
			const bool collideeMobile = (collideeMD != NULL);
			const float collideeSpeed = collidee->speed.Length();
			const float collideeRadius = collideeMobile?
				FOOTPRINT_RADIUS(collideeMD->xsize, collideeMD->zsize):
				FOOTPRINT_RADIUS(collideeUD->xsize, collideeUD->zsize);

			bool colliderMobile = (collider->mobility != NULL);
			bool pushCollider = colliderMobile;
			bool pushCollidee = (collideeMobile || collideeUD->canfly);

			const float3 separationVector = colliderCurPos - collideeCurPos;
			const float separationMinDist = (colliderRadius + collideeRadius) * (colliderRadius + collideeRadius);

			if ((separationVector.SqLength() - separationMinDist) > 0.01f) { continue; }
			if (collidee->usingScriptMoveType) { pushCollidee = false; }
			if (collideeUD->pushResistant) { pushCollidee = false; }

			if (!teamHandler->Ally(collider->allyteam, collidee->allyteam)) { pushCollider = false; pushCollidee = false; }
			if (!teamHandler->Ally(collidee->allyteam, collider->allyteam)) { pushCollider = false; pushCollidee = false; }

			// don't push either party if the collidee does not block the collider
			if (colliderMM->IsNonBlocking(*colliderMD, collidee)) { continue; }
			if (!collideeMobile && (colliderMM->IsBlocked(*colliderMD, colliderCurPos) & CMoveMath::BLOCK_STRUCTURE) == 0) { continue; }

			eventHandler.UnitUnitCollision(collider, collidee);

			const float  sepDistance    = (separationVector.Length() + 0.01f);
			const float  penDistance    = (colliderRadius + collideeRadius) - sepDistance;
			const float3 sepDirection   = (separationVector / sepDistance);
			const float3 colResponseVec = sepDirection * float3(1.0f, 0.0f, 1.0f) * (penDistance * 0.5f);

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
				const float3 colliderNextPos = colliderCurPos + collider->frontdir * currentSpeed;
				const unsigned int colliderNextPosBits = colliderMM->IsBlocked(*colliderMD, colliderNextPos);

				if ((colliderNextPosBits & CMoveMath::BLOCK_STRUCTURE) != 0 && collider->frontdir.dot(sepDirection) < -0.25f) {
					const int2   sgnVec = int2((colResponseVec.x >= 0.0f)? 1: -1, (colResponseVec.z >= 0.0f)? 1: -1);
					const float2 absVec = float2(math::fabs(colResponseVec.x * collideeMassScale), math::fabs(colResponseVec.z * collideeMassScale));
					const float3 resVec = float3(std::max(absVec.x, 0.25f) * sgnVec.x, 0.0f, std::max(absVec.y, 0.25f) * sgnVec.y);

					collider->pos = colliderOldPos + resVec;

					currentSpeed = 0.0f;
					// <requestedSpeed> is only reset every SlowUpdate, do not touch it
					// requestedSpeed = 0.0f;
				}

				if (colliderMassScale > collideeMassScale) {
					std::swap(colliderMassScale, collideeMassScale);
				}
			}

			const float3 colliderNewPos = colliderCurPos + (colResponseVec * colliderMassScale);
			const float3 collideeNewPos = collideeCurPos - (colResponseVec * collideeMassScale);

			// try to prevent both parties from being pushed onto non-traversable squares
			if (                  (colliderMM->IsBlocked(*colliderMD, colliderNewPos) & CMoveMath::BLOCK_STRUCTURE) != 0) { colliderMassScale = 0.0f; }
			if (collideeMobile && (collideeMM->IsBlocked(*collideeMD, collideeNewPos) & CMoveMath::BLOCK_STRUCTURE) != 0) { collideeMassScale = 0.0f; }
			if (                  colliderMM->GetPosSpeedMod(*colliderMD, colliderNewPos) <= 0.01f) { colliderMassScale = 0.0f; }
			if (collideeMobile && collideeMM->GetPosSpeedMod(*collideeMD, collideeNewPos) <= 0.01f) { collideeMassScale = 0.0f; }

			if (pushCollider) { collider->pos += (colResponseVec * colliderMassScale); } else if (colliderMobile) { collider->pos = colliderOldPos; }
			if (pushCollidee) { collidee->pos -= (colResponseVec * collideeMassScale); } else if (collideeMobile) { collidee->pos = collideeOldPos; }

			collider->UpdateMidPos();
			collidee->UpdateMidPos();

			if (!((gs->frameNum + collider->id) & 31) && !collider->commandAI->unimportantMove) {
				// if we do not have an internal move order, tell units around us to bugger off
				helper->BuggerOff(colliderCurPos + collider->frontdir * colliderRadius, colliderRadius, true, false, collider->team, collider);
			}
		}

		for (fit = nearFeatures.begin(); fit != nearFeatures.end(); ++fit) {
			CFeature* collidee = const_cast<CFeature*>(*fit);

			const FeatureDef* collideeFD = collidee->def;
			const float3& collideeCurPos = collidee->pos;
			const float collideeRadius = FOOTPRINT_RADIUS(collideeFD->xsize, collideeFD->zsize);

			const float3 separationVector = colliderCurPos - collideeCurPos;
			const float separationMinDist = (colliderRadius + collideeRadius) * (colliderRadius + collideeRadius);

			if ((separationVector.SqLength() - separationMinDist) > 0.01f) { continue; }
			if (colliderMM->IsNonBlocking(*colliderMD, collidee)) { continue; }

			if (!colliderMM->CrushResistant(*colliderMD, collidee)) { collidee->Kill(collider->frontdir * currentSpeed * 200.0f); }
			if ((colliderMM->IsBlocked(*colliderMD, colliderCurPos) & CMoveMath::BLOCK_STRUCTURE) == 0) { continue; }

			eventHandler.UnitFeatureCollision(collider, collidee);

			const float  sepDistance    = (separationVector.Length() + 0.01f);
			const float  penDistance    = (colliderRadius + collideeRadius) - sepDistance;
			const float3 sepDirection   = (separationVector / sepDistance);
			const float3 colResponseVec = sepDirection * float3(1.0f, 0.0f, 1.0f) * (penDistance * 0.5f);

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
			      float colliderMassScale = std::max(0.01f, std::min(0.99f, 1.0f - (s1 / collisionMassSum)));
			      float collideeMassScale = std::max(0.01f, std::min(0.99f, 1.0f - (s2 / collisionMassSum)));

			if (collidee->reachedFinalPos) {
				const float3 colliderNextPos = colliderCurPos + collider->frontdir * currentSpeed;
				const unsigned int colliderNextPosBits = colliderMM->IsBlocked(*colliderMD, colliderNextPos);

				if ((colliderNextPosBits & CMoveMath::BLOCK_STRUCTURE) != 0 && collider->frontdir.dot(sepDirection) < -0.25f) {
					// make sure the scaled response is never of epsilon-length (units would get stuck otherwise)
					const int2   sgnVec = int2((colResponseVec.x >= 0.0f)? 1: -1, (colResponseVec.z >= 0.0f)? 1: -1);
					const float2 absVec = float2(math::fabs(colResponseVec.x * collideeMassScale), math::fabs(colResponseVec.z * collideeMassScale));
					const float3 resVec = float3(std::max(absVec.x, 0.25f) * sgnVec.x, 0.0f, std::max(absVec.y, 0.25f) * sgnVec.y);

					collider->pos = colliderOldPos + resVec;

					currentSpeed = 0.0f;
					// <requestedSpeed> is only reset every SlowUpdate, do not touch it
					// requestedSpeed = 0.0f;
				}

				if (colliderMassScale > collideeMassScale) {
					std::swap(colliderMassScale, collideeMassScale);
				}
			}

			collider->pos += (colResponseVec * colliderMassScale);
		//	collidee->pos -= (colResponseVec * collideeMassScale);

			collider->UpdateMidPos();
		}

		#undef FOOTPRINT_RADIUS
	}

	collider->mobility->tempOwner = NULL;
	collider->Block();
}



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
					for (int a = 1; a < floor(to.z); ++a)
						lineTable[yt][xt].push_back(int2(0, a));
				} else {
					for (int a = -1; a > floor(to.z); --a)
						lineTable[yt][xt].push_back(int2(0, a));
				}
			} else if (floor(start.z) == floor(to.z)) {
				if (dx > 0.0f) {
					for (int a = 1; a < floor(to.x); ++a)
						lineTable[yt][xt].push_back(int2(a, 0));
				} else {
					for (int a = -1; a > floor(to.x); --a)
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
						math::fabs(xp - start.x) < math::fabs(to.x - start.x) &&
						math::fabs(zp - start.z) < math::fabs(to.z - start.z);

					lineTable[yt][xt].push_back( int2(int(floor(xp)), int(floor(zp))) );
				}

				lineTable[yt][xt].pop_back();
				lineTable[yt][xt].pop_back();
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
		const float cmod = movemath->GetPosSpeedMod(md, moveSquareX * 2, moveSquareY * 2);

		if (math::fabs(owner->frontdir.x) < math::fabs(owner->frontdir.z)) {
			if (newMoveSquareX > moveSquareX) {
				const float nmod = movemath->GetPosSpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.x = moveSquareX * MIN_WAYPOINT_DISTANCE + (MIN_WAYPOINT_DISTANCE - 0.01f);
					newMoveSquareX = moveSquareX;
				}
			} else if (newMoveSquareX < moveSquareX) {
				const float nmod = movemath->GetPosSpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.x = moveSquareX * MIN_WAYPOINT_DISTANCE + 0.01f;
					newMoveSquareX = moveSquareX;
				}
			}
			if (newMoveSquareY > moveSquareY) {
				const float nmod = movemath->GetPosSpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.z = moveSquareY * MIN_WAYPOINT_DISTANCE + (MIN_WAYPOINT_DISTANCE - 0.01f);
					newMoveSquareY = moveSquareY;
				}
			} else if (newMoveSquareY < moveSquareY) {
				const float nmod = movemath->GetPosSpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.z = moveSquareY * MIN_WAYPOINT_DISTANCE + 0.01f;
					newMoveSquareY = moveSquareY;
				}
			}
		} else {
			if (newMoveSquareY > moveSquareY) {
				const float nmod = movemath->GetPosSpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.z = moveSquareY * MIN_WAYPOINT_DISTANCE + (MIN_WAYPOINT_DISTANCE - 0.01f);
					newMoveSquareY = moveSquareY;
				}
			} else if (newMoveSquareY < moveSquareY) {
				const float nmod = movemath->GetPosSpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.z = moveSquareY * MIN_WAYPOINT_DISTANCE + 0.01f;
					newMoveSquareY = moveSquareY;
				}
			}

			if (newMoveSquareX > moveSquareX) {
				const float nmod = movemath->GetPosSpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.x = moveSquareX * MIN_WAYPOINT_DISTANCE + (MIN_WAYPOINT_DISTANCE - 0.01f);
					newMoveSquareX = moveSquareX;
				}
			} else if (newMoveSquareX < moveSquareX) {
				const float nmod = movemath->GetPosSpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
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
			owner->pos = newpos;
		}

		if (newMoveSquareX != moveSquareX || newMoveSquareY != moveSquareY) {
			moveSquareX = newMoveSquareX;
			moveSquareY = newMoveSquareY;

			// if we have moved, check if we can get to the next waypoint
			int nwsx = (int) nextWaypoint.x / (MIN_WAYPOINT_DISTANCE) - moveSquareX;
			int nwsy = (int) nextWaypoint.z / (MIN_WAYPOINT_DISTANCE) - moveSquareY;
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
						const int x = (moveSquareX + li->x) * 2;
						const int y = (moveSquareY + li->y) * 2;

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

				GetNextWaypoint();

				nwsx = (int) nextWaypoint.x / (MIN_WAYPOINT_DISTANCE) - moveSquareX;
				nwsy = (int) nextWaypoint.z / (MIN_WAYPOINT_DISTANCE) - moveSquareY;
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

	if (useMainHeading && !owner->weapons.empty()) {
		if (!owner->weapons[0]->weaponDef->waterweapon && mainHeadingPos.y <= 1) {
			mainHeadingPos.y = 1;
		}

		float3 dir1 = owner->weapons.front()->mainDir;
		dir1.y = 0;
		dir1.Normalize();
		float3 dir2 = mainHeadingPos - owner->pos;
		dir2.y = 0;
		dir2.Normalize();

		if (dir2 != ZeroVector) {
			short heading =
				GetHeadingFromVector(dir2.x, dir2.z) -
				GetHeadingFromVector(dir1.x, dir1.z);
			if (owner->heading != heading
					&& !(owner->weapons.front()->TryTarget(
					mainHeadingPos, true, 0))) {
				progressState = Active;
			}
		}
	}
}

void CGroundMoveType::KeepPointingTo(CUnit* unit, float distance, bool aggressive){
	//! wrapper
	KeepPointingTo(unit->pos, distance, aggressive);
}

/**
* @brief Orients owner so that weapon[0]'s arc includes mainHeadingPos
*/
void CGroundMoveType::SetMainHeading() {
	if (useMainHeading && !owner->weapons.empty()) {
		float3 dir1 = owner->weapons.front()->mainDir;
		dir1.y = 0;
		dir1.Normalize();
		float3 dir2 = mainHeadingPos - owner->pos;
		dir2.y = 0;
		dir2.Normalize();

		ASSERT_SYNCED_FLOAT3(dir1);
		ASSERT_SYNCED_FLOAT3(dir2);

		if (dir2 != ZeroVector) {
			short heading =
				GetHeadingFromVector(dir2.x, dir2.z) -
				GetHeadingFromVector(dir1.x, dir1.z);

			ASSERT_SYNCED_PRIMITIVE(heading);

			if (progressState == Active && owner->heading == heading) {
				// stop turning
				owner->script->StopMoving();
				progressState = Done;
			} else if (progressState == Active) {
				ChangeHeading(heading);
#ifdef TRACE_SYNC
				tracefile << "[" << __FUNCTION__ << "][1] test heading: " << heading << ", real heading: " << owner->heading << "\n";
#endif
			} else if (progressState != Active
			  && owner->heading != heading
			  && !owner->weapons.front()->TryTarget(mainHeadingPos, true, 0)) {
				// start moving
				progressState = Active;
				owner->script->StartMoving();
				ChangeHeading(heading);
#ifdef TRACE_SYNC
				tracefile << "[" << __FUNCTION__ << "][2] test heading: " << heading << ", real heading: " << owner->heading << "\n";
#endif
			}
		}
	}
}

void CGroundMoveType::SetMaxSpeed(float speed)
{
	maxSpeed        = std::min(speed, owner->maxSpeed);
	maxReverseSpeed = std::min(speed, owner->maxReverseSpeed);

	requestedSpeed = speed;
}

bool CGroundMoveType::OnSlope() {
	const UnitDef* ud = owner->unitDef;
	const float3& mp = owner->midPos;

	return
		(ud->slideTolerance >= 1.0f) &&
		(mp.IsInBounds()) &&
		(ground->GetSlope(mp.x, mp.z) > (ud->movedata->maxSlope * ud->slideTolerance));
}

void CGroundMoveType::StartSkidding() {
	skidding = true;
}

void CGroundMoveType::StartFlying() {
	skidding = true; // flying requires skidding
	flying = true;
}



float CGroundMoveType::GetGroundHeight(const float3& p) const
{
	float h = 0.0f;

	if (owner->floatOnWater) {
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

		if (owner->floatOnWater && owner->inWater && groundHeight <= 0.0f) {
			groundHeight = -owner->unitDef->waterline;
		}

		owner->pos.y = groundHeight;
		owner->midPos.y = owner->pos.y + owner->relMidPos.y;

		/*
		const UnitDef* ud = owner->unitDef;
		const MoveData* md = ud->movedata;
		const CMoveMath* mm = md->moveMath;

		owner->pos.y = mm->yLevel(owner->pos.x, owner->pos.z);

		if (owner->floatOnWater && owner->inWater) {
			owner->pos.y -= owner->unitDef->waterline;
		}

		owner->midPos.y = owner->pos.y + owner->relMidPos.y;
		*/
	}
}

bool CGroundMoveType::UpdateDirectControl()
{
	const CPlayer* myPlayer = gu->GetMyPlayer();
	const FPSUnitController& selfCon = myPlayer->fpsController;
	const FPSUnitController& unitCon = owner->fpsControlPlayer->fpsController;
	const bool wantReverse = (unitCon.back && !unitCon.forward);

	waypoint = owner->pos;
	waypoint += wantReverse ? -owner->frontdir * 100 : owner->frontdir * 100;
	waypoint.CheckInBounds();

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

	deltaHeading = 0;

	if (unitCon.left ) { deltaHeading += (short) turnRate; }
	if (unitCon.right) { deltaHeading -= (short) turnRate; }

	if (selfCon.GetControllee() == owner) {
		camera->rot.y += (deltaHeading * TAANG2RAD);
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

		// note: currentSpeed can be out of sync with
		// owner->speed.Length(), eg. when in front of
		// an obstacle ==> bad for MANY reasons, such
		// as too-quick consumption of waypoints when
		// a new path is requested
		currentSpeed += deltaSpeed;
		owner->pos += (flatFrontDir * currentSpeed * (reversing? -1.0f: 1.0f));
	}

	if (!wantReverse && currentSpeed == 0.0f) {
		reversing = false;
	}
}

bool CGroundMoveType::WantReverse(const float3& waypointDir2D) const
{
	if (!canReverse) {
		return false;
	}

	const float3 waypointDif  = goalPos - owner->pos;                                           // use final WP for ETA
	const float waypointDist  = waypointDif.Length();                                           // in elmos
	const float waypointFETA  = (waypointDist / maxSpeed);                                      // in frames (simplistic)
	const float waypointRETA  = (waypointDist / maxReverseSpeed);                               // in frames (simplistic)
	const float waypointDirDP = waypointDir2D.dot(owner->frontdir);
	const float waypointAngle = std::max(-1.0f, std::min(1.0f, waypointDirDP));                 // prevent NaN's
	const float turnAngleDeg  = streflop::acosf(waypointAngle) * (180.0f / PI);                 // in degrees
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
