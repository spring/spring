/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "TransportUnit.h"
#include "Game/GameHelper.h"
#include "Game/SelectedUnits.h"
#include "Map/Ground.h"
#include "Rendering/GroundDecalHandler.h"
#include "Sim/MoveTypes/TAAirMoveType.h"
#include "Sim/MoveTypes/GroundMoveType.h"
#include "Sim/Units/COB/CobInstance.h"
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
					frontdir * pieceMat[10] +
					updir    * pieceMat[ 6] +
					rightdir * pieceMat[ 2];

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
		const float gh = ground->GetHeightReal(u->pos.x, u->pos.z);

		u->transporter = 0;
		u->DeleteDeathDependence(this, DEPENDENCE_TRANSPORTER);

		// prevent a position teleport on the next movetype update if
		// the transport died in a place that the unit being carried
		// could not get to on its own
		if (!u->pos.IsInBounds()) {
			u->KillUnit(false, false, NULL, false);
			continue;
		} else {
			// immobile units can still be transported
			// via script trickery, guard against this
			if (!u->unitDef->IsAllowedTerrainHeight(gh)) {
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
				for (int i = 0; i < 10; ++i) {
					float3 pos = u->pos;
					pos.x += gs->randFloat()*2*k - k;
					pos.z += gs->randFloat()*2*k - k;
					pos.y = ground->GetHeightReal(u->pos.x, u->pos.z);
					if (qf->GetUnitsExact(pos, u->radius + 2).empty()) {
						u->pos = pos;
						break;
					}
				}
				u->UpdateMidPos();
			} else if (CGroundMoveType* mt = dynamic_cast<CGroundMoveType*>(u->moveType)) {
				mt->StartFlying();
			}

			((AMoveType*) u->moveType)->SlowUpdate();
			(             u->moveType)->LeaveTransport();

			// issue a move order so that unit won't try to return to pick-up pos in IdleCheck()
			if (dynamic_cast<CTAAirMoveType*>(moveType) && u->unitDef->canmove) {
				const float3& pos = u->pos;
				Command c;
				c.id = CMD_MOVE;
				c.params.push_back(pos.x);
				c.params.push_back(ground->GetHeightAboveWater(pos.x, pos.z));
				c.params.push_back(pos.z);
				u->commandAI->GiveCommand(c);
			}

			u->stunned = (u->paralyzeDamage > (modInfo.paralyzeOnMaxHealth? u->maxHealth: u->health));
			u->speed = speed * (0.5f + 0.5f * gs->randFloat());

			if (CBuilding* building = dynamic_cast<CBuilding*>(u)) {
				// this building may end up in a strange position, so kill it
				building->KillUnit(selfDestruct, reclaimed, attacker);
			}

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

	if (CBuilding* building = dynamic_cast<CBuilding*>(unit)) {
		unitLoader->RestoreGround(unit);
		groundDecals->RemoveBuilding(building, NULL);
	}

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

	commandAI->BuggerOff(pos, -1.0f); // make sure not to get buggered off :) by transportee
}


bool CTransportUnit::DetachUnitCore(CUnit* unit)
{
	if (unit->transporter != this) {
		return false;
	}

	std::list<TransportedUnit>::iterator ti;
	for (ti = transported.begin(); ti != transported.end(); ++ti) {
		if (ti->unit == unit) {
			this->DeleteDeathDependence(unit, DEPENDENCE_TRANSPORTEE);
			unit->DeleteDeathDependence(this, DEPENDENCE_TRANSPORTER);
			unit->transporter = NULL;

			if (dynamic_cast<CTAAirMoveType*>(moveType)) {
				unit->moveType->useHeading = true;
			}

			// de-stun in case it isFirePlatform=0
			unit->stunned = (unit->paralyzeDamage > (modInfo.paralyzeOnMaxHealth? unit->maxHealth: unit->health));

			((AMoveType*) unit->moveType)->SlowUpdate();
			(             unit->moveType)->LeaveTransport();

			if (CBuilding* building = dynamic_cast<CBuilding*>(unit))
				building->ForcedMove(building->pos, building->buildFacing);

			transportCapacityUsed -= ti->size;
			transportMassUsed -= ti->mass;
			transported.erase(ti);

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
		if(unit->unitDef->canmove) {
			Command c;
			c.id = CMD_MOVE;
			c.params.push_back(pos.x);
			c.params.push_back(pos.y);
			c.params.push_back(pos.z);
			unit->commandAI->GiveCommand(c);
		}
	}
}



bool CTransportUnit::CanLoadUnloadAtPos(const float3& wantedPos, const CUnit* unit) const {
	bool canLoadUnload = false;
	float loadHeight = GetLoadUnloadHeight(wantedPos, unit, &canLoadUnload);

	// for a given unit, we can *potentially* load/unload it at <wantedPos> if
	//     we are not a gunship-style transport, or
	//     the unit is not already in a transport, or
	//     we can land underwater, or
	//     the target-altitude is over dry land
	if (dynamic_cast<CTAAirMoveType*>(moveType) == NULL) { return canLoadUnload; }
	if (unit->transporter == NULL) { return canLoadUnload; }
	if (unitDef->canSubmerge) { return canLoadUnload; }
	if (loadHeight >= 5.0f) { return canLoadUnload; }

	return false;
}

float CTransportUnit::GetLoadUnloadHeight(const float3& wantedPos, const CUnit* unit, bool* allowedPos) const {
	bool isAllowedHeight = true;

	float wantedHeight = unit->pos.y;
	float clampedHeight = wantedHeight;

	if (unit->transporter != NULL) {
		// unit is being transported, set <clampedHeight> to
		// the altitude at which to unload the transportee
		wantedHeight = ground->GetHeightReal(wantedPos.x, wantedPos.z);
		isAllowedHeight = unit->unitDef->IsAllowedTerrainHeight(wantedHeight, &clampedHeight);

		if (isAllowedHeight) {
			if (unit->unitDef->floater) {
				wantedHeight = std::max(-unit->unitDef->waterline, wantedHeight);
				clampedHeight = wantedHeight;
			} else if (unit->unitDef->canhover) {
				wantedHeight = std::max(0.0f, wantedHeight);
				clampedHeight = wantedHeight;
			}
		}

		if (dynamic_cast<const CBuilding*>(unit) != NULL) {
			// for transported structures, <wantedPos> must be free/buildable
			// (note: TestUnitBuildSquare calls IsAllowedTerrainHeight again)
			BuildInfo bi(unit->unitDef, wantedPos, unit->buildFacing);
			bi.pos = helper->Pos2BuildPos(bi);
			CFeature* f = NULL;

			if (isAllowedHeight && (!uh->TestUnitBuildSquare(bi, f, -1) || f != NULL))
				isAllowedHeight = false;
		}
	}


	float contactHeight = clampedHeight + unit->model->height;
	float finalHeight = contactHeight;

	if (dynamic_cast<CTAAirMoveType*>(moveType) != NULL) {
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
	if (dynamic_cast<CTAAirMoveType*>(moveType) == NULL) { return unit->heading; }
	if (dynamic_cast<const CBuilding*>(unit) == NULL) { return unit->heading; }

	// transported structures want to face a cardinal direction
	return (GetHeadingFromFacing(unit->buildFacing));
}
