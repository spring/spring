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
#include "Sim/Projectiles/Unsynced/SmokeProjectile.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Weapons/Weapon.h"
#include "System/myMath.h"

CR_BIND_DERIVED(CStrafeAirMoveType, AAirMoveType, (NULL));

CR_REG_METADATA(CStrafeAirMoveType, (
	CR_MEMBER(maneuver),
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

	CR_MEMBER(crashAileron),
	CR_MEMBER(crashElevator),
	CR_MEMBER(crashRudder),

	CR_MEMBER(lastRudderPos),
	CR_MEMBER(lastElevatorPos),
	CR_MEMBER(lastAileronPos),

	CR_MEMBER(inefficientAttackTime),
	CR_MEMBER(exitVector),

	CR_RESERVED(63)
));



CStrafeAirMoveType::CStrafeAirMoveType(CUnit* owner):
	AAirMoveType(owner),
	maneuver(0),
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
	maxRudder(0.01f),
	inefficientAttackTime(0)
{
	assert(owner != NULL);
	assert(owner->unitDef != NULL);

	// force LOS recalculation
	owner->mapSquare += 1;

	isFighter = owner->unitDef->IsFighterAirUnit();
	collide = owner->unitDef->collide;

	wingAngle = owner->unitDef->wingAngle;
	crashDrag = 1.0f - owner->unitDef->crashDrag;

	if (accRate != 0.0f && maxSpeedDef != 0.0f) {
		invDrag = (1.0f / (maxSpeedDef * 1.1f / accRate)) - (wingAngle * wingAngle * wingDrag);
		invDrag = 1.0f - Clamp(invDrag, 0.0f, 1.0f);
	}

	frontToSpeed = owner->unitDef->frontToSpeed;
	speedToFront = owner->unitDef->speedToFront;
	myGravity = math::fabs(owner->unitDef->myGravity);

	maxBank = owner->unitDef->maxBank;
	maxPitch = owner->unitDef->maxPitch;
	turnRadius = owner->unitDef->turnRadius;

	wantedHeight =
		(owner->unitDef->wantedHeight * 1.5f) +
		((gs->randFloat() - 0.3f) * 15.0f * (isFighter? 2.0f: 1.0f));

	maxAileron = owner->unitDef->maxAileron;
	maxElevator = owner->unitDef->maxElevator;
	maxRudder = owner->unitDef->maxRudder;

	useSmoothMesh = owner->unitDef->useSmoothMesh;

	// FIXME: WHY ARE THESE RANDOMIZED?
	maxRudder   *= (0.99f + gs->randFloat() * 0.02f);
	maxElevator *= (0.99f + gs->randFloat() * 0.02f);
	maxAileron  *= (0.99f + gs->randFloat() * 0.02f);
	accRate     *= (0.99f + gs->randFloat() * 0.02f);

	crashAileron = 1 - gs->randFloat() * gs->randFloat();
	if (gs->randInt() & 1) {
		crashAileron = -crashAileron;
	}
	crashElevator = gs->randFloat();
	crashRudder = gs->randFloat() - 0.5f;

	lastRudderPos = 0.0f;
	lastElevatorPos = 0.0f;
	lastAileronPos = 0.0f;

	exitVector = gs->randVector();
	exitVector.y = math::fabs(exitVector.y);
	exitVector.y += 1.0f;
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
		UpdateAirPhysics(0.0f * lastRudderPos, lastAileronPos, lastElevatorPos, 0, ZeroVector);
		return (HandleCollisions(collide && !owner->beingBuilt && (padStatus == PAD_STATUS_FLYING) && (aircraftState != AIRCRAFT_TAKEOFF)));
	}

	// somewhat hackish, but planes that have attack orders
	// while no pad is available would otherwise continue
	// attacking even if out of fuel
	const bool outOfFuel = (owner->currentFuel <= 0.0f && owner->unitDef->maxFuel > 0.0f);
	const bool allowAttack = (reservedPad == NULL && !outOfFuel);

	if (aircraftState != AIRCRAFT_CRASHING) {
		if (owner->fpsControlPlayer != NULL) {
			SetState(AIRCRAFT_FLYING);
			inefficientAttackTime = 0;

			const FPSUnitController& con = owner->fpsControlPlayer->fpsController;

			if (con.forward || con.back || con.left || con.right) {
				float aileron = 0.0f;
				float elevator = 0.0f;

				if (con.forward) { elevator -= 1; }
				if (con.back   ) { elevator += 1; }
				if (con.right  ) { aileron  += 1; }
				if (con.left   ) { aileron  -= 1; }

				UpdateAirPhysics(0, aileron, elevator, 1, owner->frontdir);
				maneuver = 0;

				return (HandleCollisions(collide && !owner->beingBuilt && (padStatus == PAD_STATUS_FLYING) && (aircraftState != AIRCRAFT_TAKEOFF)));
			}
		}

		if (reservedPad != NULL) {
			MoveToRepairPad();
		} else {
			if (outOfFuel && airBaseHandler->HaveAirBase(owner->allyteam)) {
				// out of fuel, keep us in the air to reach our landing
				// goalPos (which is/will be set to a pad's position by
				// MobileCAI) so long as there is at least one friendly
				// base
				//
				// NOTE:
				//   technically need to pass minAirBasePower as well,
				//   otherwise aircraft might keep flying and never be
				//   serviced
				SetState(AIRCRAFT_FLYING);
			}
		}
	}

	switch (aircraftState) {
		case AIRCRAFT_FLYING: {
			owner->restTime = 0;

			const CCommandQueue& cmdQue = owner->commandAI->commandQue;
			const bool isAttacking = (!cmdQue.empty() && (cmdQue.front()).GetID() == CMD_ATTACK);
			const bool keepAttacking = ((owner->attackTarget != NULL && !owner->attackTarget->isDead) || owner->userAttackGround);

			/*
			const float brakeDistSq = Square(0.5f * lastSpd.SqLength2D() / decRate);
			const float goalDistSq = (goalPos - lastPos).SqLength2D();

			if (brakeDistSq >= goalDistSq && !owner->commandAI->HasMoreMoveCommands()) {
				SetState(AIRCRAFT_LANDING);
			} else
			*/
			{
				if (isAttacking && allowAttack && keepAttacking) {
					inefficientAttackTime = std::min(inefficientAttackTime, float(gs->frameNum) - owner->lastFireWeapon);

					if (owner->attackTarget != NULL) {
						SetGoal(owner->attackTarget->pos);
					} else {
						SetGoal(owner->attackPos);
					}

					if (maneuver) {
						UpdateManeuver();
						inefficientAttackTime = 0;
					} else if (isFighter && goalPos.SqDistance(lastPos) < Square(owner->maxRange * 4)) {
						inefficientAttackTime++;
						UpdateFighterAttack();
					} else {
						inefficientAttackTime = 0;
						UpdateAttack();
					}
				} else {
					inefficientAttackTime = 0;
					UpdateFlying(wantedHeight, 1);
				}
			}
		} break;
		case AIRCRAFT_LANDED:
			inefficientAttackTime = 0;
			UpdateLanded();
			break;
		case AIRCRAFT_LANDING:
			inefficientAttackTime = 0;
			UpdateLanding();
			break;
		case AIRCRAFT_CRASHING: {
			// NOTE: the crashing-state can only be set (and unset) by scripts
			UpdateAirPhysics(crashRudder, crashAileron, crashElevator, 0, owner->frontdir);

			if ((ground->GetHeightAboveWater(owner->pos.x, owner->pos.z) + 5.0f + owner->radius) > owner->pos.y) {
				owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_CRASHING);
				owner->KillUnit(NULL, true, false);
			}

			new CSmokeProjectile(owner->midPos, gs->randVector() * 0.08f, 100 + gs->randFloat() * 50, 5, 0.2f, owner, 0.4f);
		} break;
		case AIRCRAFT_TAKEOFF:
			UpdateTakeOff(wantedHeight);
			break;
		default:
			break;
	}

	if (lastSpd == ZeroVector && owner->speed != ZeroVector) { owner->script->StartMoving(false); }
	if (lastSpd != ZeroVector && owner->speed == ZeroVector) { owner->script->StopMoving(); }

	return (HandleCollisions(collide && !owner->beingBuilt && (padStatus == PAD_STATUS_FLYING) && (aircraftState != AIRCRAFT_TAKEOFF)));
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

		// check for collisions if not on a pad, not being built, or not taking off
		if (checkCollisions) {
			const vector<CUnit*>& nearUnits = quadField->GetUnitsExact(pos, owner->radius + 6);

			for (vector<CUnit*>::const_iterator ui = nearUnits.begin(); ui != nearUnits.end(); ++ui) {
				CUnit* unit = *ui;

				const float sqDist = (pos - unit->pos).SqLength();
				const float totRad = owner->radius + unit->radius;

				if (sqDist <= 0.1f || sqDist >= (totRad * totRad))
					continue;

				const float dist = math::sqrt(sqDist);
				const float3 dif = (pos - unit->pos).Normalize();

				if (unit->immobile) {
					const float damage = ((unit->speed - owner->speed) * 0.1f).SqLength();

					owner->DoDamage(DamageArray(damage), ZeroVector, NULL, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);
					unit->DoDamage(DamageArray(damage), ZeroVector, NULL, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);

					owner->Move(-dif * (dist - totRad), true);
					owner->SetVelocity(owner->speed * 0.99f);

					hitBuilding = true;
				} else {
					const float part = owner->mass / (owner->mass + unit->mass);
					const float damage = ((unit->speed - owner->speed) * 0.1f).SqLength();

					owner->Move(-dif * (dist - totRad) * (1 - part), true);
					unit->Move(dif * (dist - totRad) * (part), true);

					owner->DoDamage(DamageArray(damage), ZeroVector, NULL, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);
					unit->DoDamage(DamageArray(damage), ZeroVector, NULL, -CSolidObject::DAMAGE_COLLISION_OBJECT, -1);

					owner->SetVelocity(owner->speed * 0.99f);
				}
			}

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
	UpdateFuel();

	// note: NOT AAirMoveType::SlowUpdate
	AMoveType::SlowUpdate();
}



void CStrafeAirMoveType::UpdateManeuver()
{
	const float speedf = owner->speed.w;

	switch (maneuver) {
		case 1: {
			// Immelman
			int aileron = 0;
			int elevator = 0;

			if (owner->updir.y > 0.0f) {
				if (owner->rightdir.y > maxAileron * speedf) {
					aileron = 1;
				} else if (owner->rightdir.y < -maxAileron * speedf) {
					aileron = -1;
				}
			}

			if (math::fabs(owner->rightdir.y) < maxAileron * 3.0f * speedf || owner->updir.y < 0.0f) {
				elevator = 1;
			}
			UpdateAirPhysics(0, aileron, elevator, 1, owner->frontdir);

			if ((owner->updir.y < 0.0f && owner->frontdir.y < 0.0f) || speedf < 0.8f) {
				maneuver = 0;
			}
			// [?] some seem to report that the "unlimited altitude" thing is because of these maneuvers
			if (owner->pos.y - ground->GetApproximateHeight(owner->pos.x, owner->pos.z) > wantedHeight * 4.0f) {
				maneuver = 0;
			}
			break;
		}

		case 2: {
			// inverted Immelman
			int aileron = 0;
			int elevator = 0;
			if (maneuverSubState == 0) {
				if (owner->rightdir.y >= 0.0f) {
					aileron = -1;
				} else {
					aileron = 1;
				}
			}

			if (owner->frontdir.y < -0.7f) {
				maneuverSubState = 1;
			}
			if (maneuverSubState == 1 || owner->updir.y < 0.0f) {
				elevator = 1;
			}

			UpdateAirPhysics(0, aileron, elevator, 1, owner->frontdir);

			if ((owner->updir.y > 0.0f && owner->frontdir.y > 0.0f && maneuverSubState == 1) || speedf < 0.2f)
				maneuver = 0;
			break;
		}

		default:
			UpdateAirPhysics(0, 0, 0, 1, owner->frontdir);
			maneuver = 0;
			break;
	}
}



void CStrafeAirMoveType::UpdateFighterAttack()
{
	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	SyncedFloat3& rightdir = owner->rightdir;
	SyncedFloat3& frontdir = owner->frontdir;
	SyncedFloat3& updir    = owner->updir;

	const float speedf = spd.w;

	if (speedf < 0.01f) {
		UpdateAirPhysics(0, 0, 0, 1, owner->frontdir);
		return;
	}

	if (!((gs->frameNum + owner->id) & 3)) {
		CheckForCollision();
	}

	const CUnit* attackee = owner->attackTarget;

	const bool groundTarget = (attackee == NULL) || attackee->unitDef->IsGroundUnit();
	const bool airTarget = (attackee != NULL) && attackee->unitDef->IsStrafingAirUnit(); // only count "real" aircraft (not gunships)
	const bool hasFired = (!owner->weapons.empty() && owner->weapons[0]->reloadStatus > gs->frameNum && owner->weapons[0]->salvoLeft == 0);

	if (groundTarget) {
		if (frontdir.dot(goalPos - pos) < 0 && (pos - goalPos).SqLength() < (turnRadius * turnRadius * (loopbackAttack ? 4.0f : 1.0f))) {
			SetGoal(goalPos + (pos - goalPos).Normalize2D() * turnRadius * 4.0f);
		} else if (loopbackAttack && !airTarget) {
			if (frontdir.dot(goalPos - pos) < owner->maxRange * (hasFired ? 1.0f : 0.7f))
				maneuver = 1;
		} else if (frontdir.dot(goalPos - pos) < owner->maxRange * 0.7f) {
			SetGoal(goalPos + exitVector * ((attackee != NULL) ? attackee->radius + owner->radius + 10.0f : owner->radius + 40.0f));
		}
	}

	const float3 difGoalPos = (goalPos - oldGoalPos) * SQUARE_SIZE;

	oldGoalPos = goalPos;
	goalPos += difGoalPos;

	const float goalDist = pos.distance(goalPos);
	const float3 goalDir = (goalDist > 0.0f)?
		(goalPos - pos) / goalDist:
		ZeroVector;

	float aileron = 0;
	float rudder = 0;
	float elevator = 0;
	float engine = 0;
	float gHeightAW = ground->GetHeightAboveWater(pos.x, pos.z);

	float goalDotRight = rightdir.dot(goalDir);
	const float aGoalDotFront = goalDir.dot(frontdir);
	const float goalDotFront = aGoalDotFront * 0.5f + 0.501f;

	if (goalDotFront != 0.0f) {
		goalDotRight /= goalDotFront;
	}


	if (goalDir.dot(frontdir) < -0.2f + inefficientAttackTime * 0.002f && frontdir.y > -0.2f && speedf > 2.0f && gs->randFloat() > 0.996f)
		maneuver = 1;

	if (goalDir.dot(frontdir) < -0.2f + inefficientAttackTime * 0.002f && math::fabs(frontdir.y) < 0.2f && gs->randFloat() > 0.996f && gHeightAW + 400 < pos.y) {
		maneuver = 2;
		maneuverSubState = 0;
	}

	// roll
	{
		const float maxAileronSpeedf = maxAileron * speedf;
		const float maxAileronSpeedf2 = maxAileronSpeedf * 4.0f;
		const float minPredictedHeight = pos.y + spd.y * 60.0f * math::fabs(frontdir.y) + std::min(0.0f, updir.y * 1.0f) * (GAME_SPEED * 5);
		const float maxPredictedHeight = gHeightAW + 60.0f + math::fabs(rightdir.y) * (GAME_SPEED * 5);

		if (speedf > 0.45f && minPredictedHeight > maxPredictedHeight) {
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
	}

	// yaw
	if (pos.y > gHeightAW + 30) {
		const float maxRudderSpeedf = maxRudder * speedf;

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

	float upside = 1;

	if (updir.y < -0.3f)
		upside = -1;

	// pitch
	if (speedf < 1.5f) {
		if (frontdir.y < 0.0f) {
			elevator = upside;
		} else if (frontdir.y > 0.0f) {
			elevator = -upside;
		}
	} else {
		const float gHeightR = ground->GetHeightAboveWater(pos.x + spd.x * 40.0f, pos.z + spd.z * 40.0f);
		const float hdif = std::max(gHeightR, gHeightAW) + 60 - pos.y - frontdir.y * speedf * 20.0f;

		const float maxElevatorSpeedf = maxElevator * speedf;
		const float maxElevatorSpeedf2 = maxElevatorSpeedf * speedf * 20.0f;

		float minPitch = 1.0f; // min(1.0f, hdif / (maxElevator * speedf * speedf * 20));

		if (hdif < -maxElevatorSpeedf2) {
			minPitch = -1.0f;
		} else if (hdif > maxElevatorSpeedf2) {
			minPitch = 1.0f;
		} else if (maxElevatorSpeedf2 > 0.0f) {
			minPitch = hdif / maxElevatorSpeedf2;
		}

		if (lastColWarning && lastColWarningType == 2 && frontdir.dot(lastColWarning->pos + lastColWarning->speed * 20.0f - pos - spd * 20.0f) < 0) {
			elevator = (updir.dot(lastColWarning->midPos - owner->midPos) > 0.0f)? -1 : 1;
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
		if (elevator * upside < minPitch) {
			elevator = minPitch * upside;
		}
	}

	if (groundTarget) {
		engine = 1.0f;
	} else {
		engine = std::min(1.0f, (goalDist / owner->maxRange + 1.0f - goalDir.dot(frontdir) * 0.7f));
	}

	UpdateAirPhysics(rudder, aileron, elevator, engine, owner->frontdir);
}



void CStrafeAirMoveType::UpdateAttack()
{
	UpdateFlying(wantedHeight, 1.0f);
}

void CStrafeAirMoveType::UpdateFlying(float wantedHeight, float engine)
{
	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	SyncedFloat3& rightdir = owner->rightdir;
	SyncedFloat3& frontdir = owner->frontdir;
	SyncedFloat3& updir    = owner->updir;

	// NOTE:
	//   turnRadius is often way too small, but cannot calculate one
	//   because we have no turnRate (and unitDef->turnRate can be 0)
	//   --> would lead to infinite circling without adjusting goal
	if (reservedPad != NULL) {
		const float3& padPos = (reservedPad->GetUnit())->pos;
		const float3  padVec = padPos - pos;

		if (padVec.dot(frontdir) < -0.1f && padVec.SqLength2D() < Square(std::min(1000.0f, turnRadius * spd.w))) {
			SetGoal(pos + frontdir * turnRadius * spd.w);
		} else {
			SetGoal(padPos);
		}
	}

	const float3 goalVec = goalPos - pos;

	const float goalDist2D = std::max(0.001f, goalVec.Length2D());
	// const float goalDist3D = std::max(0.001f, goalVec.Length());

	const float3 goalDir2D = goalVec / goalDist2D;
	// const float3 goalDir3D = goalVec / goalDist3D;

	float aileron = 0.0f;
	float rudder = 0.0f;
	float elevator = 0.0f;

	// do not check if the plane can be submerged here,
	// since it'll cause ground collisions later on (?)
	float gHeight = 0.0f;

	if (UseSmoothMesh())
		gHeight = std::max(smoothGround->GetHeight(pos.x, pos.z), ground->GetApproximateHeight(pos.x, pos.z));
	else
		gHeight = ground->GetHeightAboveWater(pos.x, pos.z);

	if (!((gs->frameNum + owner->id) & 3)) {
		CheckForCollision();
	}

	float otherThreat = 0.0f;
	float3 otherDir; // only used if lastColWarning != NULL

	if (lastColWarning != NULL) {
		const float3 otherDif = lastColWarning->pos - pos;
		const float otherLength = otherDif.Length();

		otherDir = (otherLength > 0.0f)?
			(otherDif / otherLength):
			ZeroVector;
		otherThreat = (otherLength > 0.0f)?
			std::max(1200.0f, goalDist2D) / otherLength * 0.036f:
			0.0f;
	}

	float goalDotRight = rightdir.dot(goalDir2D);
	const float aGoalDotFront = goalDir2D.dot(frontdir);
	const float goalDotFront = aGoalDotFront * 0.5f + 0.501f;

	if (goalDotFront > 0.01f) {
		goalDotRight /= goalDotFront;
	}

	// if goal-position is behind us and goal-distance
	// is less than our turning radius, turn the other
	// way --> often insufficient for small turn radii,
	// also need to fly straight for some distance
	if (goalDir2D.dot(frontdir) < -0.1f && goalDist2D < turnRadius) {
		if (owner->fpsControlPlayer == NULL || owner->fpsControlPlayer->fpsController.mouse2)
			goalDotRight = -goalDotRight;
	}

	if (lastColWarning != NULL) {
		goalDotRight -= otherDir.dot(rightdir) * otherThreat;
	}

	// roll
	if (spd.w > 1.5f && ((pos.y + spd.y * 10.0f) > (gHeight + wantedHeight * 0.6f))) {
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

	// yaw
	if (pos.y > gHeight + 15) {
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

	// pitch
	if (spd.w > 0.8f) {
		bool notColliding = true;

		if (lastColWarningType == 2) {
			const float3 dir = lastColWarning->midPos - owner->midPos;
			const float3 sdir = lastColWarning->speed - spd;

			if (frontdir.dot(dir + sdir * 20) < 0) {
				elevator = updir.dot(dir) > 0 ? -1 : 1;
				notColliding = false;
			}
		}

		if (notColliding) {
			const float maxElevatorSpeedf = maxElevator * 20.0f * spd.w * spd.w;
			const float gHeightAW = ground->GetHeightAboveWater(pos.x + spd.x * 40.0f, pos.z + spd.z * 40.0f);
			const float hdif = std::max(gHeight, gHeightAW) + wantedHeight - pos.y - frontdir.y * spd.w * 20.0f;

			if (hdif < -maxElevatorSpeedf && frontdir.y > -maxPitch) {
				elevator = -1.0f;
			} else if (hdif > maxElevatorSpeedf && frontdir.y < maxPitch) {
				elevator = 1.0f;
			} else if (maxElevatorSpeedf > 0.0f) {
				if (math::fabs(frontdir.y) < maxPitch)
					elevator = hdif / maxElevatorSpeedf;
			}
		}
	} else {
		if (frontdir.y < -0.1f) {
			elevator = 1.0f;
		} else if (frontdir.y > 0.15f) {
			elevator = -1.0f;
		}
	}

	UpdateAirPhysics(rudder, aileron, elevator, engine, owner->frontdir);
}



void CStrafeAirMoveType::UpdateLanded()
{
	AAirMoveType::UpdateLanded();
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

void CStrafeAirMoveType::UpdateTakeOff(float wantedHeight)
{
	const float3& pos = owner->pos;
	const float4& spd = owner->speed;

	SyncedFloat3& rightdir = owner->rightdir;
	SyncedFloat3& frontdir = owner->frontdir;
	SyncedFloat3& updir    = owner->updir;

	const float currentHeight = pos.y - (owner->unitDef->canSubmerge?
		ground->GetHeightReal(pos.x, pos.z):
		ground->GetHeightAboveWater(pos.x, pos.z));
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



float3 GetWantedLandingVelocity(
	const float3& curVelocity,
	const float3& curDirection,
	float3 landingPosVec,
	float brakeRate,
	float descendRate
) {
	const float landPosDistXZ = landingPosVec.Length2D() + 0.1f;
	const float landPosDistY = landingPosVec.y;
	const float brakeDistXZ = 0.5f * curVelocity.SqLength2D() / brakeRate;

	landingPosVec.Normalize2D();

	const float curSpeedXZ = curVelocity.Length2D();
	const float newSpeedXZ = curSpeedXZ - brakeRate * (brakeDistXZ >= landPosDistXZ || landingPosVec.dot(curDirection) < 0.0f);

	// maxSpeed is set to 0 (when landing) before we are fully
	// on the ground, so make sure to not get stuck in mid-air
	// note: assume losing altitude is easier than gaining it?
	float3 wantedVel;
	wantedVel.x = landingPosVec.x * std::max(0.0f, newSpeedXZ);
	wantedVel.z = landingPosVec.z * std::max(0.0f, newSpeedXZ);
	wantedVel.y = -descendRate;

	if (landPosDistXZ > 0.1f && curSpeedXZ > 0.1f) {
		wantedVel.y = std::max(wantedVel.y, landPosDistY / (landPosDistXZ / curSpeedXZ));
	}

	return wantedVel;
}


void CStrafeAirMoveType::UpdateLanding()
{
	const float3& pos = owner->pos;

	// if not landing on a pad, use higher brake-rate
	// to overshoot less (see the fixme comment below)
	const float modDecRate = decRate * (1 + (reservedPad == NULL));

	SyncedFloat3& rightdir = owner->rightdir;
	SyncedFloat3& frontdir = owner->frontdir;
	SyncedFloat3& updir    = owner->updir;

	// find a landing spot if we do not have one yet
	if (reservedLandingPos.x < 0.0f) {
		reservedLandingPos = FindLandingPos(modDecRate);

		// if spot is valid, mark it on the blocking-map
		// so other aircraft can not claim the same spot
		if (reservedLandingPos.x >= 0.0f) {
			const float3 originalPos = pos;

			owner->Move(reservedLandingPos, false);
			owner->Block();
			owner->Move(originalPos, false);
			// Block updates the blocking-map position (mapPos)
			// via GroundBlockingObjectMap (which calculates it
			// from object->pos) and UnBlock always uses mapPos
			// --> we cannot block in one part of the map *and*
			// unblock in another
			// owner->UnBlock();

			owner->Deactivate();
		} else {
			goalPos.ClampInBounds();
			UpdateFlying(wantedHeight, 1);
			return;
		}
	}

	// FIXME:
	//   only gets called AFTER we have already passed our goal (see ::StopMoving)
	//   this means we will always overshoot any landing position (or repair-pad)
	//   FindLandingPos() picks the spot at (pos + dir * brakingDistance) for us
	const float3 reservedLandingPosDir = reservedLandingPos - pos;
	const float3 wantedVelocity = GetWantedLandingVelocity(owner->speed, owner->frontdir, reservedLandingPosDir, modDecRate, altitudeRate);

	owner->SetVelocityAndSpeed(wantedVelocity);

	// make the aircraft right itself up and turn toward goal
	     if (rightdir.y < -0.01f) { updir -= (rightdir * 0.02f); }
	else if (rightdir.y >  0.01f) { updir += (rightdir * 0.02f); }

	     if (frontdir.y < -0.01f) { frontdir += (updir * 0.02f); }
	else if (frontdir.y >  0.01f) { frontdir -= (updir * 0.02f); }

	     if (rightdir.dot(reservedLandingPosDir) >  0.01f) { frontdir += (rightdir * 0.02f); }
	else if (rightdir.dot(reservedLandingPosDir) < -0.01f) { frontdir -= (rightdir * 0.02f); }

	owner->Move(owner->speed, true);
	owner->UpdateDirVectors(owner->IsOnGround());
	owner->UpdateMidAndAimPos();

	// see if we are at the reserved (not user-clicked) landing spot
	const float gh = ground->GetHeightAboveWater(pos.x, pos.z);
	const float gah = ground->GetHeightReal(pos.x, pos.z);
	float altitude = 0.0f;

	// can we submerge and are we still above water?
	if ((owner->unitDef->canSubmerge) && (gah < 0.0f)) {
		altitude = pos.y - gah;
		reservedLandingPos.y = gah;
	} else {
		altitude = pos.y - gh;
		reservedLandingPos.y = gh;
	}

	// collision detection does not let us get
	// closer to the ground than <radius> elmos
	// (wrt. midPos.y)
	if (altitude <= owner->radius) {
		SetState(AIRCRAFT_LANDED);
	}
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

	const float gHeight = ground->GetHeightAboveWater(pos.x, pos.z);
	const float speedf = spd.w;
	const float3 speeddir = spd / (speedf + 0.1f);

	if (owner->fpsControlPlayer != NULL) {
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

			const float3& gNormal = ground->GetNormal(pos.x, pos.z);
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
	if (aircraftState == AIRCRAFT_CRASHING && newState != AIRCRAFT_CRASHING) {
		return;
	}

	if (newState == aircraftState) {
		return;
	}

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

		case AIRCRAFT_FLYING:
			owner->Activate();
			// fall-through

		case AIRCRAFT_TAKEOFF:
			// should be in the STATE_FLYING case, but these aircraft
			// take forever to reach it reducing factory cycle-times
			owner->UnBlock();
			owner->SetPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
			break;

		case AIRCRAFT_LANDED:
			owner->useAirLos = false;

			// FIXME already inform commandAI in AIRCRAFT_LANDING!
			owner->commandAI->StopMove();
			owner->Block();
			owner->ClearPhysicalStateBit(CSolidObject::PSTATE_BIT_FLYING);
			break;
		default:
			break;
	}

	if (aircraftState != AIRCRAFT_LANDING) {
		// don't need a reserved position anymore
		reservedLandingPos.x = -1.0f;
	}
}



float3 CStrafeAirMoveType::FindLandingPos(float brakeRate) const
{
	const float3 ret = -OnesVector;
	const UnitDef* ud = owner->unitDef;

	float3 tryPos = owner->pos + owner->frontdir * ((Square(owner->speed.w) * 0.5f) / brakeRate);
	tryPos.y = ground->GetHeightReal(tryPos.x, tryPos.z);

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
	if (ground->GetSlope(tryPos.x, tryPos.z) > 0.03f)
		return ret;

	return tryPos;
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
	SetGoal(pos);
}

void CStrafeAirMoveType::StopMoving(bool callScript, bool hardStop)
{
	SetGoal(owner->pos);
	SetWantedMaxSpeed(0.0f);

	if (aircraftState != AAirMoveType::AIRCRAFT_FLYING)
		return;

	if (owner->unitDef->maxFuel <= 0.0f || owner->currentFuel > 0.0f) {
		// if not using fuel or not out of fuel, allow disregarding stop
		if (owner->unitDef->DontLand() || !autoLand) {
			return;
		}
	}

	SetState(AIRCRAFT_LANDING);
}



void CStrafeAirMoveType::Takeoff()
{
	if (aircraftState != AAirMoveType::AIRCRAFT_FLYING) {
		if ((owner->currentFuel > 0.0f) || owner->unitDef->maxFuel <= 0.0f) {
			if (aircraftState == AAirMoveType::AIRCRAFT_LANDED) {
				SetState(AAirMoveType::AIRCRAFT_TAKEOFF);
			}
			if (aircraftState == AAirMoveType::AIRCRAFT_LANDING) {
				SetState(AAirMoveType::AIRCRAFT_FLYING);
			}
		}
	}
}
