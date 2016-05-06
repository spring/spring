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
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Rendering/Env/Particles/Classes/SmokeProjectile.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Weapons/Weapon.h"
#include "System/myMath.h"
#include "System/Sync/HsiehHash.h"

CR_BIND_DERIVED(CStrafeAirMoveType, AAirMoveType, (nullptr))

CR_REG_METADATA(CStrafeAirMoveType, (
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



static float TurnRadius(const float rawRadius, const float rawSpeed) {
	return (std::min(1000.0f, rawRadius * rawSpeed));
}

static float GetAileronDeflection(
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
	float maxAileron,
	float maxBank,
	float goalDotRight,
	float aGoalDotFront,
	bool avoidCollision,
	bool attacking
) {
	float aileron = 0.0f;

	if (attacking) {
		const float maxAileronSpeedf = maxAileron * spd.w;
		const float maxAileronSpeedf2 = maxAileronSpeedf * 4.0f;
		const float minPredictedHeight = pos.y + spd.y * 60.0f * math::fabs(frontdir.y) + std::min(0.0f, updir.y * 1.0f) * (GAME_SPEED * 5);
		const float maxPredictedHeight = groundHeight + 60.0f + math::fabs(rightdir.y) * (GAME_SPEED * 5);

		if (spd.w > 0.45f && minPredictedHeight > maxPredictedHeight) {
			const float goalBankDif = goalDotRight + rightdir.y * 0.2f;

			if (goalBankDif > maxAileronSpeedf2) {
				aileron = 1.0f;
			} else if (goalBankDif < -maxAileronSpeedf2) {
				aileron = -1.0f;
			} else if (maxAileronSpeedf2 > 0.0f) {
				aileron = goalBankDif / maxAileronSpeedf2;
			}
		} else {
			if (rightdir.y > 0.0f) {
				if (rightdir.y > maxAileronSpeedf || frontdir.y < -0.7f) {
					aileron = 1.0f;
				} else {
					if (maxAileronSpeedf > 0.0f) {
						aileron = rightdir.y / maxAileronSpeedf;
					}
				}
			} else {
				if (rightdir.y < -maxAileronSpeedf || frontdir.y < -0.7f) {
					aileron = -1.0f;
				} else {
					if (maxAileronSpeedf > 0.0f) {
						aileron = rightdir.y / maxAileronSpeedf;
					}
				}
			}
		}
	} else {
		if (spd.w > 1.5f && ((pos.y + spd.y * 10.0f) > (groundHeight + wantedHeight * 0.6f))) {
			const float goalBankDif = goalDotRight + rightdir.y * 0.5f;
			const float maxAileronSpeedf = maxAileron * spd.w * 4.0f;

			if (goalBankDif > maxAileronSpeedf && rightdir.y > -maxBank) {
				aileron = 1.0f;
			} else if (goalBankDif < -maxAileronSpeedf && rightdir.y < maxBank) {
				aileron = -1.0f;
			} else {
				if (math::fabs(rightdir.y) < maxBank && maxAileronSpeedf > 0.0f) {
					aileron = goalBankDif / maxAileronSpeedf;
				} else {
					if (rightdir.y < 0.0f && goalBankDif < 0.0f) {
						aileron = -1.0f;
					} else if (rightdir.y > 0.0f && goalBankDif > 0.0f) {
						aileron = 1.0f;
					}
				}
			}
		} else {
			if (rightdir.y > 0.01f) {
				aileron = 1.0f;
			} else if (rightdir.y < -0.01f) {
				aileron = -1.0f;
			}
		}
	}

	return aileron;
}

static float GetRudderDeflection(
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
	float maxRudder,
	float,
	float goalDotRight,
	float aGoalDotFront,
	bool avoidCollision,
	bool attacking
) {
	float rudder = 0.0f;

	if (attacking) {
		if (pos.y > (groundHeight + 30.0f)) {
			const float maxRudderSpeedf = maxRudder * spd.w;

			if (goalDotRight < -maxRudderSpeedf) {
				rudder = -1.0f;
			} else if (goalDotRight > maxRudderSpeedf) {
				rudder = 1.0f;
			} else {
				if (maxRudderSpeedf > 0.0f) {
					rudder = goalDotRight / maxRudderSpeedf;

					if (math::fabs(rudder) < 0.01f && aGoalDotFront < 0.0f) {
						// target almost straight behind us, we have to choose a direction
						rudder = (goalDotRight < 0.0f) ? -0.01f : 0.01f;
					}
				}
			}
		}
	} else {
		if (pos.y > (groundHeight + 15.0f)) {
			const float maxRudderSpeedf = maxRudder * spd.w * 2.0f;

			if (goalDotRight < -maxRudderSpeedf) {
				rudder = -1.0f;
			} else if (goalDotRight > maxRudderSpeedf) {
				rudder = 1.0f;
			} else {
				if (maxRudderSpeedf > 0.0f) {
					rudder = goalDotRight / maxRudderSpeedf;

					if (math::fabs(rudder) < 0.01f && aGoalDotFront < 0.0f) {
						// target almost straight behind us, we have to choose a direction
						rudder = (goalDotRight < 0.0f) ? -0.01f : 0.01f;
					}
				} else {
					rudder = 0.0f;
				}
			}
		}
	}

	return rudder;
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
	float aGoalDotFront,
	bool avoidCollision,
	bool attacking
) {
	float elevator = 0.0f;
	float upside = (updir.y >= -0.3f) * 2.0f - 1.0f;

	if (attacking) {
		if (spd.w < 1.5f) {
			if (frontdir.y < 0.0f) {
				elevator = upside;
			} else if (frontdir.y > 0.0f) {
				elevator = -upside;
			}
		} else {
			const float gHeightR = CGround::GetHeightAboveWater(pos.x + spd.x * 40.0f, pos.z + spd.z * 40.0f);
			const float hdif = std::max(gHeightR, groundHeight) + 60 - pos.y - frontdir.y * spd.w * 20.0f;

			const float maxElevatorSpeedf = maxElevator * spd.w;
			const float maxElevatorSpeedf2 = maxElevatorSpeedf * spd.w * 20.0f;

			float minPitch = 1.0f; // min(1.0f, hdif / (maxElevator * spd.w * spd.w * 20));

			if (hdif < -maxElevatorSpeedf2) {
				minPitch = -1.0f;
			} else if (hdif > maxElevatorSpeedf2) {
				minPitch = 1.0f;
			} else if (maxElevatorSpeedf2 > 0.0f) {
				minPitch = hdif / maxElevatorSpeedf2;
			}

			if (collidee != NULL && avoidCollision && frontdir.dot(collidee->pos + collidee->speed * 20.0f - pos - spd * 20.0f) < 0.0f) {
				elevator = (updir.dot(collidee->midPos - owner->midPos) > 0.0f)? -1 : 1;
			} else {
				const float hdif = goalDir.dot(updir);

				if (hdif < -maxElevatorSpeedf) {
					elevator = -1.0f;
				} else if (hdif > maxElevatorSpeedf) {
					elevator = 1.0f;
				} else if (maxElevatorSpeedf > 0.0f) {
					elevator = hdif / maxElevatorSpeedf;
				}
			}

			if ((elevator * upside) < minPitch) {
				elevator = minPitch * upside;
			}
		}
	} else {
		if (spd.w > 0.8f) {
			bool notColliding = true;

			if (avoidCollision) {
				const float3 dir = collidee->midPos - owner->midPos;
				const float3 sdir = collidee->speed - spd;

				if (frontdir.dot(dir + sdir * 20) < 0) {
					elevator = updir.dot(dir) > 0 ? -1 : 1;
					notColliding = false;
				}
			}

			if (notColliding) {
				const float maxElevatorSpeedf = maxElevator * 20.0f * spd.w * spd.w;
				const float gHeightAW = CGround::GetHeightAboveWater(pos.x + spd.x * 40.0f, pos.z + spd.z * 40.0f);
				const float hdif = std::max(groundHeight, gHeightAW) + wantedHeight - pos.y - frontdir.y * spd.w * 20.0f;

				if (hdif < -maxElevatorSpeedf && frontdir.y > -maxPitch) {
					elevator = -1.0f;
				} else if (hdif > maxElevatorSpeedf && frontdir.y < maxPitch) {
					elevator = 1.0f;
				} else if (maxElevatorSpeedf > 0.0f) {
					if (math::fabs(frontdir.y) < maxPitch) {
						elevator = hdif / maxElevatorSpeedf;
					}
				}
			}
		} else {
			if (frontdir.y < -0.1f) {
				elevator = 1.0f;
			} else if (frontdir.y > 0.15f) {
				elevator = -1.0f;
			}
		}
	}

	return elevator;
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
		if (math::fabs(frontdir.y) <= 0.2f && gs->randFloat() > 0.3f) {
			return CStrafeAirMoveType::MANEUVER_IMMELMAN_INV;
		}
	} else {
		if (frontdir.y > -0.2f && gs->randFloat() > 0.7f) {
			return CStrafeAirMoveType::MANEUVER_IMMELMAN;
		}
	}

	return CStrafeAirMoveType::MANEUVER_FLY_STRAIGHT;
}



CStrafeAirMoveType::CStrafeAirMoveType(CUnit* owner):
	AAirMoveType(owner),
	maneuverState(MANEUVER_FLY_STRAIGHT),
	maneuverSubState(0),
	loopbackAttack(false),
	isFighter(false),
	wingDrag(0.07f),
	wingAngle(0.1f),
	invDrag(0.995f),
	crashDrag(0.995f),
	frontToSpeed(0.04f),
	speedToFront(0.01f),
	myGravity(0.8f),
	maxBank(0.55f),
	maxPitch(0.35f),
	turnRadius(150),
	maxAileron(0.04f),
	maxElevator(0.02f),
	maxRudder(0.01f)
{
	// creg
	if (owner == nullptr)
		return;

	assert(owner->unitDef != nullptr);

	isFighter = owner->unitDef->IsFighterAirUnit();
	loopbackAttack = owner->unitDef->canLoopbackAttack && isFighter;
	collide = owner->unitDef->collide;

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
		((gs->randFloat() - 0.3f) * 15.0f * (isFighter? 2.0f: 1.0f));

	orgWantedHeight = wantedHeight;

	maxAileron = owner->unitDef->maxAileron;
	maxElevator = owner->unitDef->maxElevator;
	maxRudder = owner->unitDef->maxRudder;
	attackSafetyDistance = 0.0f;

	useSmoothMesh = owner->unitDef->useSmoothMesh;

	// FIXME: WHY ARE THESE RANDOMIZED?
	maxRudder   *= (0.99f + gs->randFloat() * 0.02f);
	maxElevator *= (0.99f + gs->randFloat() * 0.02f);
	maxAileron  *= (0.99f + gs->randFloat() * 0.02f);
	accRate     *= (0.99f + gs->randFloat() * 0.02f);

	crashAileron = 1.0f - (gs->randFloat() * gs->randFloat());
	crashAileron *= ((gs->randInt() & 1)? -1.0f: 1.0f);
	crashElevator = gs->randFloat();
	crashRudder = gs->randFloat() - 0.5f;

	lastRudderPos = 0.0f;
	lastElevatorPos = 0.0f;
	lastAileronPos = 0.0f;

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
		UpdateAirPhysics(0.0f * lastRudderPos, lastAileronPos, lastElevatorPos, 0.0f, ZeroVector);
		return (HandleCollisions(collide && !owner->beingBuilt && (aircraftState != AIRCRAFT_TAKEOFF)));
	}

	if (aircraftState != AIRCRAFT_CRASHING) {
		if (owner->UnderFirstPersonControl()) {
			SetState(AIRCRAFT_FLYING);

			const FPSUnitController& con = owner->fpsControlPlayer->fpsController;

			float aileron = 0.0f;
			float elevator = 0.0f;

			if (con.forward) elevator -= 1.0f;
			if (con.back   ) elevator += 1.0f;
			if (con.right  ) aileron  += 1.0f;
			if (con.left   ) aileron  -= 1.0f;

			UpdateAirPhysics(0.0f, aileron, elevator, 1.0f, owner->frontdir);
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

							if ((maneuverState = SelectLoopBackManeuver(frontdir, rightdir, lastSpd, turnRadius, altitude)) == MANEUVER_IMMELMAN_INV) {
								maneuverSubState = 0;
							}
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
			UpdateAirPhysics(crashRudder, crashAileron, crashElevator, 0.0f, owner->frontdir);

			if ((CGround::GetHeightAboveWater(owner->pos.x, owner->pos.z) + 5.0f + owner->radius) > owner->pos.y) {
				owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_CRASHING);
				owner->KillUnit(NULL, true, false);
			}

			new CSmokeProjectile(owner, owner->midPos, gs->randVector() * 0.08f, 100 + gs->randFloat() * 50, 5, 0.2f, 0.4f);
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
	if (lastColWarningType == 1) {
		const int g = geometricObjects->AddLine(pos, lastColWarning->pos, 10, 1, 1);
		geometricObjects->SetColor(g, 0.2f, 1, 0.2f, 0.6f);
	} else if (lastColWarningType == 2) {
		const int g = geometricObjects->AddLine(pos, lastColWarning->pos, 10, 1, 1);
		if (owner->frontdir.dot(lastColWarning->midPos + lastColWarning->speed * 20.0f - owner->midPos - spd * 20.0f) < 0) {
			geometricObjects->SetColor(g, 1, 0.2f, 0.2f, 0.6f);
		} else {
			geometricObjects->SetColor(g, 1, 1, 0.2f, 0.6f);
		}
	}
#endif

	if (pos != oldPos) {
		oldPos = pos;

		bool hitBuilding = false;

		// check for collisions if not being built or not taking off
		if (checkCollisions) {
			// copy on purpose, since the below can call Lua
			const std::vector<CUnit*> nearUnits = quadField->GetUnitsExact(pos, owner->radius + 6);

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
						owner->DoDamage(DamageArray(damage), ZeroVector, NULL, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);
						unit->DoDamage(DamageArray(damage), ZeroVector, NULL, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);
					}

					hitBuilding = true;
				} else {
					const float part = owner->mass / (owner->mass + unit->mass);
					const float damage = ((unit->speed - owner->speed) * 0.1f).SqLength();

					owner->Move(-dif * (dist - totRad) * (1 - part), true);
					owner->SetVelocity(owner->speed * 0.99f);

					if (!unit->UsingScriptMoveType()) {
						unit->Move(dif * (dist - totRad) * (part), true);
					}

					if (modInfo.allowUnitCollisionDamage) {
						owner->DoDamage(DamageArray(damage), ZeroVector, NULL, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);
						unit->DoDamage(DamageArray(damage), ZeroVector, NULL, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);
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
			owner->KillUnit(NULL, true, false);
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
			float elevator = 0.0f;

			if (owner->updir.y > 0.0f) {
				if (owner->rightdir.y > maxAileron * speedf) {
					aileron = 1.0f;
				} else if (owner->rightdir.y < -maxAileron * speedf) {
					aileron = -1.0f;
				}
			}

			if (math::fabs(owner->rightdir.y) < maxAileron * 3.0f * speedf || owner->updir.y < 0.0f)
				elevator = 1.0f;

			UpdateAirPhysics(0.0f, aileron, elevator, 1.0f, owner->frontdir);

			if ((owner->updir.y < 0.0f && owner->frontdir.y < 0.0f) || speedf < 0.8f) {
				maneuverState = MANEUVER_FLY_STRAIGHT;
			}
			// [?] some seem to report that the "unlimited altitude" thing is because of these maneuvers
			if ((owner->pos.y - CGround::GetApproximateHeight(owner->pos.x, owner->pos.z)) > (wantedHeight * 4.0f)) {
				maneuverState = MANEUVER_FLY_STRAIGHT;
			}
		} break;

		case MANEUVER_IMMELMAN_INV: {
			// inverted Immelman
			float aileron = 0.0f;
			float elevator = 0.0f;

			if (maneuverSubState == 0) {
				if (owner->rightdir.y >= 0.0f) {
					aileron = -1.0f;
				} else {
					aileron = 1.0f;
				}
			}

			if (owner->frontdir.y < -0.7f)
				maneuverSubState = 1;
			if (maneuverSubState == 1 || owner->updir.y < 0.0f)
				elevator = 1.0f;

			UpdateAirPhysics(0.0f, aileron, elevator, 1.0f, owner->frontdir);

			if ((owner->updir.y > 0.0f && owner->frontdir.y > 0.0f && maneuverSubState == 1) || speedf < 0.2f) {
				maneuverState = MANEUVER_FLY_STRAIGHT;
			}
		} break;

		default: {
			UpdateAirPhysics(0.0f, 0.0f, 0.0f, 1.0f, owner->frontdir);
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
		UpdateAirPhysics(0.0f, 0.0f, 0.0f, 1.0f, owner->frontdir);
		return;
	}

	if (((gs->frameNum + owner->id) & 3) == 0) {
		CheckForCollision();
	}

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

	const float aGoalDotFront = goalDir.dot(frontdir);
	const float goalDotFront = aGoalDotFront * 0.5f + 0.501f;

	if (goalDotFront != 0.0f) {
		goalDotRight /= goalDotFront;
	}

	{
		const CUnit* attackee = owner->curTarget.unit;

		const float aileron  = GetAileronDeflection (owner, lastColWarning, pos, spd, rightdir, updir, frontdir, goalDir, gHeightAW, wantedHeight,  maxAileron,  maxBank, goalDotRight, aGoalDotFront, lastColWarningType == 2, true); // roll
		const float rudder   = GetRudderDeflection  (owner, lastColWarning, pos, spd, rightdir, updir, frontdir, goalDir, gHeightAW, wantedHeight,   maxRudder,     0.0f, goalDotRight, aGoalDotFront, lastColWarningType == 2, true); // yaw
		const float elevator = GetElevatorDeflection(owner, lastColWarning, pos, spd, rightdir, updir, frontdir, goalDir, gHeightAW, wantedHeight, maxElevator, maxPitch, goalDotRight, aGoalDotFront, lastColWarningType == 2, true); // pitch
		const float engine   = ((attackee == NULL) || attackee->unitDef->IsGroundUnit())? 1.0f: std::min(1.0f, (goalDist / owner->maxRange + 1.0f - goalDir.dot(frontdir) * 0.7f));

		UpdateAirPhysics(rudder, aileron, elevator, engine, owner->frontdir);
	}
}



bool CStrafeAirMoveType::UpdateFlying(float wantedHeight, float engine)
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

	// do not check if the plane can be submerged here,
	// since it'll cause ground collisions later on (?)
	float gHeight = CGround::GetHeightAboveWater(pos.x, pos.z);
	if (UseSmoothMesh()) {
		gHeight = std::max(smoothGround->GetHeight(pos.x, pos.z), gHeight);
	}

	if (((gs->frameNum + owner->id) & 3) == 0) {
		CheckForCollision();
	}

	// RHS is needed for moving targets (when called by UpdateAttack)
	const bool allowUnlockYawRoll = (goalDist2D >= TurnRadius(turnRadius, spd.w) || goalVec.dot(owner->frontdir) > 0.0f);
	const bool forceUnlockYawRoll = ((gs->frameNum - owner->lastFireWeapon) >= GAME_SPEED * 3);

	// yaw and roll have to be unblocked after a certain time or aircraft
	// can fly straight forever if their target is another chasing aircraft
	float3 rightDir2D = rightdir;
	float3 yprMults = (XZVector * float(allowUnlockYawRoll || forceUnlockYawRoll)) + UpVector;

	float goalDotRight = goalDir2D.dot(rightDir2D.Normalize2D());

	const float aGoalDotFront = goalDir2D.dot(frontdir);

	// If goal-position is behind us and goal-distance is less
	// than our turning radius, turn the other way.
	// If goal-distance is half turn radius then turn if
	// goal-position is not in front within a 45 degree arc.
	// This is to prevent becoming stuck in a small circle
	// around goal-position.
	if ((goalDist2D < turnRadius * 0.5f && goalDir2D.dot(frontdir) < 0.7f) || (goalDist2D < turnRadius && goalDir2D.dot(frontdir) < -0.1f)) {
		if (!owner->UnderFirstPersonControl() || owner->fpsControlPlayer->fpsController.mouse2) {
			goalDotRight *= -1.0f;
		}
	}

	if (lastColWarning != NULL) {
		const float3 otherDif = lastColWarning->pos - pos;
		const float otherLength = otherDif.Length();
		const float otherThreat = (otherLength > 0.0f)?
			std::max(1200.0f, goalDist2D) / otherLength * 0.036f:
			0.0f;

		const float3 otherDir = (otherLength > 0.0f)?
			(otherDif / otherLength):
			ZeroVector;

		goalDotRight -= (otherDir.dot(rightdir) * otherThreat);
	}

	const float aileron  = GetAileronDeflection (owner, lastColWarning, pos, spd, rightdir, updir, frontdir, goalDir2D, gHeight, wantedHeight,  maxAileron,  maxBank, goalDotRight, aGoalDotFront, lastColWarningType == 2, false); // roll
	const float rudder   = GetRudderDeflection  (owner, lastColWarning, pos, spd, rightdir, updir, frontdir, goalDir2D, gHeight, wantedHeight,   maxRudder,     0.0f, goalDotRight, aGoalDotFront, lastColWarningType == 2, false); // yaw
	const float elevator = GetElevatorDeflection(owner, lastColWarning, pos, spd, rightdir, updir, frontdir, goalDir2D, gHeight, wantedHeight, maxElevator, maxPitch, goalDotRight, aGoalDotFront, lastColWarningType == 2, false); // pitch

	UpdateAirPhysics(rudder * yprMults.x, aileron * yprMults.z, elevator * yprMults.y, engine, owner->frontdir);
	return (allowUnlockYawRoll || forceUnlockYawRoll);
}


static float GetVTOLAccelerationSign(float h, float wh, float speedy, bool ascending) {
	const float nxtHeight = h + speedy * 20.0f;
	const float tgtHeight = wh * 1.02f;

	if (ascending) {
		return ((nxtHeight < tgtHeight)?  1.0f: -1.0f);
	} else {
		return ((nxtHeight > tgtHeight)? -1.0f:  1.0f);
	}
}

void CStrafeAirMoveType::UpdateTakeOff()
{
	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	wantedHeight = orgWantedHeight;

	SyncedFloat3& rightdir = owner->rightdir;
	SyncedFloat3& frontdir = owner->frontdir;
	SyncedFloat3& updir    = owner->updir;

	const float currentHeight = pos.y - (owner->unitDef->canSubmerge?
		CGround::GetHeightReal(pos.x, pos.z):
		CGround::GetHeightAboveWater(pos.x, pos.z));
	const float yawSign = Sign((goalPos - pos).dot(rightdir));

	frontdir += (rightdir * yawSign * (maxRudder * spd.y));
	frontdir.Normalize();
	rightdir = frontdir.cross(updir);

	owner->SetVelocity(spd + (UpVector * accRate * GetVTOLAccelerationSign(currentHeight, wantedHeight, spd.y, true)));

	// initiate forward motion before reaching wantedHeight
	if (currentHeight > wantedHeight * 0.4f) {
		owner->SetVelocity(spd + (owner->frontdir * accRate));
	}
	if (currentHeight > wantedHeight) {
		SetState(AIRCRAFT_FLYING);
	}

	owner->SetVelocityAndSpeed(spd * invDrag);
	owner->Move(spd, true);
	owner->SetHeadingFromDirection();
	owner->UpdateMidAndAimPos();
}


void CStrafeAirMoveType::UpdateLanding()
{
	const float3& pos = owner->pos;
	SyncedFloat3& rightdir = owner->rightdir;
	SyncedFloat3& frontdir = owner->frontdir;
	SyncedFloat3& updir    = owner->updir;

	if (!HaveLandingPos()) {
		reservedLandingPos = FindLandingPos();

		// if spot is valid, mark it on the blocking-map
		// so other aircraft can not claim the same spot
		if (HaveLandingPos()) {
			const float3 originalPos = pos;
			wantedHeight = 0;

			owner->Move(reservedLandingPos, false);
			owner->Block();
			owner->Move(originalPos, false);
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
	UpdateLandingHeight();

	const float3 reservedLandingPosDir = reservedLandingPos - pos;
	const float3 brakeSpot = pos + frontdir * BrakingDistance(owner->speed.Length2D(), decRate);

	if (brakeSpot.SqDistance2D(reservedLandingPos) > landRadiusSq) {
		// If we're not going to land inside the landRadius, keep flying.
		float tempWantedHeight = wantedHeight;
		wantedHeight = orgWantedHeight;

		UpdateFlying(wantedHeight, 1.0f);
		wantedHeight = tempWantedHeight;
		return;
	}

	     if (rightdir.y < -0.01f) { updir -= (rightdir * 0.02f); }
	else if (rightdir.y >  0.01f) { updir += (rightdir * 0.02f); }

	     if (frontdir.y < -0.01f) { frontdir += (updir * 0.02f); }
	else if (frontdir.y >  0.01f) { frontdir -= (updir * 0.02f); }

	     if (rightdir.dot(reservedLandingPosDir) >  0.01f) { frontdir += (rightdir * 0.02f); }
	else if (rightdir.dot(reservedLandingPosDir) < -0.01f) { frontdir -= (rightdir * 0.02f); }

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

		if (frontSpeed < 0.0f) {
			owner->SetVelocityAndSpeed(UpVector * owner->speed);
		}

		const float landPosDistXZ = reservedLandingPosDir.Length2D() + 0.1f;
		const float landPosDistY = reservedLandingPosDir.y;
		const float curSpeedXZ = spd.Length2D();
		owner->SetVelocity(spd - UpVector * spd.y);

		if (landPosDistXZ > curSpeedXZ && curSpeedXZ > 0.1f) {
			owner->SetVelocity(spd + UpVector * std::max(-altitudeRate, landPosDistY * curSpeedXZ / landPosDistXZ));
		} else {
			const float localAltitude = pos.y - (owner->unitDef->canSubmerge ?
				CGround::GetHeightReal(owner->pos.x, owner->pos.z):
				CGround::GetHeightAboveWater(owner->pos.x, owner->pos.z));
			if (localAltitude > wantedHeight) {
				owner->SetVelocity(spd + UpVector * std::max(-altitudeRate, wantedHeight - localAltitude));
			}
		}

		owner->SetSpeed(spd);

		owner->Move(owner->speed, true);
		owner->UpdateDirVectors(owner->IsOnGround());
		owner->UpdateMidAndAimPos();
	}

	AAirMoveType::UpdateLanding();
}



void CStrafeAirMoveType::UpdateAirPhysics(float rudder, float aileron, float elevator, float engine, const float3& engineThrustVector)
{
	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	SyncedFloat3& rightdir = owner->rightdir;
	SyncedFloat3& frontdir = owner->frontdir;
	SyncedFloat3& updir    = owner->updir;

	bool nextPosInBounds = true;

	lastAileronPos = aileron;
	lastElevatorPos = elevator;

	const float gHeight = CGround::GetHeightAboveWater(pos.x, pos.z);
	const float speedf = spd.w;
	const float3 speeddir = spd / (speedf + 0.1f);

	if (owner->UnderFirstPersonControl()) {
		if ((pos.y - gHeight) > wantedHeight * 1.2f) {
			engine = std::max(0.0f, std::min(engine, 1 - (pos.y - gHeight - wantedHeight * 1.2f) / wantedHeight));
		}
		// check next position given current (unadjusted) pos and speed
		nextPosInBounds = (pos + spd).IsInBounds();
	}


	// apply gravity only when in the air
	owner->SetVelocity(spd + UpVector * (mapInfo->map.gravity * myGravity) * int((owner->midPos.y - owner->radius) > gHeight));
	owner->SetVelocity(spd + (engineThrustVector * accRate * engine));

	if (aircraftState == AIRCRAFT_CRASHING) {
		owner->SetVelocity(spd * crashDrag);
	} else {
		owner->SetVelocity(spd * invDrag);
	}

	const float3 wingDir = updir * (1 - wingAngle) - frontdir * wingAngle;
	const float wingForce = wingDir.dot(spd) * wingDrag;

	if (!owner->IsStunned()) {
		frontdir += (rightdir * rudder   * maxRudder   * speedf); // yaw
		updir    += (rightdir * aileron  * maxAileron  * speedf); // roll
		frontdir += (updir    * elevator * maxElevator * speedf); // pitch
		frontdir += ((speeddir - frontdir) * frontToSpeed);

		owner->SetVelocity(spd - (wingDir * wingForce));
		owner->SetVelocity(spd + ((frontdir * speedf - spd) * speedToFront));
		owner->SetVelocity(spd * (1 - int(owner->beingBuilt && aircraftState == AIRCRAFT_LANDED)));
	}

	if (nextPosInBounds) {
		owner->Move(spd, true);
	}

	// [?] prevent aircraft from gaining unlimited altitude
	owner->Move(UpVector * (Clamp(pos.y, gHeight, readMap->GetCurrMaxHeight() + owner->unitDef->wantedHeight * 5.0f) - pos.y), true);

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
		const bool groundContact = (gHeight > (owner->midPos.y - owner->radius));
		const bool handleContact = (aircraftState != AIRCRAFT_LANDED && aircraftState != AIRCRAFT_TAKEOFF);

		if (groundContact && handleContact) {
			owner->Move(UpVector * (gHeight - (owner->midPos.y - owner->radius) + 0.01f), true);

			const float3& gNormal = CGround::GetNormal(pos.x, pos.z);
			const float impactSpeed = -spd.dot(gNormal) * int(1 - owner->IsStunned());

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

				owner->SetVelocity(spd + (gNormal * impactSpeed * 1.5f));

				updir = (gNormal - frontdir * 0.1f);
				frontdir = updir.cross(frontdir.cross(updir));
			}
		}
	}

	frontdir.Normalize();
	rightdir = frontdir.cross(updir);
	rightdir.Normalize();
	updir = rightdir.cross(frontdir);

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
	owner->useAirLos = true;

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
			owner->useAirLos = false;

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



float3 CStrafeAirMoveType::FindLandingPos()
{
	const float3 ret = -OnesVector;
	const UnitDef* ud = owner->unitDef;
	const float brDist = BrakingDistance(maxSpeed, decRate);
	landRadiusSq = Square(brDist);

	float3 tryPos = owner->pos + owner->frontdir * brDist;
	tryPos.y = CGround::GetHeightReal(tryPos.x, tryPos.z);

	if ((tryPos.y < 0.0f) && !(ud->floatOnWater || ud->canSubmerge))
		return ret;

	tryPos.ClampInBounds();

	const int2 mp = owner->GetMapPos(tryPos);

	for (int z = mp.y; z < mp.y + owner->zsize; z++) {
		for (int x = mp.x; x < mp.x + owner->xsize; x++) {
			if (groundBlockingObjectMap->GroundBlocked(x, z, owner)) {
				return ret;
			}
		}
	}

	// FIXME: better use ud->maxHeightDif?
	if (CGround::GetSlope(tryPos.x, tryPos.z) > 0.03f)
		return ret;

	return tryPos;
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

void CStrafeAirMoveType::StopMoving(bool callScript, bool hardStop)
{
	SetGoal(owner->pos);
	ClearLandingPos();
	SetWantedMaxSpeed(0.0f);

	if (aircraftState != AAirMoveType::AIRCRAFT_FLYING && aircraftState != AAirMoveType::AIRCRAFT_LANDING)
		return;

	if (owner->unitDef->DontLand() || !autoLand) {
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

	#define MEMBER_CHARPTR_HASH(memberName) HsiehHash(memberName, strlen(memberName),     0)
	#define MEMBER_LITERAL_HASH(memberName) HsiehHash(memberName, sizeof(memberName) - 1, 0)

	#define WANTEDHEIGHT_MEMBER_IDX 0

	static const unsigned int boolMemberHashes[] = {
		MEMBER_LITERAL_HASH(       "collide"),
		MEMBER_LITERAL_HASH( "useSmoothMesh"),
		MEMBER_LITERAL_HASH("loopbackAttack"),
	};
	static const unsigned int floatMemberHashes[] = {
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


	// unordered_map etc. perform dynallocs, so KISS here
	bool* boolMemberPtrs[] = {
		&collide,
		&useSmoothMesh,
		&loopbackAttack,
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

