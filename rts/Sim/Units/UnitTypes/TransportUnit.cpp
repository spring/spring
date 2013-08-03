/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TransportUnit.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnitsHandler.h"
#include "Map/Ground.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/HoverAirMoveType.h"
#include "Sim/MoveTypes/GroundMoveType.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/QuadField.h"
#include "System/EventHandler.h"
#include "System/myMath.h"
#include "System/creg/STL_List.h"

CR_BIND_DERIVED(CTransportUnit, CUnit, );

CR_REG_METADATA(CTransportUnit, (
	CR_MEMBER(transportedUnits),
	CR_MEMBER(transportCapacityUsed),
	CR_MEMBER(transportMassUsed),
	CR_RESERVED(16),
	CR_POSTLOAD(PostLoad)
));

CR_BIND(CTransportUnit::TransportedUnit,);

CR_REG_METADATA_SUB(CTransportUnit,TransportedUnit,(
	CR_MEMBER(unit),
	CR_MEMBER(piece),
	CR_MEMBER(size),
	CR_MEMBER(mass),
	CR_RESERVED(8)
));

CTransportUnit::CTransportUnit(): transportCapacityUsed(0), transportMassUsed(0)
{
}


void CTransportUnit::Update()
{
	CUnit::Update();

	if (isDead) {
		return;
	}

	for (std::list<TransportedUnit>::iterator ti = transportedUnits.begin(); ti != transportedUnits.end(); ++ti) {
		CUnit* transportee = ti->unit;

		transportee->mapSquare = mapSquare;

		// by default, "hide" the transportee far underneath terrain
		// FIXME: this is stupid, just set noDraw and disable coldet
		float3 relPiecePos = UpVector * -10000.0f;
		float3 absPiecePos = pos + relPiecePos;

		if (ti->piece >= 0) {
			relPiecePos = script->GetPiecePos(ti->piece);
			absPiecePos = pos +
				(frontdir * relPiecePos.z) +
				(updir    * relPiecePos.y) +
				(rightdir * relPiecePos.x);
		}

		if (unitDef->holdSteady) {
			// slave transportee orientation to piece
			if (ti->piece >= 0) {
				const CMatrix44f& transMat = GetTransformMatrix(true);
				const CMatrix44f& pieceMat = script->GetPieceMatrix(ti->piece);
				const CMatrix44f  slaveMat = transMat * pieceMat;

				transportee->SetDirVectors(slaveMat);
			}
		} else {
			// slave transportee orientation to body
			transportee->heading  = heading;
			transportee->updir    = updir;
			transportee->frontdir = frontdir;
			transportee->rightdir = rightdir;
		}

		transportee->Move(absPiecePos, false);
		transportee->UpdateMidAndAimPos();
		transportee->SetHeadingFromDirection();

		// see ::AttachUnit
		if (transportee->IsStunned()) {
			quadField->MovedUnit(transportee);
		}
	}
}


void CTransportUnit::DependentDied(CObject* o)
{
	// a unit died while we were carrying it
	std::list<TransportedUnit>::iterator ti;

	for (ti = transportedUnits.begin(); ti != transportedUnits.end(); ++ti) {
		if (ti->unit == o) {
			transportCapacityUsed -= ti->size;
			transportMassUsed -= ti->mass;
			transportedUnits.erase(ti);
			break;
		}
	}

	CUnit::DependentDied(o);
}


void CTransportUnit::KillUnit(CUnit* attacker, bool selfDestruct, bool reclaimed, bool)
{
	if (!isDead) {
		// guard against recursive invocation via
		//     transportee->KillUnit
		//     helper->Explosion
		//     helper->DoExplosionDamage
		//     unit->DoDamage
		//     unit->KillUnit
		// in the case that unit == this
		isDead = true;

		// ::KillUnit might be called multiple times while !deathScriptFinished,
		// but it makes no sense to kill/detach our transportees more than once
		std::list<TransportedUnit>::iterator ti;

		for (ti = transportedUnits.begin(); ti != transportedUnits.end(); ++ti) {
			CUnit* transportee = ti->unit;
			assert(transportee != this);

			if (transportee->isDead)
				continue;

			transportee->SetTransporter(NULL);
			transportee->DeleteDeathDependence(this, DEPENDENCE_TRANSPORTER);

			if (!unitDef->releaseHeld) {
				if (!selfDestruct) {
					// we don't want transportees to leave a corpse
					transportee->DoDamage(DamageArray(1e6f), ZeroVector, NULL, -DAMAGE_EXTSOURCE_KILLED, -1);
				}

				transportee->KillUnit(attacker, selfDestruct, reclaimed);
			} else {
				// NOTE: game's responsibility to deal with edge-cases now
				transportee->Move(transportee->pos.cClampInBounds(), false);

				// if this transporter uses the piece-underneath-ground
				// method to "hide" transportees, place transportee near
				// the transporter's place of death
				if (transportee->pos.y < ground->GetHeightReal(transportee->pos.x, transportee->pos.z)) {
					const float r1 = transportee->radius + radius;
					const float r2 = r1 * std::max(unitDef->unloadSpread, 1.0f);

					// try to unload in a presently unoccupied spot
					// (if no such spot, unload on transporter wreck)
					for (int i = 0; i < 10; ++i) {
						float3 pos = transportee->pos;
						pos.x += (gs->randFloat() * 2.0f * r2 - r2);
						pos.z += (gs->randFloat() * 2.0f * r2 - r2);
						pos.y = ground->GetHeightReal(pos.x, pos.z);

						if (!pos.IsInBounds())
							continue;

						if (quadField->GetUnitsExact(pos, transportee->radius + 2.0f).empty()) {
							transportee->Move(pos, false);
							break;
						}
					}
				} else {
					if (transportee->unitDef->IsGroundUnit()) {
						transportee->SetPhysicalStateBit(CSolidObject::STATE_BIT_FLYING);
						transportee->SetPhysicalStateBit(CSolidObject::STATE_BIT_SKIDDING);
					}
				}

				transportee->moveType->SlowUpdate();
				transportee->moveType->LeaveTransport();

				// issue a move order so that units dropped from flying
				// transports won't try to return to their pick-up spot
				if (unitDef->canfly && transportee->unitDef->canmove) {
					transportee->commandAI->GiveCommand(Command(CMD_MOVE, transportee->pos));
				}

				transportee->SetStunned(transportee->paralyzeDamage > (modInfo.paralyzeOnMaxHealth? transportee->maxHealth: transportee->health));
				transportee->speed = speed * (0.5f + 0.5f * gs->randFloat());

				eventHandler.UnitUnloaded(transportee, this);
			}
		}

		transportedUnits.clear();

		// make sure CUnit::KillUnit does not return early
		isDead = false;
	}

	CUnit::KillUnit(attacker, selfDestruct, reclaimed);
}



bool CTransportUnit::CanTransport(const CUnit* unit) const
{
	if (unit->GetTransporter() != NULL)
		return false;
	if (!unit->unitDef->transportByEnemy && !teamHandler->AlliedTeams(unit->team, team))
		return false;
	if (transportCapacityUsed >= unitDef->transportCapacity)
		return false;
	if (unit->unitDef->cantBeTransported)
		return false;
	if (unit->mass >= CSolidObject::DEFAULT_MASS || unit->beingBuilt)
		return false;
	// don't transport cloaked enemies
	if (unit->isCloaked && !teamHandler->AlliedTeams(unit->team, team))
		return false;
	if (unit->xsize > (unitDef->transportSize * SPRING_FOOTPRINT_SCALE))
		return false;
	if (unit->xsize < (unitDef->minTransportSize * SPRING_FOOTPRINT_SCALE))
		return false;
	if (unit->mass < unitDef->minTransportMass)
		return false;
	if (unit->mass + transportMassUsed > unitDef->transportMass)
		return false;
	if (!CanLoadUnloadAtPos(unit->pos, unit))
		return false;

	// check if <unit> is already (in)directly transporting <this>
	const CTransportUnit* u = this;

	while (u != NULL) {
		if (u == unit) {
			return false;
		}
		u = u->GetTransporter();
	}

	return true;
}


void CTransportUnit::AttachUnit(CUnit* unit, int piece)
{
	assert(unit != this);

	if (unit->GetTransporter() == this) {
		// assume we are already transporting this unit,
		// and just want to move it to a different piece
		// with script logic (this means the UnitLoaded
		// event is only sent once)
		std::list<TransportedUnit>::iterator transporteesIt;

		for (transporteesIt = transportedUnits.begin(); transporteesIt != transportedUnits.end(); ++transporteesIt) {
			TransportedUnit& tu = *transporteesIt;

			if (tu.unit == unit) {
				tu.piece = piece;
				break;
			}
		}

		return;
	} else {
		// handle transfers from another transport to us
		// (can still fail depending on CanTransport())
		if (unit->GetTransporter() != NULL) {
			unit->GetTransporter()->DetachUnit(unit);
		}
	}

	// covers the case where unit->transporter != NULL
	if (!CanTransport(unit)) {
		return;
	}

	AddDeathDependence(unit, DEPENDENCE_TRANSPORTEE);
	unit->AddDeathDependence(this, DEPENDENCE_TRANSPORTER);

	unit->SetTransporter(this);
	unit->loadingTransportId = -1;
	unit->SetStunned(!unitDef->isFirePlatform);

	if (unit->IsStunned()) {
		// make sure unit does not fire etc in transport
		selectedUnitsHandler.RemoveUnit(unit);
	}

	unit->UnBlock();
	loshandler->FreeInstance(unit->los);
	radarhandler->RemoveUnit(unit);

	// do not remove unit from QF, otherwise projectiles
	// will not be able to connect with (ie. damage) it
	//
	// for NON-stunned transportees, QF position is kept
	// up-to-date by MoveType::SlowUpdate, otherwise by
	// ::Update
	//
	// quadField->RemoveUnit(unit);

	unit->los = NULL;

	if (dynamic_cast<CBuilding*>(unit) != NULL) {
		unitLoader->RestoreGround(unit);
	}

	if (dynamic_cast<CHoverAirMoveType*>(moveType)) {
		unit->moveType->useHeading = false;
	}

	TransportedUnit tu;
		tu.unit = unit;
		tu.piece = piece;
		tu.size = unit->xsize / SPRING_FOOTPRINT_SCALE;
		tu.mass = unit->mass;

	transportCapacityUsed += tu.size;
	transportMassUsed += tu.mass;
	transportedUnits.push_back(tu);

	unit->moveType->StopMoving(true, true);
	unit->CalculateTerrainType();
	unit->UpdateTerrainType();

	eventHandler.UnitLoaded(unit, this);
	commandAI->BuggerOff(pos, -1.0f);
}


bool CTransportUnit::DetachUnitCore(CUnit* unit)
{
	if (unit->GetTransporter() != this) {
		return false;
	}

	std::list<TransportedUnit>::iterator ti;

	for (ti = transportedUnits.begin(); ti != transportedUnits.end(); ++ti) {
		if (ti->unit == unit) {
			this->DeleteDeathDependence(unit, DEPENDENCE_TRANSPORTEE);
			unit->DeleteDeathDependence(this, DEPENDENCE_TRANSPORTER);
			unit->SetTransporter(NULL);

			if (dynamic_cast<CHoverAirMoveType*>(moveType)) {
				unit->moveType->useHeading = true;
			}

			// de-stun detaching units in case we are not a fire-platform
			unit->SetStunned(unit->paralyzeDamage > (modInfo.paralyzeOnMaxHealth? unit->maxHealth: unit->health));

			unit->moveType->SlowUpdate();
			unit->moveType->LeaveTransport();

			if (CBuilding* building = dynamic_cast<CBuilding*>(unit))
				building->ForcedMove(building->pos);

			transportCapacityUsed -= ti->size;
			transportMassUsed -= ti->mass;
			transportedUnits.erase(ti);

			unit->CalculateTerrainType();
			unit->UpdateTerrainType();

			eventHandler.UnitUnloaded(unit, this);
			return true;
		}
	}

	return false;
}


bool CTransportUnit::DetachUnit(CUnit* unit)
{
	if (DetachUnitCore(unit)) {
		unit->Block();

		// erase command queue unless it's a wait command
		const CCommandQueue& queue = unit->commandAI->commandQue;
		if (queue.empty() || (queue.front().GetID() != CMD_WAIT)) {
			Command c(CMD_STOP);
			unit->commandAI->GiveCommand(c);
		}

		return true;
	}

	return false;
}


bool CTransportUnit::DetachUnitFromAir(CUnit* unit, const float3& pos)
{
	if (DetachUnitCore(unit)) {
		unit->Drop(this->pos, this->frontdir, this);

		// add an additional move command for after we land
		if (unit->unitDef->canmove) {
			Command c(CMD_MOVE, pos);
			unit->commandAI->GiveCommand(c);
		}

		return true;
	}

	return false;
}



bool CTransportUnit::CanLoadUnloadAtPos(const float3& wantedPos, const CUnit* unit, float* loadingHeight) const {
	bool canLoadUnload = false;
	float loadHeight = GetLoadUnloadHeight(wantedPos, unit, &canLoadUnload);

	if (loadingHeight != NULL)
		*loadingHeight = loadHeight;

	return canLoadUnload;
}

float CTransportUnit::GetLoadUnloadHeight(const float3& wantedPos, const CUnit* unit, bool* allowedPos) const {
	bool isAllowedHeight = true;

	float wantedHeight = unit->pos.y;
	float clampedHeight = wantedHeight;

	const UnitDef* transporteeUnitDef = unit->unitDef;
	const MoveDef* transporteeMoveDef = unit->moveDef;

	if (unit->GetTransporter() != NULL) {
		// unit is being transported, set <clampedHeight> to
		// the altitude at which to UNload the transportee
		wantedHeight = ground->GetHeightReal(wantedPos.x, wantedPos.z);
		isAllowedHeight = transporteeUnitDef->IsAllowedTerrainHeight(transporteeMoveDef, wantedHeight, &clampedHeight);

		if (isAllowedHeight) {
			if (transporteeMoveDef != NULL) {
				switch (transporteeMoveDef->moveFamily) {
					case MoveDef::Ship: {
						wantedHeight = std::max(-transporteeUnitDef->waterline, wantedHeight);
						clampedHeight = wantedHeight;
					} break;
					case MoveDef::Hover: {
						wantedHeight = std::max(0.0f, wantedHeight);
						clampedHeight = wantedHeight;
					} break;
					default: {
					} break;
				}
			} else {
				// transportee is a building or an airplane
				wantedHeight = (transporteeUnitDef->floatOnWater)? 0.0f: wantedHeight;
				clampedHeight = wantedHeight;
			}
		}

		if (dynamic_cast<const CBuilding*>(unit) != NULL) {
			// for transported structures, <wantedPos> must be free/buildable
			// (note: TestUnitBuildSquare calls IsAllowedTerrainHeight again)
			BuildInfo bi(transporteeUnitDef, wantedPos, unit->buildFacing);
			bi.pos = CGameHelper::Pos2BuildPos(bi, true);
			CFeature* f = NULL;

			if (isAllowedHeight && (!CGameHelper::TestUnitBuildSquare(bi, f, -1, true) || f != NULL))
				isAllowedHeight = false;
		}
	}


	float contactHeight = clampedHeight + unit->model->height;
	float finalHeight = contactHeight;

	// *we* must be capable of reaching the point-of-contact height
	// however this check fails for eg. ships that want to (un)load
	// land units on shore --> would require too many special cases
	// therefore restrict its use to transport aircraft
	if (this->moveDef == NULL) {
		isAllowedHeight &= unitDef->IsAllowedTerrainHeight(NULL, contactHeight, &finalHeight);
	}

	if (allowedPos != NULL) {
		*allowedPos = isAllowedHeight;
	}

	return finalHeight;
}



float CTransportUnit::GetLoadUnloadHeading(const CUnit* unit) const {
	if (unit->GetTransporter() == NULL) { return unit->heading; }
	if (dynamic_cast<CHoverAirMoveType*>(moveType) == NULL) { return unit->heading; }
	if (dynamic_cast<const CBuilding*>(unit) == NULL) { return unit->heading; }

	// transported structures want to face a cardinal direction
	return (GetHeadingFromFacing(unit->buildFacing));
}
