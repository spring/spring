/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "StrafeAirMoveType.h"
#include "Game/Players/Player.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Weapons/Weapon.h"
#include "System/SpringMath.h"
#include "System/Sync/HsiehHash.h"

CR_BIND_DERIVED(CStrafeAirMoveType, AAirMoveType, (nullptr))

CR_REG_METADATA(CStrafeAirMoveType, (
	CR_MEMBER(maneuverBlockTime),
	CR_MEMBER(maneuverState),
	CR_MEMBER(maneuverSubState),

	CR_MEMBER(loopbackAttack),
	CR_MEMBER(isFighter),

	CR_MEMBER(wingDrag),
	CR_MEMBER(wingAngle),
	CR_MEMBER(invDrag),
	CR_MEMBER(crashDrag),
	CR_MEMBER(frontToSpeed),
	CR_MEMBER(speedToFront),
	CR_MEMBER(myGravity),

	CR_MEMBER(maxBank),
	CR_MEMBER(maxPitch),
	CR_MEMBER(turnRadius),

	CR_MEMBER(maxAileron),
	CR_MEMBER(maxElevator),
	CR_MEMBER(maxRudder),
	CR_MEMBER(attackSafetyDistance),

	CR_MEMBER(crashAileron),
	CR_MEMBER(crashElevator),
	CR_MEMBER(crashRudder),

	CR_MEMBER(lastRudderPos),
	CR_MEMBER(lastElevatorPos),
	CR_MEMBER(lastAileronPos)
))



#define MEMBER_CHARPTR_HASH(memberName) HsiehHash(memberName, strlen(memberName),     0)
#define MEMBER_LITERAL_HASH(memberName) HsiehHash(memberName, sizeof(memberName) - 1, 0)

static const unsigned int BOOL_MEMBER_HASHES[] = {
	MEMBER_LITERAL_HASH(       "collide"),
	MEMBER_LITERAL_HASH( "useSmoothMesh"),
	MEMBER_LITERAL_HASH("loopbackAttack"),
};

static const unsigned int INT_MEMBER_HASHES[] = {
	MEMBER_LITERAL_HASH("maneuverBlockTime"),
};

static const unsigned int FLOAT_MEMBER_HASHES[] = {
	MEMBER_LITERAL_HASH(        "wantedHeight"),
	MEMBER_LITERAL_HASH(          "turnRadius"),
	MEMBER_LITERAL_HASH(             "accRate"),
	MEMBER_LITERAL_HASH(             "decRate"),
	MEMBER_LITERAL_HASH(              "maxAcc"), // synonym for accRate
	MEMBER_LITERAL_HASH(              "maxDec"), // synonym for decRate
	MEMBER_LITERAL_HASH(             "maxBank"),
	MEMBER_LITERAL_HASH(            "maxPitch"),
	MEMBER_LITERAL_HASH(          "maxAileron"),
	MEMBER_LITERAL_HASH(         "maxElevator"),
	MEMBER_LITERAL_HASH(           "maxRudder"),
	MEMBER_LITERAL_HASH("attackSafetyDistance"),
	MEMBER_LITERAL_HASH(           "myGravity"),
};

#undef MEMBER_CHARPTR_HASH
#undef MEMBER_LITERAL_HASH

extern AAirMoveType::GetGroundHeightFunc amtGetGroundHeightFuncs[6];
extern AAirMoveType::EmitCrashTrailFunc amtEmitCrashTrailFuncs[2];



static float TurnRadius(const float rawRadius, const float rawSpeed) {
	return (std::min(1000.0f, rawRadius * rawSpeed));
}


static float GetRudderDeflection(
	const CUnit* owner,
	const CUnit* /*collidee*/,
	const float3& pos,
	const float4& spd,
	const SyncedFloat3& rightdir,
	const SyncedFloat3& updir,
	const SyncedFloat3& frontdir,
	const float3& goalDir,
	float groundHeight,
	float wantedHeight,
	float maxRudder,
	float /*maxYaw*/,
	float goalDotRight,
	float goalDotFront,
	bool /*avoidCollision*/,
	bool isAttacking
) {
	float rudder = 0.0f;

	const float minGroundHeight = groundHeight + 15.0f * (1.0f + isAttacking);
	const float maxRudderSpeedf = std::max(0.01f, maxRudder * spd.w * (1.0f + isAttacking));

	const float minHeightMult = (pos.y > minGroundHeight);

	rudder -= (1.0f * minHeightMult * (goalDotRight < -maxRudderSpeedf)                   );
	rudder += (1.0f * minHeightMult * (goalDotRight >  maxRudderSpeedf)                   );
	rudder += (1.0f * minHeightMult * (goalDotRight /  maxRudderSpeedf) * (rudder == 0.0f));
	// we have to choose a direction in case our target is almost straight behind us
	rudder += (1.0f * minHeightMult * ((goalDotRight < 0.0f)? -0.01f: 0.01f) * (math::fabs(rudder) < 0.01f && goalDotFront < 0.0f));

	return rudder;
}

static float GetAileronDeflection(
	const CUnit* owner,
	const CUnit* /*collidee*/,
	const float3& pos,
	const float4& spd,
	const SyncedFloat3& rightdir,
	const SyncedFloat3& updir,
	const SyncedFloat3& frontdir,
	const float3& goalDir,
	float groundHeight,
	float wantedHeight,
	float maxAileron,
	float maxBank,
	float goalDotRight,
	float goalDotFront,
	bool /*avoidCollision*/,
	bool isAttacking
) {
	float aileron = 0.0f;

	// ailerons function less effectively at low (forward) speed
	const float maxAileronSpeedf  = std::max(0.01f, maxAileron * spd.w);
	const float maxAileronSpeedf2 = std::max(0.01f, maxAileronSpeedf * 4.0f);

	if (isAttacking) {
		const float minPredictedHeight = pos.y + spd.y * 60.0f * math::fabs(frontdir.y) + std::min(0.0f, updir.y * 1.0f) * (GAME_SPEED * 5);
		const float maxPredictedHeight = groundHeight + 60.0f + math::fabs(rightdir.y) * (GAME_SPEED * 5);

		const float minSpeedMult = (spd.w > 0.45f && minPredictedHeight > maxPredictedHeight);
		const float  goalBankDif = goalDotRight + rightdir.y * 0.2f;

		aileron += (1.0f * (       minSpeedMult) * (goalBankDif >  maxAileronSpeedf2)                    );
		aileron -= (1.0f * (       minSpeedMult) * (goalBankDif < -maxAileronSpeedf2)                    );
		aileron += (1.0f * (       minSpeedMult) * (goalBankDif /  maxAileronSpeedf2) * (aileron == 0.0f));

		aileron += (1.0f * (1.0f - minSpeedMult) *                     (rightdir.y > 0.0f) * (rightdir.y >  maxAileronSpeedf || frontdir.y < -0.7f));
		aileron += (1.0f * (1.0f - minSpeedMult) * (aileron == 0.0f) * (rightdir.y > 0.0f) * (rightdir.y /  maxAileronSpeedf                      ));

		aileron -= (1.0f * (1.0f - minSpeedMult) *                     (rightdir.y < 0.0f) * (rightdir.y < -maxAileronSpeedf || frontdir.y < -0.7f));
		aileron += (1.0f * (1.0f - minSpeedMult) * (aileron == 0.0f) * (rightdir.y < 0.0f) * (rightdir.y /  maxAileronSpeedf                      ));
	} else {
		const float minPredictedHeight = pos.y + spd.y * 10.0f;
		const float maxPredictedHeight = groundHeight + wantedHeight * 0.6f;

		const float  absRightDirY = math::fabs(rightdir.y);
		const float  goalBankDif  = goalDotRight + rightdir.y * 0.5f;

		const float   relBankMult = (absRightDirY < maxBank && maxAileronSpeedf2 > 0.0f);
		const float  minSpeedMult = (spd.w > 1.5f && minPredictedHeight > maxPredictedHeight);

		const float clampedBank = 1.0f;
		// using this directly does not function well at higher pitch-angles, interplay with GetElevatorDeflection
		// const float clampedBankAbs = math::fabs(Clamp(absRightDirY - maxBank, -1.0f, 1.0f));
		// const float clampedBank    = std::max(clampedBankAbs, 0.3f);

		aileron -= (clampedBank * (minSpeedMult) * (goalBankDif < -maxAileronSpeedf2 && rightdir.y <  maxBank));
		aileron += (clampedBank * (minSpeedMult) * (goalBankDif >  maxAileronSpeedf2 && rightdir.y > -maxBank));

		/// NB: these ignore maxBank
		aileron += (1.0f * (minSpeedMult) * (aileron == 0.0f) * (       relBankMult) * (goalBankDif / maxAileronSpeedf2));
		aileron -= (1.0f * (minSpeedMult) * (aileron == 0.0f) * (1.0f - relBankMult) * (rightdir.y < 0.0f && goalBankDif < 0.0f));
		aileron += (1.0f * (minSpeedMult) * (aileron == 0.0f) * (1.0f - relBankMult) * (rightdir.y > 0.0f && goalBankDif > 0.0f));

		// if right wing too high, roll right (cw)
		// if right wing too low, roll left (ccw)
		aileron += std::copysign(1.0f, float(rightdir.y)) * (1.0f - minSpeedMult) * (absRightDirY > maxAileronSpeedf);
	}

	return aileron;
}

static float GetElevatorDeflection(
	const CUnit* owner,
	const CUnit* collidee,
	const float3& pos,
	const float4& spd,
	const SyncedFloat3& rightdir,
	const SyncedFloat3& updir,
	const SyncedFloat3& frontdir,
	const float3& goalDir,
	float groundHeight,
	float wantedHeight,
	float maxElevator,
	float maxPitch,
	float goalDotRight,
	float goalDotFront,
	bool avoidCollision,
	bool isAttacking
) {
	float elevator = 0.0f;

	const float upside = (updir.y >= -0.3f) * 2.0f - 1.0f;
	const float speedMult = (spd.w >= 1.5f);

	if (collidee == nullptr || !avoidCollision)
		collidee = owner;

	if (isAttacking) {
		elevator += (upside * (1.0f - speedMult) * (frontdir.y < (-maxElevator * spd.w)));
		elevator -= (upside * (1.0f - speedMult) * (frontdir.y > ( maxElevator * spd.w)));

		const float posHeight = CGround::GetHeightAboveWater(pos.x + spd.x * 40.0f, pos.z + spd.z * 40.0f);
		const float difHeight = std::max(posHeight, groundHeight) + 60 - pos.y - frontdir.y * spd.w * 20.0f;
		const float  goalDotY = goalDir.dot(updir);

		const float maxElevatorSpeedf  = std::max(0.01f, maxElevator       * spd.w        );
		const float maxElevatorSpeedf2 = std::max(0.01f, maxElevatorSpeedf * spd.w * 20.0f);

		// const float ydirMult = (updir.dot(collidee->midPos - owner->midPos) > 0.0f);
		const float zdirMult = (frontdir.dot(collidee->pos + collidee->speed * 20.0f - pos - spd * 20.0f) < 0.0f);

		float minPitch = 0.0f;

		minPitch -= (1.0f * speedMult *                      (difHeight < -maxElevatorSpeedf2));
		minPitch += (1.0f * speedMult *                      (difHeight >  maxElevatorSpeedf2));
		minPitch += (1.0f * speedMult * (minPitch == 0.0f) * (difHeight /  maxElevatorSpeedf2));

		// busted collision avoidance
		// elevator += ((ydirMult * -2.0f + 1.0f) * zdirMult * speedMult * (collidee != owner));

		elevator -= (1.0f * speedMult *                      (goalDotY < -maxElevatorSpeedf) * (collidee == owner) * (1.0f - zdirMult));
		elevator += (1.0f * speedMult *                      (goalDotY >  maxElevatorSpeedf) * (collidee == owner) * (1.0f - zdirMult));
		elevator += (1.0f * speedMult * (elevator == 0.0f) * (goalDotY /  maxElevatorSpeedf) * (collidee == owner) * (1.0f - zdirMult));

		elevator = mix(elevator, minPitch * upside, (elevator * upside) < minPitch && spd.w >= 1.5f);
	} else {
		#if 0
		bool colliding = false;

		if (avoidCollision) {
			const float3 cvec = collidee->midPos - owner->midPos;
			const float3 svec = collidee->speed - spd;

			// pitch down or up based on relative direction to
			// a (collisionState == COLLISION_DIRECT) collidee
			// this usually just results in jittering, callers
			// disable it
			// FIXME? zdirDot < 0 means collidee is behind us
			const float ydirDot = updir.dot(cvec) * -1.0f;
			const float zdirDot = frontdir.dot(cvec + svec * 20.0f);

			elevator = Sign(ydirDot) * (spd.w > 0.8f) * (colliding = (zdirDot < 0.0f));
		}

		if (!colliding) {
		#endif
		{
			const float maxElevatorSpeedf = std::max(0.001f, maxElevator * 20.0f * spd.w * spd.w);

			const float posHeight = CGround::GetHeightAboveWater(pos.x + spd.x * 40.0f, pos.z + spd.z * 40.0f);
			const float difHeight = std::max(groundHeight, posHeight) + wantedHeight - pos.y - frontdir.y * spd.w * 20.0f;

			const float absFrontDirY = math::fabs(frontdir.y);
			const float clampedPitch = 1.0f;
			// using this directly does not function well at higher bank-angles
			// const float clampedPitchAbs = math::fabs(Clamp(absFrontDirY - maxPitch, -1.0f, 1.0f));
			// const float clampedPitch    = std::max(clampedPitchAbs, 0.3f);

			elevator -= (clampedPitch * (spd.w > 0.8f) * (difHeight < -maxElevatorSpeedf && frontdir.y  > -maxPitch));
			elevator += (clampedPitch * (spd.w > 0.8f) * (difHeight >  maxElevatorSpeedf && frontdir.y  <  maxPitch));

			elevator += (1.0f * (elevator == 0.0f) * (spd.w > 0.8f) * (difHeight / maxElevatorSpeedf) * (absFrontDirY < maxPitch));
		}

		elevator += (1.0f * (spd.w < 0.8f) * (frontdir.y < -0.10f));
		elevator -= (1.0f * (spd.w < 0.8f) * (frontdir.y >  0.15f));
	}

	return elevator;
}

static float3 GetControlSurfaceAngles(
	const CUnit* owner,
	const CUnit* collidee,
	const float3& pos,
	const float4& spd,
	const SyncedFloat3& rightdir,
	const SyncedFloat3& updir,
	const SyncedFloat3& frontdir,
	const float3& goalDir,
	const float3& yprInputLocks,
	const float3& maxBodyAngles, // .x := maxYaw, .y := maxPitch, .z := maxBank
	const float3& maxCtrlAngles, // .x := maxRudder, .y := maxElevator, .z := maxAileron
	const float3* prvCtrlAngles,
	float groundHeight,
	float wantedHeight,
	float goalDotRight,
	float goalDotFront,
	bool avoidCollision,
	bool isAttacking
) {
	float3 ctrlAngles;

	// yaw (rudder), pitch (elevator), roll (aileron)
	ctrlAngles.x = (yprInputLocks.x != 0.0f)? GetRudderDeflection  (owner, collidee,  pos, spd,  rightdir, updir, frontdir, goalDir,  groundHeight, wantedHeight,  maxCtrlAngles.x, maxBodyAngles.x,  goalDotRight, goalDotFront,  avoidCollision, isAttacking): 0.0f;
	ctrlAngles.y = (yprInputLocks.y != 0.0f)? GetElevatorDeflection(owner, collidee,  pos, spd,  rightdir, updir, frontdir, goalDir,  groundHeight, wantedHeight,  maxCtrlAngles.y, maxBodyAngles.y,  goalDotRight, goalDotFront,  avoidCollision, isAttacking): 0.0f;
	ctrlAngles.z = (yprInputLocks.z != 0.0f)? GetAileronDeflection (owner, collidee,  pos, spd,  rightdir, updir, frontdir, goalDir,  groundHeight, wantedHeight,  maxCtrlAngles.z, maxBodyAngles.z,  goalDotRight, goalDotFront,  avoidCollision, isAttacking): 0.0f;

	// let the previous control angles have some authority
	return (ctrlAngles * 0.6f + prvCtrlAngles[0] * 0.3f + prvCtrlAngles[1] * 0.1f);
}


static int SelectLoopBackManeuver(
	const SyncedFloat3& frontdir,
	const SyncedFloat3& rightdir,
	const float4& spd,
	float turnRadius,
	float groundDist
) {
	// do not start looping if already banked
	if (math::fabs(rightdir.y) > 0.05f)
		return CStrafeAirMoveType::MANEUVER_FLY_STRAIGHT;

	if (groundDist > TurnRadius(turnRadius, spd.w)) {
		if (math::fabs(frontdir.y) <= 0.2f && gsRNG.NextFloat() > 0.3f)
			return CStrafeAirMoveType::MANEUVER_IMMELMAN_INV;

	} else {
		if (frontdir.y > -0.2f && gsRNG.NextFloat() > 0.7f)
			return CStrafeAirMoveType::MANEUVER_IMMELMAN;

	}

	return CStrafeAirMoveType::MANEUVER_FLY_STRAIGHT;
}



CStrafeAirMoveType::CStrafeAirMoveType(CUnit* owner): AAirMoveType(owner)
{
	maneuverBlockTime = GAME_SPEED * 3;

	// creg
	if (owner == nullptr)
		return;

	assert(owner->unitDef != nullptr);

	isFighter = owner->unitDef->IsFighterAirUnit();
	loopbackAttack = owner->unitDef->canLoopbackAttack && isFighter;

	wingAngle = owner->unitDef->wingAngle;
	crashDrag = 1.0f - owner->unitDef->crashDrag;

	frontToSpeed = owner->unitDef->frontToSpeed;
	speedToFront = owner->unitDef->speedToFront;
	myGravity = math::fabs(owner->unitDef->myGravity);

	maxBank = owner->unitDef->maxBank;
	maxPitch = owner->unitDef->maxPitch;
	turnRadius = owner->unitDef->turnRadius;

	wantedHeight =
		(owner->unitDef->wantedHeight * 1.5f) +
		((gsRNG.NextFloat() - 0.3f) * 15.0f * (isFighter? 2.0f: 1.0f));

	orgWantedHeight = wantedHeight;

	maxAileron = owner->unitDef->maxAileron;
	maxElevator = owner->unitDef->maxElevator;
	maxRudder = owner->unitDef->maxRudder;
	attackSafetyDistance = 0.0f;

	maxRudder   *= (0.99f + gsRNG.NextFloat() * 0.02f);
	maxElevator *= (0.99f + gsRNG.NextFloat() * 0.02f);
	maxAileron  *= (0.99f + gsRNG.NextFloat() * 0.02f);
	accRate     *= (0.99f + gsRNG.NextFloat() * 0.02f);

	// magic constant ensures the same (0.25) average as urand() * urand()
	crashAileron   = 1.0f - Square(gsRNG.NextFloat() * 0.8660254037844386f);
	crashAileron  *= ((gsRNG.NextInt(2) == 1)? -1.0f: 1.0f);
	crashElevator  = gsRNG.NextFloat();
	crashRudder    = gsRNG.NextFloat() - 0.5f;

	SetMaxSpeed(maxSpeedDef);
}



bool CStrafeAirMoveType::Update()
{
	const float3 lastPos = owner->pos;
	const float4 lastSpd = owner->speed;

	AAirMoveType::Update();

	// need to additionally check that we are not crashing,
	// otherwise we might fall through the map when stunned
	// (the kill-on-impact code is not reached in that case)
	if ((owner->IsStunned() && !owner->IsCrashing()) || owner->beingBuilt) {
		UpdateAirPhysics({0.0f * lastRudderPos[0], lastElevatorPos[0], lastAileronPos[0], 0.0f}, ZeroVector);
		return (HandleCollisions(collide && !owner->beingBuilt && (aircraftState != AIRCRAFT_TAKEOFF)));
	}

	if (aircraftState != AIRCRAFT_CRASHING) {
		if (owner->UnderFirstPersonControl()) {
			SetState(AIRCRAFT_FLYING);

			const CPlayer* fpsPlayer = owner->fpsControlPlayer;
			const FPSUnitController& fpsCon = fpsPlayer->fpsController;

			float aileron = 0.0f;
			float elevator = 0.0f;

			elevator -= (1.0f * fpsCon.forward);
			elevator += (1.0f * fpsCon.back   );
			aileron  += (1.0f * fpsCon.right  );
			aileron  -= (1.0f * fpsCon.left   );

			UpdateAirPhysics({0.0f, elevator, aileron, 1.0f}, owner->frontdir);
			maneuverState = MANEUVER_FLY_STRAIGHT;

			return (HandleCollisions(collide && !owner->beingBuilt && (aircraftState != AIRCRAFT_TAKEOFF)));

		}
	}

	switch (aircraftState) {
		case AIRCRAFT_FLYING: {

			const CCommandQueue& cmdQue = owner->commandAI->commandQue;

			const bool isAttacking = (!cmdQue.empty() && (cmdQue.front()).GetID() == CMD_ATTACK);
			const bool keepAttacking = ((owner->curTarget.type == Target_Unit && !owner->curTarget.unit->isDead) || owner->curTarget.type == Target_Pos);

			/*
			const float brakeDistSq = Square(0.5f * lastSpd.SqLength2D() / decRate);
			const float goalDistSq = (goalPos - lastPos).SqLength2D();

			if (brakeDistSq >= goalDistSq && !owner->commandAI->HasMoreMoveCommands()) {
				SetState(AIRCRAFT_LANDING);
			} else
			*/
			{
				if (isAttacking && keepAttacking) {
					switch (owner->curTarget.type) {
						case Target_None: { } break;
						case Target_Unit: { SetGoal(owner->curTarget.unit->pos); } break;
						case Target_Pos:  { SetGoal(owner->curTarget.groundPos); } break;
						case Target_Intercept: { } break;
					}

					const bool goalInFront = ((goalPos - lastPos).dot(owner->frontdir) > 0.0f);
					const bool goalInRange = (goalPos.SqDistance(lastPos) < Square(owner->maxRange * 4.0f));

					// NOTE: UpdateAttack changes goalPos
					if (maneuverState != MANEUVER_FLY_STRAIGHT) {
						UpdateManeuver();
					} else if (goalInFront && goalInRange) {
						UpdateAttack();
					} else {
						if (UpdateFlying(wantedHeight, 1.0f) && !goalInFront && loopbackAttack) {
							// once yaw and roll are unblocked, semi-randomly decide to turn or loop
							const SyncedFloat3& rightdir = owner->rightdir;
							const SyncedFloat3& frontdir = owner->frontdir;

							const float altitude = CGround::GetHeightAboveWater(owner->pos.x, owner->pos.z) - lastPos.y;

							if ((maneuverState = SelectLoopBackManeuver(frontdir, rightdir, lastSpd, turnRadius, altitude)) == MANEUVER_IMMELMAN_INV)
								maneuverSubState = 0;
						}
					}
				} else {
					UpdateFlying(wantedHeight, 1.0f);
				}
			}
		} break;
		case AIRCRAFT_LANDED:
			UpdateLanded();
			break;
		case AIRCRAFT_LANDING:
			UpdateLanding();
			break;
		case AIRCRAFT_CRASHING: {
			// NOTE: the crashing-state can only be set (and unset) by scripts
			UpdateAirPhysics({crashRudder, crashElevator, crashAileron, 0.0f}, owner->frontdir);

			if ((CGround::GetHeightAboveWater(owner->pos.x, owner->pos.z) + 5.0f + owner->radius) > owner->pos.y)
				owner->ForcedKillUnit(nullptr, true, false);

			amtEmitCrashTrailFuncs[crashExpGenID != -1u](owner, crashExpGenID);
		} break;
		case AIRCRAFT_TAKEOFF:
			UpdateTakeOff();
			break;
		default:
			break;
	}

	if (lastSpd == ZeroVector && owner->speed != ZeroVector) { owner->script->StartMoving(false); }
	if (lastSpd != ZeroVector && owner->speed == ZeroVector) { owner->script->StopMoving(); }

	return (HandleCollisions(collide && !owner->beingBuilt && (aircraftState != AIRCRAFT_TAKEOFF)));
}




bool CStrafeAirMoveType::HandleCollisions(bool checkCollisions) {
	const float3& pos = owner->pos;

#ifdef DEBUG_AIRCRAFT
	switch (collisionState) {
		case COLLISION_NEARBY: {
			const int g = geometricObjects->AddLine(pos, lastCollidee->pos, 10, 1, 1);
			geometricObjects->SetColor(g, 0.2f, 1, 0.2f, 0.6f);
		} break;
		case COLLISION_DIRECT: {
			const int g = geometricObjects->AddLine(pos, lastCollidee->pos, 10, 1, 1);
			if (owner->frontdir.dot(lastCollidee->midPos + lastCollidee->speed * 20.0f - owner->midPos - spd * 20.0f) < 0) {
				geometricObjects->SetColor(g, 1, 0.2f, 0.2f, 0.6f);
			} else {
				geometricObjects->SetColor(g, 1, 1, 0.2f, 0.6f);
			}
		} break;
		default: {
		} break;
	}
#endif

	if (pos != oldPos) {
		oldPos = pos;

		bool hitBuilding = false;

		// check for collisions if not being built or not taking off
		if (checkCollisions) {
			// copy on purpose, since the below can call Lua
			QuadFieldQuery qfQuery;
			quadField.GetUnitsExact(qfQuery, pos, owner->radius + 6);

			for (CUnit* unit: *qfQuery.units) {
				const bool unloadingUnit  = ( unit->unloadingTransportId == owner->id);
				const bool unloadingOwner = (owner->unloadingTransportId ==  unit->id);
				const bool   loadingUnit  = ( unit->id == owner->loadingTransportId);
				const bool   loadingOwner = (owner->id ==  unit->loadingTransportId);

				if (unloadingUnit)
					unit->unloadingTransportId = -1;
				if (unloadingOwner)
					owner->unloadingTransportId = -1;

				if (loadingUnit || loadingOwner || unit == owner->transporter || unit->transporter != nullptr)
					continue;


				const float sqDist = (pos - unit->pos).SqLength();
				const float totRad = owner->radius + unit->radius;

				if (sqDist <= 0.1f || sqDist >= (totRad * totRad))
					continue;


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

				if (unit->immobile) {
					const float damage = ((unit->speed - owner->speed) * 0.1f).SqLength();

					owner->Move(-dif * (dist - totRad), true);
					owner->SetVelocity(owner->speed * 0.99f);

					if (modInfo.allowUnitCollisionDamage) {
						owner->DoDamage(DamageArray(damage), ZeroVector, nullptr, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);
						unit->DoDamage(DamageArray(damage), ZeroVector, nullptr, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);
					}

					hitBuilding = true;
				} else {
					const float part = owner->mass / (owner->mass + unit->mass);
					const float damage = ((unit->speed - owner->speed) * 0.1f).SqLength();

					owner->Move(-dif * (dist - totRad) * (1 - part), true);
					owner->SetVelocity(owner->speed * 0.99f);

					if (!unit->UsingScriptMoveType())
						unit->Move(dif * (dist - totRad) * (part), true);

					if (modInfo.allowUnitCollisionDamage) {
						owner->DoDamage(DamageArray(damage), ZeroVector, nullptr, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);
						unit->DoDamage(DamageArray(damage), ZeroVector, nullptr, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);
					}
				}
			}

			// update speed.w
			owner->SetSpeed(owner->speed);
		}

		if (hitBuilding && owner->IsCrashing()) {
			// if crashing and we hit a building, die right now
			// rather than waiting until we are close enough to
			// the ground
			owner->ForcedKillUnit(nullptr, true, false);
			return true;
		}

		if (pos.x < 0.0f) {
			owner->Move( RgtVector * 1.5f, true);
		} else if (pos.x > float3::maxxpos) {
			owner->Move(-RgtVector * 1.5f, true);
		}

		if (pos.z < 0.0f) {
			owner->Move( FwdVector * 1.5f, true);
		} else if (pos.z > float3::maxzpos) {
			owner->Move(-FwdVector * 1.5f, true);
		}

		return true;
	}

	return false;
}



void CStrafeAirMoveType::SlowUpdate()
{
	// note: NOT AAirMoveType::SlowUpdate
	AMoveType::SlowUpdate();
}



void CStrafeAirMoveType::UpdateManeuver()
{
	const float speedf = owner->speed.w;

	switch (maneuverState) {
		case MANEUVER_IMMELMAN: {
			float aileron = 0.0f;
			float elevator = 1.0f * (math::fabs(owner->rightdir.y) < maxAileron * 3.0f * speedf || owner->updir.y < 0.0f);

			aileron += (1.0f * (owner->updir.y > 0.0f) * (owner->rightdir.y >  maxAileron * speedf));
			aileron -= (1.0f * (owner->updir.y > 0.0f) * (owner->rightdir.y < -maxAileron * speedf));

			UpdateAirPhysics({0.0f, elevator, aileron, 1.0f}, owner->frontdir);

			if ((owner->updir.y < 0.0f && owner->frontdir.y < 0.0f) || speedf < 0.8f)
				maneuverState = MANEUVER_FLY_STRAIGHT;

			// [?] some seem to report that the "unlimited altitude" thing is because of these maneuvers
			if ((owner->pos.y - CGround::GetApproximateHeight(owner->pos.x, owner->pos.z)) > (wantedHeight * 4.0f))
				maneuverState = MANEUVER_FLY_STRAIGHT;

		} break;

		case MANEUVER_IMMELMAN_INV: {
			// inverted Immelman
			float aileron = 0.0f;
			float elevator = 0.0f;

			aileron -= (1.0f * (maneuverSubState == 0) * (owner->rightdir.y >= 0.0f));
			aileron += (1.0f * (maneuverSubState == 0) * (owner->rightdir.y <  0.0f));

			if (owner->frontdir.y < -0.7f)
				maneuverSubState = 1;
			if (maneuverSubState == 1 || owner->updir.y < 0.0f)
				elevator = 1.0f;

			UpdateAirPhysics({0.0f, elevator, aileron, 1.0f}, owner->frontdir);

			if ((owner->updir.y > 0.0f && owner->frontdir.y > 0.0f && maneuverSubState == 1) || speedf < 0.2f)
				maneuverState = MANEUVER_FLY_STRAIGHT;

		} break;

		default: {
			UpdateAirPhysics({0.0f, 0.0f, 0.0f, 1.0f}, owner->frontdir);
			maneuverState = MANEUVER_FLY_STRAIGHT;
		} break;
	}
}



void CStrafeAirMoveType::UpdateAttack()
{
	if (!isFighter) {
		UpdateFlying(wantedHeight, 1.0f);
		return;
	}

	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	const SyncedFloat3& rightdir = owner->rightdir;
	const SyncedFloat3& frontdir = owner->frontdir;
	const SyncedFloat3& updir    = owner->updir;

	if (spd.w < 0.01f) {
		UpdateAirPhysics({0.0f, 0.0f, 0.0f, 1.0f}, owner->frontdir);
		return;
	}

	if (((gs->frameNum + owner->id) & 3) == 0)
		CheckForCollision();

	      float3 rightDir2D = rightdir;
	const float3 difGoalPos = (goalPos - oldGoalPos) * SQUARE_SIZE;

	oldGoalPos = goalPos;
	goalPos += difGoalPos;

	const float gHeightAW = CGround::GetHeightAboveWater(pos.x, pos.z);
	const float goalDist = pos.distance(goalPos);
	const float3 goalDir = (goalDist > 0.0f)?
		(goalPos - pos) / goalDist:
		ZeroVector;

	// if goal too close, stop dive and resume flying at normal desired height
	// to avoid colliding with target, evade blast, friendly and enemy fire, etc.
	if (goalDist < attackSafetyDistance) {
		UpdateFlying(wantedHeight, 1.0f);
		return;
	}

	float goalDotRight = goalDir.dot(rightDir2D.Normalize2D());

	const float goalDotFront   = goalDir.dot(frontdir);
	const float goalDotFront01 = goalDotFront * 0.5f + 0.501f; // [0,1]

	if (goalDotFront01 != 0.0f)
		goalDotRight /= goalDotFront01;

	{
		const float3  maxBodyAngles    = {0.0f, maxPitch, maxBank};
		const float3  maxCtrlAngles    = {maxRudder, maxElevator, maxAileron};
		const float3  prvCtrlAngles[2] = {{lastRudderPos[0], lastElevatorPos[0], lastAileronPos[0]}, {lastRudderPos[1], lastElevatorPos[1], lastAileronPos[1]}};
		const float3& curCtrlAngles    = GetControlSurfaceAngles(owner, lastCollidee,  pos, spd,  rightdir, updir, frontdir, goalDir,  OnesVector, maxBodyAngles, maxCtrlAngles, prvCtrlAngles,  gHeightAW, wantedHeight,  goalDotRight, goalDotFront,  false && collisionState == COLLISION_DIRECT, true);

		const CUnit* attackee = owner->curTarget.unit;

		// limit thrust when in range of (air) target and directly behind it
		const float  rangeLim = goalDist / owner->maxRange;
		const float  angleLim = 1.0f - goalDotFront * 0.7f;
		const float thrustLim = ((attackee == nullptr) || attackee->unitDef->IsGroundUnit())? 1.0f: std::min(1.0f, rangeLim + angleLim);

		UpdateAirPhysics({curCtrlAngles.x, curCtrlAngles.y, curCtrlAngles.z, thrustLim}, frontdir);
	}
}



bool CStrafeAirMoveType::UpdateFlying(float wantedHeight, float wantedThrottle)
{
	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	const SyncedFloat3& rightdir = owner->rightdir;
	const SyncedFloat3& frontdir = owner->frontdir;
	const SyncedFloat3& updir    = owner->updir;

	// NOTE:
	//   turnRadius is often way too small, but cannot calculate one
	//   because we have no turnRate (and unitDef->turnRate can be 0)
	//   --> would lead to infinite circling without adjusting goal
	const float3 goalVec = goalPos - pos;

	const float goalDist2D = std::max(0.001f, goalVec.Length2D());
	// const float goalDist3D = std::max(0.001f, goalVec.Length());

	const float3 goalDir2D = goalVec / goalDist2D;
	// const float3 goalDir3D = goalVec / goalDist3D;

	const float3 rightDir2D = (rightdir * XZVector).Normalize2D();

	if (((gs->frameNum + owner->id) & 3) == 0)
		CheckForCollision();


	// RHS is needed for moving targets (when called by UpdateAttack)
	// yaw and roll have to be unconditionally unblocked after a certain
	// time or aircraft can fly straight forever e.g. if their target is
	// another chasing aircraft
	const bool allowUnlockYawRoll = (goalDist2D >= TurnRadius(turnRadius, spd.w) || goalVec.dot(owner->frontdir) > 0.0f);
	const bool forceUnlockYawRoll = ((gs->frameNum - owner->lastFireWeapon) >= maneuverBlockTime);


	// do not check if the plane can be submerged here,
	// since it'll cause ground collisions later on (?)
	const float groundHeight = amtGetGroundHeightFuncs[5 * UseSmoothMesh()](pos.x, pos.z);

	// If goal-distance is half turn radius then turn if
	// goal-position is not in front within a ~45 degree
	// arc.
	// If goal-position is behind us and goal-distance is
	// less than our turning radius, turn the other way.
	// These conditions prevent becoming stuck in a small
	// circle around goal-position.
	const float nearGoal = ((goalDist2D < turnRadius * 0.5f && goalDir2D.dot(frontdir) < 0.7f) || (goalDist2D < turnRadius && goalDir2D.dot(frontdir) < -0.1f));
	const float turnFlip = ((!owner->UnderFirstPersonControl() || owner->fpsControlPlayer->fpsController.mouse2) * -2.0f + 1.0f);

	// if nearGoal=0, multiply by 1
	// if nearGoal=1, multiply by turnFlip
	const float goalDotFront = goalDir2D.dot(frontdir);
	const float goalDotRight = goalDir2D.dot(rightDir2D) * ((1.0f - nearGoal) + (nearGoal * turnFlip));

	#if 0
	// try to steer (yaw) away from nearby aircraft in front of us
	if (lastCollidee != nullptr) {
		const float3 collideeVec = lastCollidee->pos - pos;

		const float collideeDist = collideeVec.Length();
		const float relativeDist = (collideeDist > 0.0f)?
			std::max(1200.0f, goalDist2D) / collideeDist * 0.036f:
			0.0f;

		const float3 collideeDir = (collideeDist > 0.0f)?
			(collideeVec / collideeDist):
			ZeroVector;

		goalDotRight -= (collideeDir.dot(rightdir) * relativeDist * std::max(0.0f, collideeDir.dot(frontdir)));
	}
	#endif

	const float3  yprInputLocks    = (XZVector * float(allowUnlockYawRoll || forceUnlockYawRoll)) + UpVector;
	const float3  maxBodyAngles    = {0.0f, maxPitch, maxBank};
	const float3  maxCtrlAngles    = {maxRudder, maxElevator, maxAileron};
	const float3  prvCtrlAngles[2] = {{lastRudderPos[0], lastElevatorPos[0], lastAileronPos[0]}, {lastRudderPos[1], lastElevatorPos[1], lastAileronPos[1]}};
	const float3& curCtrlAngles    = GetControlSurfaceAngles(owner, lastCollidee,  pos, spd,  rightdir, updir, frontdir, goalDir2D,  yprInputLocks, maxBodyAngles, maxCtrlAngles, prvCtrlAngles,  groundHeight, wantedHeight,  goalDotRight, goalDotFront,  false && collisionState == COLLISION_DIRECT, false);

	UpdateAirPhysics({curCtrlAngles, wantedThrottle}, owner->frontdir);

	return (allowUnlockYawRoll || forceUnlockYawRoll);
}


static float GetVTOLAccelerationSign(float curHeight, float wtdHeight, float vertSpeed) {
	const float nxtHeight = curHeight + vertSpeed * 20.0f;
	const float tgtHeight = wtdHeight * 1.02f;

	return (Sign<float>(nxtHeight < tgtHeight));
}

void CStrafeAirMoveType::UpdateTakeOff()
{
	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	wantedHeight = orgWantedHeight;

	SyncedFloat3& rightdir = owner->rightdir;
	SyncedFloat3& frontdir = owner->frontdir;
	SyncedFloat3& updir    = owner->updir;

	const float3 goalDir = (goalPos - pos).Normalize();

	const float yawWeight = maxRudder * spd.y;
	const float dirWeight = Clamp(goalDir.dot(rightdir), -1.0f, 1.0f);
	// this tends to alternate between -1 and +1 when goalDir and rightdir are ~orthogonal
	// const float yawSign = Sign(goalDir.dot(rightdir));
	const float currentHeight = pos.y - amtGetGroundHeightFuncs[canSubmerge](pos.x, pos.z);
	const float minAccHeight = wantedHeight * 0.4f;

	frontdir += (rightdir * dirWeight * yawWeight);
	frontdir.Normalize();
	rightdir = frontdir.cross(updir);

	owner->SetVelocity(spd + (UpVector * accRate * GetVTOLAccelerationSign(currentHeight, wantedHeight, spd.y)));
	owner->SetVelocity(spd + (owner->frontdir * accRate * (currentHeight > minAccHeight)));

	// initiate forward motion before reaching wantedHeight
	// normally aircraft start taking off from the ground below wantedHeight,
	// but state can also change to LANDING via StopMoving and then again to
	// TAKEOFF (via StartMoving) while still in mid-air
	if (currentHeight > wantedHeight || spd.SqLength2D() >= Square(maxWantedSpeed * 0.8f))
		SetState(AIRCRAFT_FLYING);

	owner->SetVelocityAndSpeed(spd * invDrag);
	owner->Move(spd, true);
	owner->SetHeadingFromDirection();
	owner->UpdateMidAndAimPos();
}


void CStrafeAirMoveType::UpdateLanding()
{
	const float3 pos = owner->pos;

	SyncedFloat3& rightdir = owner->rightdir;
	SyncedFloat3& frontdir = owner->frontdir;
	SyncedFloat3& updir    = owner->updir;

	if (!HaveLandingPos()) {
		reservedLandingPos = FindLandingPos(pos + frontdir * BrakingDistance(maxSpeed, decRate));

		// if spot is valid, mark it on the blocking-map
		// so other aircraft can not claim the same spot
		if (HaveLandingPos()) {
			wantedHeight = 0.0f;

			owner->Move(reservedLandingPos, false);
			owner->Block();
			owner->Move(pos, false);
			// Block updates the blocking-map position (mapPos)
			// via GroundBlockingObjectMap (which calculates it
			// from object->pos) and UnBlock always uses mapPos
			// --> we cannot block in one part of the map *and*
			// unblock in another
			// owner->UnBlock();
		} else {
			goalPos.ClampInBounds();
			UpdateFlying(wantedHeight, 1.0f);
			return;
		}
	}

	SetGoal(reservedLandingPos);
	UpdateLandingHeight(wantedHeight);

	const float3 reservedLandingPosDir = reservedLandingPos - pos;
	const float3 brakeSpot = pos + frontdir * BrakingDistance(owner->speed.Length2D(), decRate);

	if (brakeSpot.SqDistance2D(reservedLandingPos) > landRadiusSq) {
		// If we're not going to land inside the landRadius, keep flying.
		const float tempWantedHeight = wantedHeight;

		UpdateFlying(wantedHeight = orgWantedHeight, 1.0f);

		wantedHeight = tempWantedHeight;
		return;
	}

	updir -= ((rightdir * 0.02f) * (rightdir.y < -0.01f));
	updir += ((rightdir * 0.02f) * (rightdir.y >  0.01f));

	frontdir += ((updir * 0.02f) * (frontdir.y < -0.01f));
	frontdir -= ((updir * 0.02f) * (frontdir.y >  0.01f));

	frontdir += ((rightdir * 0.02f) * (rightdir.dot(reservedLandingPosDir) >  0.01f));
	frontdir -= ((rightdir * 0.02f) * (rightdir.dot(reservedLandingPosDir) < -0.01f));

	{
		//A Mangled UpdateAirPhysics
		float4& spd = owner->speed;

		float frontSpeed = spd.dot2D(frontdir);

		if (frontSpeed > 0.0f) {
			//Slow down before vertical landing
			owner->SetVelocity(frontdir * (spd.Length2D() * invDrag - decRate));

			//Calculate again for next check
			frontSpeed = spd.dot2D(frontdir);
		}

		if (frontSpeed < 0.0f)
			owner->SetVelocityAndSpeed(UpVector * owner->speed);

		const float landPosDistXZ = reservedLandingPosDir.Length2D() + 0.1f;
		const float landPosDistY = reservedLandingPosDir.y;
		const float curSpeedXZ = spd.Length2D();

		owner->SetVelocity(spd - UpVector * spd.y);

		if (landPosDistXZ > curSpeedXZ && curSpeedXZ > 0.1f) {
			owner->SetVelocity(spd + UpVector * std::max(-altitudeRate, landPosDistY * curSpeedXZ / landPosDistXZ));
		} else {
			const float localAltitude = pos.y - amtGetGroundHeightFuncs[canSubmerge](pos.x, pos.z);
			const float deltaAltitude = wantedHeight - localAltitude;

			if (deltaAltitude < 0.0f)
				owner->SetVelocity(spd + UpVector * std::max(-altitudeRate, deltaAltitude));
		}

		owner->SetSpeed(spd);

		owner->Move(owner->speed, true);
		owner->UpdateDirVectors(owner->IsOnGround());
		owner->UpdateMidAndAimPos();
	}

	AAirMoveType::UpdateLanding();
}



void CStrafeAirMoveType::UpdateAirPhysics(const float4& controlInputs, const float3& thrustVector)
{
	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	SyncedFloat3& rightdir = owner->rightdir;
	SyncedFloat3& frontdir = owner->frontdir;
	SyncedFloat3& updir    = owner->updir;

	const float groundHeight = CGround::GetHeightAboveWater(pos.x, pos.z);
	const float linearSpeed = spd.w;

	const float   rudder = controlInputs.x;
	const float elevator = controlInputs.y;
	const float  aileron = controlInputs.z;
	      float throttle = controlInputs.w;

	bool nextPosInBounds = true;

	{
		// keep previous two inputs to reduce temporal jitter
		lastRudderPos[1] = lastRudderPos[0];
		lastRudderPos[0] = rudder;
		lastElevatorPos[1] = lastElevatorPos[0];
		lastElevatorPos[0] = elevator;
		lastAileronPos[1] = lastAileronPos[0];
		lastAileronPos[0] = aileron;
	}

	if (owner->UnderFirstPersonControl()) {
		if ((pos.y - groundHeight) > wantedHeight * 1.2f)
			throttle = Clamp(throttle, 0.0f, 1.0f - (pos.y - groundHeight - wantedHeight * 1.2f) / wantedHeight);

		// check next position given current (unadjusted) pos and speed
		nextPosInBounds = (pos + spd).IsInBounds();
	}

	// apply gravity only when in the air
	// unlike IRL, thrust points forward
	owner->SetVelocity(spd + UpVector * (mapInfo->map.gravity * myGravity) * int((owner->midPos.y - owner->radius) > groundHeight));
	owner->SetVelocity(spd + (thrustVector * accRate * throttle));
	owner->SetVelocity(spd * ((aircraftState == AIRCRAFT_CRASHING)? crashDrag: invDrag));

	if (!owner->IsStunned()) {
		const float3 speedDir = spd / (linearSpeed + 0.1f);
		const float3 wingDir = updir * (1.0f - wingAngle) - frontdir * wingAngle;

		const float3 yprDeltas = {
			rudder   * maxRudder   * linearSpeed,
			elevator * maxElevator * linearSpeed,
			aileron  * maxAileron  * linearSpeed,
		};
		const float3 yprScales = {
			#if 0
			// do not want locks when crashing or during "special" maneuvers (Immelman, etc)
			// when both pitch and roll are locked (e.g during a climbing turn), yawing also
			// becomes impossible since yaw motion can cause |rightdir.y| to exceed maxBank
			(math::fabs((((frontdir + rightdir * yprDeltas.x).Normalize()).cross(updir)).y) < maxBank), // yawing changes rightdir if pitched or banked
			(math::fabs(frontdir.y + yprDeltas.y) < maxPitch), // pitching up (ypr.y=+1) increases frontdir.y
			(math::fabs(rightdir.y - yprDeltas.z) < maxBank ), // rolling left (ypr.z=-1) increases rightdir.y
			#endif
			#if 0
			1.0f,
			(math::fabs(rightdir.y) < 0.1f || math::fabs(frontdir.y + yprDeltas.y) < math::fabs(frontdir.y)), // allow pitch when not banked
			(math::fabs(frontdir.y) < 0.1f || math::fabs(rightdir.y - yprDeltas.z) < math::fabs(rightdir.y)), // allow roll when not pitched
			#endif
			1.0f,
			1.0f,
			1.0f,
		};

		frontdir += (rightdir * yprDeltas.x * yprScales.x); // yaw
		frontdir += (updir    * yprDeltas.y * yprScales.y); // pitch
		updir    += (rightdir * yprDeltas.z * yprScales.z); // roll (via updir, new rightdir derives from it)
		frontdir += ((speedDir - frontdir) * frontToSpeed);

		owner->SetVelocity(spd - (wingDir * wingDir.dot(spd) * wingDrag));
		owner->SetVelocity(spd + ((frontdir * linearSpeed - spd) * speedToFront));
		owner->SetVelocity(spd * (1 - int(owner->beingBuilt && aircraftState == AIRCRAFT_LANDED)));
	}

	if (nextPosInBounds)
		owner->Move(spd, true);

	// prevent aircraft from gaining unlimited altitude
	owner->Move(UpVector * (Clamp(pos.y, groundHeight, readMap->GetCurrMaxHeight() + owner->unitDef->wantedHeight * 5.0f) - pos.y), true);

	// bounce away on ground collisions (including water surface)
	// NOTE:
	//   as soon as we get stunned, Update calls UpdateAirPhysics
	//   and pops us into the air because of the ground-collision
	//   logic --> check if we are already on the ground first
	//
	//   impulse from weapon impacts can add speed and cause us
	//   to start bouncing with ever-increasing amplitude while
	//   stunned, so the same applies there
	if (modInfo.allowAircraftToHitGround) {
		const bool groundContact = (groundHeight > (owner->midPos.y - owner->radius));
		const bool handleContact = (aircraftState != AIRCRAFT_LANDED && aircraftState != AIRCRAFT_TAKEOFF);

		if (groundContact && handleContact) {
			const float3  groundOffset = UpVector * (groundHeight - (owner->midPos.y - owner->radius) + 0.01f);
			const float3& groundNormal = CGround::GetNormal(pos.x, pos.z);

			const float impactSpeed = -spd.dot(groundNormal) * int(1 - owner->IsStunned());

			owner->Move(groundOffset, true);

			if (impactSpeed > 0.0f) {
				// fix for mantis #1355
				// aircraft could get stuck in the ground and never recover (on takeoff
				// near map edges) where their forward speed wasn't allowed to build up
				// therefore add a vertical component help get off the ground if |speed|
				// is below a certain threshold
				if (spd.SqLength() > (0.09f * Square(owner->unitDef->speed))) {
					owner->SetVelocity(spd * 0.95f);
				} else {
					owner->SetVelocity(spd + (UpVector * impactSpeed));
				}

				owner->SetVelocity(spd + (groundNormal * impactSpeed * 1.5f));

				updir = groundNormal - frontdir * 0.1f;
				rightdir = frontdir.cross(updir.Normalize());
				frontdir = updir.cross(rightdir.Normalize());
			}
		}
	}

	frontdir.Normalize();
	rightdir = frontdir.cross(updir);
	rightdir.Normalize();
	updir = rightdir.cross(frontdir);
	updir.Normalize();

	owner->SetSpeed(spd);
	owner->UpdateMidAndAimPos();
	owner->SetHeadingFromDirection();
}



void CStrafeAirMoveType::SetState(AAirMoveType::AircraftState newState)
{
	// this state is only used by CHoverAirMoveType, so we should never enter it
	assert(newState != AIRCRAFT_HOVERING);

	// once in crashing, we should never change back into another state
	if (aircraftState == AIRCRAFT_CRASHING && newState != AIRCRAFT_CRASHING)
		return;

	if (newState == aircraftState)
		return;

	// make sure we only go into takeoff mode when landed
	if (aircraftState == AIRCRAFT_LANDED) {
		//assert(newState == AIRCRAFT_TAKEOFF);
		aircraftState = AIRCRAFT_TAKEOFF;
	} else {
		aircraftState = newState;
	}

	owner->UpdatePhysicalStateBit(CSolidObject::PSTATE_BIT_MOVING, (aircraftState != AIRCRAFT_LANDED));
	owner->useAirLos = (aircraftState != AIRCRAFT_LANDED);

	switch (aircraftState) {
		case AIRCRAFT_CRASHING:
			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_CRASHING);
			break;

		case AIRCRAFT_TAKEOFF:
			// should be in the STATE_FLYING case, but these aircraft
			// take forever to reach it reducing factory cycle-times
			owner->Activate();
			owner->UnBlock();
			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
			break;

		case AIRCRAFT_LANDED:
			// FIXME already inform commandAI in AIRCRAFT_LANDING!
			owner->commandAI->StopMove();
			owner->Deactivate();
			// missing? not really
			// owner->Block();
			owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
			break;

		default:
			break;
	}

	if (aircraftState != AIRCRAFT_LANDING) {
		// don't need a reserved position anymore
		ClearLandingPos();
		wantedHeight = orgWantedHeight;
	}
}



float3 CStrafeAirMoveType::FindLandingPos(float3 landPos)
{
	if (((landPos.y = CGround::GetHeightReal(landPos)) < 0.0f) && ((mapInfo->water.damage > 0.0f) || !(floatOnWater || canSubmerge)))
		return -OnesVector;

	const int2 os = {owner->xsize, owner->zsize};
	const int2 mp = owner->GetMapPos(landPos = landPos.cClampInBounds());

	for (int z = mp.y; z < (mp.y + os.y); z++) {
		for (int x = mp.x; x < (mp.x + os.x); x++) {
			if (groundBlockingObjectMap.GroundBlocked(x, z, owner))
				return -OnesVector;
		}
	}

	// FIXME: better compare against ud->maxHeightDif?
	if (CGround::GetSlope(landPos.x, landPos.z) > 0.03f)
		return -OnesVector;

	// update this only for valid landing positions
	landRadiusSq = landPos.SqDistance(owner->pos);

	return landPos;
}


float CStrafeAirMoveType::BrakingDistance(float speed, float rate) const
{
	// Denote:
	//		a_i: Speed after i frames
	//		s_i: Distance passed after i frames
	//		d: decRate
	//		r: invDrag
	//
	// Braking is done according to the formula:
	//		a_i = a_i-1 * r - d
	// Which is equivalent to:
	//		a_i = a_0 * r^i - d * (r^i - 1) / (r - 1)
	// Distance is the sum of a_j for 0<j<=i and is equal to:
	//		s_i = a_0 * r * (r^i - 1) / (r - 1) - d * (r * (r^i - 1) / (r - 1) - i) / (r - 1)
	// The number of frames until speed is 0 can be calculated with:
	//		n = floor(log(d / (d - a_0 * (r - 1))) / math::log(r))
	//
	// Using all these you can know how much distance your plane will move
	// until it is stopped
	const float d = rate;
	const float r = invDrag;

	const int n = math::floor(math::log(d / (d - speed * (r - 1))) / math::log(r));
	const float r_n = math::pow(r, n);

	const float dist = speed * r * (r_n - 1) / (r - 1) - d * (r * (r_n - 1) / (r - 1) - n) / (r - 1);

	return dist;
}


void CStrafeAirMoveType::SetMaxSpeed(float speed)
{
	maxSpeed = speed;

	if (accRate != 0.0f && maxSpeed != 0.0f) {
		// meant to set the drag such that the maxspeed becomes what it should be
		float drag = 1.0f / (maxSpeed * 1.1f / accRate);
		drag -= (wingAngle * wingAngle * wingDrag);
		drag = std::min(1.0f, std::max(0.0f, drag));
		invDrag = 1.0f - drag;
	}
}



void CStrafeAirMoveType::StartMoving(float3 gpos, float goalRadius)
{
	StartMoving(gpos, goalRadius, maxSpeed);
}

void CStrafeAirMoveType::StartMoving(float3 pos, float goalRadius, float speed)
{
	SetWantedMaxSpeed(speed);

	if (aircraftState == AIRCRAFT_LANDED || aircraftState == AIRCRAFT_LANDING)
		SetState(AIRCRAFT_TAKEOFF);

	SetGoal(pos);
}

void CStrafeAirMoveType::StopMoving(bool callScript, bool hardStop, bool)
{
	SetGoal(owner->pos);
	ClearLandingPos();
	SetWantedMaxSpeed(0.0f);

	if (aircraftState != AAirMoveType::AIRCRAFT_FLYING && aircraftState != AAirMoveType::AIRCRAFT_LANDING)
		return;

	if (dontLand || !autoLand) {
		SetState(AIRCRAFT_FLYING);
		return;
	}

	SetState(AIRCRAFT_LANDING);
}



void CStrafeAirMoveType::Takeoff()
{
	if (aircraftState != AAirMoveType::AIRCRAFT_FLYING) {
		if (aircraftState == AAirMoveType::AIRCRAFT_LANDED) {
			SetState(AAirMoveType::AIRCRAFT_TAKEOFF);
		}
		if (aircraftState == AAirMoveType::AIRCRAFT_LANDING) {
			SetState(AAirMoveType::AIRCRAFT_FLYING);
		}
	}
}



bool CStrafeAirMoveType::SetMemberValue(unsigned int memberHash, void* memberValue) {
	// try the generic members first
	if (AMoveType::SetMemberValue(memberHash, memberValue))
		return true;

	#define WANTEDHEIGHT_MEMBER_IDX 0

	// unordered_map etc. perform dynallocs, so KISS here
	bool* boolMemberPtrs[] = {
		&collide,
		&useSmoothMesh,
		&loopbackAttack,
	};
	int* intMemberPtrs[] = {
		&maneuverBlockTime,
	};
	float* floatMemberPtrs[] = {
		&wantedHeight,
		&turnRadius,

		&accRate, // hash("accRate") case
		&decRate, // hash("decRate") case
		&accRate, // hash( "maxAcc") case
		&decRate, // hash( "maxDec") case

		&maxBank,
		&maxPitch,

		&maxAileron,
		&maxElevator,
		&maxRudder,
		&attackSafetyDistance,

		&myGravity,
	};

	// special cases
	if (memberHash == FLOAT_MEMBER_HASHES[WANTEDHEIGHT_MEMBER_IDX]) {
		SetDefaultAltitude(*(reinterpret_cast<float*>(memberValue)));
		return true;
	}

	// note: <memberHash> should be calculated via HsiehHash
	for (size_t n = 0; n < sizeof(boolMemberPtrs) / sizeof(boolMemberPtrs[0]); n++) {
		if (memberHash == BOOL_MEMBER_HASHES[n]) {
			*(boolMemberPtrs[n]) = *(reinterpret_cast<bool*>(memberValue));
			return true;
		}
	}

	for (size_t n = 0; n < sizeof(intMemberPtrs) / sizeof(intMemberPtrs[0]); n++) {
		if (memberHash == INT_MEMBER_HASHES[n]) {
			*(intMemberPtrs[n]) = int(*(reinterpret_cast<float*>(memberValue))); // sic (see SetMoveTypeValue)
			return true;
		}
	}

	for (size_t n = 0; n < sizeof(floatMemberPtrs) / sizeof(floatMemberPtrs[0]); n++) {
		if (memberHash == FLOAT_MEMBER_HASHES[n]) {
			*(floatMemberPtrs[n]) = *(reinterpret_cast<float*>(memberValue));
			return true;
		}
	}

	return false;
}

