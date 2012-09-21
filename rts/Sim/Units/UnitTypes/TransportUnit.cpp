/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TransportUnit.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
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
#include "System/mmgr.h"

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

		float3 relPiecePos;
		float3 absPiecePos;

		if (ti->piece >= 0) {
			relPiecePos = script->GetPiecePos(ti->piece);
		} else {
			relPiecePos = float3(0.0f, -1000.0f, 0.0f);
		}

		absPiecePos = pos +
			(frontdir * relPiecePos.z) +
			(updir    * relPiecePos.y) +
			(rightdir * relPiecePos.x);

		transportee->mapSquare = mapSquare;

		if (unitDef->holdSteady) {
			// slave transportee orientation to piece
			if (ti->piece >= 0) {
				const CMatrix44f& transMat = GetTransformMatrix(true);
				const CMatrix44f& pieceMat = script->GetPieceMatrix(ti->piece);
				const CMatrix44f slaveMat = pieceMat * transMat;

				transportee->SetDirVectors(slaveMat);
			}
		} else {
			// slave transportee orientation to body
			transportee->heading  = heading;
			transportee->updir    = updir;
			transportee->frontdir = frontdir;
			transportee->rightdir = rightdir;
		}

		transportee->Move3D(absPiecePos, false);
		transportee->UpdateMidAndAimPos();
		transportee->SetHeadingFromDirection();

		// see ::AttachUnit
		if (transportee->IsStunned()) {
			qf->MovedUnit(transportee);
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


void CTransportUnit::KillUnit(bool selfDestruct, bool reclaimed, CUnit* attacker, bool)
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

			const float gh = ground->GetHeightReal(transportee->pos.x, transportee->pos.z);

			transportee->transporter = NULL;
			transportee->DeleteDeathDependence(this, DEPENDENCE_TRANSPORTER);

			// prevent a position teleport on the next movetype update if
			// the transport died in a place that the unit being carried
			// could not get to on its own
			if (!transportee->pos.IsInBounds()) {
				transportee->KillUnit(false, false, NULL, false);
				continue;
			} else {
				// immobile units can still be transported
				// via script trickery, guard against this
				if (!transportee->unitDef->IsAllowedTerrainHeight(gh)) {
					transportee->KillUnit(false, false, NULL, false);
					continue;
				}
			}

			if (!unitDef->releaseHeld) {
				if (!selfDestruct) {
					// we don't want it to leave a corpse
					transportee->DoDamage(DamageArray(1e6f), ZeroVector, NULL, -DAMAGE_EXTSOURCE_KILLED);
				}

				transportee->KillUnit(selfDestruct, reclaimed, attacker);
			} else {
				// place unit near the place of death of the transport
				// if it's a ground transport and uses a piece-in-ground method
				// to hide units
				if (transportee->pos.y < gh) {
					const float k = (transportee->radius + radius) * std::max(unitDef->unloadSpread, 1.0f);

					// try to unload in a presently unoccupied spot
					// unload on a wreck if suitable position not found
					for (int i = 0; i < 10; ++i) {
						float3 pos = transportee->pos;
						pos.x += (gs->randFloat() * 2 * k - k);
						pos.z += (gs->randFloat() * 2 * k - k);
						pos.y = ground->GetHeightReal(transportee->pos.x, transportee->pos.z);

						if (qf->GetUnitsExact(pos, transportee->radius + 2).empty()) {
							transportee->Move3D(pos, false);
							break;
						}
					}
				} else if (CGroundMoveType* mt = dynamic_cast<CGroundMoveType*>(transportee->moveType)) {
					mt->StartFlying();
				}

				transportee->moveType->SlowUpdate();
				transportee->moveType->LeaveTransport();

				// issue a move order so that unit won't try to return to pick-up pos in IdleCheck()
				if (unitDef->canfly && transportee->unitDef->canmove) {
					Command c(CMD_MOVE, float3(transportee->pos.x, ground->GetHeightAboveWater(transportee->pos.x, transportee->pos.z), transportee->pos.z));
					transportee->commandAI->GiveCommand(c);
				}

				transportee->SetStunned(transportee->paralyzeDamage > (modInfo.paralyzeOnMaxHealth? transportee->maxHealth: transportee->health));
				transportee->speed = speed * (0.5f + 0.5f * gs->randFloat());

				if (CBuilding* building = dynamic_cast<CBuilding*>(transportee)) {
					// this building may end up in a strange position, so kill it
					building->KillUnit(selfDestruct, reclaimed, attacker);
				}

				eventHandler.UnitUnloaded(transportee, this);
			}
		}

		transportedUnits.clear();

		// make sure CUnit::KillUnit does not return early
		isDead = false;
	}

	CUnit::KillUnit(selfDestruct, reclaimed, attacker);
}



bool CTransportUnit::CanTransport(const CUnit* unit) const
{
	if (unit->transporter != NULL)
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

	if (unit->xsize > unitDef->transportSize*2)
		return false;

	if (unit->xsize < unitDef->minTransportSize*2)
		return false;

	if (unit->mass < unitDef->minTransportMass)
		return false;

	if (unit->mass + transportMassUsed > unitDef->transportMass)
		return false;

	if (!CanLoadUnloadAtPos(unit->pos, unit))
		return false;

	// check if <unit> is already (in)directly transporting <this>
	const CTransportUnit* u = this;
	while (u) {
		if (u == unit) {
			return false;
		}
		u = u->transporter;
	}

	return true;
}


void CTransportUnit::AttachUnit(CUnit* unit, int piece)
{
	assert(unit != this);

	if (unit->transporter == this) {
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
		if (unit->transporter != NULL) {
			unit->transporter->DetachUnit(unit);
		}
	}

	// covers the case where unit->transporter != NULL
	if (!CanTransport(unit)) {
		return;
	}

	AddDeathDependence(unit, DEPENDENCE_TRANSPORTEE);
	unit->AddDeathDependence(this, DEPENDENCE_TRANSPORTER);

	unit->transporter = this;
	unit->toBeTransported = false;
	unit->SetStunned(!unitDef->isFirePlatform);

	if (unit->IsStunned()) {
		// make sure unit does not fire etc in transport
		selectedUnits.RemoveUnit(unit);
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
	// qf->RemoveUnit(unit);

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
		tu.size = unit->xsize / 2;
		tu.mass = unit->mass;

	transportCapacityUsed += tu.size;
	transportMassUsed += tu.mass;
	transportedUnits.push_back(tu);

	unit->CalculateTerrainType();
	unit->UpdateTerrainType();

	eventHandler.UnitLoaded(unit, this);
	commandAI->BuggerOff(pos, -1.0f); // make sure not to get buggered off :) by transportee
}


bool CTransportUnit::DetachUnitCore(CUnit* unit)
{
	if (unit->transporter != this) {
		return false;
	}

	std::list<TransportedUnit>::iterator ti;

	for (ti = transportedUnits.begin(); ti != transportedUnits.end(); ++ti) {
		if (ti->unit == unit) {
			this->DeleteDeathDependence(unit, DEPENDENCE_TRANSPORTEE);
			unit->DeleteDeathDependence(this, DEPENDENCE_TRANSPORTER);
			unit->transporter = NULL;

			if (dynamic_cast<CHoverAirMoveType*>(moveType)) {
				unit->moveType->useHeading = true;
			}

			// de-stun detaching units in case we are not a fire-platform
			unit->SetStunned(unit->paralyzeDamage > (modInfo.paralyzeOnMaxHealth? unit->maxHealth: unit->health));

			unit->moveType->SlowUpdate();
			unit->moveType->LeaveTransport();

			if (CBuilding* building = dynamic_cast<CBuilding*>(unit))
				building->ForcedMove(building->pos, building->buildFacing);

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
	if (loadingHeight) *loadingHeight = loadHeight;

	// for a given unit, we can *potentially* load/unload it at <wantedPos> if
	//     we are not a gunship-style transport, or
	//     the unit is not already in a transport, or
	//     we can land underwater, or
	//     the target-altitude is over dry land
	if (dynamic_cast<CHoverAirMoveType*>(moveType) == NULL) { return canLoadUnload; }
	if (unit->transporter == NULL) { return canLoadUnload; }
	if (unitDef->canSubmerge) { return canLoadUnload; }
	if (loadHeight >= 5.0f) { return canLoadUnload; }

	return false;
}

float CTransportUnit::GetLoadUnloadHeight(const float3& wantedPos, const CUnit* unit, bool* allowedPos) const {
	bool isAllowedHeight = true;

	float wantedHeight = unit->pos.y;
	float clampedHeight = wantedHeight;

	const UnitDef* unitDef = unit->unitDef;
	const MoveDef* moveDef = unitDef->moveDef;

	if (unit->transporter != NULL) {
		// unit is being transported, set <clampedHeight> to
		// the altitude at which to unload the transportee
		wantedHeight = ground->GetHeightReal(wantedPos.x, wantedPos.z);
		isAllowedHeight = unitDef->IsAllowedTerrainHeight(wantedHeight, &clampedHeight);

		if (isAllowedHeight) {
			if (moveDef != NULL) {
				switch (moveDef->moveType) {
					case MoveDef::Ship_Move: {
						wantedHeight = std::max(-unitDef->waterline, wantedHeight);
						clampedHeight = wantedHeight;
					} break;
					case MoveDef::Hover_Move: {
						wantedHeight = std::max(0.0f, wantedHeight);
						clampedHeight = wantedHeight;
					} break;
					default: {
					} break;
				}
			} else {
				wantedHeight = (unitDef->floatOnWater)? 0.0f: wantedHeight;
				clampedHeight = wantedHeight;
			}
		}

		if (dynamic_cast<const CBuilding*>(unit) != NULL) {
			// for transported structures, <wantedPos> must be free/buildable
			// (note: TestUnitBuildSquare calls IsAllowedTerrainHeight again)
			BuildInfo bi(unitDef, wantedPos, unit->buildFacing);
			bi.pos = helper->Pos2BuildPos(bi, true);
			CFeature* f = NULL;

			if (isAllowedHeight && (!uh->TestUnitBuildSquare(bi, f, -1, true) || f != NULL))
				isAllowedHeight = false;
		}
	}


	float contactHeight = clampedHeight + unit->model->height;
	float finalHeight = contactHeight;

	if (dynamic_cast<CHoverAirMoveType*>(moveType) != NULL) {
		// if we are a gunship-style transport, we must be
		// capable of reaching the point-of-contact height
		isAllowedHeight = unitDef->IsAllowedTerrainHeight(contactHeight, &finalHeight);
	}

	if (allowedPos) {
		*allowedPos = isAllowedHeight;
	}

	return finalHeight;
}



float CTransportUnit::GetLoadUnloadHeading(const CUnit* unit) const {
	if (unit->transporter == NULL) { return unit->heading; }
	if (dynamic_cast<CHoverAirMoveType*>(moveType) == NULL) { return unit->heading; }
	if (dynamic_cast<const CBuilding*>(unit) == NULL) { return unit->heading; }

	// transported structures want to face a cardinal direction
	return (GetHeadingFromFacing(unit->buildFacing));
}
