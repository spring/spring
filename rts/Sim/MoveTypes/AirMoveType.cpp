#include "StdAfx.h"
#include "AirMoveType.h"
#include "Game/GameHelper.h"
#include "Game/Player.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "Mobility.h"
#include "myMath.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Sim/Misc/GeometricObjects.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/RadarHandler.h"
#include "Sim/Projectiles/Unsynced/SmokeProjectile.h"
#include "Sim/Units/COB/CobFile.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Weapons/Weapon.h"
#include "Sound.h"
#include "mmgr.h"

CR_BIND_DERIVED(CAirMoveType, AAirMoveType, (NULL));
CR_BIND(CAirMoveType::DrawLine, );
CR_BIND(CAirMoveType::RudderInfo, );

CR_REG_METADATA(CAirMoveType, (
		CR_MEMBER(subState),

		CR_MEMBER(maneuver),
		CR_MEMBER(maneuverSubState),

		CR_MEMBER(loopbackAttack),
		CR_MEMBER(isFighter),

		CR_MEMBER(wingDrag),
		CR_MEMBER(wingAngle),
		CR_MEMBER(invDrag),
		CR_MEMBER(frontToSpeed),
		CR_MEMBER(speedToFront),
		CR_MEMBER(myGravity),

		CR_MEMBER(maxBank),
		CR_MEMBER(maxPitch),
		CR_MEMBER(maxSpeed),
		CR_MEMBER(turnRadius),

		CR_MEMBER(maxAcc),
		CR_MEMBER(maxAileron),
		CR_MEMBER(maxElevator),
		CR_MEMBER(maxRudder),

		CR_MEMBER(inSupply),

		CR_MEMBER(mySide),
		CR_MEMBER(crashAileron),
		CR_MEMBER(crashElevator),
		CR_MEMBER(crashRudder),
		
		CR_MEMBER(oldSlowUpdatePos),

		CR_MEMBER(lines),

		CR_MEMBER(rudder),
		CR_MEMBER(elevator),
		CR_MEMBER(aileronRight),
		CR_MEMBER(aileronLeft),
		CR_MEMBER(rudders),

		CR_MEMBER(lastRudderUpdate),
		CR_MEMBER(lastRudderPos),
		CR_MEMBER(lastElevatorPos),
		CR_MEMBER(lastAileronPos),

		CR_MEMBER(inefficientAttackTime),
		CR_MEMBER(exitVector),
		
		CR_RESERVED(63)
		));

CR_REG_METADATA_SUB(CAirMoveType, DrawLine, (
		CR_MEMBER(pos1), CR_MEMBER(pos2),
		CR_MEMBER(color)));

CR_REG_METADATA_SUB(CAirMoveType, RudderInfo, (CR_MEMBER(rotation)));



CAirMoveType::CAirMoveType(CUnit* owner):
	AAirMoveType(owner),
	wingDrag(0.07f),
	wingAngle(0.1f),
	frontToSpeed(0.04f),
	speedToFront(0.01f),
	maxBank(0.55f),
	maxPitch(0.35f),
	maxAcc(0.006f),
	maxAileron(0.04f),
	maxElevator(0.02f),
	maxRudder(0.01f),
	invDrag(0.995f),
	inSupply(0),
	subState(0),
	myGravity(0.8f),
	mySide(1),
	isFighter(false),
	maneuver(0),
	maneuverSubState(0),
	inefficientAttackTime(0),
	oldSlowUpdatePos(-1, -1, -1),
	loopbackAttack(false)
{
	turnRadius = 150;
	if (owner) owner->mapSquare += 1;						//to force los recalculation

	//From Aircraft::Init
	maxRudder *= 0.99f + gs->randFloat() * 0.02f;
	maxElevator *= 0.99f + gs->randFloat() * 0.02f;
	maxAileron *= 0.99f + gs->randFloat() * 0.02f;
	maxAcc *= 0.99f + gs->randFloat() * 0.02f;

	crashAileron = 1 - gs->randFloat() * gs->randFloat();
	if (gs->randInt() & 1)
		crashAileron = -crashAileron;
	crashElevator = gs->randFloat();
	crashRudder = gs->randFloat() - 0.5f;

	lastRudderUpdate = gs->frameNum;
	lastElevatorPos = 0;
	lastRudderPos = 0;
	lastAileronPos = 0;

	exitVector = gs->randVector();
	if (exitVector.y < 0)
		exitVector.y = -exitVector.y;
	exitVector.y += 1;
}



CAirMoveType::~CAirMoveType(void)
{
}



void CAirMoveType::Update(void)
{
	float3 &pos = owner->pos;

	//This is only set to false after the plane has finished constructing
	if (useHeading) {
		useHeading = false;
		SetState(AIRCRAFT_TAKEOFF);
	}

	if (owner->stunned) {
		UpdateAirPhysics(0, lastAileronPos, lastElevatorPos, 0, ZeroVector);
		goto EndNormalControl;
	}

#ifdef DIRECT_CONTROL_ALLOWED
	if (owner->directControl && !(aircraftState == AIRCRAFT_CRASHING)) {
		SetState(AIRCRAFT_FLYING);
		DirectControlStruct* dc = owner->directControl;
		inefficientAttackTime = 0;

		if (dc->forward || dc->back || dc->left || dc->right) {
			float aileron = 0;
			float elevator = 0;
			if (dc->forward)
				elevator -= 1;
			if (dc->back)
				elevator += 1;
			if (dc->right)
				aileron += 1;
			if (dc->left)
				aileron -= 1;

			UpdateAirPhysics(0, aileron, elevator, 1, owner->frontdir);
			maneuver = 0;

			goto EndNormalControl;		//ok so goto is bad i know
		}
	}
#endif

	if (reservedPad) {
		CUnit* unit = reservedPad->GetUnit();
		float3 relPos = unit->localmodel->GetPiecePos(reservedPad->GetPiece());
		float3 pos = unit->pos + (unit->frontdir * relPos.z) + (unit->updir * relPos.y) + (unit->rightdir * relPos.x);

		if (padStatus == 0) {
			if (aircraftState != AIRCRAFT_FLYING && aircraftState != AIRCRAFT_TAKEOFF)
				SetState(AIRCRAFT_FLYING);

			goalPos = pos;

			if (pos.distance2D(owner->pos) < 400) {
				padStatus = 1;
			}
			//			geometricObjects->AddLine(owner->pos,pos,1,0,1);
		} else if (padStatus == 1) {
			if (aircraftState != AIRCRAFT_LANDING)
				SetState(AIRCRAFT_LANDING);

			goalPos = pos;
			reservedLandingPos = pos;

			if (owner->pos.distance(pos) < 3 || aircraftState == AIRCRAFT_LANDED){
				padStatus = 2;
			}
			//			geometricObjects->AddLine(owner->pos,pos,10,0,1);
		} else {
			if (aircraftState != AIRCRAFT_LANDED)
				SetState(AIRCRAFT_LANDED);

			owner->pos = pos;

			owner->AddBuildPower(unit->unitDef->buildSpeed/30,unit);
			owner->currentFuel = min (owner->unitDef->maxFuel, owner->currentFuel + (owner->unitDef->maxFuel / (GAME_SPEED * owner->unitDef->refuelTime)));

			if (owner->health >= owner->maxHealth - 1 && owner->currentFuel >= owner->unitDef->maxFuel) {
				airBaseHandler->LeaveLandingPad(reservedPad);
				reservedPad=0;
				padStatus=0;
				goalPos=oldGoalPos;
				SetState(AIRCRAFT_TAKEOFF);
			}
		}
	}


	switch (aircraftState) {
	case AIRCRAFT_FLYING:
#ifdef DEBUG_AIRCRAFT
		if (selectedUnits.selectedUnits.find(this) != selectedUnits.selectedUnits.end()) {
			logOutput.Print("Flying %i %i %.1f %i", moveState, fireState, inefficientAttackTime, (int) isFighter);
		}
#endif
		owner->restTime = 0;
		if (owner->userTarget || owner->userAttackGround) {
			inefficientAttackTime = min(inefficientAttackTime, (float)gs->frameNum - owner->lastFireWeapon);
			if (owner->userTarget) {
				goalPos = owner->userTarget->pos;
			} else {
				goalPos = owner->userAttackPos;
			}
			if(maneuver) {
				UpdateManeuver();
				inefficientAttackTime = 0;
			} else if(isFighter && goalPos.distance(pos) < owner->maxRange * 4) {
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
		break;
	case AIRCRAFT_LANDED:
		inefficientAttackTime = 0;
		UpdateLanded();
		break;
	case AIRCRAFT_LANDING:
		inefficientAttackTime = 0;
		UpdateLanding();
		break;
	case AIRCRAFT_CRASHING:
		owner->crashing = true;
		UpdateAirPhysics(crashRudder, crashAileron, crashElevator, 0, owner->frontdir);
		SAFE_NEW CSmokeProjectile(owner->midPos, gs->randVector() * 0.08f, 100 + gs->randFloat() * 50, 5, 0.2f, owner, 0.4f);
		if (!(gs->frameNum & 3) && max(0.f, ground->GetApproximateHeight(pos.x, pos.z)) + 5 + owner->radius > pos.y)
			owner->KillUnit(true, false, 0);
		break;
	case AIRCRAFT_TAKEOFF:
		UpdateTakeOff(wantedHeight);
	default:
		break;
	}

EndNormalControl:


	if (pos != oldpos) {
		oldpos = pos;
		bool hitBuilding = false;

		if (collide && (aircraftState == AIRCRAFT_FLYING || aircraftState == AIRCRAFT_CRASHING)) {
			vector<CUnit*> nearUnits = qf->GetUnitsExact(pos, owner->radius + 6);
			vector<CUnit*>::iterator ui;

			for (ui = nearUnits.begin(); ui != nearUnits.end(); ++ui) {
				float sqDist = (pos - (*ui)->pos).SqLength();
				float totRad = owner->radius + (*ui)->radius;

				if (sqDist < totRad * totRad && sqDist != 0) {
					float dist = sqrt(sqDist);
					float3 dif = pos - (*ui)->pos;
					dif /= dist;

					if ((*ui)->immobile) {
						pos -= dif * (dist - totRad);
						owner->midPos = pos + owner->frontdir * owner->relMidPos.z + owner->updir * owner->relMidPos.y + owner->rightdir * owner->relMidPos.x;
						owner->speed *= 0.99f;
						float damage = (((*ui)->speed - owner->speed) * 0.1f).SqLength();
						owner->DoDamage(DamageArray() * damage, 0, ZeroVector);
						(*ui)->DoDamage(DamageArray() * damage, 0, ZeroVector);
						hitBuilding = true;
					} else {
						float part = owner->mass / (owner->mass + (*ui)->mass);
						pos -= dif * (dist - totRad) * (1 - part);
						owner->midPos = pos + owner->frontdir * owner->relMidPos.z + owner->updir * owner->relMidPos.y + owner->rightdir * owner->relMidPos.x;
						CUnit* u = (CUnit*)(*ui);
						u->pos += dif * (dist - totRad) * (part);
						u->midPos = u->pos + u->frontdir * u->relMidPos.z + u->updir * u->relMidPos.y + u->rightdir * u->relMidPos.x;
						float damage = (((*ui)->speed - owner->speed) * 0.1f).SqLength();
						owner->DoDamage(DamageArray() * damage, 0, ZeroVector);
						(*ui)->DoDamage(DamageArray() * damage, 0, ZeroVector);
						owner->speed *= 0.99f;
					}
				}
			}
			if (hitBuilding && owner->crashing) {
				// if our collision sphere overlapped with that
				// of a building and we're crashing, die right
				// now rather than waiting until we're close
				// enough to the ground (which may never happen
				// if eg. we're going down over a crowded field
				// of windmills due to col-det)
				owner->KillUnit(true, false, 0);
				return;
			}
		}

		if (pos.x < 0) {
			pos.x += 1.5f;
			owner->midPos.x += 1.5f;
		} else if (pos.x > float3::maxxpos) {
			pos.x -= 1.5f;
			owner->midPos.x -= 1.5f;
		}

		if (pos.z < 0) {
			pos.z += 1.5f;
			owner->midPos.z += 1.5f;
		} else if (pos.z > float3::maxzpos) {
			pos.z -= 1.5f;
			owner->midPos.z -= 1.5f;
		}
	}

#ifdef DEBUG_AIRCRAFT
	if (lastColWarningType==1) {
		int g = geometricObjects->AddLine(owner->pos, lastColWarning->pos, 10, 1, 1);
		geometricObjects->SetColor(g, 0.2f, 1, 0.2f, 0.6f);
	} else if(lastColWarningType == 2) {
		int g=geometricObjects->AddLine(owner->pos, lastColWarning->pos, 10, 1, 1);
		if (owner->frontdir.dot(lastColWarning->midPos + lastColWarning->speed * 20 - owner->midPos - owner->speed * 20) < 0)
			geometricObjects->SetColor(g, 1, 0.2f, 0.2f, 0.6f);
		else
			geometricObjects->SetColor(g, 1, 1, 0.2f, 0.6f);
	}
#endif
}



void CAirMoveType::SlowUpdate(void)
{
	if (aircraftState != AIRCRAFT_LANDED && owner->unitDef->maxFuel > 0)
		owner->currentFuel = max (0.f, owner->currentFuel - (16.f / GAME_SPEED));

	if (owner->pos != oldSlowUpdatePos) {
		oldSlowUpdatePos = owner->pos;

		//try to handle aircraft getting unlimited height
		if (owner->pos.y - ground->GetApproximateHeight(owner->pos.x, owner->pos.z) > wantedHeight * 5 + 100)
			owner->pos.y = ground->GetApproximateHeight(owner->pos.x, owner->pos.z) + wantedHeight * 5  + 100;

		int newmapSquare = ground->GetSquare(owner->pos);
		if(newmapSquare != owner->mapSquare){
			owner->mapSquare = newmapSquare;
			float oldlh = owner->losHeight;
			float h = owner->pos.y - ground->GetApproximateHeight(owner->pos.x, owner->pos.z);
			owner->losHeight = h + 5;
			loshandler->MoveUnit(owner, false);
			if (owner->hasRadarCapacity)
				radarhandler->MoveUnit(owner);

			owner->losHeight = oldlh;
		}
		qf->MovedUnit(owner);
		owner->isUnderWater = owner->pos.y + owner->model->height < 0;
	}
}



void CAirMoveType::UpdateManeuver(void)
{
#ifdef DEBUG_AIRCRAFT
	if (selectedUnits.selectedUnits.find(this) != selectedUnits.selectedUnits.end()) {
		logOutput.Print("UpdataMan %i %i", maneuver, maneuverSubState);
	}
#endif
	float speedf = owner->speed.Length();
	switch(maneuver) {
	case 1: {	//immelman
		int aileron = 0, elevator = 0;
		if (owner->updir.y > 0) {
			if (owner->rightdir.y > maxAileron * speedf) {
				aileron = 1;
			} else if (owner->rightdir.y < -maxAileron * speedf) {
				aileron = -1;
			}
		}
		if (fabs(owner->rightdir.y) < maxAileron * 3 * speedf || owner->updir.y < 0)
			elevator = 1;
		UpdateAirPhysics(0, aileron, elevator, 1, owner->frontdir);
		if ((owner->updir.y < 0 && owner->frontdir.y < 0) || speedf < 0.8f)
			maneuver = 0;
		//some seem to report that the "unlimited altitude" thing is because of these maneuvers
		if(owner->pos.y - ground->GetApproximateHeight(owner->pos.x, owner->pos.z) > wantedHeight * 4)
			maneuver = 0;
		break;}
	case 2: {	//inverted immelman
		int aileron = 0, elevator = 0;
		if (maneuverSubState == 0) {
			if (owner->rightdir.y >= 0) {
				aileron = -1;
			} else {
				aileron = 1;
			}
		}
		if (owner->frontdir.y < -0.7f)
			maneuverSubState = 1;
		if (maneuverSubState == 1 || owner->updir.y < 0)
			elevator = 1;
		UpdateAirPhysics(0, aileron, elevator, 1, owner->frontdir);

		if ((owner->updir.y > 0 && owner->frontdir.y > 0 && maneuverSubState == 1) || speedf < 0.2f)
			maneuver = 0;
		break;}
	default:
		UpdateAirPhysics(0, 0, 0, 1, owner->frontdir);
		maneuver = 0;
		break;
	}
}



void CAirMoveType::UpdateFighterAttack(void)
{
	float3 &pos = owner->pos;
	SyncedFloat3 &rightdir = owner->rightdir;
	SyncedFloat3 &frontdir = owner->frontdir;
	SyncedFloat3 &updir = owner->updir;
	float3 &speed = owner->speed;

	float speedf = owner->speed.Length();
	if (speedf < 0.01f){
		UpdateAirPhysics(0, 0, 0, 1, owner->frontdir);
		return;
	}

	if (!((gs->frameNum + owner->id) & 3))
		CheckForCollision();

	bool groundTarget = !owner->userTarget || !owner->userTarget->unitDef->canfly || owner->userTarget->unitDef->hoverAttack;
	bool airTarget = owner->userTarget && owner->userTarget->unitDef->canfly && !owner->userTarget->unitDef->hoverAttack;	//only "real" aircrafts (non gunship)
	if (groundTarget) {
		if (frontdir.dot(goalPos - pos) < 0 && (pos - goalPos).SqLength() < turnRadius * turnRadius * (loopbackAttack ? 4 : 1)) {
			float3 dif = pos - goalPos;
			dif.y = 0;
			dif.Normalize();
			goalPos = goalPos + dif * turnRadius * 4;
		} else if (loopbackAttack && !airTarget) {
			bool hasFired = false;
			if (!owner->weapons.empty() && owner->weapons[0]->reloadStatus > gs->frameNum && owner->weapons[0]->salvoLeft == 0)
				hasFired = true;
			if (frontdir.dot(goalPos - pos) < owner->maxRange * (hasFired ? 1.0f : 0.7f))
				maneuver = 1;
		} else if (frontdir.dot(goalPos - pos) < owner->maxRange * 0.7f) {
			goalPos += exitVector * (owner->userTarget ? owner->userTarget->radius + owner->radius + 10 : owner->radius + 40);
		}
	}
	float3 tgp = goalPos + (goalPos - oldGoalPos) * 8;
	oldGoalPos = goalPos;
	goalPos = tgp;

	float goalLength = (goalPos - pos).Length();
	float3 goalDir = (goalPos - pos) / goalLength;

	float aileron = 0;
	float rudder = 0;
	float elevator = 0;
	float engine = 0;

	float gHeight = ground->GetHeight(pos.x, pos.z);

	float goalDot = rightdir.dot(goalDir);
	goalDot /= goalDir.dot(frontdir) * 0.5f + 0.501f;

	if (goalDir.dot(frontdir) < -0.2f + inefficientAttackTime * 0.002f && frontdir.y > -0.2f && speedf > 2.0f && gs->randFloat() > 0.996f)
		maneuver = 1;

	if (goalDir.dot(frontdir) < -0.2f + inefficientAttackTime * 0.002f && fabs(frontdir.y) < 0.2f && gs->randFloat() > 0.996f && gHeight + 400 < pos.y) {
		maneuver = 2;
		maneuverSubState = 0;
	}

	//roll
	if (speedf > 0.45f && pos.y + owner->speed.y * 60 * fabs(frontdir.y) + min(0.f, updir.y) * 150 > gHeight + 60 + fabs(rightdir.y) * 150) {
		float goalBankDif = goalDot + rightdir.y * 0.2f;
		if (goalBankDif > maxAileron * speedf * 4) {
			aileron = 1;
		} else if(goalBankDif < -maxAileron * speedf * 4) {
			aileron = -1;
		} else {
			aileron = goalBankDif / (maxAileron * speedf * 4);
		}
	} else {
		if (rightdir.y > 0) {
			if (rightdir.y > maxAileron * speedf || frontdir.y < -0.7f)
				aileron = 1;
			else
				aileron = rightdir.y / (maxAileron * speedf);
		} else {
			if (rightdir.y < -maxAileron * speedf || frontdir.y < -0.7f)
				aileron = -1;
			else
				aileron = rightdir.y / (maxAileron * speedf);
		}
	}

	//yaw
	if (pos.y > gHeight + 30) {
		if (goalDot < -maxRudder * speedf) {
			rudder = -1;
		} else if (goalDot > maxRudder * speedf) {
			rudder = 1;
		} else {
			rudder = goalDot / (maxRudder * speedf);
		}
	}

	float upside = 1;
	if (updir.y < -0.3f)
		upside = -1;

	//pitch
	if (speedf < 1.5f) {
		if (frontdir.y < 0.0f) {
			elevator=upside;
		} else if(frontdir.y > 0.0f) {
			elevator=-upside;
		}
	} else {
		float gHeight2 = ground->GetHeight(pos.x + speed.x * 40, pos.z + speed.z * 40);
		float hdif = max(gHeight, gHeight2) + 60 - pos.y - frontdir.y * speedf * 20;
		float minPitch;//=min(1.0f,hdif/(maxElevator*speedf*speedf*20));

		if (hdif < -(maxElevator * speedf * speedf * 20)) {
			minPitch = -1;
		} else if (hdif > (maxElevator * speedf * speedf * 20)) {
			minPitch = 1;
		} else {
			minPitch = hdif / (maxElevator * speedf * speedf * 20);
		}

/*		if(pos.y+min(0,owner->speed.y)*70*fabs(frontdir.y)+min(0,updir.y)*50<gHeight+50){
		if(frontdir.y<0.5f){
			elevator=upside;
		} else if(frontdir.y>0.55f){
			elevator=-upside;
		}*/
//	} else {
		if(lastColWarning && lastColWarningType == 2 && frontdir.dot(lastColWarning->pos + lastColWarning->speed * 20 - pos-owner->speed * 20) < 0) {
/*			float pitchMod=updir.y>0?1:-1;
			if(lastColWarning->pos.y>pos.y)
				elevator=-pitchMod;
			else
				elevator=pitchMod;
/*/			elevator = updir.dot(lastColWarning->midPos - owner->midPos) > 0 ? -1 : 1;/**/
		} else {
			float hdif = goalDir.dot(updir);
			if (hdif < -maxElevator * speedf) {
				elevator = -1;
			} else if (hdif > maxElevator * speedf) {
				elevator = 1;
			} else {
				elevator = hdif / (maxElevator * speedf);
			}
		}
		if (elevator * upside < minPitch)
			elevator = minPitch * upside;
	}
#ifdef DEBUG_AIRCRAFT
	if (selectedUnits.selectedUnits.find(this) != selectedUnits.selectedUnits.end()){
		logOutput.Print("FAttack %.1f %.1f %.2f", pos.y - gHeight, goalLength, goalDir.dot(frontdir));
	}
#endif

	if(groundTarget)
		engine = 1;
	else
		engine = min(1.f, (float)(goalLength / owner->maxRange + 1 - goalDir.dot(frontdir) * 0.7f));

	UpdateAirPhysics(rudder, aileron, elevator, engine, owner->frontdir);
/*
	std::vector<CWeapon*>::iterator wi;
	for(wi=owner->weapons.begin();wi!=owner->weapons.end();++wi){
		(*wi)->targetPos=goalPos;
		if(owner->userTarget){
			(*wi)->AttackUnit(owner->userTarget,true);
		}
	}*/
/*	DrawLine dl;
	dl.color=UpVector;
	dl.pos1=pos;
	dl.pos2=goalPos;
	lines.push_back(dl);
	dl.color=float3(1,0,0);
	dl.pos1=pos;
	dl.pos2=pos+frontdir*maxRange;
	lines.push_back(dl);/**/
}



void CAirMoveType::UpdateAttack(void)
{
	/*
	std::vector<CWeapon*>::iterator wi;
	for (wi = owner->weapons.begin(); wi != owner->weapons.end(); ++wi) {
		(*wi)->targetPos = goalPos;
		if (owner->userTarget) {
			(*wi)->AttackUnit(owner->userTarget, true);
		}
	}
	*/
	UpdateFlying(wantedHeight, 1);
}



void CAirMoveType::UpdateFlying(float wantedHeight, float engine)
{
	float3 &pos = owner->pos;
	SyncedFloat3 &rightdir = owner->rightdir;
	SyncedFloat3 &frontdir = owner->frontdir;
	SyncedFloat3 &updir = owner->updir;
	float3 &speed = owner->speed;

	float speedf = speed.Length();
	float goalLength = (goalPos - pos).Length2D() + 0.01f;
	float3 goalDir = (goalPos - pos) / goalLength;
	float3 adjustedGoalDir = float3(goalPos.x,0,goalPos.z) - float3(pos.x,0,pos.z);
	goalDir.Normalize();
	adjustedGoalDir.Normalize();

	float aileron = 0.0f;
	float rudder = 0.0f;
	float elevator = 0.0f;

	float gHeight = ground->GetHeight(pos.x, pos.z);

	if (!((gs->frameNum + owner->id) & 3))
		CheckForCollision();

	float otherThreat = 0;
	float3 otherDir;
	if (lastColWarning) {
		float3 otherDif = lastColWarning->pos - pos;
		float otherLength = otherDif.Length();
		otherDir = otherDif / (otherLength + 0.01f);
		otherThreat = max(1200.0f, goalLength) / otherLength * 0.036f;
	}

	float goalDot = rightdir.dot(adjustedGoalDir);
	goalDot /= adjustedGoalDir.dot(frontdir) * 0.5f + 0.501f;
	if (adjustedGoalDir.dot(frontdir) < -0.1f && goalLength < turnRadius
#ifdef DIRECT_CONTROL_ALLOWED
		&& (!owner->directControl || owner->directControl->mouse2)
#endif
		)
		goalDot =- goalDot;
	if (lastColWarning) {
		goalDot -= otherDir.dot(rightdir) * otherThreat;
	}

	// roll
	if (speedf > 1.5f && pos.y + speed.y * 10 > gHeight + wantedHeight * 0.6f) {
		float goalBankDif = goalDot + rightdir.y * 0.5f;
		if (goalBankDif > maxAileron*speedf * 4 && rightdir.y > -maxBank) {
			aileron = 1;
		} else if (goalBankDif < -maxAileron * speedf * 4 && rightdir.y < maxBank) {
			aileron = -1;
		} else {
			if (fabs(rightdir.y) < maxBank)
				aileron = goalBankDif / (maxAileron * speedf * 4);
			else {
				if (rightdir.y < 0 && goalBankDif < 0)
					aileron = -1;
				else if (rightdir.y > 0 && goalBankDif > 0)
					aileron = 1;
			}
		}
	} else {
		if (rightdir.y > 0.01f) {
			aileron = 1;
		} else if (rightdir.y < -0.01f) {
			aileron = -1;
		}
	}

	// yaw
	if (pos.y > gHeight + 15) {
		if (goalDot < -maxRudder * speedf * 2) {
			rudder = -1;
		} else if (goalDot > maxRudder * speedf * 2) {
			rudder = 1;
		} else {
			rudder = goalDot / (maxRudder * speedf * 2);
		}
	}

	// pitch
	if (speedf > 0.8f) {
		bool notColliding = true;
		if (lastColWarningType == 2) {
			const float3 dir = lastColWarning->midPos - owner->midPos;
			const float3 sdir = lastColWarning->speed - owner->speed;
			if (frontdir.dot(dir + sdir * 20) < 0) {
				elevator = updir.dot(dir) > 0 ? -1 : 1;
				notColliding = false;
			}
		}
		if (notColliding) {
			float gHeight2 = ground->GetHeight(pos.x + speed.x * 40, pos.z + speed.z * 40);
			float hdif = max(gHeight, gHeight2) + wantedHeight - pos.y - frontdir.y * speedf * 20;
			if (hdif < -(maxElevator * speedf * speedf * 20) && frontdir.y > -maxPitch) {
				elevator = -1;
			} else if (hdif > (maxElevator * speedf * speedf * 20) && frontdir.y < maxPitch) {
				elevator = 1;
			} else {
				if (fabs(frontdir.y) < maxPitch)
					elevator = hdif / (maxElevator * speedf * speedf * 20);
			}
		}
	}
	else {
		if (frontdir.y < -0.1f) {
			elevator = 1;
		} else if (frontdir.y > 0.15f) {
			elevator = -1;
		}
	}

	UpdateAirPhysics(rudder, aileron, elevator, engine, owner->frontdir);
}



void CAirMoveType::UpdateLanded(void)
{
	float3 &pos = owner->pos;
	SyncedFloat3 &rightdir = owner->rightdir;
	SyncedFloat3 &frontdir = owner->frontdir;
	SyncedFloat3 &updir = owner->updir;
	float3 &speed = owner -> speed;

	speed = ZeroVector;

	updir = ground->GetNormal(pos.x, pos.z);
	frontdir.Normalize();
	rightdir = frontdir.cross(updir);
	rightdir.Normalize();
	updir = rightdir.cross(frontdir);
}



void CAirMoveType::UpdateTakeOff(float wantedHeight)
{
	float3& pos=owner->pos;
	float3& speed=owner->speed;

	float h = 0.0f;
	if (owner->unitDef->canSubmerge)
		h = pos.y - ground->GetApproximateHeight(pos.x, pos.z);
	else
		h = pos.y - ground->GetHeight(pos.x,pos.z);

	if (h > wantedHeight)
		SetState(AIRCRAFT_FLYING);

	if ((h + speed.y * 20) < (wantedHeight * 1.02f))
		speed.y += maxAcc;
	else
		speed.y -= maxAcc;

	if (h > wantedHeight * 0.4f)
		speed += owner->frontdir * maxAcc;

	speed *= invDrag;

	owner->frontdir.Normalize();
	owner->rightdir = owner->frontdir.cross(owner->updir);
	owner->rightdir.Normalize();
	owner->updir = owner->rightdir.cross(owner->frontdir);

	pos += speed;
	owner->midPos = pos + (owner->frontdir * owner->relMidPos.z) + (owner->updir * owner->relMidPos.y) + (owner->rightdir * owner->relMidPos.x);
}



void CAirMoveType::UpdateLanding(void)
{
	float3 &pos = owner->pos;
	SyncedFloat3 &rightdir = owner->rightdir;
	SyncedFloat3 &frontdir = owner->frontdir;
	SyncedFloat3 &updir = owner->updir;
	float3 &speed = owner->speed;
	float speedf = speed.Length();

	// find a landing spot if we dont have one
	if (reservedLandingPos.x < 0) {
		reservedLandingPos = FindLandingPos();

		if (reservedLandingPos.x > 0) {
			reservedLandingPos.y += wantedHeight;
			float3 tp = pos;
			pos = reservedLandingPos;
			owner->physicalState = CSolidObject::OnGround;
			owner->Block();
			owner->physicalState = CSolidObject::Flying;
			pos = tp;
			owner->Deactivate();
			owner->cob->Call(COBFN_StopMoving);
		} else {
			goalPos.CheckInBounds();
			UpdateFlying(wantedHeight, 1);
			return;
		}
	} else {
		// see if landing spot is still empty
		/*
		float3 tpos = owner->pos;
		owner->pos = reservedLandingPos;
		int2 mp = owner->GetMapPos();
		owner->pos = tpos;

		for (int z = mp.y; z < mp.y + owner -> ysize; z++) {
			for (int x = mp.x; x < mp.x + owner -> xsize; x++) {
				if (readmap -> groundBlockingObjectMap[z * gs -> mapx + x] != owner){
					owner -> UnBlock();
					reservedLandingPos.x = -1;
					UpdateFlying(wantedHeight, 1);
					return;
				}
			}
		}
		*/
	}

	// update our speed
	float3 dif = reservedLandingPos - pos;
	float dist = dif.Length();
	dif /= dist;

	float wsf = min(owner->maxSpeed, dist / speedf * 1.8f * maxAcc);
	float3 wantedSpeed = dif * wsf;

	float3 delta = wantedSpeed - speed;
	float dl = delta.Length();

	if (dl < maxAcc * 3)
		speed = wantedSpeed;
	else
		speed += delta / dl * maxAcc * 3;

	pos += speed;

	// make the aircraft right itself up and turn toward goal
	if (rightdir.y < -0.01f)
		updir -= rightdir * 0.02f;
	else if (rightdir.y > 0.01f)
		updir += rightdir * 0.02f;

	if (frontdir.y < -0.01f)
		frontdir += updir * 0.02f;
	else if (frontdir.y > 0.01f)
		frontdir -= updir * 0.02f;

	if (rightdir.dot(dif) > 0.01f)
		frontdir += rightdir * 0.02f;
	else if (rightdir.dot(dif) < -0.01f)
		frontdir -= rightdir * 0.02f;

	frontdir.Normalize();
	rightdir = frontdir.cross(updir);
	rightdir.Normalize();
	updir = rightdir.cross(frontdir);

	owner->midPos = pos + (frontdir * owner->relMidPos.z) + (updir * owner->relMidPos.y) + (rightdir * owner->relMidPos.x);

	// see if we are at the reserved (not user-clicked) landing spot
	if (dist < 1) {
		float gh = ground -> GetHeight(pos.x, pos.z);
		float gah = ground -> GetApproximateHeight(pos.x, pos.z);
		float alt = 0.0f;

		// can we submerge and are we still above water?
		if ((owner -> unitDef -> canSubmerge) && (gah < 0)) {
			alt = pos.y - gah;
			reservedLandingPos.y = gah;
		}
		else {
			alt = pos.y - gh;
			reservedLandingPos.y = gh;
		}

		if (alt <= 1)
			SetState(AIRCRAFT_LANDED);
	}
}



void CAirMoveType::UpdateAirPhysics(float rudder, float aileron, float elevator, float engine, const float3& engineVector)
{
	float3 &pos = owner->pos;
	SyncedFloat3 &rightdir = owner->rightdir;
	SyncedFloat3 &frontdir = owner->frontdir;
	SyncedFloat3 &updir = owner->updir;
	float3 &speed = owner->speed;
	bool nextPosInBounds = true;

	lastRudderPos = rudder;
	lastAileronPos = aileron;
	lastElevatorPos = elevator;

	float speedf = speed.Length();
	float3 speeddir = frontdir;
	if (speedf != 0)
		speeddir = speed / speedf;

	float gHeight = ground->GetHeight(pos.x, pos.z);

#ifdef DIRECT_CONTROL_ALLOWED
	if (owner->directControl) {
		if ((pos.y - gHeight) > wantedHeight * 1.2f) {
			engine = max(0.0f, min(engine, 1 - (pos.y - gHeight - wantedHeight * 1.2f) / wantedHeight));
		}
		// check next position given current (unadjusted) pos and speed
		nextPosInBounds = (pos + speed).CheckInBounds();
	}
#endif


	speed += engineVector * maxAcc * engine;
	speed.y += gs->gravity * myGravity;
	speed *= invDrag;

	float3 wingDir = updir * (1 - wingAngle) - frontdir * wingAngle;
	float wingForce = wingDir.dot(speed) * wingDrag;
	speed -= wingDir * wingForce;

	frontdir += rightdir * rudder * maxRudder * speedf;
	updir += rightdir * aileron * maxAileron * speedf;
	frontdir += updir * elevator * maxElevator * speedf;
	frontdir += (speeddir - frontdir) * frontToSpeed;
	speed += (frontdir * speedf - speed) * speedToFront;

	if (nextPosInBounds) {
		pos += speed;
	}


	if (gHeight > owner->pos.y - owner->model->radius * 0.2f && !owner->crashing) {
		float3 gNormal = ground->GetNormal(pos.x, pos.z);
		float impactSpeed =- speed.dot(gNormal);

		if (impactSpeed > 0) {
			if (owner->stunned) {
				float damage = 0;
				if (impactSpeed > 0.5f)
					damage += impactSpeed * impactSpeed * 1000;
				if (updir.dot(gNormal) < 0.95f)
					damage += (1 - (updir.dot(gNormal))) * 1000;
				if (damage > 0)
					owner->DoDamage(DamageArray() * (damage * 0.4f), 0, ZeroVector);	//only do damage while stunned for now
			}
			pos.y = gHeight + owner->model->radius * 0.2f + 0.01f;
			speed += gNormal * (impactSpeed * 1.5f);
			speed *= 0.95f;
			updir = gNormal - frontdir * 0.1f;
			frontdir = updir.cross(frontdir.cross(updir));
		}
	}

	frontdir.Normalize();
	rightdir = frontdir.cross(updir);
	rightdir.Normalize();
	updir = rightdir.cross(frontdir);

	owner->midPos = pos + frontdir * owner->relMidPos.z + updir * owner->relMidPos.y + rightdir * owner->relMidPos.x;

#ifdef DEBUG_AIRCRAFT
	if(selectedUnits.selectedUnits.find(this)!=selectedUnits.selectedUnits.end()){
		logOutput.Print("UpdataAP %.1f %.1f %.1f %.1f",speedf,pos.x,pos.y,pos.z);
//		logOutput.Print("Rudders %.1f %.1f %.1f %.1f",rudder,aileron,elevator,engine);
	}
#endif
}



void CAirMoveType::SetState(AAirMoveType::AircraftState state)
{
	assert(state != AIRCRAFT_HOVERING);
	if (aircraftState == AIRCRAFT_CRASHING || state == aircraftState)
		return;

/*	if (state == AIRCRAFT_LANDING)
		owner->cob->Call(COBFN_Deactivate);
		else if (state == aircraft_flying)
		//cob->Call(COBFN_Activate); */

	if (state == AIRCRAFT_FLYING) {
		owner->Activate();
		owner->cob->Call(COBFN_StartMoving);
	}

	if (state == AIRCRAFT_LANDED) {
		owner->heading = GetHeadingFromVector(owner->frontdir.x, owner->frontdir.z);
		owner->physicalState = CSolidObject::OnGround;
		owner->useAirLos = false;
	} else {
		owner->physicalState = CSolidObject::Flying;
		owner->useAirLos = true;
		if(state != AIRCRAFT_LANDING) {
			reservedLandingPos.x = -1;
			owner->UnBlock();
		}
	}

	if (aircraftState == AIRCRAFT_LANDED && reservedLandingPos.x > 0) {
		reservedLandingPos.x = -1;
	}
	subState = 0;

	//make sure we only go into takeoff if actually landed
	if (state != AIRCRAFT_TAKEOFF || aircraftState == AIRCRAFT_LANDED)
		aircraftState = state;
	else
		aircraftState = AIRCRAFT_TAKEOFF;
}



void CAirMoveType::ImpulseAdded(void)
{
	if (aircraftState == AIRCRAFT_FLYING) {
		owner->speed += owner->residualImpulse;
		owner->residualImpulse = ZeroVector;
	}
}



float3 CAirMoveType::FindLandingPos(void)
{
	float3 ret(-1, -1, -1);
	float3 tryPos = owner->pos + owner->speed * owner->speed.Length() / (maxAcc * 3);
	tryPos.y = ground -> GetHeight2(tryPos.x, tryPos.z);

	if ((tryPos.y < 0) && !(owner->unitDef->floater || owner->unitDef->canSubmerge)) {
		return ret;
	}

	tryPos.CheckInBounds();

	float3 tpos = owner->pos;
	owner->pos = tryPos;
	int2 mp = owner->GetMapPos();
	owner->pos = tpos;

	for (int z = mp.y; z < mp.y + owner -> ysize; z++) {
		for (int x = mp.x; x < mp.x + owner -> xsize; x++) {
			if (readmap -> groundBlockingObjectMap[z * gs->mapx + x]) {
				return ret;
			}
		}
	}

	if (ground->GetSlope(tryPos.x, tryPos.z) > 0.03f)
		return ret;

	return tryPos;
}



void CAirMoveType::CheckForCollision(void)
{
	if (!collide) return;

	SyncedFloat3& pos = owner->midPos;
	SyncedFloat3& forward = owner->frontdir;
	float3 midTestPos = pos + forward * 121;

	std::vector<CUnit*> others = qf->GetUnitsExact(midTestPos, 115);

	float dist = 200;
	if (lastColWarning) {
		DeleteDeathDependence(lastColWarning);
		lastColWarning = 0;
		lastColWarningType = 0;
	}

	for (std::vector<CUnit*>::iterator ui = others.begin(); ui != others.end(); ++ui) {
		if (*ui == owner || !(*ui)->unitDef->canfly)
			continue;
		SyncedFloat3& op = (*ui)->midPos;
		float3 dif = op - pos;
		float3 forwardDif = forward * (forward.dot(dif));
		if (forwardDif.SqLength() < dist * dist) {
			float frontLength = forwardDif.Length();
			float3 ortoDif = dif - forwardDif;
			//note that the radiuses is multiplied by two since we rely on
			//the aircrafts having to small radiuses (see unitloader)
			
			// FIXME: with the new collision volumes, is this still true?

			float minOrtoDif = ((*ui)->radius + owner->radius) * 2 + frontLength * 0.1f + 10;
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
	for (std::vector<CUnit*>::iterator ui = others.begin(); ui != others.end(); ++ui) {
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



void CAirMoveType::DependentDied(CObject* o)
{
	if(o == lastColWarning){
		lastColWarning = 0;
		lastColWarningType = 0;
	}
	AMoveType::DependentDied(o);
}



void CAirMoveType::SetMaxSpeed(float speed)
{
	maxSpeed = speed;
	if (maxAcc != 0 && maxSpeed != 0) {
		//meant to set the drag such that the maxspeed becomes what it should be
		float drag = 1.0f / (maxSpeed / GAME_SPEED * 1.1f / maxAcc) - wingAngle * wingAngle * wingDrag;
		invDrag = 1 - drag;
	}
}



void CAirMoveType::KeepPointingTo(float3 pos, float distance, bool aggressive)
{
}



void CAirMoveType::StartMoving(float3 pos, float goalRadius)
{
	SetGoal(pos);
}



void CAirMoveType::StartMoving(float3 pos, float goalRadius, float speed)
{
	SetWantedMaxSpeed(speed);
	SetGoal(pos);
}



void CAirMoveType::StopMoving()
{
	SetGoal(owner->pos);
	if ((aircraftState == AAirMoveType::AIRCRAFT_FLYING)
	   && !owner->unitDef->DontLand() && autoLand) {
		SetState(AAirMoveType::AIRCRAFT_LANDING);
	}
}



void CAirMoveType::Takeoff()
{
	if (aircraftState == AAirMoveType::AIRCRAFT_LANDED) {
		SetState(AAirMoveType::AIRCRAFT_TAKEOFF);
	}
	if (aircraftState == AAirMoveType::AIRCRAFT_LANDING) {
		SetState(AAirMoveType::AIRCRAFT_FLYING);
	}
}



bool CAirMoveType::IsFighter()
{
	return isFighter;
}
