/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "TransportUnit.h"
#include "Game/SelectedUnits.h"
#include "Map/Ground.h"
#include "Sim/MoveTypes/TAAirMoveType.h"
#include "Sim/MoveTypes/GroundMoveType.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Misc/DamageArray.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/QuadField.h"
#include "System/EventHandler.h"
#include "System/myMath.h"
#include "System/creg/STL_List.h"
#include "mmgr.h"

CR_BIND_DERIVED(CTransportUnit, CUnit, );

CR_REG_METADATA(CTransportUnit, (
				CR_MEMBER(transported),
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


CTransportUnit::~CTransportUnit()
{
}

void CTransportUnit::PostLoad()
{
}

void CTransportUnit::Update()
{
	CUnit::Update();

	if (isDead) {
		return;
	}

	for (std::list<TransportedUnit>::iterator ti = transported.begin(); ti != transported.end(); ++ti) {
		CUnit* transportee = ti->unit;

		float3 relPiecePos;

		if (ti->piece >= 0) {
			relPiecePos = script->GetPiecePos(ti->piece);
		} else {
			relPiecePos = float3(0.0f, -1000.0f, 0.0f);
		}

		transportee->pos = pos +
			(frontdir * relPiecePos.z) +
			(updir    * relPiecePos.y) +
			(rightdir * relPiecePos.x);
		transportee->UpdateMidPos();
		transportee->mapSquare = mapSquare;

		if (unitDef->holdSteady) {
			// slave transportee orientation to piece
			if (ti->piece >= 0) {
				const CMatrix44f& pieceMat = script->GetPieceMatrix(ti->piece);
				const float3 pieceDir =
					frontdir *  pieceMat[10] +
					updir    *  pieceMat[ 6] +
					rightdir * -pieceMat[ 2];

				transportee->heading  = GetHeadingFromVector(pieceDir.x, pieceDir.z);
				transportee->frontdir = pieceDir;
				transportee->rightdir = transportee->frontdir.cross(UpVector);
				transportee->updir    = transportee->rightdir.cross(transportee->frontdir);
			}
		} else {
			// slave transportee orientation to body
			transportee->heading  = heading;
			transportee->updir    = updir;
			transportee->frontdir = frontdir;
			transportee->rightdir = rightdir;
		}
	}
}


void CTransportUnit::DependentDied(CObject* o)
{
	// a unit died while we were carrying it
	for (std::list<TransportedUnit>::iterator ti = transported.begin(); ti != transported.end(); ++ti) {
		if (ti->unit == o) {
			transportCapacityUsed -= ti->size;
			transportMassUsed -= ti->mass;
			transported.erase(ti);
			break;
		}
	}

	CUnit::DependentDied(o);
}


void CTransportUnit::KillUnit(bool selfDestruct, bool reclaimed, CUnit* attacker, bool)
{
	std::list<TransportedUnit>::iterator ti;
	for (ti = transported.begin(); ti != transported.end(); ++ti) {
		CUnit* u = ti->unit;
		const float gh = ground->GetHeight2(u->pos.x, u->pos.z);

		u->transporter = 0;
		u->DeleteDeathDependence(this);

		// prevent a position teleport on the next movetype update if
		// the transport died in a place that the unit being carried
		// could not get to on its own
		if (!u->pos.IsInBounds()) {
			u->KillUnit(false, false, NULL, false);
			continue;
		} else {
			// immobile units can still be transported
			// via script trickery, guard against this
			if(!u->unitDef->IsTerrainHeightOK(gh)) {
				u->KillUnit(false, false, NULL, false);
				continue;
			}
		}


		if (!unitDef->releaseHeld) {
			if (!selfDestruct) {
				// we don't want it to leave a corpse
				u->DoDamage(DamageArray(1000000), 0, ZeroVector);
			}
			u->KillUnit(selfDestruct, reclaimed, attacker);
		} else {
			// place unit near the place of death of the transport
			// if it's a ground transport and uses a piece-in-ground method
			// to hide units
			if (u->pos.y < gh) {
				const float k = (u->radius + radius)*std::max(unitDef->unloadSpread, 1.f);
				// try to unload in a presently unoccupied spot
				// unload on a wreck if suitable position not found
				for (int i = 0; i<10; ++i) {
					float3 pos = u->pos;
					pos.x += gs->randFloat()*2*k - k;
					pos.z += gs->randFloat()*2*k - k;
					pos.y = ground->GetHeight2(u->pos.x, u->pos.z);
					if (qf->GetUnitsExact(pos, u->radius + 2).empty()) {
						u->pos = pos;
						break;
					}
				}
				u->UpdateMidPos();
			} else if (CGroundMoveType* mt = dynamic_cast<CGroundMoveType*>(u->moveType)) {
				mt->StartFlying();
			}

			u->stunned = (u->paralyzeDamage > (modInfo.paralyzeOnMaxHealth? u->maxHealth: u->health));
			loshandler->MoveUnit(u, false);
			qf->MovedUnit(u);
			radarhandler->MoveUnit(u);
			u->moveType->LeaveTransport();

			// issue a move order so that unit won't try to return to pick-up pos in IdleCheck()
			if (dynamic_cast<CTAAirMoveType*>(moveType)) {
				const float3& pos = u->pos;
				Command c;
				c.id = CMD_MOVE;
				c.params.push_back(pos.x);
				c.params.push_back(ground->GetHeight(pos.x, pos.z));
				c.params.push_back(pos.z);
				u->commandAI->GiveCommand(c);
			}

			u->speed = speed * (0.5f + 0.5f * gs->randFloat());

			eventHandler.UnitUnloaded(u, this);
		}
	}

	CUnit::KillUnit(selfDestruct, reclaimed, attacker);
}



bool CTransportUnit::CanTransport(const CUnit *unit) const
{
	if (unit->transporter)
		return false;

	if (!unit->unitDef->transportByEnemy && !teamHandler->AlliedTeams(unit->team, team))
		return false;

	if (transportCapacityUsed >= unitDef->transportCapacity)
		return false;

	if (unit->unitDef->cantBeTransported)
		return false;

	if (unit->mass >= 100000 || unit->beingBuilt)
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

	// is unit already (in)directly transporting this?
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
	DetachUnit(unit);
	if (!CanTransport(unit)) {
		return;
	}

	AddDeathDependence(unit);
	unit->AddDeathDependence(this);

	unit->transporter = this;
	unit->toBeTransported = false;

	if (!unitDef->isFirePlatform) {
		// make sure unit doesnt fire etc in transport
		unit->stunned = true;
		selectedUnits.RemoveUnit(unit);
	}
	unit->UnBlock();
	loshandler->FreeInstance(unit->los);
	unit->los = 0;
	radarhandler->RemoveUnit(unit);
	qf->RemoveUnit(unit);

	if (dynamic_cast<CTAAirMoveType*>(moveType)) {
		unit->moveType->useHeading = false;
	}

	TransportedUnit tu;
		tu.unit = unit;
		tu.piece = piece;
		tu.size = unit->xsize / 2;
		tu.mass = unit->mass;
	transportCapacityUsed += tu.size;
	transportMassUsed += tu.mass;
	transported.push_back(tu);

	unit->CalculateTerrainType();
	unit->UpdateTerrainType();

	eventHandler.UnitLoaded(unit, this);
}


bool CTransportUnit::DetachUnitCore(CUnit* unit)
{
	if (unit->transporter != this) {
		return false;
	}

	std::list<TransportedUnit>::iterator ti;
	for (ti = transported.begin(); ti != transported.end(); ++ti) {
		if (ti->unit == unit) {
			this->DeleteDeathDependence(unit);
			unit->DeleteDeathDependence(this);
			unit->transporter = NULL;

			if (dynamic_cast<CTAAirMoveType*>(moveType)) {
				unit->moveType->useHeading = true;
			}

			// de-stun in case it isFirePlatform=0
			unit->stunned = (unit->paralyzeDamage > (modInfo.paralyzeOnMaxHealth? unit->maxHealth: unit->health));

			loshandler->MoveUnit(unit, false);
			qf->MovedUnit(unit);
			radarhandler->MoveUnit(unit);

			transportCapacityUsed -= ti->size;
			transportMassUsed -= ti->mass;
			transported.erase(ti);

			unit->moveType->LeaveTransport();

			unit->CalculateTerrainType();
			unit->UpdateTerrainType();

			eventHandler.UnitUnloaded(unit, this);

			return true;
		}
	}
	return false;
}


void CTransportUnit::DetachUnit(CUnit* unit)
{
	if (DetachUnitCore(unit)) {
		unit->Block();

		// erase command queue unless it's a wait command
		const CCommandQueue& queue = unit->commandAI->commandQue;
		if (queue.empty() || (queue.front().id != CMD_WAIT)) {
			Command c;
			c.id = CMD_STOP;
			unit->commandAI->GiveCommand(c);
		}
	}
}


void CTransportUnit::DetachUnitFromAir(CUnit* unit, float3 pos)
{
	if (DetachUnitCore(unit)) {
		unit->Drop(this->pos, this->frontdir, this);

		//add an additional move command for after we land
		Command c;
		c.id = CMD_MOVE;
		c.params.push_back(pos.x);
		c.params.push_back(pos.y);
		c.params.push_back(pos.z);
		unit->commandAI->GiveCommand(c);
	}
}

bool CTransportUnit::CanLoadUnloadAtPos(const float3& wantedPos, const CUnit *unit) const {
	bool isok;
	float loadAlt = GetLoadUnloadHeight(wantedPos, unit, &isok);
	if(dynamic_cast<CTAAirMoveType*>(moveType) && unit->transporter && 
		!unitDef->canSubmerge && loadAlt < 5.0f) // dont drop it so deep that we cannot pick it up again
		return false;
	return isok;
}

float CTransportUnit::GetLoadUnloadHeight(const float3& wantedPos, const CUnit *unit, bool *ok) const {
	float wantedYpos = unit->pos.y;
	float adjustedYpos = wantedYpos;
	bool isok = true;
	if(unit->transporter) { // return unloading altitude
		wantedYpos = ground->GetHeight2(wantedPos.x, wantedPos.z);
		if(unit->unitDef->floater) {
			if(unit->unitDef->GetAllowedTerrainHeight(wantedYpos) != wantedYpos)
				isok = false;
			wantedYpos = std::max(-unit->unitDef->waterline, wantedYpos);
			adjustedYpos = wantedYpos;
		}
		else if(unit->unitDef->canhover) {
			wantedYpos = std::max(0.0f, wantedYpos);
			adjustedYpos = wantedYpos;
		}
		else
			adjustedYpos = unit->unitDef->GetAllowedTerrainHeight(wantedYpos);
	}

	float terrainHeight = adjustedYpos + unit->model->height;
	float adjustedTerrainHeight = terrainHeight;
	if(dynamic_cast<CTAAirMoveType*>(moveType)) // check if transport can reach the loading altitude
		adjustedTerrainHeight = unitDef->GetAllowedTerrainHeight(terrainHeight);
	if(ok)
		*ok = isok && (adjustedTerrainHeight == terrainHeight) && (adjustedYpos == wantedYpos);
	return adjustedTerrainHeight;
}
