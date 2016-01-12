/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
/*
 * code here roughly corresponds to HEAD as of
 *     218e9c410fd9bef905bed5350640836bbd9402a7
 * but includes cherry-picked commits
 *     512b3775601891ca9ff3ff4a5409b3cbfd77f81e (line-table changes ONLY)
 *     9e296e6d685eb5c059d083880cb05df6809eb721
 *     2362149b3c2b3e09606bd1614984a9ef5f6f85d5
 * plus whatever "random" changes were needed to make it compile
 * in present-day HEAD (0bee6d1e20c7e2f9ece99f0ace76d0f601d63b55)
 *
 * Any and all bugs in this version (and there are many, whether as a result of
 * transporting the old code forward in time to a changed engine environment or
 * pre-existing ones that were removed from GroundMoveType at a later point) WILL
 * NEVER BE FIXED, new features WILL NEVER BE ADDED. EVERY LAST LINE OF CODE HERE
 * IS COMPLETELY UNMAINTAINED, PERIOD.
 *
 * This code also does NOT work with QTPFS, nor will it EVER be adapted to.
 */

#include "ClassicGroundMoveType.h"

#define CLASSIC_GROUNDMOVETYPE_ENABLED
#ifndef CLASSIC_GROUNDMOVETYPE_ENABLED

CClassicGroundMoveType::CClassicGroundMoveType(CUnit* owner): AMoveType(owner) {}
CClassicGroundMoveType::~CClassicGroundMoveType() {}
bool CClassicGroundMoveType::Update() { return false; }
void CClassicGroundMoveType::SlowUpdate() {}
void CClassicGroundMoveType::StartMoving(float3, float) {}
void CClassicGroundMoveType::StartMoving(float3, float, float) {}
void CClassicGroundMoveType::StopMoving(bool, bool) {}
void CClassicGroundMoveType::KeepPointingTo(float3, float, bool) {}
void CClassicGroundMoveType::KeepPointingTo(CUnit*, float, bool) {}
bool CClassicGroundMoveType::CanApplyImpulse() { return false; }
void CClassicGroundMoveType::SetMaxSpeed(float) {}
void CClassicGroundMoveType::LeaveTransport() {}

#else

#include "MoveDefHandler.h"
#include "ExternalAI/EngineOutHandler.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/SelectedUnitsHandler.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "MoveMath/MoveMath.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/CommandAI/MobileCAI.h"
#include "Sim/Weapons/WeaponDef.h"
#include "Sim/Weapons/Weapon.h"
#include "System/EventHandler.h"
#include "System/Sound/ISoundChannels.h"
#include "System/FastMath.h"
#include "System/myMath.h"
#include "System/type2.h"

std::vector<int2> CClassicGroundMoveType::lineTable[LINETABLE_SIZE][LINETABLE_SIZE];

CClassicGroundMoveType::CClassicGroundMoveType(CUnit* owner):
	AMoveType(owner),

	waypoint(ZeroVector),
	nextWaypoint(ZeroVector),

	pathId(0),
	goalRadius(0),

	atGoal(false),
	haveFinalWaypoint(false),

	turnRate(0.1f),
	accRate(0.01f),
	decRate(0.01f),

	skidRotSpeed(0.0f),
	//dropSpeed(0.0f),
	//dropHeight(0.0f),

	skidRotVector(UpVector),
	skidRotSpeed2(0.0f),
	skidRotPos2(0.0f),

	flatFrontDir(FwdVector),
	mainHeadingPos(ZeroVector),
	lastGetPathPos(ZeroVector),

	useMainHeading(false),
	deltaHeading(0),

	wantedSpeed(0.0f),
	currentSpeed(0.0f),
	requestedSpeed(0.0f),
	deltaSpeed(0.0f),
	terrainSpeed(1.0f),
	currentDistanceToWaypoint(0),

	etaWaypoint(0),
	etaWaypoint2(0),

	nextDeltaSpeedUpdate(0),
	nextObstacleAvoidanceUpdate(0),
	restartDelay(0),

	pathFailures(0),
	etaFailures(0),
	nonMovingFailures(0)
{
	assert(owner != NULL);
	assert(owner->unitDef != NULL);

	moveSquareX = owner->pos.x / (SQUARE_SIZE * 2);
	moveSquareY = owner->pos.z / (SQUARE_SIZE * 2);

	turnRate = owner->unitDef->turnRate;
	accRate = std::max(0.01f, owner->unitDef->maxAcc);
	decRate = std::max(0.01f, owner->unitDef->maxDec);

	owner->moveDef->avoidMobilesOnPath = true;
}

CClassicGroundMoveType::~CClassicGroundMoveType()
{
	if (pathId != 0) {
		pathManager->DeletePath(pathId);
	}
}

bool CClassicGroundMoveType::Update()
{
	ASSERT_SYNCED(owner->pos);

	if (owner->transporter != NULL) {
		return false;
	}

	if (OnSlope() &&
		(!owner->unitDef->floatOnWater || CGround::GetHeightAboveWater(owner->midPos.x, owner->midPos.z) > 0))
	{
		owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING);
	}

	if (owner->IsSkidding()) {
		UpdateSkid();
		return false;
	}

	ASSERT_SYNCED(owner->pos);

	if (owner->IsFalling()) {
		UpdateControlledDrop();
		return false;
	}

	ASSERT_SYNCED(owner->pos);

	bool hasMoved = false;

	if (owner->IsStunned() || owner->beingBuilt) {
		owner->script->StopMoving();
		owner->SetVelocityAndSpeed(ZeroVector);
	} else {
		if (owner->UnderFirstPersonControl()) {
			UpdateDirectControl();
			ChangeHeading(owner->heading + deltaHeading);
		} else {
			if (pathId != 0 || currentSpeed != 0.0f) {
				ASSERT_SYNCED(waypoint);
				ASSERT_SYNCED(owner->pos);

				currentDistanceToWaypoint = owner->pos.distance2D(waypoint);

				if (pathId != 0 && !atGoal && gs->frameNum > etaWaypoint) {
					etaFailures += 10;
					etaWaypoint = INT_MAX;
				}
				if (pathId != 0 && !atGoal && gs->frameNum > etaWaypoint2) {
					if (owner->pos.SqDistance2D(goalPos) > (200 * 200) || CheckGoalFeasability()) {
						etaWaypoint2 += 100;
					} else {
						Fail();
					}
				}

				float3 waypointDir = waypoint - owner->pos;

				ASSERT_SYNCED(waypointDir);

				waypointDir.y = 0;
				waypointDir.SafeNormalize();

				if (pathId != 0 && !atGoal && haveFinalWaypoint && currentDistanceToWaypoint < SQUARE_SIZE * 2) {
					Arrived();
				}

				const float3 avoidVec = ObstacleAvoidance(waypointDir);

				ASSERT_SYNCED(avoidVec);

				if (avoidVec != ZeroVector) {
					ChangeHeading(GetHeadingFromVector(avoidVec.x, avoidVec.z));
				} else {
					SetMainHeading();
				}


				if (nextDeltaSpeedUpdate <= gs->frameNum) {
					wantedSpeed = (pathId != 0)? requestedSpeed: 0.0f;

					if (!owner->commandAI->HasMoreMoveCommands() && currentDistanceToWaypoint < BreakingDistance(currentSpeed) + SQUARE_SIZE) {
						wantedSpeed = std::min(wantedSpeed, fastmath::apxsqrt(currentDistanceToWaypoint * decRate));
					}

					wantedSpeed *= std::max(0.0f, std::min(1.0f, avoidVec.dot(owner->frontdir) + 0.1f));
					ChangeSpeed();
				}
			} else {
				SetMainHeading();
			}

			pathManager->UpdatePath(owner, pathId);
		}

		UpdateOwnerPos();
	}

	if (owner->pos != oldPos) {
		TestNewTerrainSquare();
		CheckCollision();
		AdjustPosToWaterLine();

		owner->SetVelocityAndSpeed(owner->pos - oldPos);
		owner->UpdateMidAndAimPos();

		oldPos = owner->pos;

		hasMoved = true;
	} else {
		owner->SetVelocityAndSpeed(ZeroVector);
	}

	return hasMoved;
}

void CClassicGroundMoveType::SlowUpdate()
{
	AMoveType::SlowUpdate();

	if (owner->transporter) {
		if (progressState == Active)
			StopEngine();
		return;
	}

	if (progressState == Active && etaFailures > 8) {
		if (owner->pos.SqDistance2D(goalPos) > (200 * 200) || CheckGoalFeasability()) {
			StopEngine();
			StartEngine();
		} else {
			Fail();
		}
	}

	if (progressState == Active && !pathId && gs->frameNum > restartDelay) {
		StartEngine();
	}

	if (!owner->IsFlying()) {
		if (!owner->pos.IsInBounds()) {
			owner->pos.ClampInBounds();
			oldPos = owner->pos;
		}
	}

	if (!(owner->IsFalling() || owner->IsFlying())) {
		float wh = 0.0f;

		if (owner->unitDef->floatOnWater) {
			wh = CGround::GetHeightAboveWater(owner->pos.x, owner->pos.z);
			if (wh == 0.0f) {
				wh = -owner->unitDef->waterline;
			}
		} else {
			wh = CGround::GetHeightReal(owner->pos.x, owner->pos.z);
		}
		owner->Move(UpVector * (wh - owner->pos.y), true);
	}
}



void CClassicGroundMoveType::StartMoving(float3 pos, float goalRadius) {
	StartMoving(pos, goalRadius, maxSpeed * 2);
}

void CClassicGroundMoveType::StartMoving(float3 moveGoalPos, float moveGoalRadius, float speed)
{
	if (progressState == Active) {
		StopEngine();
	}

	goalPos = moveGoalPos;
	goalRadius = moveGoalRadius;
	requestedSpeed = speed;
	atGoal = false;
	useMainHeading = false;
	progressState = Active;

	StartEngine();

	if (owner->team == gu->myTeam) {
		Channels::General->PlayRandomSample(owner->unitDef->sounds.activate, owner);
	}
}

void CClassicGroundMoveType::StopMoving(bool callScript, bool hardStop) {
	StopEngine();

	useMainHeading = false;
	progressState = Done;
}



void CClassicGroundMoveType::ChangeSpeed()
{
	if (wantedSpeed == 0.0f && currentSpeed < 0.01f) {
		currentSpeed = 0.0f;
		deltaSpeed = 0.0f;
		return;
	}

	float wSpeed = maxSpeed;

	if (wantedSpeed > 0.0f) {
		const float groundMod = CMoveMath::GetPosSpeedMod(*owner->moveDef, owner->pos, flatFrontDir);
		const float3 goalDif = waypoint - owner->pos;
		const short turn = owner->heading - GetHeadingFromVector(goalDif.x, goalDif.z);

		wSpeed *= groundMod;

		if (turn != 0) {
			const float goalLength = goalDif.Length();
			const float turnSpeed = (goalLength + 8.0f) / (std::abs(turn) / turnRate) * 0.5f;

			if (turnSpeed < wSpeed) {
				wSpeed = turnSpeed;
			}
		}

		if (wSpeed > wantedSpeed) {
			wSpeed = wantedSpeed;
		}
	} else {
		wSpeed = 0.0f;
	}

	const float dif = wSpeed - currentSpeed;

	if (math::fabs(dif) < 0.05f) {
		deltaSpeed = dif * 0.125f;
		nextDeltaSpeedUpdate = gs->frameNum + 8;
	} else if (dif > 0.0f) {
		if (dif < accRate) {
			deltaSpeed = dif;
			nextDeltaSpeedUpdate = gs->frameNum;
		} else {
			deltaSpeed = accRate;
			nextDeltaSpeedUpdate = (int) (gs->frameNum + std::min(8.0f, dif / accRate));
		}
	} else {
		if (dif > -10.0f * decRate) {
			deltaSpeed = dif;
			nextDeltaSpeedUpdate = gs->frameNum + 1;
		} else {
			deltaSpeed = -10.0f * decRate;
			nextDeltaSpeedUpdate = (int) (gs->frameNum + std::min(8.0f, dif / deltaSpeed));
		}
	}
}

void CClassicGroundMoveType::ChangeHeading(short wantedHeading) {
	SyncedSshort& heading = owner->heading;

	deltaHeading = wantedHeading - heading;

	ASSERT_SYNCED(deltaHeading);
	ASSERT_SYNCED(turnRate);
	ASSERT_SYNCED((short)turnRate);

	short sTurnRate = short(turnRate);

	if (deltaHeading > 0) {
		short tmp = (deltaHeading < sTurnRate)? deltaHeading: sTurnRate;
		ASSERT_SYNCED(tmp);
		heading += tmp;
	} else {
		short tmp = (deltaHeading > -sTurnRate)? deltaHeading: -sTurnRate;
		ASSERT_SYNCED(tmp);
		heading += tmp;
	}

	owner->frontdir = GetVectorFromHeading(heading);
	if (owner->upright) {
		owner->updir = UpVector;
		owner->rightdir = owner->frontdir.cross(owner->updir);
	} else {
		owner->updir = CGround::GetNormal(owner->pos.x, owner->pos.z);
		owner->rightdir = owner->frontdir.cross(owner->updir);
		owner->rightdir.Normalize();
		owner->frontdir = owner->updir.cross(owner->rightdir);
	}

	flatFrontDir = owner->frontdir;
	flatFrontDir.y = 0;
	flatFrontDir.Normalize();
}

bool CClassicGroundMoveType::CanApplyImpulse(const float3& extImpulse)
{
	if (owner->beingBuilt || owner->moveDef->speedModClass == MoveDef::Ship)
		return false;

	float3 impulse = extImpulse;

	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	if (owner->IsSkidding()) {
		owner->SetVelocity(spd + impulse);
		impulse = ZeroVector;
	}

	float3 groundNormal = CGround::GetNormal(pos.x, pos.z);
	float3 skidDir = spd;

	if (impulse.dot(groundNormal) < 0)
		impulse -= groundNormal * impulse.dot(groundNormal);

	const float sqstrength = impulse.SqLength();

	if (sqstrength > 9 || impulse.dot(groundNormal) > 0.3f) {
		owner->SetVelocity(spd + impulse);

		skidRotSpeed += (gs->randFloat() - 0.5f) * 1500;
		skidRotPos2 = 0;
		skidRotSpeed2 = 0;
		skidRotVector = (skidDir.SafeNormalize2D()).cross(UpVector);

		owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING);
		owner->moveType->useHeading = false;

		if (spd.dot(groundNormal) > 0.2f) {
			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
			skidRotSpeed2 = (gs->randFloat() - 0.5f) * 0.04f;
		}
	}

	owner->SetSpeed(spd);
	return true;
}

void CClassicGroundMoveType::UpdateSkid()
{
	const float4& spd = owner->speed;

	const float3& pos = owner->pos;
	const SyncedFloat3& midPos = owner->midPos;

	if (owner->IsFlying()) {
		owner->SetVelocity(spd + (UpVector * mapInfo->map.gravity));

		if (midPos.y < 0.0f)
			owner->SetVelocity(spd * 0.95f);

		float wh = 0.0f;

		if (owner->unitDef->floatOnWater)
			wh = CGround::GetHeightAboveWater(midPos.x, midPos.z);
		else
			wh = CGround::GetHeightReal(midPos.x, midPos.z);

		if(wh>midPos.y-owner->relMidPos.y){
			skidRotSpeed += (gs->randFloat()-0.5f)*1500;

			owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
			owner->Move(UpVector * (wh + owner->relMidPos.y - spd.y * 0.5f - pos.y), true);

			const float impactSpeed = -spd.dot(CGround::GetNormal(midPos.x,midPos.z));

			if (impactSpeed > owner->unitDef->minCollisionSpeed && owner->unitDef->minCollisionSpeed >= 0) {
				owner->DoDamage(DamageArray(impactSpeed * owner->mass * 0.2f), ZeroVector, NULL, -1, -1);
			}
		}
	} else {
		float speedf = spd.w;
		float speedReduction=0.35f;

		const bool onSlope =
			(CGround::GetSlope(owner->midPos.x, owner->midPos.z) > owner->moveDef->maxSlope) &&
			(!owner->unitDef->floatOnWater || CGround::GetHeightAboveWater(midPos.x, midPos.z) > 0.0f);

		if (speedf < speedReduction && !onSlope) {
			owner->SetVelocity(ZeroVector);

			currentSpeed = 0.0f;
			skidRotSpeed = 0.0f;
			skidRotSpeed2 = (math::floor(skidRotPos2 + skidRotSpeed2 + 0.5f) - skidRotPos2) * 0.5f;

			owner->moveType->useHeading = true;
			owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING);
			ChangeHeading(owner->heading);
		} else {
			if (onSlope) {
				const float3& dir = CGround::GetNormal(midPos.x, midPos.z);
				const float3 normalForce = dir * dir.dot(UpVector * mapInfo->map.gravity);
				const float3 newForce = UpVector * mapInfo->map.gravity - normalForce;

				owner->SetVelocity(spd + newForce);
				owner->SetVelocity(spd * (1.0f - (0.1f * dir.y)));
			} else {
				owner->SetVelocity(spd * (speedf - speedReduction) / speedf);
			}

			speedf = owner->SetSpeed(spd);

			float remTime = speedf / speedReduction - 1.0f;
			float rp = math::floor(skidRotPos2 + skidRotSpeed2 * remTime + 0.5f);

			skidRotSpeed2 = (remTime + 1.0f == 0.0f)? 0.0f: (rp - skidRotPos2) / (remTime + 1);

			if (math::floor(skidRotPos2) != math::floor(skidRotPos2+skidRotSpeed2)) {
				skidRotPos2 = 0.0f;
				skidRotSpeed2 = 0.0f;
			}
		}

		float wh = 0.0f;

		if (owner->unitDef->floatOnWater)
			wh = CGround::GetHeightAboveWater(pos.x, pos.z);
		else
			wh = CGround::GetHeightReal(pos.x, pos.z);

		if (wh - pos.y < spd.y + mapInfo->map.gravity){
			owner->SetVelocity(spd + (UpVector * mapInfo->map.gravity));

			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_SKIDDING);
		} else if (wh - pos.y > spd.y) {
			const float3& normal = CGround::GetNormal(pos.x, pos.z);
			const float dot = spd.dot(normal);

			if (dot > 0.0f) {
				owner->SetVelocity(spd * 0.95f);
			} else {
				owner->SetVelocity(spd + (normal * (math::fabs(spd.dot(normal)) + 0.1f)) * 1.9f);
				owner->SetVelocity(spd * 0.8f);
			}
		}
	}

	CalcSkidRot();
	owner->SetSpeed(spd);
	owner->Move(spd, true);
	CheckCollisionSkid();
}

void CClassicGroundMoveType::UpdateControlledDrop()
{
	const float4& spd = owner->speed;
	const SyncedFloat3& midPos = owner->midPos;

	if (owner->IsFalling()) {
		owner->SetVelocity(spd + (UpVector * mapInfo->map.gravity * owner->fallSpeed));

		if (spd.y > 0.0f)
			owner->SetVelocity(spd * XZVector);

		owner->Move(spd, true);

		if (midPos.y < 0.0f)
			owner->SetVelocity(spd * 0.9f);

		float wh = 0.0f;

		if (owner->unitDef->floatOnWater)
			wh = CGround::GetHeightAboveWater(midPos.x, midPos.z);
		else
			wh = CGround::GetHeightReal(midPos.x, midPos.z);

		if (wh > midPos.y - owner->relMidPos.y) {
			owner->Move(UpVector * (wh + owner->relMidPos.y - spd.y * 0.8 - midPos.y), true);
			owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_FALLING);
			owner->script->Landed();
		}

		owner->SetSpeed(spd);
	}
}

void CClassicGroundMoveType::CheckCollisionSkid()
{
	const SyncedFloat3& midPos = owner->midPos;

	const vector<CUnit*>& nearUnits = quadField->GetUnitsExact(midPos, owner->radius);
	const vector<CFeature*>& nearFeatures = quadField->GetFeaturesExact(midPos, owner->radius);

	for (vector<CUnit*>::const_iterator ui = nearUnits.begin(); ui != nearUnits.end(); ++ui) {
		CUnit* unit = *ui;

		if (!unit->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
			continue;

		const float sqDist = (midPos - unit->midPos).SqLength();
		const float totRad = owner->radius + unit->radius;

		if (sqDist < totRad * totRad && sqDist != 0) {
			float dist = math::sqrt(sqDist);
			float3 dif = midPos - unit->midPos;
			dif /= std::max(dist, 1.0f);

			if (unit->moveDef == NULL) {
				float impactSpeed = -owner->speed.dot(dif);

				if (impactSpeed > 0) {
					owner->Move(dif * impactSpeed, true);
					owner->SetVelocity(owner->speed + dif * (impactSpeed * 1.8f));

					if (impactSpeed > owner->unitDef->minCollisionSpeed && owner->unitDef->minCollisionSpeed >= 0) {
						owner->DoDamage(DamageArray(impactSpeed * owner->mass * 0.2f), ZeroVector, NULL, -1, -1);
					}
					if (impactSpeed > unit->unitDef->minCollisionSpeed && unit->unitDef->minCollisionSpeed >= 0) {
						unit->DoDamage(DamageArray(impactSpeed * owner->mass * 0.2f), ZeroVector, NULL, -1, -1);
					}
				}
			} else {
				float part = (owner->mass / (owner->mass + unit->mass));
				float impactSpeed = (unit->speed - owner->speed).dot(dif) * 0.5f;

				if (impactSpeed > 0) {
					owner->Move(dif * (impactSpeed * (1 - part) * 2), true);
					unit->Move(dif * (impactSpeed * part * 2), true);

					owner->SetVelocity(owner->speed + dif * (impactSpeed * (1 - part) * 2));
					unit->SetVelocity(unit->speed - dif * (impactSpeed * part * 2));

					if (impactSpeed > owner->unitDef->minCollisionSpeed && owner->unitDef->minCollisionSpeed >= 0) {
						owner->DoDamage(
							DamageArray(impactSpeed * owner->mass * 0.2f * (1 - part)),
							dif * impactSpeed * (owner->mass * (1 - part)), NULL, -1, -1);
					}

					if (impactSpeed > unit->unitDef->minCollisionSpeed && unit->unitDef->minCollisionSpeed >= 0) {
						unit->DoDamage(
							DamageArray(impactSpeed * owner->mass * 0.2f * part),
							dif * -impactSpeed * (unit->mass * part), NULL, -1, -1);
					}

					owner->SetVelocity(owner->speed * 0.9f);
					unit->SetVelocityAndSpeed(unit->speed * 0.9f);
				}
			}
		}
	}

	for (vector<CFeature*>::const_iterator fi = nearFeatures.begin(); fi != nearFeatures.end(); ++fi) {
		CFeature* feature = *fi;

		if (!feature->HasCollidableStateBit(CSolidObject::CSTATE_BIT_SOLIDOBJECTS))
			continue;

		const float sqDist = (midPos - feature->midPos).SqLength();
		const float totRad = owner->radius + feature->radius;

		if (sqDist<totRad*totRad && sqDist!=0) {
			float dist = math::sqrt(sqDist);
			float3 dif = midPos - feature->midPos;
			dif /= std::max(dist, 1.0f);
			float impactSpeed = -owner->speed.dot(dif);

			if (impactSpeed > 0.0f) {
				owner->Move(dif * impactSpeed, true);
				owner->SetVelocity(owner->speed + dif*(impactSpeed*1.8f));

				if (impactSpeed > owner->unitDef->minCollisionSpeed && owner->unitDef->minCollisionSpeed >= 0) {
					owner->DoDamage(DamageArray(impactSpeed * owner->mass * 0.2f), ZeroVector, NULL, -1, -1);
				}

				feature->DoDamage(DamageArray(impactSpeed * owner->mass * 0.2f), -dif*impactSpeed, NULL, -1, -1);
			}
		}
	}

	owner->SetSpeed(owner->speed);
}

float CClassicGroundMoveType::GetFlyTime(float3 pos, float3 speed)
{
	return 0;
}

void CClassicGroundMoveType::CalcSkidRot()
{
	owner->heading += (short int) skidRotSpeed;

	owner->frontdir = GetVectorFromHeading(owner->heading);

	if (owner->upright) {
		owner->updir = UpVector;
		owner->rightdir = owner->frontdir.cross(owner->updir);
	} else {
		owner->updir = CGround::GetSmoothNormal(owner->pos.x, owner->pos.z);
		owner->rightdir = owner->frontdir.cross(owner->updir);
		owner->rightdir.Normalize();
		owner->frontdir = owner->updir.cross(owner->rightdir);
	}

	skidRotPos2 += skidRotSpeed2;

	float cosp = math::cos(skidRotPos2 * PI * 2.0f);
	float sinp = math::sin(skidRotPos2 * PI * 2.0f);

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



float3 CClassicGroundMoveType::ObstacleAvoidance(float3 desiredDir) {
	// NOTE: based on the requirement that all objects have symetrical footprints.
	// If this is false, then radius has to be calculated in a different way!
	const float AVOIDANCE_STRENGTH = 2.0f;

	if (pathId != 0) {
		float3 avoidanceVec = ZeroVector;
		float3 avoidanceDir = desiredDir;

		if (gs->frameNum >= nextObstacleAvoidanceUpdate) {
			nextObstacleAvoidanceUpdate = gs->frameNum + 4;

			const int wsx = (int) waypoint.x / (SQUARE_SIZE * 2);
			const int wsy = (int) waypoint.z / (SQUARE_SIZE * 2);
			const int ltx = wsx - moveSquareX + (LINETABLE_SIZE / 2);
			const int lty = wsy - moveSquareY + (LINETABLE_SIZE / 2);

			if (ltx >= 0 && ltx < LINETABLE_SIZE && lty >= 0 && lty < LINETABLE_SIZE) {
				for (std::vector<int2>::iterator li = lineTable[lty][ltx].begin(); li != lineTable[lty][ltx].end(); ++li) {
					const int x = (moveSquareX + li->x) * 2;
					const int y = (moveSquareY + li->y) * 2;
					const int blockBits = CMoveMath::BLOCK_STRUCTURE |
					                      CMoveMath::BLOCK_MOBILE_BUSY;
					MoveDef& moveDef = *owner->moveDef;
					if ((CMoveMath::IsBlocked(moveDef, x, y, owner) & blockBits) ||
					    (CMoveMath::GetPosSpeedMod(moveDef, x, y) <= 0.01f)) {
						++etaFailures;

						break;
					}
				}
			}

			const float currentDistanceToGoal = owner->pos.distance2D(goalPos);
			const float currentDistanceToGoalSq = currentDistanceToGoal * currentDistanceToGoal;
			const float3 rightOfPath = desiredDir.cross(UpVector);
			const float speedf = owner->speed.Length2D();

			float avoidLeft = 0.0f;
			float avoidRight = 0.0f;
			float3 rightOfAvoid = rightOfPath;


			MoveDef* moveDef = owner->moveDef;

			vector<CSolidObject*> nearbyObjects = quadField->GetSolidsExact(owner->pos, speedf * 35 + 30 + owner->xsize / 2, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS);
			vector<CSolidObject*> objectsOnPath;
			vector<CSolidObject*>::iterator oi;

			for (oi = nearbyObjects.begin(); oi != nearbyObjects.end(); ++oi) {
				CSolidObject* o = *oi;

				if (CMoveMath::IsNonBlocking(*moveDef, o, owner)) {
					continue;
				}

				if (o != owner && CMoveMath::CrushResistant(*moveDef, o) && desiredDir.dot(o->pos - owner->pos) > 0) {
					float3 objectToUnit = (owner->pos - o->pos - o->speed * 30);
					float distanceToObjectSq = objectToUnit.SqLength();
					float radiusSum = (owner->xsize + o->xsize) * SQUARE_SIZE / 2;
					float distanceLimit = speedf * 35 + 10 + radiusSum;
					float distanceLimitSq = distanceLimit * distanceLimit;

					if (distanceToObjectSq < distanceLimitSq && distanceToObjectSq < currentDistanceToGoalSq
							&& distanceToObjectSq > 0.0001f) {
						float objectDistToAvoidDirCenter = objectToUnit.dot(rightOfAvoid);

						if (objectToUnit.dot(avoidanceDir) < radiusSum &&
							math::fabs(objectDistToAvoidDirCenter) < radiusSum &&
							(o->moveDef != NULL || Distance2D(owner, o) >= 0)) {

							if (objectDistToAvoidDirCenter > 0.0f) {
								avoidRight +=
									(radiusSum - objectDistToAvoidDirCenter) *
									AVOIDANCE_STRENGTH * fastmath::isqrt2(distanceToObjectSq);
								avoidanceDir += (rightOfAvoid * avoidRight);
								avoidanceDir.Normalize();
								rightOfAvoid = avoidanceDir.cross(UpVector);
							} else {
								avoidLeft +=
									(radiusSum - math::fabs(objectDistToAvoidDirCenter)) *
									AVOIDANCE_STRENGTH * fastmath::isqrt2(distanceToObjectSq);
								avoidanceDir -= (rightOfAvoid * avoidLeft);
								avoidanceDir.Normalize();
								rightOfAvoid = avoidanceDir.cross(UpVector);
							}
							objectsOnPath.push_back(o);
						}
					}

				}
			}

			avoidanceVec = (desiredDir.cross(UpVector) * (avoidRight - avoidLeft));
		}

		avoidanceDir = (desiredDir + avoidanceVec).Normalize();

		return avoidanceDir;
	} else {
		return ZeroVector;
	}
}




float CClassicGroundMoveType::Distance2D(CSolidObject* object1, CSolidObject* object2, float marginal)
{
	float dist2D;
	if (object1->xsize == object1->zsize || object2->xsize == object2->zsize) {
		float3 distVec = (object1->midPos - object2->midPos);
		dist2D = distVec.Length2D() - (object1->xsize + object2->xsize) * SQUARE_SIZE / 2 + 2 * marginal;
	} else {
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



void CClassicGroundMoveType::GetNewPath()
{
	if (owner->pos.SqDistance2D(lastGetPathPos) < 400) {
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
	pathId = pathManager->RequestPath(owner, owner->moveDef, owner->pos, goalPos, goalRadius, true);

	nextWaypoint = owner->pos;

	if (pathId != 0) {
		atGoal = false;
		haveFinalWaypoint = false;
		GetNextWayPoint();
		GetNextWayPoint();
	}

	restartDelay = gs->frameNum + GAME_SPEED;
}

void CClassicGroundMoveType::GetNextWayPoint()
{
	if (pathId != 0) {
		waypoint = nextWaypoint;
		nextWaypoint = pathManager->NextWayPoint(owner, pathId, 0, waypoint, 1.25f * SQUARE_SIZE, true);

		if (nextWaypoint.x != -1) {
			etaWaypoint = int(30.0f / (requestedSpeed * terrainSpeed + 0.001f)) + gs->frameNum + 50;
			etaWaypoint2 = int(25.0f / (requestedSpeed * terrainSpeed + 0.001f)) + gs->frameNum + 10;
			atGoal = false;
		} else {
			pathFailures++;
			if (pathFailures > 0) {
				pathFailures = 0;
				Fail();
			}
			etaWaypoint = INT_MAX;
			etaWaypoint2 =INT_MAX;
			nextWaypoint = waypoint;
		}

		if (waypoint.SqDistance2D(goalPos) < Square(SQUARE_SIZE * 2)) {
			waypoint = goalPos;
			haveFinalWaypoint = true;
		}
	}
}






float CClassicGroundMoveType::BreakingDistance(float speed)
{
	if (!decRate) {
		return 0.0f;
	}
	return math::fabs(speed*speed / decRate);
}

float3 CClassicGroundMoveType::Here()
{
	float3 motionDir = owner->speed;
	if (motionDir.SqLength2D() < float3::NORMALIZE_EPS) {
		return owner->midPos;
	} else {
		motionDir.Normalize();
		return owner->midPos + (motionDir * BreakingDistance(owner->speed.Length2D()));
	}
}






void CClassicGroundMoveType::StartEngine() {
	if (!pathId && !atGoal) {
		GetNewPath();

		if (pathId != 0) {
			pathFailures = 0;
			etaFailures = 0;

			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING);
			owner->script->StartMoving(false);
		} else {
			Fail();
		}
	}

	nextObstacleAvoidanceUpdate = gs->frameNum;
}

void CClassicGroundMoveType::StopEngine() {
	if (pathId != 0) {
		pathManager->DeletePath(pathId);
		pathId = 0;
		if (!atGoal) {
			waypoint = Here();
		}

		owner->script->StopMoving();
	}

	owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING);
	wantedSpeed = 0;
}

void CClassicGroundMoveType::Arrived()
{
	if (progressState == Active) {
		atGoal = true;

		StopEngine();

		if (owner->team == gu->myTeam) {
			Channels::General->PlayRandomSample(owner->unitDef->sounds.arrived, owner);
		}

		progressState = Done;
		owner->commandAI->SlowUpdate();
	}
}

void CClassicGroundMoveType::Fail()
{
	StopEngine();

	progressState = Failed;

	eventHandler.UnitMoveFailed(owner);
	eoh->UnitMoveFailed(*owner);
}



void CClassicGroundMoveType::CheckCollision()
{
	int2 newmp = owner->GetMapPos();

	if (newmp.x != owner->mapPos.x || newmp.y != owner->mapPos.y) {
		bool haveCollided = false;
		int retest = 0;

		do {
			const float zmove = (owner->mapPos.y + owner->zsize / 2) * SQUARE_SIZE;
			const float xmove = (owner->mapPos.x + owner->xsize / 2) * SQUARE_SIZE;

			if (math::fabs(owner->frontdir.x) > math::fabs(owner->frontdir.z)) {
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


bool CClassicGroundMoveType::CheckColH(int x, int y1, int y2, float xmove, int squareTestX)
{
	MoveDef* m = owner->moveDef;

	bool ret = false;

	for (int y = y1; y <= y2; ++y) {
		bool blocked = false;
		const int idx1 = y * mapDims.mapx + x;
		const int idx2 = y * mapDims.mapx + squareTestX;
		const BlockingMapCell& c = groundBlockingObjectMap->GetCellUnsafeConst(idx1);
		const BlockingMapCell& d = groundBlockingObjectMap->GetCellUnsafeConst(idx2);

		float3 posDelta = ZeroVector;

		if (!d.empty() && std::find(d.begin(), d.end(), owner) == d.end()) {
			continue;
		}

		for (auto it = c.begin(); it != c.end(); ++it) {
			CSolidObject* obj = *it;

			if (CMoveMath::IsNonBlocking(*m, obj, owner)) {
				continue;
			} else {
				blocked = true;

				if (obj->moveDef != NULL) {
					float part = owner->mass / (owner->mass + obj->mass * 2.0f);
					float3 dif = obj->pos - owner->pos;
					float dl = dif.Length();
					float colDepth = math::fabs(owner->pos.x - xmove);

					dif *= (dl != 0.0f)? (colDepth / dl): 0.0f;
					posDelta -= (dif * (1.0f - part));

					CUnit* u = static_cast<CUnit*>(obj);

					const int uAllyTeam = u->allyteam;
					const int oAllyTeam = owner->allyteam;
					const bool allied = (teamHandler->Ally(uAllyTeam, oAllyTeam) || teamHandler->Ally(oAllyTeam, uAllyTeam));

					if (!u->unitDef->pushResistant && !u->UsingScriptMoveType() && allied) {
						u->Move((dif * part), true);
						u->UpdateMidAndAimPos();
					}
				}

				if (!CMoveMath::CrushResistant(*m, obj)) {
					obj->Kill(owner, owner->frontdir * currentSpeed * 200.0f, true);
				}
			}
		}

		if (blocked) {
			if (groundBlockingObjectMap->GetCellUnsafeConst(y1 * mapDims.mapx + x).empty()) {
				posDelta.z -= (math::fabs(owner->pos.x - xmove) * 0.5f);
			}
			if (groundBlockingObjectMap->GetCellUnsafeConst(y2 * mapDims.mapx + x).empty()) {
				posDelta.z += (math::fabs(owner->pos.x - xmove) * 0.5f);
			}

			if (!((gs->frameNum + owner->id) & 31) && !owner->commandAI->unimportantMove) {
				CGameHelper::BuggerOff(owner->pos + owner->frontdir * owner->radius, owner->radius, true, false, owner->team, owner);
			}

			owner->Move(posDelta, true);
			owner->Move(RgtVector * (xmove - owner->pos.x), true);

			currentSpeed *= 0.97f;
			ret = true;
			break;
		}
	}

	return ret;
}

bool CClassicGroundMoveType::CheckColV(int y, int x1, int x2, float zmove, int squareTestY)
{
	MoveDef* m = owner->moveDef;

	bool ret = false;

	for (int x = x1; x <= x2; ++x) {
		bool blocked = false;
		const int idx1 = y * mapDims.mapx + x;
		const int idx2 = squareTestY * mapDims.mapx + x;
		const BlockingMapCell& c = groundBlockingObjectMap->GetCellUnsafeConst(idx1);
		const BlockingMapCell& d = groundBlockingObjectMap->GetCellUnsafeConst(idx2);

		float3 posDelta = ZeroVector;

		if (!d.empty() && std::find(d.begin(), d.end(), owner) == d.end()) {
			continue;
		}

		for (auto it = c.begin(); it != c.end(); ++it) {
			CSolidObject* obj = *it;

			if (CMoveMath::IsNonBlocking(*m, obj, owner)) {
				continue;
			} else {
				blocked = true;

				if (obj->moveDef != NULL) {
					float part = owner->mass / (owner->mass + obj->mass * 2.0f);
					float3 dif = obj->pos - owner->pos;
					float dl = dif.Length();
					float colDepth = math::fabs(owner->pos.z - zmove);

					dif *= (dl != 0.0f)? (colDepth / dl): 0.0f;
					posDelta -= (dif * (1.0f - part));

					CUnit* u = static_cast<CUnit*>(obj);

					const int uAllyTeam = u->allyteam;
					const int oAllyTeam = owner->allyteam;
					const bool allied = (teamHandler->Ally(uAllyTeam, oAllyTeam) || teamHandler->Ally(oAllyTeam, uAllyTeam));

					if (!u->unitDef->pushResistant && !u->UsingScriptMoveType() && allied) {
						u->Move((dif * part), true);
						u->UpdateMidAndAimPos();
					}
				}

				if (!CMoveMath::CrushResistant(*m, obj)) {
					obj->Kill(owner, owner->frontdir * currentSpeed * 200.0f, true);
				}
			}
		}

		if (blocked) {
			if (groundBlockingObjectMap->GetCellUnsafeConst(y * mapDims.mapx + x1).empty()) {
				posDelta.x -= (math::fabs(owner->pos.z - zmove) * 0.5f);
			}
			if (groundBlockingObjectMap->GetCellUnsafeConst(y * mapDims.mapx + x2).empty()) {
				posDelta.x += (math::fabs(owner->pos.z - zmove) * 0.5f);
			}

			if (!((gs->frameNum + owner->id) & 31) && !owner->commandAI->unimportantMove) {
				CGameHelper::BuggerOff(owner->pos + owner->frontdir * owner->radius, owner->radius, true, false, owner->team, owner);
			}

			owner->Move(posDelta, true);
			owner->Move(FwdVector * (zmove - owner->pos.z), true);

			currentSpeed *= 0.97f;
			ret = true;
			break;
		}
	}

	return ret;
}




void CClassicGroundMoveType::CreateLineTable()
{
	for (int yt = 0; yt < LINETABLE_SIZE; ++yt) {
		for (int xt = 0; xt < LINETABLE_SIZE; ++xt) {
			const float3 start(0.5f, 0.0f, 0.5f);
			const float3 to((xt - 5) + 0.5f, 0.0f, (yt - 5) + 0.5f);

			const float dx = to.x - start.x;
			const float dz = to.z - start.z;
			float xp = start.x;
			float zp = start.z;

			if (math::floor(start.x) == math::floor(to.x)) {
				if (dz > 0.0f) {
					for (int a = 1; a < math::floor(to.z); ++a)
						lineTable[yt][xt].push_back(int2(0, a));
				} else {
					for (int a = -1; a > math::floor(to.z); --a)
						lineTable[yt][xt].push_back(int2(0, a));
				}
			} else if (math::floor(start.z) == math::floor(to.z)) {
				if (dx > 0.0f) {
					for (int a = 1; a < math::floor(to.x); ++a)
						lineTable[yt][xt].push_back(int2(a, 0));
				} else {
					for (int a = -1; a > math::floor(to.x); --a)
						lineTable[yt][xt].push_back(int2(a, 0));
				}
			} else {
				float xn, zn;
				bool keepgoing = true;

				while (keepgoing) {
					if (dx > 0.0f) {
						xn = (math::floor(xp) + 1.0f - xp) / dx;
					} else {
						xn = (math::floor(xp)        - xp) / dx;
					}
					if (dz > 0.0f) {
						zn = (math::floor(zp) + 1.0f - zp) / dz;
					} else {
						zn = (math::floor(zp)        - zp) / dz;
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

					lineTable[yt][xt].push_back( int2(int(math::floor(xp)), int(math::floor(zp))) );
				}

				lineTable[yt][xt].pop_back();
				lineTable[yt][xt].pop_back();
			}
		}
	}
}

void CClassicGroundMoveType::DeleteLineTable()
{
	for (int yt = 0; yt < LINETABLE_SIZE; ++yt) {
		for (int xt = 0; xt < LINETABLE_SIZE; ++xt) {
			lineTable[yt][xt].clear();
		}
	}
}

void CClassicGroundMoveType::TestNewTerrainSquare()
{
	int newMoveSquareX = (int) owner->pos.x / (SQUARE_SIZE * 2);
	int newMoveSquareY = (int) owner->pos.z / (SQUARE_SIZE * 2);
	float3 newpos = owner->pos;

	if (newMoveSquareX != moveSquareX || newMoveSquareY != moveSquareY) {
		const MoveDef& md = *(owner->moveDef);
		const float cmod = CMoveMath::GetPosSpeedMod(md, moveSquareX * 2, moveSquareY * 2);

		if (math::fabs(owner->frontdir.x) < math::fabs(owner->frontdir.z)) {
			if (newMoveSquareX > moveSquareX) {
				const float nmod = CMoveMath::GetPosSpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.x = moveSquareX * SQUARE_SIZE * 2 + (SQUARE_SIZE * 2 - 0.01f);
					newMoveSquareX = moveSquareX;
				}
			} else if (newMoveSquareX < moveSquareX) {
				const float nmod = CMoveMath::GetPosSpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.x = moveSquareX * SQUARE_SIZE * 2 + 0.01f;
					newMoveSquareX = moveSquareX;
				}
			}
			if (newMoveSquareY > moveSquareY) {
				const float nmod = CMoveMath::GetPosSpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.z = moveSquareY * SQUARE_SIZE * 2 + (SQUARE_SIZE * 2 - 0.01f);
					newMoveSquareY = moveSquareY;
				}
			} else if (newMoveSquareY < moveSquareY) {
				const float nmod = CMoveMath::GetPosSpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.z = moveSquareY * SQUARE_SIZE * 2 + 0.01f;
					newMoveSquareY = moveSquareY;
				}
			}
		} else {
			if (newMoveSquareY > moveSquareY) {
				const float nmod = CMoveMath::GetPosSpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod>0.01f && nmod <= 0.01f) {
					newpos.z = moveSquareY * SQUARE_SIZE * 2 + (SQUARE_SIZE * 2 - 0.01f);
					newMoveSquareY = moveSquareY;
				}
			} else if (newMoveSquareY < moveSquareY) {
				const float nmod = CMoveMath::GetPosSpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.z = moveSquareY * SQUARE_SIZE * 2 + 0.01f;
					newMoveSquareY = moveSquareY;
				}
			}

			if (newMoveSquareX > moveSquareX) {
				const float nmod = CMoveMath::GetPosSpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.x = moveSquareX * SQUARE_SIZE * 2 + (SQUARE_SIZE * 2 - 0.01f);
					newMoveSquareX = moveSquareX;
				}
			} else if (newMoveSquareX < moveSquareX) {
				const float nmod = CMoveMath::GetPosSpeedMod(md, newMoveSquareX * 2, newMoveSquareY * 2);
				if (cmod > 0.01f && nmod <= 0.01f) {
					newpos.x = moveSquareX * SQUARE_SIZE * 2 + 0.01f;
					newMoveSquareX = moveSquareX;
				}
			}
		}

		if (newpos.SqDistance2D(owner->pos) > 4*SQUARE_SIZE*SQUARE_SIZE) {
			newMoveSquareX = (int) owner->pos.x / (SQUARE_SIZE * 2);
			newMoveSquareY = (int) owner->pos.z / (SQUARE_SIZE * 2);
		} else {
			owner->Move(newpos, false);
		}

		if (newMoveSquareX != moveSquareX || newMoveSquareY != moveSquareY) {
			moveSquareX = newMoveSquareX;
			moveSquareY = newMoveSquareY;
			terrainSpeed = CMoveMath::GetPosSpeedMod(md, moveSquareX * 2, moveSquareY * 2);

			int nwsx = (int) nextWaypoint.x / (SQUARE_SIZE * 2) - moveSquareX;
			int nwsy = (int) nextWaypoint.z / (SQUARE_SIZE * 2) - moveSquareY;
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
							CMoveMath::BLOCK_STRUCTURE |
							CMoveMath::BLOCK_MOBILE |
							CMoveMath::BLOCK_MOBILE_BUSY;

						if ((CMoveMath::IsBlocked(md, x, y, owner) & blockMask) ||
							CMoveMath::GetPosSpeedMod(md, x, y) <= 0.01f) {
							wpOk = false;
							break;
						}
					}
				}

				if (!wpOk || numIter > 6) {
					break;
				}

				GetNextWayPoint();

				nwsx = (int) nextWaypoint.x / (SQUARE_SIZE * 2) - moveSquareX;
				nwsy = (int) nextWaypoint.z / (SQUARE_SIZE * 2) - moveSquareY;
				++numIter;
			}
		}
	}
}

bool CClassicGroundMoveType::CheckGoalFeasability()
{
	float goalDist = goalPos.distance2D(owner->pos);

	int minx = (int) std::max(0.0f, (goalPos.x - goalDist) / (SQUARE_SIZE * 2));
	int minz = (int) std::max(0.0f, (goalPos.z - goalDist) / (SQUARE_SIZE * 2));
	int maxx = (int) std::min(float(mapDims.hmapx - 1), (goalPos.x + goalDist) / (SQUARE_SIZE * 2));
	int maxz = (int) std::min(float(mapDims.hmapy - 1), (goalPos.z + goalDist) / (SQUARE_SIZE * 2));

	MoveDef* md = owner->moveDef;

	float numBlocked = 0.0f;
	float numSquares = 0.0f;

	for (int z = minz; z <= maxz; ++z) {
		for (int x = minx; x <= maxx; ++x) {
			float3 pos(x * SQUARE_SIZE * 2, 0, z * SQUARE_SIZE * 2);
			if ((pos - goalPos).SqLength2D() < goalDist * goalDist) {
				int blockingType = CMoveMath::SquareIsBlocked(*md, x * 2, z * 2, owner);

				if ((blockingType & CMoveMath::BLOCK_STRUCTURE) || CMoveMath::GetPosSpeedMod(*md, x * 2, z * 2) < 0.01f) {
					numBlocked += 0.3f;
					numSquares += 0.3f;
				} else {
					numSquares += 1.0f;
					if (blockingType) {
						numBlocked += 1.0f;
					}
				}
			}
		}
	}

	if (numSquares > 0.0f) {
		if ((numBlocked / numSquares) > 0.4f) {
			return false;
		}
	}
	return true;
}



void CClassicGroundMoveType::KeepPointingTo(float3 pos, float distance, bool aggressive){
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
			if (
				(owner->heading != heading)
				&& !(owner->weapons.front()->TryTarget(SWeaponTarget(mainHeadingPos, true)))
			) {
				progressState = Active;
			}
		}
	}
}

void CClassicGroundMoveType::KeepPointingTo(CUnit* unit, float distance, bool aggressive) {
	KeepPointingTo(unit->pos, distance, aggressive);
}



void CClassicGroundMoveType::SetMainHeading() {
	if (useMainHeading && !owner->weapons.empty()) {
		float3 dir1 = owner->weapons.front()->mainDir;
		dir1.y = 0;
		dir1.Normalize();
		float3 dir2 = mainHeadingPos - owner->pos;
		dir2.y = 0;
		dir2.Normalize();

		ASSERT_SYNCED(dir1);
		ASSERT_SYNCED(dir2);

		if (dir2 != ZeroVector) {
			short heading = GetHeadingFromVector(dir2.x, dir2.z)
				- GetHeadingFromVector(dir1.x, dir1.z);

			ASSERT_SYNCED(heading);

			if (progressState == Active && owner->heading == heading) {
				owner->script->StopMoving();
				progressState = Done;
			} else if (progressState == Active) {
				ChangeHeading(heading);
			} else if (progressState != Active
			  && owner->heading != heading
			  && !owner->weapons.front()->TryTarget(SWeaponTarget(mainHeadingPos, true))) {
				progressState = Active;
				owner->script->StartMoving(false);
				ChangeHeading(heading);
			}
		}
	}
}

void CClassicGroundMoveType::SetMaxSpeed(float speed)
{
	maxSpeed = std::min(speed, maxSpeed);
	requestedSpeed = speed * 2.0f;
}

void CClassicGroundMoveType::LeaveTransport()
{
	oldPos = owner->pos + UpVector * 0.001f;
}

bool CClassicGroundMoveType::OnSlope(){
	return owner->unitDef->slideTolerance >= 1
		&& (CGround::GetSlope(owner->midPos.x, owner->midPos.z) >
		owner->moveDef->maxSlope*owner->unitDef->slideTolerance);
}



void CClassicGroundMoveType::AdjustPosToWaterLine()
{
	float wh = 0.0f;
	if (owner->unitDef->floatOnWater) {
		wh = CGround::GetHeightAboveWater(owner->pos.x, owner->pos.z);
		if (wh == 0.0f)
			wh = -owner->unitDef->waterline;
	} else {
		wh = CGround::GetHeightReal(owner->pos.x, owner->pos.z);
	}

	if (!(owner->IsFalling() || owner->IsFlying())) {
		owner->Move(UpVector * (wh - owner->pos.y), true);
	}
}

bool CClassicGroundMoveType::UpdateDirectControl()
{
	const CPlayer* myPlayer = gu->GetMyPlayer();
	const FPSUnitController& selfCon = myPlayer->fpsController;
	const FPSUnitController& unitCon = owner->fpsControlPlayer->fpsController;
	float turnSign = 0.0f;

	waypoint.x = owner->pos.x + owner->frontdir.x * 100.0f;
	waypoint.z = owner->pos.z + owner->frontdir.z * 100.0f;
	waypoint.ClampInBounds();

	if (unitCon.forward) {
		ChangeSpeed();

		owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING);
		owner->script->StartMoving(false);
	} else if (unitCon.back) {
		ChangeSpeed();

		owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING);
		owner->script->StartMoving(false);
	} else {
		ChangeSpeed();

		owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING);
		owner->script->StopMoving();
	}

	if (unitCon.left ) { ChangeHeading(owner->heading + turnRate); turnSign =  1.0f; }
	if (unitCon.right) { ChangeHeading(owner->heading - turnRate); turnSign = -1.0f; }

	if (selfCon.GetControllee() == owner) {
		camera->SetRotY(camera->GetRot().y + turnRate * turnSign * TAANG2RAD);
	}

	return false;
}

void CClassicGroundMoveType::UpdateOwnerPos()
{
	if (wantedSpeed > 0.0f || currentSpeed != 0.0f) {
		currentSpeed += deltaSpeed;
		owner->Move(flatFrontDir * currentSpeed, true);

		AdjustPosToWaterLine();
	}
}

#endif

