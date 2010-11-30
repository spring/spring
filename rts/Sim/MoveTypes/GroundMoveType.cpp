/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "GroundMoveType.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/Player.h"
#include "Game/SelectedUnits.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "MoveMath/MoveMath.h"
#include "Rendering/GroundDecalHandler.h"
#include "Rendering/Models/3DModel.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Path/PathManager.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/CommandAI/MobileCAI.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/Sync/SyncTracer.h"
#include "System/GlobalUnsynced.h"
#include "System/EventHandler.h"
#include "System/LogOutput.h"
#include "Sound/IEffectChannel.h"
#include "System/FastMath.h"
#include "System/myMath.h"
#include "System/Vec2.h"

#define MIN_WAYPOINT_DISTANCE (SQUARE_SIZE << 1)
#define DEBUG_OUTPUT 0

CR_BIND_DERIVED(CGroundMoveType, AMoveType, (NULL));

CR_REG_METADATA(CGroundMoveType, (
		CR_MEMBER(turnRate),
		CR_MEMBER(accRate),
		CR_MEMBER(decRate),

		CR_MEMBER(maxReverseSpeed),
		CR_MEMBER(wantedSpeed),
		CR_MEMBER(currentSpeed),
		CR_MEMBER(deltaSpeed),
		CR_MEMBER(deltaHeading),
		CR_MEMBER(flatFrontDir),

		CR_MEMBER(pathId),
		CR_MEMBER(goalRadius),

		CR_MEMBER(waypoint),
		CR_MEMBER(nextWaypoint),
		CR_MEMBER(atGoal),
		CR_MEMBER(haveFinalWaypoint),
		CR_MEMBER(terrainSpeed),

		CR_MEMBER(requestedSpeed),
		CR_MEMBER(requestedTurnRate),

		CR_MEMBER(currentDistanceToWaypoint),

		CR_MEMBER(restartDelay),
		CR_MEMBER(lastGetPathPos),

		CR_MEMBER(etaFailures),
		CR_MEMBER(nonMovingFailures),

		CR_MEMBER(floatOnWater),

		CR_MEMBER(moveSquareX),
		CR_MEMBER(moveSquareY),

		CR_MEMBER(nextDeltaSpeedUpdate),
		CR_MEMBER(nextObstacleAvoidanceUpdate),

		CR_MEMBER(lastTrackUpdate),

		CR_MEMBER(skidding),
		CR_MEMBER(flying),
		CR_MEMBER(reversing),
		CR_MEMBER(idling),
		CR_MEMBER(canReverse),

		CR_MEMBER(skidRotSpeed),
		CR_MEMBER(dropSpeed),
		CR_MEMBER(dropHeight),

		CR_MEMBER(skidRotVector),
		CR_MEMBER(skidRotSpeed2),
		CR_MEMBER(skidRotPos2),
		CR_ENUM_MEMBER(oldPhysState),

		CR_MEMBER(mainHeadingPos),
		CR_MEMBER(useMainHeading),
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
	deltaSpeed(0.0f),
	deltaHeading(0),

	flatFrontDir(0.0f, 0.0, 1.0f),
	pathId(0),
	goalRadius(0),

	waypoint(ZeroVector),
	nextWaypoint(ZeroVector),
	atGoal(false),
	haveFinalWaypoint(false),

	terrainSpeed(1.0f),
	requestedSpeed(0.0f),
	requestedTurnRate(0),
	currentDistanceToWaypoint(0),
	restartDelay(0),
	lastGetPathPos(ZeroVector),
	etaFailures(0),
	nonMovingFailures(0),

	floatOnWater(false),

	nextDeltaSpeedUpdate(0),
	nextObstacleAvoidanceUpdate(0),
	lastTrackUpdate(0),

	skidding(false),
	flying(false),
	reversing(false),
	idling(false),
	canReverse(owner->unitDef->rSpeed > 0.0f),
	skidRotSpeed(0.0f),

	dropSpeed(0.0f),
	dropHeight(0.0f),
	skidRotVector(UpVector),
	skidRotSpeed2(0.0f),
	skidRotPos2(0.0f),
	oldPhysState(CSolidObject::OnGround),
	mainHeadingPos(0.0f, 0.0f, 0.0f),
	useMainHeading(false)
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
	if (pathId) {
		pathManager->DeletePath(pathId);
	}
	if (owner->myTrack) {
		groundDecals->RemoveUnit(owner);
	}
}

void CGroundMoveType::PostLoad()
{
	// FIXME: HACK: re-initialize path after load
	if (pathId) {
		RequestPath(owner->pos, goalPos, goalRadius);
	}
}

void CGroundMoveType::Update()
{
	ASSERT_SYNCED_FLOAT3(owner->pos);

	// Update mobility.
	owner->mobility->maxSpeed = reversing? maxReverseSpeed: maxSpeed;

	if (owner->transporter) {
		return;
	}

	if (OnSlope() && (!floatOnWater || ground->GetHeight(owner->midPos.x, owner->midPos.z) > 0))
		skidding = true;

	if (skidding) {
		UpdateSkid();
		return;
	}

	ASSERT_SYNCED_FLOAT3(owner->pos);

	//set drop height when we start to drop
	if (owner->falling) {
		UpdateControlledDrop();
		return;
	}

	ASSERT_SYNCED_FLOAT3(owner->pos);

	const UnitDef* ud = owner->unitDef;

	if (owner->stunned || owner->beingBuilt) {
		owner->script->StopMoving();
		owner->speed = ZeroVector;
	} else {
		bool wantReverse = false;

		if (owner->directControl) {
			wantReverse = UpdateDirectControl();
			ChangeHeading(owner->heading + deltaHeading);
		} else {
			if (pathId == 0) {
				wantedSpeed = 0.0f;

				SetDeltaSpeed(false);
				SetMainHeading();
			} else {
				// TODO: Stop the unit from moving as a reaction on collision/explosion physics.
				ASSERT_SYNCED_FLOAT3(waypoint);
				ASSERT_SYNCED_FLOAT3(owner->pos);

				currentDistanceToWaypoint = owner->pos.distance2D(waypoint);
				atGoal = ((owner->pos - goalPos).SqLength2D() < Square(MIN_WAYPOINT_DISTANCE));

				if (!atGoal) {
					if (!idling) {
						etaFailures = std::max(    0, int(etaFailures - 1));
					} else {
						// note: the unit could just be turning in-place
						// over several frames (eg. to maneuver around an
						// obstacle), which unlike actual immobilization
						// does not count as an ETA failure
						etaFailures = std::min(SHORTINT_MAXVALUE, int(etaFailures + 1));

						#if (DEBUG_OUTPUT == 1)
						logOutput.Print(
							"[CGMT::U] ETA failure for unit %i with pathID %i (at goal: %i)",
							owner->id, pathId, atGoal
						);
						#endif
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


				if (nextDeltaSpeedUpdate <= gs->frameNum) {
					wantedSpeed = requestedSpeed;

					const bool moreCommands = owner->commandAI->HasMoreMoveCommands();
					const bool startBreaking = (currentDistanceToWaypoint < BreakingDistance(currentSpeed) + SQUARE_SIZE);

					// If arriving at waypoint, then need to slow down, or may pass it.
					if (!moreCommands && startBreaking) {
						wantedSpeed = std::min(wantedSpeed, fastmath::apxsqrt(currentDistanceToWaypoint * -owner->mobility->maxBreaking));
					}
					if (waypointDir.SqLength() > 0.1f) {
						const float reqTurnAngle = streflop::acosf(waypointDir.dot(flatFrontDir)) * (180.0f / PI);
						const float maxTurnAngle = (turnRate / SPRING_CIRCLE_DIVS) * 360.0f;
						const float reducedSpeed = (maxSpeed * 2.0f) * (maxTurnAngle / reqTurnAngle);

						if (!wantReverse && reqTurnAngle > maxTurnAngle) {
							wantedSpeed = std::min(wantedSpeed, std::max(ud->turnInPlaceSpeedLimit, reducedSpeed));
						}
					}

					if (ud->turnInPlace) {
						if (wantReverse) {
							wantedSpeed *= std::max(0.0f, std::min(1.0f, avoidVec.dot(-owner->frontdir) + 0.1f));
						} else {
							wantedSpeed *= std::max(0.0f, std::min(1.0f, avoidVec.dot( owner->frontdir) + 0.1f));
						}
					}

					SetDeltaSpeed(wantReverse);
				}

				pathManager->UpdatePath(owner, pathId);
			}
		}

		UpdateOwnerPos(wantReverse);
	}

	if (owner->pos != oldPos) {
		// these checks must be executed even when we are stunned
		TestNewTerrainSquare();
		CheckCollision();
		AdjustPosToWaterLine();

		owner->speed = owner->pos - oldPos;
		owner->UpdateMidPos();

		// CheckCollision() may have negated UpdateOwnerPos()
		// (so that owner->pos is again equal to oldPos)
		idling = (owner->speed.SqLength() < 1.0f);
		oldPos = owner->pos;

		if (ud->leaveTracks && (lastTrackUpdate < gs->frameNum - 7) &&
			((owner->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectatingFullView)) {
			lastTrackUpdate = gs->frameNum;
			groundDecals->UnitMoved(owner);
		}
	} else {
		owner->speed = ZeroVector;
		idling = true;
	}
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
			if (etaFailures > (SHORTINT_MAXVALUE / turnRate)) {
				// we have a path but are not moving (based on the ETA failure count)
				#if (DEBUG_OUTPUT == 1)
				logOutput.Print("[CGMT::SU] unit %i has path %i but %i ETA failures", owner->id, pathId, etaFailures);
				#endif

				StopEngine();
				StartEngine();
			}
		} else {
			if (gs->frameNum > restartDelay) {
				// we want to be moving but don't have a path
				#if (DEBUG_OUTPUT == 1)
				logOutput.Print("[CGMT::SU] unit %i has no path", owner->id);
				#endif
				StartEngine();
			}
		}
	}


	if (!flying) {
		// just kindly move it into the map again instead of deleting
		owner->pos.CheckInBounds();
	}

	if (!(owner->falling || flying)) {
		float wh = 0.0f;

		// need the following if the ground changes
		// height while the unit is standing still
		if (floatOnWater) {
			wh = ground->GetHeight(owner->pos.x, owner->pos.z);
			if (wh == 0.0f) {
				wh = -owner->unitDef->waterline;
			}
		} else {
			wh = ground->GetHeight2(owner->pos.x, owner->pos.z);
		}
		owner->pos.y = wh;
	}

	if (owner->pos != oldSlowUpdatePos) {
		oldSlowUpdatePos = owner->pos;

		const int newmapSquare = ground->GetSquare(owner->pos);
		if (newmapSquare != owner->mapSquare) {
			owner->mapSquare = newmapSquare;

			loshandler->MoveUnit(owner, false);
			radarhandler->MoveUnit(owner);

		}
		qf->MovedUnit(owner);

		// submarines aren't always deep enough to be fully
		// submerged (yet should have the isUnderWater flag
		// set at all times)
		const float s = (owner->mobility->subMarine)? 0.5f: 1.0f;
		owner->isUnderWater = ((owner->pos.y + owner->height * s) < 0.0f);
	}
}


/*
Sets unit to start moving against given position with max speed.
*/
void CGroundMoveType::StartMoving(float3 pos, float goalRadius) {
	StartMoving(pos, goalRadius, (reversing? maxReverseSpeed * 2: maxSpeed * 2));
}


/*
Sets owner unit to start moving against given position with requested speed.
*/
void CGroundMoveType::StartMoving(float3 moveGoalPos, float goalRadius, float speed)
{
#ifdef TRACE_SYNC
	tracefile << "Start moving called: ";
	tracefile << owner->pos.x << " " << owner->pos.y << " " << owner->pos.z << " " << owner->id << "\n";
#endif

	if (progressState == Active) {
		StopEngine();
	}

	// set the new goal
	goalPos = moveGoalPos;
	goalRadius = goalRadius;
	requestedSpeed = speed;
	requestedTurnRate = owner->mobility->maxTurnRate;
	atGoal = false;
	useMainHeading = false;
	progressState = Active;

	#if (DEBUG_OUTPUT == 1)
	logOutput.Print("[CGMT::StartMove] starting engine for unit %i", owner->id);
	#endif

	StartEngine();

	if (owner->team == gu->myTeam) {
		// Play "activate" sound.
		const int soundIdx = owner->unitDef->sounds.activate.getRandomIdx();
		if (soundIdx >= 0) {
			Channels::UnitReply.PlaySample(
				owner->unitDef->sounds.activate.getID(soundIdx), owner,
				owner->unitDef->sounds.activate.getVolume(soundIdx));
		}
	}
}

void CGroundMoveType::StopMoving() {
#ifdef TRACE_SYNC
	tracefile << "Stop moving called: ";
	tracefile << owner->pos.x << " " << owner->pos.y << " " << owner->pos.z << " " << owner->id << "\n";
#endif

	#if (DEBUG_OUTPUT == 1)
	logOutput.Print("[CGMT::StopMove] stopping engine for unit %i", owner->id);
	#endif

	StopEngine();

	useMainHeading = false;
	if (progressState != Done)
		progressState = Done;
}

void CGroundMoveType::SetDeltaSpeed(bool wantReverse)
{
	// round low speeds to zero
	if (wantedSpeed == 0.0f && currentSpeed < 0.01f) {
		currentSpeed = 0.0f;
		deltaSpeed = 0.0f;
		return;
	}

	// wanted speed and acceleration
	float wSpeed = reversing? maxReverseSpeed: maxSpeed;

	// limit speed and acceleration
	if (wantedSpeed > 0.0f) {
		const float groundMod = owner->unitDef->movedata->moveMath->SpeedMod(*owner->unitDef->movedata, owner->pos, flatFrontDir);

		wSpeed *= groundMod;

		const float3 goalDifFwd = waypoint - owner->pos;
		const float3 goalDifRev = -goalDifFwd;
		const float3 goalPosTmp = owner->pos + goalDifRev;

		const float3 goalDif = reversing? goalDifRev: goalDifFwd;
		const short turn = owner->heading - GetHeadingFromVector(goalDif.x, goalDif.z);

		if (turn != 0) {
			const float goalLength = goalDif.Length();
			const float turnSpeed = (goalLength + 8.0f) / (abs(turn) / turnRate) * 0.5f;

			if (turnSpeed < wSpeed) {
				// make sure we can turn fast enough to hit the goal
				// (but don't slow us down to a complete crawl either)
				//
				// NOTE: can cause units with large turning circles to
				// circle around close waypoints indefinitely, but one
				// GetNextWaypoint() often is sufficient prevention
				// If current waypoint is the last one and it's near,
				// hit the brakes a bit more.
				const UnitDef* ud = owner->unitDef;
				const float finalGoalSqDist = owner->pos.SqDistance2D(goalPos);
				const float tipSqDist = ud->turnInPlaceDistance * ud->turnInPlaceDistance;
				const bool inPlaceTurn = (ud->turnInPlace && (tipSqDist > finalGoalSqDist || ud->turnInPlaceDistance <= 0.0f));

				if (inPlaceTurn || (currentSpeed < ud->turnInPlaceSpeedLimit)) {
					// keep the turn mostly in-place
					wSpeed = turnSpeed;
				} else {
					if (haveFinalWaypoint && goalLength < ((reversing? maxReverseSpeed * GAME_SPEED: maxSpeed * GAME_SPEED))) {
						// hit the brakes if this is the last waypoint of the path
						wSpeed = std::max(turnSpeed, (turnSpeed + wSpeed) * 0.2f);
					} else {
						// just skip, assume close enough
						GetNextWaypoint();
						wSpeed = (turnSpeed + wSpeed) * 0.625f;
					}
				}
			}
		}

		if (wSpeed > wantedSpeed) {
			wSpeed = wantedSpeed;
		}
	} else {
		wSpeed = 0.0f;
	}


	float dif = wSpeed - currentSpeed;

	// make the forward/reverse transitions more fluid
	if ( wantReverse && !reversing) { dif = -currentSpeed; }
	if (!wantReverse &&  reversing) { dif = -currentSpeed; }

	// limit speed change according to acceleration
	if (fabs(dif) < 0.05f) {
		// we are already going (mostly) how fast we want to go
		deltaSpeed = dif * 0.125f;
		nextDeltaSpeedUpdate = gs->frameNum + 8;
	} else if (dif > 0.0f) {
		// we want to accelerate
		if (dif < accRate) {
			deltaSpeed = dif;
			nextDeltaSpeedUpdate = gs->frameNum;
		} else {
			deltaSpeed = accRate;
			nextDeltaSpeedUpdate = (int) (gs->frameNum + std::min(8.0f, dif / accRate));
		}
	} else {
		// we want to decelerate
		if (dif > -10.0f * decRate) {
			deltaSpeed = dif;
			nextDeltaSpeedUpdate = gs->frameNum + 1;
		} else {
			deltaSpeed = -10.0f * decRate;
			nextDeltaSpeedUpdate = (int) (gs->frameNum + std::min(8.0f, dif / deltaSpeed));
		}
	}

#ifdef TRACE_SYNC
	tracefile << "Unit delta speed: ";
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
	tracefile << "Unit " << owner->id << " changed heading to " << heading << " from " << _oldheading << " (wantedHeading: " << wantedHeading << ")\n";
#endif

	owner->frontdir = GetVectorFromHeading(heading);
	if (owner->upright) {
		owner->updir = UpVector;
		owner->rightdir = owner->frontdir.cross(owner->updir);
	} else {
		owner->updir = ground->GetNormal(owner->pos.x, owner->pos.z);
		owner->rightdir = owner->frontdir.cross(owner->updir);
		owner->rightdir.Normalize();
		owner->frontdir = owner->updir.cross(owner->rightdir);
	}

	flatFrontDir = owner->frontdir;
	flatFrontDir.y = 0.0f;
	flatFrontDir.Normalize();
}

void CGroundMoveType::ImpulseAdded(void)
{
	if (owner->beingBuilt || owner->unitDef->movedata->moveType == MoveData::Ship_Move)
		return;

	float3& impulse = owner->residualImpulse;
	float3& speed = owner->speed;

	if (skidding) {
		speed += impulse;
		impulse = ZeroVector;
	}

	float3 groundNormal = ground->GetNormal(owner->pos.x, owner->pos.z);

	if (impulse.dot(groundNormal) < 0)
		impulse -= groundNormal * impulse.dot(groundNormal);

	const float sqstrength = impulse.SqLength();

	if (sqstrength > 9 || impulse.dot(groundNormal) > 0.3f) {
		skidding = true;
		speed += impulse;
		impulse = ZeroVector;

		skidRotSpeed += (gs->randFloat() - 0.5f) * 1500;
		skidRotPos2 = 0;
		skidRotSpeed2 = 0;
		float3 skidDir(speed);
			skidDir.y = 0;
			skidDir.Normalize();
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

void CGroundMoveType::UpdateSkid(void)
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

		float wh = 0.0f;

		if (floatOnWater)
			wh = ground->GetHeight(midPos.x, midPos.z);
		else
			wh = ground->GetHeight2(midPos.x, midPos.z);

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
			(!floatOnWater || ground->GetHeight(midPos.x, midPos.z) > 0.0f);

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

			float remTime = speedf / speedReduction - 1.0f;
			float rp = floor(skidRotPos2 + skidRotSpeed2 * remTime + 0.5f);

			skidRotSpeed2 = (remTime + 1.0f == 0.0f ) ? 0.0f : (rp - skidRotPos2) / (remTime + 1.0f);

			if (floor(skidRotPos2) != floor(skidRotPos2 + skidRotSpeed2)) {
				skidRotPos2 = 0.0f;
				skidRotSpeed2 = 0.0f;
			}
		}

		float wh = 0.0f;
		if (floatOnWater)
			wh = ground->GetHeight(pos.x, pos.z);
		else
			wh = ground->GetHeight2(pos.x, pos.z);

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
				speed += (normal * (fabs(speed.dot(normal)) + 0.1f)) * 1.9f;
				speed *= 0.8f;
			}
		}
	}
	CalcSkidRot();

	midPos += speed;
	pos = midPos -
		owner->frontdir * owner->relMidPos.z -
		owner->updir    * owner->relMidPos.y -
		owner->rightdir * owner->relMidPos.x;
	owner->midPos = midPos;

	CheckCollisionSkid();
	ASSERT_SYNCED_FLOAT3(owner->midPos);
}

void CGroundMoveType::UpdateControlledDrop(void)
{
	float3& speed = owner->speed;
	float3& pos = owner->pos;
	SyncedFloat3& midPos = owner->midPos;

	if (owner->falling) {
		speed.y += mapInfo->map.gravity * owner->fallSpeed;

		if (owner->speed.y > 0) //sometimes the dropped unit gets an upward force, still unsure where its coming from
			owner->speed.y = 0;

		midPos += speed;
		pos = midPos -
			owner->frontdir * owner->relMidPos.z -
			owner->updir * owner->relMidPos.y -
			owner->rightdir * owner->relMidPos.x;

		owner->midPos.y = owner->pos.y + owner->relMidPos.y;

		if(midPos.y < 0)
			speed*=0.90;

		float wh;

		if(floatOnWater)
			wh = ground->GetHeight(midPos.x, midPos.z);
		else
			wh = ground->GetHeight2(midPos.x, midPos.z);

		if(wh > midPos.y-owner->relMidPos.y){
			owner->falling = false;
			midPos.y = wh + owner->relMidPos.y - speed.y*0.8;
			owner->script->Landed(); //stop parachute animation
		}
	}
}

void CGroundMoveType::CheckCollisionSkid(void)
{
	float3& pos = owner->pos;
	SyncedFloat3& midPos = owner->midPos;

	const UnitDef* ud = owner->unitDef;
	const vector<CUnit*> &nearUnits = qf->GetUnitsExact(midPos, owner->radius);

	for (vector<CUnit*>::const_iterator ui = nearUnits.begin(); ui != nearUnits.end(); ++ui) {
		CUnit* u = (*ui);
		float sqDist = (midPos - u->midPos).SqLength();
		float totRad = owner->radius + u->radius;

		if (sqDist < totRad * totRad && sqDist != 0) {
			float dist = sqrt(sqDist);
			float3 dif = midPos - u->midPos;
			// stop units from reaching escape velocity
			dif /= std::max(dist, 1.f);

			if (!u->mobility) {
				float impactSpeed = -owner->speed.dot(dif);

				if (impactSpeed > 0) {
					midPos += dif * impactSpeed;
					pos = midPos -
						owner->frontdir * owner->relMidPos.z -
						owner->updir    * owner->relMidPos.y -
						owner->rightdir * owner->relMidPos.x;
					owner->speed += dif * (impactSpeed * 1.8f);

					if (impactSpeed > ud->minCollisionSpeed
						&& ud->minCollisionSpeed >= 0) {
						owner->DoDamage(DamageArray(impactSpeed * owner->mass * 0.2f), 0, ZeroVector);
					}
					if (impactSpeed > u->unitDef->minCollisionSpeed
						&& u->unitDef->minCollisionSpeed >= 0) {
						u->DoDamage(DamageArray(impactSpeed * owner->mass * 0.2f), 0, ZeroVector);
					}
				}
			} else {
				// don't conserve momentum
				float part = (owner->mass / (owner->mass + u->mass));
				float impactSpeed = (u->speed - owner->speed).dot(dif) * 0.5f;

				if (impactSpeed > 0) {
					midPos += dif * (impactSpeed * (1 - part) * 2);
					pos = midPos -
						owner->frontdir * owner->relMidPos.z -
						owner->updir    * owner->relMidPos.y -
						owner->rightdir * owner->relMidPos.x;
					owner->speed += dif * (impactSpeed * (1 - part) * 2);
					u->midPos -= dif * (impactSpeed * part * 2);
					u->pos = u->midPos -
						u->frontdir * u->relMidPos.z -
						u->updir    * u->relMidPos.y -
						u->rightdir * u->relMidPos.x;
					u->speed -= dif * (impactSpeed * part * 2);

					if (CGroundMoveType* mt = dynamic_cast<CGroundMoveType*>(u->moveType)) {
						mt->skidding = true;
					}
					if (impactSpeed > ud->minCollisionSpeed
						&& ud->minCollisionSpeed >= 0.0f) {
						owner->DoDamage(
							DamageArray(impactSpeed * owner->mass * 0.2f * (1 - part)),
							0, dif * impactSpeed * (owner->mass * (1 - part)));
					}

					if (impactSpeed > u->unitDef->minCollisionSpeed
						&& u->unitDef->minCollisionSpeed >= 0.0f) {
						u->DoDamage(
							DamageArray(impactSpeed * owner->mass * 0.2f * part),
							0, dif * -impactSpeed * (u->mass * part));
					}
					owner->speed *= 0.9f;
					u->speed *= 0.9f;
				}
			}
		}
	}
	vector<CFeature*> nearFeatures=qf->GetFeaturesExact(midPos,owner->radius);
	for(vector<CFeature*>::iterator fi=nearFeatures.begin();
		fi!=nearFeatures.end();++fi)
	{
		CFeature* u=(*fi);
		if(!u->blocking)
			continue;
		float sqDist=(midPos-u->midPos).SqLength();
		float totRad=owner->radius+u->radius;
		if(sqDist<totRad*totRad && sqDist!=0){
			float dist=sqrt(sqDist);
			float3 dif=midPos-u->midPos;
			dif/=std::max(dist, 1.f);
			float impactSpeed = -owner->speed.dot(dif);
			if(impactSpeed > 0){
				midPos+=dif*(impactSpeed);
				pos = midPos - owner->frontdir*owner->relMidPos.z
					- owner->updir*owner->relMidPos.y
					- owner->rightdir*owner->relMidPos.x;
				owner->speed+=dif*(impactSpeed*1.8f);
				if(impactSpeed > owner->unitDef->minCollisionSpeed
					&& owner->unitDef->minCollisionSpeed >= 0)
				{
					owner->DoDamage(DamageArray(impactSpeed*owner->mass*0.2f),
						0, ZeroVector);
				}
				u->DoDamage(DamageArray(impactSpeed*owner->mass*0.2f),
					-dif*impactSpeed);
			}
		}
	}
}

float CGroundMoveType::GetFlyTime(float3 pos, float3 speed)
{
	return 0;
}

void CGroundMoveType::CalcSkidRot(void)
{
	owner->heading += (short int) skidRotSpeed;

	owner->frontdir = GetVectorFromHeading(owner->heading);

	if (owner->upright) {
		owner->updir = UpVector;
		owner->rightdir = owner->frontdir.cross(owner->updir);
	} else {
		owner->updir = ground->GetSmoothNormal(owner->pos.x, owner->pos.z);
		owner->rightdir = owner->frontdir.cross(owner->updir);
		owner->rightdir.Normalize();
		owner->frontdir = owner->updir.cross(owner->rightdir);
	}

	skidRotPos2 += skidRotSpeed2;

	float cosp = cos(skidRotPos2 * PI * 2.0f);
	float sinp = sin(skidRotPos2 * PI * 2.0f);

	float3 f1 = skidRotVector * skidRotVector.dot(owner->frontdir);
	float3 f2 = owner->frontdir - f1;
	f2 = f2 * cosp + f2.cross(skidRotVector) * sinp;
	owner->frontdir = f1 + f2;

	float3 r1 = skidRotVector * skidRotVector.dot(owner->rightdir);
	float3 r2 = owner->rightdir - r1;
	r2 = r2 * cosp + r2.cross(skidRotVector) * sinp;
	owner->rightdir = r1 + r2;

	float3 u1 = skidRotVector * skidRotVector.dot(owner->updir);
	float3 u2 = owner->updir - u1;
	u2 = u2 * cosp + u2.cross(skidRotVector) * sinp;
	owner->updir = u1 + u2;
}



/*
 * Dynamic obstacle avoidance, helps the unit to
 * follow the path even when it's not perfect.
 */
float3 CGroundMoveType::ObstacleAvoidance(float3 desiredDir) {
	// multiplier for how strongly an object should be avoided
	static const float AVOIDANCE_STRENGTH = 2.0f;

	// NOTE: based on the requirement that all objects have symetrical footprints.
	// If this is false, then radius has to be calculated in a different way!

	// Obstacle-avoidance-system only needs to be run if the unit wants to move
	if (pathId) {
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

			if (ltx >= 0 && ltx < LINETABLE_SIZE && lty >= 0 && lty < LINETABLE_SIZE) {
				for (std::vector<int2>::iterator li = lineTable[lty][ltx].begin(); li != lineTable[lty][ltx].end(); ++li) {
					const int x = (moveSquareX + li->x) * 2;
					const int y = (moveSquareY + li->y) * 2;
					const int blockBits = CMoveMath::BLOCK_STRUCTURE |
					                      CMoveMath::BLOCK_TERRAIN   |
					                      CMoveMath::BLOCK_MOBILE_BUSY;
					const MoveData& moveData = *owner->unitDef->movedata;

					if ((moveData.moveMath->IsBlocked(moveData, x, y) & blockBits) ||
					    (moveData.moveMath->SpeedMod(moveData, x, y) <= 0.01f)) {

						// one of the potential avoidance squares is blocked
						etaFailures++;

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
			moveData->tempOwner = owner;

			vector<CSolidObject*> nearbyObjects = qf->GetSolidsExact(owner->pos, speedf * 35 + 30 + owner->xsize / 2);
			vector<CSolidObject*> objectsOnPath;
			vector<CSolidObject*>::iterator oi;

			for (oi = nearbyObjects.begin(); oi != nearbyObjects.end(); oi++) {
				CSolidObject* o = *oi;
				CMoveMath* moveMath = moveData->moveMath;

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
							fabs(objectDistToAvoidDirCenter) < radiusSum &&
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
									(radiusSum - fabs(objectDistToAvoidDirCenter)) *
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
		tracefile << __FUNCTION__ << " avoidanceVec = " << avoidanceVec.x << " " << avoidanceVec.y << " " << avoidanceVec.z << "\n";
#endif

		return avoidanceDir;
	} else {
		return ZeroVector;
	}
}


unsigned int CGroundMoveType::RequestPath(float3 startPos, float3 goalPos, float goalRadius)
{
	pathId = pathManager->RequestPath(owner->mobility, owner->pos, goalPos, goalRadius, owner);
	return pathId;
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
		float xdiff = streflop::fabs(object1->midPos.x - object2->midPos.x);
		float zdiff = streflop::fabs(object1->midPos.z - object2->midPos.z);

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
	if (owner->pos.SqDistance2D(lastGetPathPos) < 400) {
		#if (DEBUG_OUTPUT == 1)
		logOutput.Print("[CGMT::GNP] non-moving path failures for unit %i: %i", owner->id, nonMovingFailures);
		#endif

		nonMovingFailures++;

		if (nonMovingFailures > 10) {
			nonMovingFailures = 0;
			Fail();
			pathId = 0;
			return;
		}
	} else {
		lastGetPathPos = owner->pos;
		nonMovingFailures = 0;
	}

	pathManager->DeletePath(pathId);
	RequestPath(owner->pos, goalPos, goalRadius);

	// if new path received, can't be at waypoint
	if (pathId) {
		atGoal = false;
		haveFinalWaypoint = false;

		waypoint = owner->pos;
		nextWaypoint = pathManager->NextWaypoint(pathId, waypoint, 1.25f * SQUARE_SIZE, 0, owner->id);
	}

	// limit path-requests to one per second
	restartDelay = gs->frameNum + GAME_SPEED;
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

		if ((currentDistanceToWaypoint) > (turnRadius * 2.0f)) {
			return;
		}
		if (currentDistanceToWaypoint > MIN_WAYPOINT_DISTANCE && waypointDir.dot(flatFrontDir) >= 0.995f) {
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
float CGroundMoveType::BreakingDistance(float speed)
{
	if (!owner->mobility->maxBreaking) {
		return 0.0f;
	}
	return fabs(speed*speed / owner->mobility->maxBreaking);
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






/* Initializes motion. */
void CGroundMoveType::StartEngine() {
	// ran only if the unit has no path and is not already at goal
	if (!pathId && !atGoal) {
		GetNewPath();

		// activate "engine" only if a path was found
		if (pathId) {
			pathManager->UpdatePath(owner, pathId);
			etaFailures = 0;
			owner->isMoving = true;
			owner->script->StartMoving();

			#if (DEBUG_OUTPUT == 1)
			logOutput.Print("[CGMT::StartEngine] engine started for unit %i", owner->id);
			#endif
		} else {
			#if (DEBUG_OUTPUT == 1)
			logOutput.Print("[CGMT::StartEngine] failed to start engine for unit %i", owner->id);
			#endif

			Fail();
		}
	}

	nextObstacleAvoidanceUpdate = gs->frameNum;
}

/* Stops motion. */
void CGroundMoveType::StopEngine() {
	// ran only if engine is active
	if (pathId) {
		// Deactivating engine.
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
	wantedSpeed = 0;
}



/* Called when the unit arrives at its goal. */
void CGroundMoveType::Arrived()
{
	// can only "arrive" if the engine is active
	if (progressState == Active) {
		// we have reached our goal
		StopEngine();

		if (owner->team == gu->myTeam) {
			const int soundIdx = owner->unitDef->sounds.arrived.getRandomIdx();
			if (soundIdx >= 0) {
				Channels::UnitReply.PlaySample(
					owner->unitDef->sounds.arrived.getID(soundIdx), owner,
					owner->unitDef->sounds.arrived.getVolume(soundIdx));
			}
		}

		// and the action is done
		progressState = Done;
		owner->commandAI->SlowUpdate();

		#if (DEBUG_OUTPUT == 1)
		logOutput.Print("[CGMT::Arrive] unit %i arrived", owner->id);
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


	if (gu->moveWarnings && (owner->team == gu->myTeam)) {
		const int soundIdx = owner->unitDef->sounds.cant.getRandomIdx();
		if (soundIdx >= 0) {
			Channels::UnitReply.PlaySample(
				owner->unitDef->sounds.cant.getID(soundIdx), owner,
				owner->unitDef->sounds.cant.getVolume(soundIdx));
		}
		if (!owner->commandAI->unimportantMove &&
		    (owner->pos.SqDistance(goalPos) > Square(goalRadius + 150.0f))) {
			logOutput.Print(owner->unitDef->humanName + ": Can't reach destination!");
			logOutput.SetLastMsgPos(owner->pos);
		}
	}
}



void CGroundMoveType::CheckCollision(void)
{
	int2 newmp = owner->GetMapPos();

	if (newmp.x != owner->mapPos.x || newmp.y != owner->mapPos.y) {
		// now make sure we don't overrun any other units
		bool haveCollided = false;
		int retest = 0;

		do {
			const float zmove = (owner->mapPos.y + owner->zsize / 2) * SQUARE_SIZE;
			const float xmove = (owner->mapPos.x + owner->xsize / 2) * SQUARE_SIZE;

			if (fabs(owner->frontdir.x) > fabs(owner->frontdir.z)) {
				if (newmp.y < owner->mapPos.y) {
					haveCollided |= CheckColV(newmp.y, newmp.x, newmp.x + owner->xsize - 1,  zmove - 3.99f, owner->mapPos.y);
					newmp = owner->GetMapPos();
				} else if (newmp.y > owner->mapPos.y) {
					haveCollided |= CheckColV(newmp.y + owner->zsize - 1, newmp.x, newmp.x + owner->xsize - 1,  zmove + 3.99f, owner->mapPos.y + owner->zsize - 1);
					newmp = owner->GetMapPos();
				}
				if (newmp.x < owner->mapPos.x) {
					haveCollided |= CheckColH(newmp.x, newmp.y, newmp.y + owner->zsize - 1,  xmove - 3.99f, owner->mapPos.x);
					newmp = owner->GetMapPos();
				} else if (newmp.x > owner->mapPos.x) {
					haveCollided |= CheckColH(newmp.x + owner->xsize - 1, newmp.y, newmp.y + owner->zsize - 1,  xmove + 3.99f, owner->mapPos.x + owner->xsize - 1);
					newmp = owner->GetMapPos();
				}
			} else {
				if (newmp.x < owner->mapPos.x) {
					haveCollided |= CheckColH(newmp.x, newmp.y, newmp.y + owner->zsize - 1,  xmove - 3.99f, owner->mapPos.x);
					newmp = owner->GetMapPos();
				} else if (newmp.x > owner->mapPos.x) {
					haveCollided |= CheckColH(newmp.x + owner->xsize - 1, newmp.y, newmp.y + owner->zsize - 1,  xmove + 3.99f, owner->mapPos.x + owner->xsize - 1);
					newmp = owner->GetMapPos();
				}
				if (newmp.y < owner->mapPos.y) {
					haveCollided |= CheckColV(newmp.y, newmp.x, newmp.x + owner->xsize - 1,  zmove - 3.99f, owner->mapPos.y);
					newmp = owner->GetMapPos();
				} else if (newmp.y > owner->mapPos.y) {
					haveCollided |= CheckColV(newmp.y + owner->zsize - 1, newmp.x, newmp.x + owner->xsize - 1,  zmove + 3.99f, owner->mapPos.y + owner->zsize - 1);
					newmp = owner->GetMapPos();
				}
			}
			++retest;
		}
		while (haveCollided && retest < 2);

		// owner->UnBlock();
		owner->Block();
	}

	return;
}


bool CGroundMoveType::CheckColH(int x, int y1, int y2, float xmove, int squareTestX)
{
	MoveData* m = owner->mobility;
	m->tempOwner = owner;

	bool ret = false;

	for (int y = y1; y <= y2; ++y) {
		bool blocked = false;
		const int idx1 = y * gs->mapx + x;
		const int idx2 = y * gs->mapx + squareTestX;
		const BlockingMapCell& c = groundBlockingObjectMap->GetCell(idx1);
		const BlockingMapCell& d = groundBlockingObjectMap->GetCell(idx2);
		BlockingMapCellIt it;
		float3 posDelta = ZeroVector;

		if (!d.empty() && d.find(owner->id) == d.end()) {
			continue;
		}

		for (it = c.begin(); it != c.end(); it++) {
			CSolidObject* obj = it->second;

			if (m->moveMath->IsNonBlocking(*m, obj)) {
				// no collision possible
				continue;
			} else {
				blocked = true;

				if (obj->mobility) {
					float part = owner->mass / (owner->mass + obj->mass * 2.0f);
					float3 dif = obj->pos - owner->pos;
					float dl = dif.Length();
					float colDepth = streflop::fabs(owner->pos.x - xmove);

					// adjust our own position a bit so we have to
					// turn less (FIXME: can place us in building)
					dif *= (dl != 0.0f)? (colDepth / dl): 0.0f;
					posDelta -= (dif * (1.0f - part));

					// safe cast (only units can be mobile)
					CUnit* u = (CUnit*) obj;

					const int uAllyTeam = u->allyteam;
					const int oAllyTeam = owner->allyteam;
					const bool allied = (teamHandler->Ally(uAllyTeam, oAllyTeam) || teamHandler->Ally(oAllyTeam, uAllyTeam));

					if (!u->unitDef->pushResistant && !u->usingScriptMoveType && allied) {
						// push the blocking unit out of the way
						// (FIXME: can place object in building)
						u->pos += (dif * part);
						u->UpdateMidPos();
					}
				}

				// if we can overrun this object (eg.
				// DTs, trees, wreckage) then do so
				if (!m->moveMath->CrushResistant(*m, obj)) {
					float3 fix = owner->frontdir * currentSpeed * 200.0f;
					obj->Kill(fix);
				}
			}
		}

		if (blocked) {
			// HACK: make units find openings on the blocking map more easily
			if (groundBlockingObjectMap->GetCell(y1 * gs->mapx + x).empty()) {
				posDelta.z -= streflop::fabs(owner->pos.x - xmove) * 0.5f;
			}
			if (groundBlockingObjectMap->GetCell(y2 * gs->mapx + x).empty()) {
				posDelta.z += streflop::fabs(owner->pos.x - xmove) * 0.5f;
			}

			if (!((gs->frameNum + owner->id) & 31) && !owner->commandAI->unimportantMove) {
				// if we are doing something important, tell units around us to bugger off
				helper->BuggerOff(owner->pos + owner->frontdir * owner->radius, owner->radius, true, false, owner->team, owner);
			}

			owner->pos += posDelta;
			owner->pos.x = xmove;
			currentSpeed *= 0.97f;
			ret = true;
			break;
		}
	}

	m->tempOwner = NULL;
	return ret;
}

bool CGroundMoveType::CheckColV(int y, int x1, int x2, float zmove, int squareTestY)
{
	MoveData* m = owner->mobility;
	m->tempOwner = owner;

	bool ret = false;

	for (int x = x1; x <= x2; ++x) {
		bool blocked = false;
		const int idx1 = y * gs->mapx + x;
		const int idx2 = squareTestY * gs->mapx + x;
		const BlockingMapCell& c = groundBlockingObjectMap->GetCell(idx1);
		const BlockingMapCell& d = groundBlockingObjectMap->GetCell(idx2);
		BlockingMapCellIt it;
		float3 posDelta = ZeroVector;

		if (!d.empty() && d.find(owner->id) == d.end()) {
			continue;
		}

		for (it = c.begin(); it != c.end(); it++) {
			CSolidObject* obj = it->second;

			if (m->moveMath->IsNonBlocking(*m, obj)) {
				// no collision possible
				continue;
			} else {
				blocked = true;

				if (obj->mobility) {
					float part = owner->mass / (owner->mass + obj->mass * 2.0f);
					float3 dif = obj->pos - owner->pos;
					float dl = dif.Length();
					float colDepth = streflop::fabs(owner->pos.z - zmove);

					// adjust our own position a bit so we have to
					// turn less (FIXME: can place us in building)
					dif *= (dl != 0.0f)? (colDepth / dl): 0.0f;
					posDelta -= (dif * (1.0f - part));

					// safe cast (only units can be mobile)
					CUnit* u = (CUnit*) obj;

					const int uAllyTeam = u->allyteam;
					const int oAllyTeam = owner->allyteam;
					const bool allied = (teamHandler->Ally(uAllyTeam, oAllyTeam) || teamHandler->Ally(oAllyTeam, uAllyTeam));

					if (!u->unitDef->pushResistant && !u->usingScriptMoveType && allied) {
						// push the blocking unit out of the way
						// (FIXME: can place object in building)
						u->pos += (dif * part);
						u->UpdateMidPos();
					}
				}

				// if we can overrun this object (eg.
				// DTs, trees, wreckage) then do so
				if (!m->moveMath->CrushResistant(*m, obj)) {
					float3 fix = owner->frontdir * currentSpeed * 200.0f;
					obj->Kill(fix);
				}
			}
		}

		if (blocked) {
			// HACK: make units find openings on the blocking map more easily
			if (groundBlockingObjectMap->GetCell(y * gs->mapx + x1).empty()) {
				posDelta.x -= streflop::fabs(owner->pos.z - zmove) * 0.5f;
			}
			if (groundBlockingObjectMap->GetCell(y * gs->mapx + x2).empty()) {
				posDelta.x += streflop::fabs(owner->pos.z - zmove) * 0.5f;
			}

			if (!((gs->frameNum + owner->id) & 31) && !owner->commandAI->unimportantMove) {
				// if we are doing something important, tell units around us to bugger off
				helper->BuggerOff(owner->pos + owner->frontdir * owner->radius, owner->radius, true, false, owner->team, owner);
			}

			owner->pos += posDelta;
			owner->pos.z = zmove;
			currentSpeed *= 0.97f;
			ret = true;
			break;
		}
	}

	m->tempOwner = NULL;
	return ret;
}



void CGroundMoveType::CreateLineTable(void)
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
						fabs(xp - start.x) < fabs(to.x - start.x) &&
						fabs(zp - start.z) < fabs(to.z - start.z);

					lineTable[yt][xt].push_back( int2(int(floor(xp)), int(floor(zp))) );
				}

				lineTable[yt][xt].pop_back();
				lineTable[yt][xt].pop_back();
			}
		}
	}
}

void CGroundMoveType::DeleteLineTable(void)
{
	for (int yt = 0; yt < LINETABLE_SIZE; ++yt) {
		for (int xt = 0; xt < LINETABLE_SIZE; ++xt) {
			lineTable[yt][xt].clear();
		}
	}
}

void CGroundMoveType::TestNewTerrainSquare(void)
{
	// first make sure we don't go into any terrain we cant get out of
	int newMoveSquareX = owner->pos.x / (MIN_WAYPOINT_DISTANCE);
	int newMoveSquareY = owner->pos.z / (MIN_WAYPOINT_DISTANCE);

	float3 newpos = owner->pos;

	if (newMoveSquareX != moveSquareX || newMoveSquareY != moveSquareY) {
		const CMoveMath* movemath = owner->unitDef->movedata->moveMath;
		const MoveData& md = *(owner->unitDef->movedata);
		const float cmod = movemath->SpeedMod(md, moveSquareX * 2, moveSquareY * 2);

		if (fabs(owner->frontdir.x) < fabs(owner->frontdir.z)) {
			if (newMoveSquareX > moveSquareX) {
				const float nmod = movemath->SpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.x = moveSquareX * MIN_WAYPOINT_DISTANCE + (MIN_WAYPOINT_DISTANCE - 0.01f);
					newMoveSquareX = moveSquareX;
				}
			} else if (newMoveSquareX < moveSquareX) {
				const float nmod = movemath->SpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.x = moveSquareX * MIN_WAYPOINT_DISTANCE + 0.01f;
					newMoveSquareX = moveSquareX;
				}
			}
			if (newMoveSquareY > moveSquareY) {
				const float nmod = movemath->SpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.z = moveSquareY * MIN_WAYPOINT_DISTANCE + (MIN_WAYPOINT_DISTANCE - 0.01f);
					newMoveSquareY = moveSquareY;
				}
			} else if (newMoveSquareY < moveSquareY) {
				const float nmod = movemath->SpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.z = moveSquareY * MIN_WAYPOINT_DISTANCE + 0.01f;
					newMoveSquareY = moveSquareY;
				}
			}
		} else {
			if (newMoveSquareY > moveSquareY) {
				const float nmod = movemath->SpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod>0.01f && nmod <= 0.01f) {
					newpos.z = moveSquareY * MIN_WAYPOINT_DISTANCE + (MIN_WAYPOINT_DISTANCE - 0.01f);
					newMoveSquareY = moveSquareY;
				}
			} else if (newMoveSquareY < moveSquareY) {
				const float nmod = movemath->SpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.z = moveSquareY * MIN_WAYPOINT_DISTANCE + 0.01f;
					newMoveSquareY = moveSquareY;
				}
			}

			if (newMoveSquareX > moveSquareX) {
				const float nmod = movemath->SpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.x = moveSquareX * MIN_WAYPOINT_DISTANCE + (MIN_WAYPOINT_DISTANCE - 0.01f);
					newMoveSquareX = moveSquareX;
				}
			} else if (newMoveSquareX < moveSquareX) {
				const float nmod = movemath->SpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
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
			terrainSpeed = movemath->SpeedMod(md, moveSquareX * 2, moveSquareY * 2);

			// if we have moved, check if we can get to the next waypoint
			int nwsx = (int) nextWaypoint.x / (MIN_WAYPOINT_DISTANCE) - moveSquareX;
			int nwsy = (int) nextWaypoint.z / (MIN_WAYPOINT_DISTANCE) - moveSquareY;
			int numIter = 0;

			while ((nwsx * nwsx + nwsy * nwsy) < LINETABLE_SIZE && !haveFinalWaypoint && pathId) {
				const int ltx = nwsx + LINETABLE_SIZE / 2;
				const int lty = nwsy + LINETABLE_SIZE / 2;
				bool wpOk = true;

				if (ltx >= 0 && ltx < LINETABLE_SIZE && lty >= 0 && lty < LINETABLE_SIZE) {
					for (std::vector<int2>::iterator li = lineTable[lty][ltx].begin(); li != lineTable[lty][ltx].end(); ++li) {
						const int x = (moveSquareX + li->x) * 2;
						const int y = (moveSquareY + li->y) * 2;
						static const int blockMask =
							(CMoveMath::BLOCK_STRUCTURE | CMoveMath::BLOCK_TERRAIN |
							CMoveMath::BLOCK_MOBILE | CMoveMath::BLOCK_MOBILE_BUSY);

						if ((movemath->IsBlocked(md, x, y) & blockMask) ||
							movemath->SpeedMod(md, x, y) <= 0.01f) {
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

void CGroundMoveType::LeaveTransport(void)
{
	oldPos = owner->pos + UpVector * 0.001f;
}

void CGroundMoveType::KeepPointingTo(float3 pos, float distance, bool aggressive){
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
				tracefile << "Test heading: " << heading << ",  Real heading: " << owner->heading << "\n";
#endif
			} else if (progressState != Active
			  && owner->heading != heading
			  && !owner->weapons.front()->TryTarget(mainHeadingPos, true, 0)) {
				// start moving
				progressState = Active;
				owner->script->StartMoving();
				ChangeHeading(heading);
#ifdef TRACE_SYNC
				tracefile << "Start moving; Test heading: " << heading << ",  Real heading: " << owner->heading << "\n";
#endif
			}
		}
	}
}

void CGroundMoveType::SetMaxSpeed(float speed)
{
	maxSpeed        = std::min(speed, owner->maxSpeed);
	maxReverseSpeed = std::min(speed, owner->maxReverseSpeed);

	requestedSpeed = speed * 2.0f;
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


void CGroundMoveType::AdjustPosToWaterLine()
{
	float wh = 0.0f;
	if (floatOnWater) {
		wh = ground->GetHeight(owner->pos.x, owner->pos.z);
		if (wh == 0.0f)
			wh = -owner->unitDef->waterline;
	} else {
		wh = ground->GetHeight2(owner->pos.x, owner->pos.z);
	}

	if (!(owner->falling || flying)) {
		owner->pos.y = wh;
	}
}

bool CGroundMoveType::UpdateDirectControl()
{
	bool wantReverse = (owner->directControl->back && !owner->directControl->forward);
	waypoint = owner->pos;
	waypoint += wantReverse ? -owner->frontdir * 100 : owner->frontdir * 100;
	waypoint.CheckInBounds();

	if (owner->directControl->forward) {
		assert(!wantReverse);
		wantedSpeed = maxSpeed * 2.0f;
		SetDeltaSpeed(wantReverse);
		owner->isMoving = true;
		owner->script->StartMoving();
	} else if (owner->directControl->back) {
		assert(wantReverse);
		wantedSpeed = maxReverseSpeed * 2.0f;
		SetDeltaSpeed(wantReverse);
		owner->isMoving = true;
		owner->script->StartMoving();
	} else {
		wantedSpeed = 0;
		SetDeltaSpeed(false);
		owner->isMoving = false;
		owner->script->StopMoving();
	}
	deltaHeading = 0;
	if (owner->directControl->left) {
		deltaHeading += (short) turnRate;
	}
	if (owner->directControl->right) {
		deltaHeading -= (short) turnRate;
	}

	if (gu->directControl == owner)
		camera->rot.y += deltaHeading * TAANG2RAD;
	return wantReverse;
}

void CGroundMoveType::UpdateOwnerPos(bool wantReverse)
{
	if (wantedSpeed > 0.0f || currentSpeed != 0.0f) {
		if (wantReverse) {
			if (!reversing) {
				reversing = (currentSpeed <= 0.0f);
			}
		} else {
			if (reversing) {
				reversing = (currentSpeed > 0.0f);
			}
		}

		// note: currentSpeed can be out of sync with
		// owner->speed.Length(), eg. when in front of
		// an obstacle ==> bad for MANY reasons, such
		// as too-quick consumption of waypoints when
		// a new path is requested
		currentSpeed += deltaSpeed;
		owner->pos += (flatFrontDir * currentSpeed * (reversing? -1.0f: 1.0f));

		AdjustPosToWaterLine();
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
