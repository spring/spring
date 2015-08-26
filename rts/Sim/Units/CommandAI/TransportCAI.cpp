/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>

#include "TransportCAI.h"
#include "Game/GameHelper.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/Scripts/CobInstance.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/HoverAirMoveType.h"
#include "System/creg/STL_List.h"
#include "System/myMath.h"

#define AIRTRANSPORT_DOCKING_RADIUS 16
#define AIRTRANSPORT_DOCKING_ANGLE 50

CR_BIND_DERIVED(CTransportCAI,CMobileCAI , )

CR_REG_METADATA(CTransportCAI, (
	CR_RESERVED(1),
	CR_MEMBER(lastCall),
	CR_MEMBER(unloadType),
	CR_MEMBER(dropSpots),
	CR_MEMBER(isFirstIteration),
	CR_MEMBER(startingDropPos),
	CR_MEMBER(lastDropPos),
	CR_MEMBER(approachVector),
	CR_MEMBER(endDropPos),
	CR_RESERVED(16)
))

CTransportCAI::CTransportCAI()
	: CMobileCAI()
	, unloadType(-1)
	, lastCall(0)
	, isFirstIteration(true)
{}


CTransportCAI::CTransportCAI(CUnit* owner)
	: CMobileCAI(owner)
	, lastCall(0)
	, isFirstIteration(true)
{
	// for new transport methods
	dropSpots.clear();
	approachVector = ZeroVector;
	unloadType = owner->unitDef->transportUnloadMethod;
	startingDropPos = -OnesVector;
	lastDropPos = -OnesVector;
	endDropPos = -OnesVector;
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
	if (orderTarget != nullptr && orderTarget->loadingTransportId == owner->id) {
		orderTarget->loadingTransportId = -1;
	}
	SetOrderTarget(unit);
	if (unit != nullptr) {
		CUnit* transport = (unit->loadingTransportId == -1) ? NULL : unitHandler->GetUnitUnsafe(unit->loadingTransportId);
		// let the closest transport be loadingTransportId, in case of multiple fighting transports
		if ((transport == NULL) || ((transport != owner) && (transport->pos.SqDistance(unit->pos) > owner->pos.SqDistance(unit->pos)))) {
			unit->loadingTransportId = owner->id;
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
	if (c.params.size() == 1) {
		// load single unit
		CUnit* unit = unitHandler->GetUnit(c.params[0]);

		if (unit == NULL) {
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
		if (unit != NULL && CanTransport(unit) && UpdateTargetLostTimer(int(c.params[0]))) {
			SetTransportee(unit);

			const float sqDist = unit->pos.SqDistance2D(owner->pos);
			const bool inLoadingRadius = (sqDist <= Square(owner->unitDef->loadingRadius));

			CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);

			// subtract 1 square to account for PFS/GMT inaccuracy
			const bool outOfRange = (goalPos.SqDistance2D(unit->pos) > Square(owner->unitDef->loadingRadius - SQUARE_SIZE));
			const bool moveCloser = (!inLoadingRadius && (!owner->IsMoving() || (am != NULL && am->aircraftState != AAirMoveType::AIRCRAFT_FLYING)));

			if (outOfRange || moveCloser) {
				SetGoal(unit->pos, owner->pos, std::min(64.0f, owner->unitDef->loadingRadius));
			}

			if (inLoadingRadius) {
				if (am != NULL) {
					// handle air transports differently
					float3 wantedPos = unit->pos;
					wantedPos.y = owner->GetTransporteeWantedHeight(wantedPos, unit);

					// calls am->StartMoving() which sets forceHeading to false (and also
					// changes aircraftState, possibly in mid-pickup) --> must check that
					// wantedPos == goalPos using some epsilon tolerance
					// we do not want the forceHeading change at point of pickup because
					// am->UpdateHeading() will suddenly notice a large deltaHeading and
					// break the DOCKING_ANGLE constraint so call am->ForceHeading() next
					SetGoal(wantedPos, owner->pos, 1.0f);

					am->ForceHeading(owner->GetTransporteeWantedHeading(unit));
					am->SetWantedAltitude(wantedPos.y - CGround::GetHeightAboveWater(wantedPos.x, wantedPos.z));
					am->maxDrift = 1.0f;

					// FIXME: kill the hardcoded constants, use the command's radius
					const bool b1 = (owner->pos.SqDistance(wantedPos) < Square(AIRTRANSPORT_DOCKING_RADIUS));
					const bool b2 = (std::abs(owner->heading - unit->heading) < AIRTRANSPORT_DOCKING_ANGLE);
					const bool b3 = (owner->updir.dot(UpVector) > 0.995f);

					if (b1 && b2 && b3) {
						am->SetAllowLanding(false);
						am->SetWantedAltitude(0.0f);

						owner->script->BeginTransport(unit);
						SetTransportee(NULL);
						owner->AttachUnit(unit, owner->script->QueryTransport(unit));

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
	startingDropPos = -OnesVector;
}


void CTransportCAI::ExecuteUnloadUnits(Command& c)
{
	switch (unloadType) {
		case UNLOAD_LAND: {
			UnloadUnits_Land(c);
		} break;
		case UNLOAD_DROP: {
			if (owner->unitDef->canfly) {
				UnloadUnits_Drop(c);
			} else {
				UnloadUnits_Land(c);
			}
		} break;

		case UNLOAD_LANDFLOOD: {
			UnloadUnits_LandFlood(c);
		} break;
		default: {
			UnloadUnits_Land(c);
		} break;
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
	return owner->CanTransport(unit);
}


bool CTransportCAI::FindEmptySpot(const float3& center, float radius, float spread, float3& found, const CUnit* unitToUnload, bool fromSynced)
{
	const MoveDef* moveDef = unitToUnload->moveDef;

	const bool isAirTrans = (dynamic_cast<AAirMoveType*>(owner->moveType) != NULL);
	const float amax = std::max(100.0f, std::min(1000.0f, (radius * radius / 100.0f)));

	spread = std::max(1.0f, math::ceil(spread / SQUARE_SIZE)) * SQUARE_SIZE;

	// more attempts for large unloading zone
	for (int a = 0; a < amax; ++a) {
		float3 delta;
		float3 pos;

		const float bmax = std::max(10, a / 10);

		for (int b = 0; b < bmax; ++b) {
			// FIXME: using a deterministic technique might be better, since it would allow an unload command to be tested for validity from unsynced (with predictable results)
			const float ang = 2.0f * PI * (fromSynced ? gs->randFloat() : gu->RandFloat());
			const float len = math::sqrt(fromSynced ? gs->randFloat() : gu->RandFloat());

			delta.x = len * math::sin(ang);
			delta.z = len * math::cos(ang);
			pos = center + delta * radius;
			if (pos.IsInBounds())
				break;
		}

		if (!pos.IsInBounds())
			continue;

		// returns loading height in pos.y
		if (!owner->CanLoadUnloadAtPos(pos, unitToUnload, &pos.y))
			continue;

		// adjust to middle pos
		pos.y -= unitToUnload->radius;

		// don't unload unit on too-steep slopes
		if (moveDef != NULL && CGround::GetSlope(pos.x, pos.z) > moveDef->maxSlope)
			continue;

		const std::vector<CSolidObject*>& units = quadField->GetSolidsExact(pos, spread, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS);

		if (isAirTrans && (units.size() > 1 || (units.size() == 1 && units[0] != owner)))
			continue;
		if (!isAirTrans && !units.empty())
			continue;

		found = pos;
		return true;
	}

	return false;
}


bool CTransportCAI::SpotIsClear(float3 pos, CUnit* unitToUnload)
{
	if (!owner->CanLoadUnloadAtPos(pos, unitToUnload))
		return false;
	if (unitToUnload->moveDef != NULL && CGround::GetSlope(pos.x, pos.z) > unitToUnload->moveDef->maxSlope)
		return false;

	const float radius = std::max(1.0f, math::ceil(unitToUnload->radius / SQUARE_SIZE)) * SQUARE_SIZE;

	if (!quadField->GetSolidsExact(pos, radius, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS).empty()) {
		return false;
	}

	return true;
}


bool CTransportCAI::SpotIsClearIgnoreSelf(float3 pos, CUnit* unitToUnload)
{
	if (!owner->CanLoadUnloadAtPos(pos, unitToUnload))
		return false;
	if (unitToUnload->moveDef != NULL && CGround::GetSlope(pos.x, pos.z) > unitToUnload->moveDef->maxSlope)
		return false;

	const float radius = std::max(1.0f, math::ceil(unitToUnload->radius / SQUARE_SIZE)) * SQUARE_SIZE;

	const std::vector<CSolidObject*>& units = quadField->GetSolidsExact(pos, radius, 0xFFFFFFFF, CSolidObject::CSTATE_BIT_SOLIDOBJECTS);

	for (auto objectsIt = units.begin(); objectsIt != units.end(); ++objectsIt) {
		// check if the units are in the transport
		bool found = false;

		for (auto& tu: owner->transportedUnits) {
			if ((found |= (*objectsIt == tu.unit)))
				break;
		}
		if (!found && (*objectsIt != owner)) {
			return false;
		}
	}

	return true;
}


// should only be used by air
bool CTransportCAI::FindEmptyDropSpots(float3 startpos, float3 endpos, std::list<float3>& dropSpots)
{
	const float gap = 25.5f; // TODO - set tag for this?
	const float3 dir = (endpos - startpos).Normalize();

	float3 nextPos = startpos;

	dropSpots.push_front(nextPos);

	if (dynamic_cast<CHoverAirMoveType*>(owner->moveType) == NULL)
		return false;

	const auto& transportees = owner->transportedUnits;
	auto ti = transportees.begin();

	// first spot
	if (ti != transportees.end()) {
		nextPos += (dir * (gap + ti->unit->radius));
		++ti;
	}

	// remaining spots
	while (ti != transportees.end() && startpos.SqDistance(nextPos) < startpos.SqDistance(endpos)) {
		nextPos += (dir * (ti->unit->radius));
		nextPos.y = CGround::GetHeightAboveWater(nextPos.x, nextPos.z);

		// check landing spot is ok for landing on
		if (!SpotIsClear(nextPos, ti->unit))
			continue;

		dropSpots.push_front(nextPos);
		nextPos += (dir * (gap + ti->unit->radius));
		++ti;
	}

	return true;
}


bool CTransportCAI::FindEmptyFloodSpots(float3 startpos, float3 endpos, std::list<float3>& dropSpots, std::vector<float3> exitDirs)
{
	// TODO: select suitable spots according to directions we are allowed to exit transport from
	return false;
}


void CTransportCAI::UnloadUnits_Land(Command& c)
{
	if (lastCall == gs->frameNum) {
		// avoid infinite loops
		return;
	}

	lastCall = gs->frameNum;

	const auto& transportees = owner->transportedUnits;

	if (transportees.empty()) {
		FinishCommand();
		return;
	}

	CUnit* transportee = NULL;
	float3 unloadPos;

	for (auto it = transportees.begin(); it != transportees.end(); ++it) {
		const float3 pos = c.GetPos(0);
		const float radius = c.params[3];
		const float spread = (it->unit)->radius * owner->unitDef->unloadSpread;

		if (FindEmptySpot(pos, radius, spread, unloadPos, it->unit)) {
			transportee = it->unit; break;
		}
	}

	if (transportee != NULL) {
		Command c2(CMD_UNLOAD_UNIT, c.options | INTERNAL_ORDER, unloadPos);
		c2.PushParam(transportee->id);
		commandQue.push_front(c2);
		SlowUpdate();
	} else {
		FinishCommand();
	}
}


void CTransportCAI::UnloadUnits_Drop(Command& c)
{
	// called repeatedly for each unit till units are unloaded
	if (lastCall == gs->frameNum) {
		// avoid infinite loops
		return;
	}

	lastCall = gs->frameNum;

	const auto& transportees = owner->transportedUnits;

	if (transportees.empty()) {
		FinishCommand();
		return;
	}

	bool canUnload = false;

	// at the start of each user command
	if (isFirstIteration)	{
		dropSpots.clear();

		startingDropPos = c.GetPos(0);
		approachVector = (startingDropPos - owner->pos).Normalize();

		canUnload = FindEmptyDropSpots(startingDropPos, startingDropPos + approachVector * std::max(16.0f, c.params[3]), dropSpots);
	} else if (!dropSpots.empty() ) {
		// make sure we check current spot in front of us each
		// unload, take last landing pos as new start spot
		// pos = dropSpots.back();
		canUnload = !dropSpots.empty();
	}

	if (canUnload) {
		if (SpotIsClear(dropSpots.back(), (transportees.front()).unit)) {
			Command c2(CMD_UNLOAD_UNIT, c.options | INTERNAL_ORDER, dropSpots.back());
			commandQue.push_front(c2);

			SlowUpdate();
			isFirstIteration = false;
			return;
		} else {
			dropSpots.pop_back();
		}
	} else {
		startingDropPos = -OnesVector;
		isFirstIteration = true;
		dropSpots.clear();
		FinishCommand();
	}
}


void CTransportCAI::UnloadUnits_CrashFlood(Command& c)
{
	// TODO - fly into the ground, doing damage to units at landing pos, then unload.
	// needs heavy modification of HoverAirMoveType
}


void CTransportCAI::UnloadUnits_LandFlood(Command& c)
{
	if (lastCall == gs->frameNum) {
		// avoid infinite loops
		return;
	}

	lastCall = gs->frameNum;

	const auto& transportees = owner->transportedUnits;

	if (transportees.empty()) {
		FinishCommand();
		return;
	}

	float3 pos = c.GetPos(0);
	float3 found;

	const CUnit* transportee = (transportees.front()).unit;
	const float radius = c.params[3];
	const float spread = transportee->radius * owner->unitDef->unloadSpread;
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
	}

	FinishCommand();
}


void CTransportCAI::UnloadLand(Command& c)
{
	// default unload
	CUnit* transportee = NULL;

	float3 wantedPos = c.GetPos(0);

	if (inCommand) {
		if (!owner->script->IsBusy()) {
			FinishCommand();
		}
	} else {
		const auto& transportees = owner->transportedUnits;

		if (transportees.empty()) {
			FinishCommand();
			return;
		}

		SetGoal(wantedPos, owner->pos);

		if (c.params.size() < 4) {
			// unload the first transportee
			transportee = (transportees.front()).unit;
		} else {
			const int unitID = c.params[3];

			// unload a specific transportee
			for (auto& tu: transportees) {
				CUnit* carried = tu.unit;

				if (unitID == carried->id) {
					transportee = carried;
					break;
				}
			}
			if (transportee == NULL) {
				FinishCommand();
				return;
			}
		}

		if (wantedPos.SqDistance2D(owner->pos) < Square(owner->unitDef->loadingRadius * 0.9f)) {
			CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);
			wantedPos.y = owner->GetTransporteeWantedHeight(wantedPos, transportee);

			if (am != NULL) {
				// handle air transports differently
				SetGoal(wantedPos, owner->pos);

				am->SetWantedAltitude(wantedPos.y - CGround::GetHeightAboveWater(wantedPos.x, wantedPos.z));
				am->ForceHeading(owner->GetTransporteeWantedHeading(transportee));

				am->maxDrift = 1.0f;

				// FIXME: kill the hardcoded constants, use the command's radius
				// NOTE: 2D distance-check would mean units get dropped from air
				const bool b1 = (owner->pos.SqDistance(wantedPos) < Square(AIRTRANSPORT_DOCKING_RADIUS));
				const bool b2 = (std::abs(owner->heading - am->GetForcedHeading()) < AIRTRANSPORT_DOCKING_ANGLE);
				const bool b3 = (owner->updir.dot(UpVector) > 0.99f);

				if (b1 && b2 && b3) {
					wantedPos.y -= transportee->radius;

					if (!SpotIsClearIgnoreSelf(wantedPos, transportee)) {
						// chosen spot is no longer clear to land, choose a new one
						// if a new spot cannot be found, don't unload at all
						float3 newWantedPos;

						if (FindEmptySpot(wantedPos, std::max(16.0f * SQUARE_SIZE, transportee->radius * 4.0f), transportee->radius, newWantedPos, transportee)) {
							c.SetPos(0, newWantedPos);
							SetGoal(newWantedPos + UpVector * transportee->model->height, owner->pos);
							return;
						}
					} else {
						owner->DetachUnit(transportee);

						if (transportees.empty()) {
							am->SetAllowLanding(true);
							owner->script->EndTransport();
						}
					}

					// move the transport away slightly
					SetGoal(owner->pos + owner->frontdir * 20.0f, owner->pos);
					FinishCommand();
				}
			} else {
				inCommand = true;
				StopMove();
				owner->script->TransportDrop(transportee, wantedPos);
			}
		}
	}
}


void CTransportCAI::UnloadDrop(Command& c)
{
	// fly over and drop unit
	if (inCommand) {
		if (!owner->script->IsBusy()) {
			FinishCommand();
		}
	} else {
		const auto& transportees = owner->transportedUnits;

		if (transportees.empty() || dropSpots.empty()) {
			FinishCommand();
			return;
		}

		float3 pos = c.GetPos(0);

		// head towards goal
		// note that HoverAirMoveType must be modified to allow
		// non-stop movement through goals for this to work well
		if (goalPos.SqDistance2D(pos) > Square(20.0f)) {
			SetGoal(lastDropPos = pos, owner->pos);
		}

		CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);
		CUnit* transportee = (transportees.front()).unit;

		if (am != NULL) {
			pos.y = CGround::GetHeightAboveWater(pos.x, pos.z);
			am->maxDrift = 1.0f;

			// if near target or passed it accidentally, drop unit
			if (owner->pos.SqDistance2D(pos) < Square(40.0f) || (((pos - owner->pos).Normalize()).SqDistance(owner->frontdir) > 0.25 && owner->pos.SqDistance2D(pos)< (205*205))) {
				am->SetAllowLanding(false);

				owner->script->EndTransport();
				owner->DetachUnitFromAir(transportee, pos);

				dropSpots.pop_back();

				if (dropSpots.empty()) {
					// move the transport away after last drop
					SetGoal(owner->pos + owner->frontdir * 200, owner->pos);
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
	CUnit* transportee = NULL;

	if (inCommand) {
		if (!owner->script->IsBusy()) {
			FinishCommand();
		}
	} else {
		const auto& transportees = owner->transportedUnits;

		if (transportees.empty()) {
			FinishCommand();
			return;
		}

		if (c.params.size() < 4) {
			transportee = (transportees.front()).unit;
		} else {
			const int unitID = c.params[3];

			for (auto& tu: transportees) {
				CUnit* carried = tu.unit;

				if (unitID == carried->id) {
					transportee = carried;
					break;
				}
			}
			if (transportee == NULL) {
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
			CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);

			if (am != NULL) {
				// lower to ground
				startingDropPos.y = CGround::GetHeightAboveWater(startingDropPos.x, startingDropPos.z);
				const float3 wantedPos = startingDropPos + UpVector * transportee->model->height;
				SetGoal(wantedPos, owner->pos);

				am->SetWantedAltitude(1.0f);
				am->SetAllowLanding(true);
				am->maxDrift = 1.0f;

				// when on our way down start animations for unloading gear
				if (isFirstIteration) {
					owner->script->StartUnload();
				}
				isFirstIteration = false;

				// once at ground
				if (owner->pos.y - CGround::GetHeightAboveWater(wantedPos.x, wantedPos.z) < SQUARE_SIZE) {
					// nail it to the ground before it tries jumping up, only to land again...
					am->SetState(am->AIRCRAFT_LANDED);
					// call this so that other animations such as opening doors may be started
					owner->script->TransportDrop((transportees.front()).unit, pos);
					owner->DetachUnitFromAir(transportee, pos);

					FinishCommand();

					if (transportees.empty()) {
						am->SetAllowLanding(true);
						owner->script->EndTransport();
						am->UpdateLanded();
					}
				}
			} else {
				// land transports
				inCommand = true;
				isFirstIteration = false;

				StopMove();
				owner->script->TransportDrop((transportees.front()).unit, pos);
				owner->DetachUnitFromAir(transportee, pos);
				FinishCommand();

				if (transportees.empty()) {
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

	if (owner->unitDef->canmove) {
		return CMD_MOVE;
	}

	return CMD_STOP;
}



void CTransportCAI::FinishCommand()
{
	SetTransportee(NULL);
	CMobileCAI::FinishCommand();

	CHoverAirMoveType* am = dynamic_cast<CHoverAirMoveType*>(owner->moveType);

	if (am == NULL || !commandQue.empty())
		return;

	am->SetWantedAltitude(0.0f);
	am->wantToStop = true;
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

	if (owner->CanLoadUnloadAtPos(cmdPos, unit)) {
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
			const auto& transportees = owner->transportedUnits;

			// allow unloading empty transports for easier setup of transport bridges
			if (transportees.empty())
				return true;

			if (c.GetParamsCount() == 5) {
				if (fromSynced) {
					// point transported buildings (...) in their wanted direction after unloading
					for (auto& tu: transportees) {
						CBuilding* building = dynamic_cast<CBuilding*>(tu.unit);

						if (building == NULL)
							continue;

						building->buildFacing = std::abs(int(c.GetParam(4))) % NUM_FACINGS;
					}
				}
			}

			if (c.GetParamsCount() >= 4) {
				// find unload positions for transportees (WHY can this run in unsynced context?)
				for (auto it = transportees.begin(); it != transportees.end(); ++it) {
					CUnit* u = it->unit;

					const float radius = (c.GetID() == CMD_UNLOAD_UNITS)? c.GetParam(3): 0.0f;
					const float spread = u->radius * owner->unitDef->unloadSpread;

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
