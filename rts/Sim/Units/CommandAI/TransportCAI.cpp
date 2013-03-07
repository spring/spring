/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "TransportCAI.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/HoverAirMoveType.h"
#include "System/creg/STL_List.h"
#include "System/myMath.h"

#define AIRTRANSPORT_DOCKING_RADIUS 16
#define AIRTRANSPORT_DOCKING_ANGLE 50

CR_BIND_DERIVED(CTransportCAI,CMobileCAI , );

CR_REG_METADATA(CTransportCAI, (
	CR_MEMBER(toBeTransportedUnitId),
	CR_RESERVED(1),
	CR_MEMBER(lastCall),
	CR_MEMBER(unloadType),
	CR_MEMBER(dropSpots),
	CR_MEMBER(isFirstIteration),
	CR_MEMBER(lastDropPos),
	CR_MEMBER(approachVector),
	CR_MEMBER(endDropPos),
	CR_RESERVED(16)
));

CTransportCAI::CTransportCAI()
	: CMobileCAI()
	, unloadType(-1)
	, toBeTransportedUnitId(-1)
	, lastCall(0)
	, isFirstIteration(true)
{}


CTransportCAI::CTransportCAI(CUnit* owner)
	: CMobileCAI(owner)
	, toBeTransportedUnitId(-1)
	, lastCall(0)
	, isFirstIteration(true)
{
	// for new transport methods
	dropSpots.clear();
	approachVector = ZeroVector;
	unloadType = owner->unitDef->transportUnloadMethod;
	startingDropPos = float3(-1.0f, -1.0f, -1.0f);
	lastDropPos = float3(-1.0f, -1.0f, -1.0f);
	endDropPos = float3(-1.0f, -1.0f, -1.0f);
	//
	CommandDescription c;
	c.id = CMD_LOAD_UNITS;
	c.action = "loadunits";
	c.type = CMDTYPE_ICON_UNIT_OR_AREA;
	c.name = "Load units";
	c.mouseicon = c.name;
	c.tooltip = "Sets the transport to load a unit or units within an area";
	possibleCommands.push_back(c);

	c.id = CMD_UNLOAD_UNITS;
	c.action = "unloadunits";
	c.type = CMDTYPE_ICON_AREA;
	c.name = "Unload units";
	c.mouseicon = c.name;
	c.tooltip = "Sets the transport to unload units in an area";
	possibleCommands.push_back(c);
}


CTransportCAI::~CTransportCAI()
{
	// if uh == NULL then all pointers to units should be considered dangling pointers
	if (unitHandler != NULL) {
		SetTransportee(NULL);
	}
}


void CTransportCAI::SetTransportee(CUnit* unit) {
	if (unit != NULL) {
		// reset existing transportee
		if (toBeTransportedUnitId != unit->id) {
			CUnit* transportee = (toBeTransportedUnitId == -1) ? NULL : unitHandler->GetUnitUnsafe(toBeTransportedUnitId);
			if ((transportee != NULL) && (transportee->loadingTransportId == owner->id))
				transportee->loadingTransportId = -1;
			toBeTransportedUnitId = unit->id;
		}
		CUnit* transport = (unit->loadingTransportId == -1) ? NULL : unitHandler->GetUnitUnsafe(unit->loadingTransportId);
		// let the closest transport be loadingTransportId, in case of multiple fighting transports
		if ((transport == NULL) || ((transport != owner) && (transport->pos.SqDistance(unit->pos) > owner->pos.SqDistance(unit->pos)))) {
			unit->loadingTransportId = owner->id;
		}
	} else {
		if (toBeTransportedUnitId != -1) {
			CUnit* transportee = unitHandler->GetUnitUnsafe(toBeTransportedUnitId);
			// only reset loadingTransportId if it is this transport
			if ((transportee != NULL) && (transportee->loadingTransportId == owner->id))
				transportee->loadingTransportId = -1;
			toBeTransportedUnitId = -1;
		}
	}
}


void CTransportCAI::GiveCommandReal(const Command& c, bool fromSynced) {
	if (!(c.options & SHIFT_KEY) && nonQueingCommands.find(c.GetID()) == nonQueingCommands.end()){
		SetTransportee(NULL);
	}
	return CMobileCAI::GiveCommandReal(c, fromSynced);
}


void CTransportCAI::SlowUpdate()
{
	if (gs->paused) { // Commands issued may invoke SlowUpdate when paused
		return;
	}
	if (commandQue.empty()) {
		CMobileCAI::SlowUpdate();
		return;
	}
	Command& c = commandQue.front();
	switch (c.GetID()) {
		case CMD_LOAD_UNITS:   { ExecuteLoadUnits(c); dropSpots.clear(); return; }
		case CMD_UNLOAD_UNITS: { ExecuteUnloadUnits(c); return; }
		case CMD_UNLOAD_UNIT:  { ExecuteUnloadUnit(c);  return; }
		default: {
			dropSpots.clear();
			CMobileCAI::SlowUpdate();
			return;
		}
	}
}


void CTransportCAI::ExecuteLoadUnits(Command& c)
{
	CTransportUnit* transport = reinterpret_cast<CTransportUnit*>(owner);

	if (c.params.size() == 1) {
		// load single unit
		CUnit* unit = unitHandler->GetUnit(c.params[0]);

		if (!unit) {
			FinishCommand();
			return;
		}
		if (c.options & INTERNAL_ORDER) {
			if (unit->commandAI->commandQue.empty()) {
				if (!LoadStillValid(unit)) {
					FinishCommand();
					return;
				}
			} else {
				Command& currentUnitCommand = unit->commandAI->commandQue[0];
				if ((currentUnitCommand.GetID() == CMD_LOAD_ONTO) && (currentUnitCommand.params.size() == 1) && (int(currentUnitCommand.params[0]) == owner->id)) {
					if ((unit->moveType->progressState == AMoveType::Failed) && (owner->moveType->progressState == AMoveType::Failed)) {
						unit->commandAI->FinishCommand();
						FinishCommand();
						return;
					}
				} else if (!LoadStillValid(unit)) {
					FinishCommand();
					return;
				}
			}
		}
		if (inCommand) {
			if (!owner->script->IsBusy()) {
				FinishCommand();
			}
			return;
		}
		if (unit && CanTransport(unit) && UpdateTargetLostTimer(int(c.params[0]))) {
			SetTransportee(unit);

			const float sqDist = unit->pos.SqDistance2D(owner->pos);
			const bool inLoadingRadius = (sqDist <= Square(owner->unitDef->loadingRadius));
			
			CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);

			// subtracting 1 square to account for pathfinder/groundmovetype inaccuracy
			if (goalPos.SqDistance2D(unit->pos) > Square(owner->unitDef->loadingRadius - SQUARE_SIZE) || 
				(!inLoadingRadius && (!owner->isMoving || (am && am->aircraftState != AAirMoveType::AIRCRAFT_FLYING)))) {
				SetGoal(unit->pos, owner->pos, std::min(64.0f, owner->unitDef->loadingRadius));
			}

			if (inLoadingRadius) {
				if (am) { // handle air transports differently
					float3 wantedPos = unit->pos;
					wantedPos.y = static_cast<CTransportUnit*>(owner)->GetLoadUnloadHeight(wantedPos, unit);
					SetGoal(wantedPos, owner->pos);

					am->loadingUnits = true;
					am->ForceHeading(static_cast<CTransportUnit*>(owner)->GetLoadUnloadHeading(unit));
					am->SetWantedAltitude(wantedPos.y - ground->GetHeightAboveWater(wantedPos.x, wantedPos.z));
					am->maxDrift = 1;

					if ((owner->pos.SqDistance(wantedPos) < Square(AIRTRANSPORT_DOCKING_RADIUS)) &&
						(abs(owner->heading-unit->heading) < AIRTRANSPORT_DOCKING_ANGLE) &&
						(owner->updir.dot(UpVector) > 0.995f))
					{
						am->loadingUnits = false;
						am->dontLand = true;

						owner->script->BeginTransport(unit);
						SetTransportee(NULL);
						transport->AttachUnit(unit, owner->script->QueryTransport(unit));
						am->SetWantedAltitude(0);

						FinishCommand();
						return;
					}
				} else {
					inCommand = true;

					StopMove();
					owner->script->TransportPickup(unit);
				}
			} else if (owner->moveType->progressState == AMoveType::Failed && sqDist < (200 * 200)) {
				// if we're pretty close already but CGroundMoveType fails because it considers
				// the goal clogged (with the future passenger...), just try to move to the
				// point halfway between the transport and the passenger.
				SetGoal((unit->pos + owner->pos) * 0.5f, owner->pos);
			}
		} else {
			FinishCommand();
		}
	} else if (c.params.size() == 4) { // area-load
		if (lastCall == gs->frameNum) { // avoid infinite loops
			return;
		}

		lastCall = gs->frameNum;

		const float3 pos = c.GetPos(0);
		const float radius = c.params[3];

		CUnit* unit = FindUnitToTransport(pos, radius);

		if (unit && CanTransport(unit)) {
			Command c2(CMD_LOAD_UNITS, c.options|INTERNAL_ORDER, unit->id);
			commandQue.push_front(c2);
			inCommand = false;

			SlowUpdate();
			return;
		} else {
			FinishCommand();
			return;
		}
	}

	isFirstIteration = true;
	startingDropPos = float3(-1.0f, -1.0f, -1.0f);
}


void CTransportCAI::ExecuteUnloadUnits(Command& c)
{
	CTransportUnit* transport = static_cast<CTransportUnit*>(owner);

	switch (unloadType) {
		case UNLOAD_LAND: { UnloadUnits_Land(c, transport); } break;
		case UNLOAD_DROP: {
			if (owner->unitDef->canfly) {
				UnloadUnits_Drop(c, transport);
			} else {
				UnloadUnits_Land(c, transport);
			}
		} break;

		case UNLOAD_LANDFLOOD: { UnloadUnits_LandFlood(c, transport); } break;
		default: { UnloadUnits_Land(c, transport); } break;
	}
}


void CTransportCAI::ExecuteUnloadUnit(Command& c)
{
	// new methods
	switch (unloadType) {
		case UNLOAD_LAND: UnloadLand(c); break;

		case UNLOAD_DROP: {
			if (owner->unitDef->canfly) {
				UnloadDrop(c);
			} else {
				UnloadLand(c);
			}
		} break;

		case UNLOAD_LANDFLOOD: UnloadLandFlood(c); break;

		default: UnloadLand(c); break;
	}
}


bool CTransportCAI::CanTransport(const CUnit* unit) const
{
	const CTransportUnit* transport = static_cast<CTransportUnit*>(owner);

	return transport->CanTransport(unit);
}


bool CTransportCAI::FindEmptySpot(const float3& center, float radius, float spread, float3& found, const CUnit* unitToUnload, bool fromSynced)
{
	const CTransportUnit* ownerTrans = static_cast<CTransportUnit*>(owner);
	const MoveDef* moveDef = unitToUnload->moveDef;
	const bool isAirTrans = dynamic_cast<AAirMoveType*>(owner->moveType);

	const float amax = std::max(100, std::min(1000, (int)(radius * radius / 100)));

	spread = std::max(1.0f, math::ceil(spread / SQUARE_SIZE)) * SQUARE_SIZE;

	for (int a = 0; a < amax; ++a) { // more attempts for large unloading zone
		float3 delta;
		float3 pos;

		const float bmax = std::max(10, a / 10);

		for (int b = 0; b < bmax; ++b) {
			// FIXME: using a deterministic technique might be better, since it would allow an unload command to be tested for validity from unsynced (with predictable results)
			const float ang = 2.0f * PI * (fromSynced ? gs->randFloat() : gu->RandFloat());
			float len = amax * math::sqrt(fromSynced ? gs->randFloat() : gu->RandFloat());
			len /= amax;
			delta.x = len * math::sin(ang);
			delta.z = len * math::cos(ang);
			pos = center + delta * radius;
			if (pos.IsInBounds())
				break;
		}

		if (!pos.IsInBounds())
			continue;

		if (!ownerTrans->CanLoadUnloadAtPos(pos, unitToUnload, &pos.y)) // returns loading height in pos.y
			continue;

		pos.y -= unitToUnload->radius; // adjust to middle pos

		// don't unload unit on too-steep slopes
		if (moveDef != NULL && ground->GetSlope(pos.x, pos.z) > moveDef->maxSlope)
			continue;

		const std::vector<CUnit*>& units = quadField->GetUnitsExact(pos, spread);

		if ((isAirTrans && (units.size() > 1 || (units.size() == 1 && units[0] != owner))) ||
			(!isAirTrans && !units.empty()))
			continue;

		found = pos;
		return true;
	}

	return false;
}


bool CTransportCAI::SpotIsClear(float3 pos, CUnit* unitToUnload)
{
	if (!static_cast<CTransportUnit*>(owner)->CanLoadUnloadAtPos(pos, unitToUnload)) {
		return false;
	}
	if (unitToUnload->moveDef && ground->GetSlope(pos.x,pos.z) > unitToUnload->moveDef->maxSlope) {
		return false;
	}
	if (!quadField->GetUnitsExact(pos, std::max(1.0f, math::ceil(unitToUnload->radius / SQUARE_SIZE)) * SQUARE_SIZE).empty()) {
		return false;
	}

	return true;
}


bool CTransportCAI::SpotIsClearIgnoreSelf(float3 pos, CUnit* unitToUnload)
{
	if (!static_cast<CTransportUnit*>(owner)->CanLoadUnloadAtPos(pos, unitToUnload)) {
		return false;
	}

	if (unitToUnload->moveDef && ground->GetSlope(pos.x,pos.z) > unitToUnload->moveDef->maxSlope) {
		return false;
	}

	const std::vector<CUnit*>& units = quadField->GetUnitsExact(pos, std::max(1.0f, math::ceil(unitToUnload->radius / SQUARE_SIZE)) * SQUARE_SIZE);
	CTransportUnit* me = static_cast<CTransportUnit*>(owner);
	for (std::vector<CUnit*>::const_iterator it = units.begin(); it != units.end(); ++it) {
		// check if the units are in the transport
		bool found = false;
		for (std::list<CTransportUnit::TransportedUnit>::const_iterator it2 = me->GetTransportedUnits().begin();
				it2 != me->GetTransportedUnits().end(); ++it2) {
			if (it2->unit == *it) {
				found = true;
				break;
			}
		}
		if (!found && (*it != owner)) {
			return false;
		}
	}

	return true;
}


bool CTransportCAI::FindEmptyDropSpots(float3 startpos, float3 endpos, std::list<float3>& dropSpots)
{
	//should only be used by air

	CTransportUnit* transport = static_cast<CTransportUnit*>(owner);
	//dropSpots.clear();
	float gap = 25.5; // TODO - set tag for this?
	float3 dir = endpos - startpos;
	dir.Normalize();

	float3 nextPos = startpos;
	float3 pos;

	std::list<CTransportUnit::TransportedUnit>::const_iterator ti = transport->GetTransportedUnits().begin();
	dropSpots.push_front(nextPos);

	// first spot
	if (ti != transport->GetTransportedUnits().end()) {
		nextPos += dir * (gap + ti->unit->radius);
		++ti;
	}

	// remaining spots
	if (dynamic_cast<CHoverAirMoveType*>(owner->moveType)) {
		while (ti != transport->GetTransportedUnits().end() && startpos.SqDistance(nextPos) < startpos.SqDistance(endpos)) {
			nextPos += dir * (ti->unit->radius);
			nextPos.y = ground->GetHeightAboveWater(nextPos.x, nextPos.z);

			// check landing spot is ok for landing on
			if (!SpotIsClear(nextPos,ti->unit))
				continue;

			dropSpots.push_front(nextPos);
			nextPos += dir * (gap + ti->unit->radius);
			++ti;
		}
		return true;
	}
	return false;
}


bool CTransportCAI::FindEmptyFloodSpots(float3 startpos, float3 endpos, std::list<float3>& dropSpots, std::vector<float3> exitDirs)
{
	// TODO: select suitable spots according to directions we are allowed to exit transport from
	return false;
}


void CTransportCAI::UnloadUnits_Land(Command& c, CTransportUnit* transport)
{
	if (lastCall == gs->frameNum) {
		// avoid infinite loops
		return;
	}

	lastCall = gs->frameNum;

	if (static_cast<CTransportUnit*>(owner)->GetTransportedUnits().empty()) {
		FinishCommand();
		return;
	}

	bool canUnload = false;
	float3 unloadPos;
	CUnit* u = NULL;
	const CTransportUnit* ownerTrans = static_cast<CTransportUnit*>(owner);
	const std::list<CTransportUnit::TransportedUnit>& transpunits = ownerTrans->GetTransportedUnits();

	for(std::list<CTransportUnit::TransportedUnit>::const_iterator it = transpunits.begin(); it != transpunits.end(); ++it) {
		u = it->unit;
		const float3 pos = c.GetPos(0);
		const float radius = c.params[3];
		const float spread = u->radius * ownerTrans->unitDef->unloadSpread;
		canUnload = FindEmptySpot(pos, radius, spread, unloadPos, u);
		if (canUnload) {
			break;
		}
	}

	if (canUnload) {
		Command c2(CMD_UNLOAD_UNIT, c.options | INTERNAL_ORDER, unloadPos);
		c2.PushParam(u->id);
		commandQue.push_front(c2);
		SlowUpdate();
	} else {
		FinishCommand();
	}
}


void CTransportCAI::UnloadUnits_Drop(Command& c, CTransportUnit* transport)
{
	// called repeatedly for each unit till units are unloaded
	if (lastCall == gs->frameNum) { // avoid infinite loops
		return;
	}
	lastCall = gs->frameNum;

	if (static_cast<CTransportUnit*>(owner)->GetTransportedUnits().empty()) {
		FinishCommand();
		return;
	}

	float3 pos = c.GetPos(0);
	float radius = c.params[3];
	bool canUnload = false;

	// at the start of each user command
	if (isFirstIteration)	{
		dropSpots.clear();
		startingDropPos = pos;

		approachVector = startingDropPos - owner->pos;
		approachVector.Normalize();
		canUnload = FindEmptyDropSpots(pos, pos + approachVector * std::max(16.0f,radius), dropSpots);
	} else if (!dropSpots.empty() ) {
		// make sure we check current spot infront of us each unload
		pos = dropSpots.back(); // take last landing pos as new start spot
		canUnload = !dropSpots.empty();
	}

	if (canUnload) {
		if (SpotIsClear(dropSpots.back(), static_cast<CTransportUnit*>(owner)->GetTransportedUnits().front().unit)) {
			const float3 pos = dropSpots.back();
			Command c2(CMD_UNLOAD_UNIT, c.options | INTERNAL_ORDER, pos);
			commandQue.push_front(c2);

			SlowUpdate();
			isFirstIteration = false;
			return;
		} else {
			dropSpots.pop_back();
		}
	} else {
		startingDropPos = float3(-1.0f, -1.0f, -1.0f);
		isFirstIteration = true;
		dropSpots.clear();
		FinishCommand();
	}
}


void CTransportCAI::UnloadUnits_CrashFlood(Command& c, CTransportUnit* transport)
{
	// TODO - fly into the ground, doing damage to units at landing pos, then unload.
	// needs heavy modification of HoverAirMoveType
}


void CTransportCAI::UnloadUnits_LandFlood(Command& c, CTransportUnit* transport)
{
	if (lastCall == gs->frameNum) { // avoid infinite loops
		return;
	}
	lastCall = gs->frameNum;
	if (static_cast<CTransportUnit*>(owner)->GetTransportedUnits().empty()) {
		FinishCommand();
		return;
	}

	float3 pos = c.GetPos(0);
	float3 found;

	const CTransportUnit* ownerTrans = static_cast<CTransportUnit*>(owner);
	const std::list<CTransportUnit::TransportedUnit>& transportees = ownerTrans->GetTransportedUnits();
	const CUnit* transportee = transportees.front().unit;
	const float radius = c.params[3];
	const float spread = transportee->radius * ownerTrans->unitDef->unloadSpread;
	const bool canUnload = FindEmptySpot(pos, radius, spread, found, transportee);

	if (canUnload) {
		Command c2(CMD_UNLOAD_UNIT, c.options | INTERNAL_ORDER, found);
		commandQue.push_front(c2);

		if (isFirstIteration)	{
			Command c1(CMD_MOVE, c.options | INTERNAL_ORDER, pos);
			commandQue.push_front(c1);
			startingDropPos = pos;
		}

		SlowUpdate();
		return;
	} else {
		FinishCommand();
	}
}


void CTransportCAI::UnloadLand(Command& c)
{
	//default unload
	CTransportUnit* transport = static_cast<CTransportUnit*>(owner);
	if (inCommand) {
		if (!owner->script->IsBusy()) {
			FinishCommand();
		}
	} else {
		const std::list<CTransportUnit::TransportedUnit>& transList = transport->GetTransportedUnits();

		if (transList.empty()) {
			FinishCommand();
			return;
		}

		float3 pos = c.GetPos(0);
		if (goalPos.SqDistance2D(pos) > 400) {
			SetGoal(pos, owner->pos);
		}

		CUnit* unit = NULL;
		if (c.params.size() < 4) {
			unit = transList.front().unit;
		} else {
			const int unitID = (int)c.params[3];
			std::list<CTransportUnit::TransportedUnit>::const_iterator it;
			for (it = transList.begin(); it != transList.end(); ++it) {
				CUnit* carried = it->unit;
				if (unitID == carried->id) {
					unit = carried;
					break;
				}
			}
			if (unit == NULL) {
				FinishCommand();
				return;
			}
		}

		if (pos.SqDistance2D(owner->pos) < Square(owner->unitDef->loadingRadius * 0.9f)) {
			CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);
			pos.y = static_cast<CTransportUnit*>(owner)->GetLoadUnloadHeight(pos, unit);
			if (am != NULL) {
				// handle air transports differently
				SetGoal(pos, owner->pos);
				am->SetWantedAltitude(pos.y - ground->GetHeightAboveWater(pos.x, pos.z));
				float unloadHeading = static_cast<CTransportUnit*>(owner)->GetLoadUnloadHeading(unit);
				am->ForceHeading(unloadHeading);
				am->maxDrift = 1;
				if ((owner->pos.SqDistance(pos) < 64) &&
						(owner->updir.dot(UpVector) > 0.99f) && math::fabs(owner->heading - unloadHeading) < AIRTRANSPORT_DOCKING_ANGLE) {
					pos.y -= unit->radius;
					if (!SpotIsClearIgnoreSelf(pos, unit)) {
						// chosen spot is no longer clear to land, choose a new one
						// if a new spot cannot be found, don't unload at all
						float3 newpos;
						if (FindEmptySpot(pos, std::max(16.0f * SQUARE_SIZE, unit->radius * 4.0f), unit->radius, newpos, unit)) {
							c.SetPos(0, newpos);
							SetGoal(newpos + UpVector * unit->model->height, owner->pos);
							return;
						}
					} else {
						transport->DetachUnit(unit);
						if (transport->GetTransportedUnits().empty()) {
							am->dontLand = false;
							owner->script->EndTransport();
						}
					}
					const float3 fix = owner->pos + owner->frontdir * 20;
					SetGoal(fix, owner->pos); // move the transport away slightly
					FinishCommand();
				}
			} else {
				inCommand = true;
				StopMove();
				owner->script->TransportDrop(unit, pos);
			}
		}
	}
}


void CTransportCAI::UnloadDrop(Command& c)
{
	CTransportUnit* transport = static_cast<CTransportUnit*>(owner);

	// fly over and drop unit
	if (inCommand) {
		if (!owner->script->IsBusy()) {
			FinishCommand();
		}
	} else {
		const std::list<CTransportUnit::TransportedUnit>& transportees = transport->GetTransportedUnits();

		if (transportees.empty() || dropSpots.empty()) {
			FinishCommand();
			return;
		}

		float3 pos = c.GetPos(0); // head towards goal

		// note that HoverAirMoveType must be modified to allow non stop movement
		// through goals for this to work well
		if (goalPos.SqDistance2D(pos) > 400) {
			SetGoal(pos, owner->pos);
			lastDropPos = pos;
		}

		CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);
		CUnit* transportee = (transportees.front()).unit;

		if (am != NULL) {
			pos.y = ground->GetHeightAboveWater(pos.x, pos.z);
			am->maxDrift = 1;

			// if near target or have past it accidentally- drop unit
			if (owner->pos.SqDistance2D(pos) < 1600 || (((pos - owner->pos).Normalize()).SqDistance(owner->frontdir.Normalize()) > 0.25 && owner->pos.SqDistance2D(pos)< (205*205))) {
				am->dontLand = true;

				owner->script->EndTransport();
				transport->DetachUnitFromAir(transportee, pos);

				dropSpots.pop_back();

				if (dropSpots.empty()) {
					SetGoal(owner->pos + owner->frontdir * 200, owner->pos); // move the transport away after last drop
				}

				FinishCommand();
			}
		} else {
			inCommand = true;
			StopMove();
			owner->script->TransportDrop(transportee, pos);
		}
	}
}


void CTransportCAI::UnloadCrashFlood(Command& c)
{
	// TODO - will require heavy modification of HoverAirMoveType.cpp
}


void CTransportCAI::UnloadLandFlood(Command& c)
{
	//land, then release all units at once
	CTransportUnit* transport = static_cast<CTransportUnit*>(owner);
	if (inCommand) {
		if (!owner->script->IsBusy()) {
			FinishCommand();
		}
	} else {
		const std::list<CTransportUnit::TransportedUnit>& transList = transport->GetTransportedUnits();

		if (transList.empty()) {
			FinishCommand();
			return;
		}

		//check units are all carried
		CUnit* unit = NULL;
		if (c.params.size() < 4) {
			unit = transList.front().unit;
		} else {
			const int unitID = (int)c.params[3];
			std::list<CTransportUnit::TransportedUnit>::const_iterator it;
			for (it = transList.begin(); it != transList.end(); ++it) {
				CUnit* carried = it->unit;
				if (unitID == carried->id) {
					unit = carried;
					break;
				}
			}
			if (unit == NULL) {
				FinishCommand();
				return;
			}
		}

		// move to position
		float3 pos = c.GetPos(0);
		if (isFirstIteration) {
			if (goalPos.SqDistance2D(pos) > 400) {
				SetGoal(startingDropPos, owner->pos);
			}
		}

		if (startingDropPos.SqDistance2D(owner->pos) < Square(owner->unitDef->loadingRadius * 0.9f)) {
			// create aircraft movetype instance
			CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);

			if (am != NULL) {
				// lower to ground

				startingDropPos.y = ground->GetHeightAboveWater(startingDropPos.x,startingDropPos.z);
				const float3 wantedPos = startingDropPos + UpVector * unit->model->height;
				SetGoal(wantedPos, owner->pos);

				am->SetWantedAltitude(1);
				am->maxDrift = 1;
				am->dontLand = false;

				// when on our way down start animations for unloading gear
				if (isFirstIteration) {
					owner->script->StartUnload();
				}
				isFirstIteration = false;

				// once at ground
				if (owner->pos.y - ground->GetHeightAboveWater(wantedPos.x,wantedPos.z) < 8) {

					// nail it to the ground before it tries jumping up, only to land again...
					am->SetState(am->AIRCRAFT_LANDED);
					// call this so that other animations such as opening doors may be started
					owner->script->TransportDrop(transList.front().unit, pos);

					transport->DetachUnitFromAir(unit,pos);

					FinishCommand();
					if (transport->GetTransportedUnits().empty()) {
						am->dontLand = false;
						owner->script->EndTransport();
						am->UpdateLanded();
					}
				}
			} else {
				// land transports
				inCommand = true;
				isFirstIteration = false;

				StopMove();
				owner->script->TransportDrop(transList.front().unit, pos);
				transport->DetachUnitFromAir(unit, pos);
				FinishCommand();

				if (transport->GetTransportedUnits().empty()) {
					owner->script->EndTransport();
				}
			}
		}
	}
}


CUnit* CTransportCAI::FindUnitToTransport(float3 center, float radius)
{
	CUnit* bestUnit = NULL;
	float bestDist = std::numeric_limits<float>::max();

	const std::vector<CUnit*>& units = quadField->GetUnitsExact(center, radius);

	for (std::vector<CUnit*>::const_iterator ui = units.begin(); ui != units.end(); ++ui) {
		CUnit* unit = (*ui);
		float dist = unit->pos.SqDistance2D(owner->pos);

		if (unit->loadingTransportId != -1 && unit->loadingTransportId != owner->id) {
			CUnit* trans = unitHandler->GetUnitUnsafe(unit->loadingTransportId);
			if ((trans != NULL) && teamHandler->AlliedTeams(owner->team, trans->team)) {
				continue; // don't refuse to load comm only because the enemy is trying to nap it at the same time
			}
		}
		if (dist >= bestDist)
			continue;
		if (!CanTransport(unit))
			continue;

		if (unit->losStatus[owner->allyteam] & (LOS_INRADAR | LOS_INLOS)) {
			bestDist = dist;
			bestUnit = unit;
		}
	}

	return bestUnit;
}


int CTransportCAI::GetDefaultCmd(const CUnit* pointed, const CFeature* feature)
{
	if (pointed) {
		if (!teamHandler->Ally(gu->myAllyTeam, pointed->allyteam)) {
			if (owner->unitDef->canAttack) {
				return CMD_ATTACK;
			} else if (CanTransport(pointed)) {
				return CMD_LOAD_UNITS; // comm napping?
			}
		} else {
			if (CanTransport(pointed)) {
				return CMD_LOAD_UNITS;
			} else if (owner->unitDef->canGuard) {
				return CMD_GUARD;
			}
		}
	}
//	if(static_cast<CTransportUnit*>(owner)->transported.empty())
	if (owner->unitDef->canmove) {
		return CMD_MOVE;
	} else {
		return CMD_STOP;
	}
//	else
//		return CMD_UNLOAD_UNITS;
}



void CTransportCAI::FinishCommand()
{
	CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);
	if (am) {
		am->loadingUnits = false;
	}

	SetTransportee(NULL);

	CMobileCAI::FinishCommand();

	if (am && commandQue.empty()) {
		am->SetWantedAltitude(0);
		am->wantToStop = true;
	}
}


bool CTransportCAI::IsBusyLoading(const CUnit* unit) const
{
	if (commandQue.empty())
		return false;

	const Command& cmd = commandQue[0];

	if (cmd.GetID() != CMD_LOAD_UNITS)
		return false;

	switch (cmd.GetParamsCount()) {
		case 1: {
			const CUnit* loadee = unitHandler->GetUnit(cmd.GetParam(0));

			if (loadee == NULL)
				return false;
			if (loadee != unit)
				return false;

			return (owner->pos.SqDistance2D(loadee->pos) <= Square(owner->unitDef->loadingRadius));
		} break;

		case 4: {
			// this can return true even when owner itself is still
			// outside loading zone, if the transport has a loading
			// radius larger than the user's command radius
			return (owner->pos.SqDistance2D(cmd.GetPos(0)) <= Square(cmd.GetParam(3) * 2.0f));
		} break;

		default: {
		} break;
	}

	return false;
}

bool CTransportCAI::LoadStillValid(CUnit* unit)
{
	if (commandQue.size() < 2) {
		return false;
	}

	const Command& cmd = commandQue[1];

	// we are called from ExecuteLoadUnits only in the case that
	// that commandQue[0].id == CMD_LOAD_UNITS, so if the second
	// command is NOT an area- but a single-unit-loading command
	// (which has one parameter) then the first one will be valid
	// (ELU keeps pushing CMD_LOAD_UNITS as long as there are any
	// units to pick up)
	// 
	if (cmd.GetID() != CMD_LOAD_UNITS || cmd.GetParamsCount() != 4) {
		return true;
	}

	const float3& cmdPos = cmd.GetPos(0);

	if (!static_cast<CTransportUnit*>(owner)->CanLoadUnloadAtPos(cmdPos, unit)) {
		return false;
	}

	return (unit->pos.SqDistance2D(cmdPos) <= Square(cmd.GetParam(3) * 2.0f));
}


bool CTransportCAI::AllowedCommand(const Command& c, bool fromSynced)
{
	if (!CMobileCAI::AllowedCommand(c, fromSynced)) {
		return false;
	}

	switch (c.GetID()) {
		case CMD_UNLOAD_UNIT:
		case CMD_UNLOAD_UNITS: {
			const CTransportUnit* transport = static_cast<CTransportUnit*>(owner);
			const std::list<CTransportUnit::TransportedUnit>& transportees = transport->GetTransportedUnits();

			// allow unloading empty transports for easier setup of transport bridges
			if (transportees.empty())
				return true;

			if (c.GetParamsCount() == 5) {
				if (fromSynced) {
					// point transported buildings (...) in their wanted direction after unloading
					for (std::list<CTransportUnit::TransportedUnit>::const_iterator it = transportees.begin(); it != transportees.end(); ++it) {
						CBuilding* building = dynamic_cast<CBuilding*>(it->unit);

						if (building == NULL)
							continue;

						building->buildFacing = std::abs(int(c.GetParam(4))) % NUM_FACINGS;
					}
				}
			}

			if (c.GetParamsCount() >= 4) {
				// find unload positions for transportees (WHY can this run in unsynced context?)
				for (std::list<CTransportUnit::TransportedUnit>::const_iterator it = transportees.begin(); it != transportees.end(); ++it) {
					CUnit* u = it->unit;

					const float radius = (c.GetID() == CMD_UNLOAD_UNITS)? c.GetParam(3): 0.0f;
					const float spread = u->radius * transport->unitDef->unloadSpread;

					float3 foundPos;

					if (FindEmptySpot(c.GetPos(0), radius, spread, foundPos, u, fromSynced)) {
						return true;
					}
					 // FIXME: support arbitrary unloading order for other unload types also
					if (unloadType != UNLOAD_LAND) {
						return false;
					}
				}

				// no empty spot found for any transported unit
				return false;
			}

			break;
		}
	}

	return true;
}
