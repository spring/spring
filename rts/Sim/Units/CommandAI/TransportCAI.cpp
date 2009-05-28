#include "StdAfx.h"
#include "mmgr.h"

#include "TransportCAI.h"
#include "LineDrawer.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Map/Ground.h"
#include "Sim/Misc/QuadField.h"
#include "Game/UI/CommandColors.h"
#include "LogOutput.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "Game/GameHelper.h"
#include "Sim/MoveTypes/TAAirMoveType.h"
#include "Sim/Misc/ModInfo.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "creg/STL_List.h"
#include "GlobalUnsynced.h"
#include "myMath.h"

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

CTransportCAI::CTransportCAI():
	CMobileCAI(),
	toBeTransportedUnitId(-1),
	lastCall(0)
{}


CTransportCAI::CTransportCAI(CUnit* owner):
	CMobileCAI(owner),
	toBeTransportedUnitId(-1),
	lastCall(0)
{
	//for new transport methods
	dropSpots.clear();
	approachVector= float3(0,0,0);
	unloadType = owner->unitDef->transportUnloadMethod;
	startingDropPos = float3(-1,-1,-1);
	lastDropPos = float3(-1,-1,-1);
	endDropPos = float3(-1,-1,-1);
	isFirstIteration = true;
	//
	CommandDescription c;
	c.id=CMD_LOAD_UNITS;
	c.action="loadunits";
	c.type=CMDTYPE_ICON_UNIT_OR_AREA;
	c.name="Load units";
	c.mouseicon=c.name;
	c.tooltip="Sets the transport to load a unit or units within an area";
	possibleCommands.push_back(c);

	c.id=CMD_UNLOAD_UNITS;
	c.action="unloadunits";
	c.type=CMDTYPE_ICON_AREA;
	c.name="Unload units";
	c.mouseicon=c.name;
	c.tooltip="Sets the transport to unload units in an area";
	possibleCommands.push_back(c);
}

CTransportCAI::~CTransportCAI(void)
{
	// if uh == NULL then all pointers to units should be considered dangling pointers
	if (uh && toBeTransportedUnitId != -1) {
		if (uh->units[toBeTransportedUnitId])
			uh->units[toBeTransportedUnitId]->toBeTransported = false;
		toBeTransportedUnitId = -1;
	}
}

void CTransportCAI::SlowUpdate(void)
{
	if(commandQue.empty()){
		CMobileCAI::SlowUpdate();
		return;
	}
	Command& c=commandQue.front();
	switch(c.id){
		case CMD_LOAD_UNITS:   { ExecuteLoadUnits(c); dropSpots.clear(); return; }
		case CMD_UNLOAD_UNITS: { ExecuteUnloadUnits(c); return; }
		case CMD_UNLOAD_UNIT:  { ExecuteUnloadUnit(c);  return; }
		default:{
			dropSpots.clear();
			CMobileCAI::SlowUpdate();
			return;
		}
	}
}

void CTransportCAI::ExecuteLoadUnits(Command &c)
{
	CTransportUnit* transport=(CTransportUnit*)owner;
	if(c.params.size()==1){		//load single unit
		if(transport->transportCapacityUsed >= owner->unitDef->transportCapacity){
			FinishCommand();
			return;
		}
		CUnit* unit=uh->units[(int)c.params[0]];
		if (!unit) {
			FinishCommand();
			return;
		}
		if(c.options & INTERNAL_ORDER) {
			if(unit->commandAI->commandQue.empty()){
				if(!LoadStillValid(unit)){
					FinishCommand();
					return;
				}
			} else {
				Command & currentUnitCommand = unit->commandAI->commandQue[0];
				if(currentUnitCommand.id == CMD_LOAD_ONTO && currentUnitCommand.params.size() == 1 && int(currentUnitCommand.params[0]) == owner->id){
					if((unit->moveType->progressState == AMoveType::Failed) && (owner->moveType->progressState == AMoveType::Failed)){
						unit->commandAI->FinishCommand();
						FinishCommand();
						return;
					}
				} else if(!LoadStillValid(unit)) {
					FinishCommand();
					return;
				}
			}
		}
		if(inCommand){
			if(!owner->script->IsBusy())
				FinishCommand();
			return;
		}
		if(unit && CanTransport(unit) && UpdateTargetLostTimer(int(c.params[0]))){
			toBeTransportedUnitId=unit->id;
			unit->toBeTransported=true;
			if(unit->mass+transport->transportMassUsed > owner->unitDef->transportMass){
				FinishCommand();
				return;
			}
			if(goalPos.SqDistance2D(unit->pos)>100){
				float3 fix = unit->pos;
				SetGoal(fix,owner->pos,64);
			}
			if(unit->pos.SqDistance2D(owner->pos)<Square(owner->unitDef->loadingRadius*0.9f)){
				if(CTAAirMoveType* am=dynamic_cast<CTAAirMoveType*>(owner->moveType)){		//handle air transports differently
					float3 wantedPos=unit->pos+UpVector*unit->model->height;
					SetGoal(wantedPos,owner->pos);
					am->dontCheckCol=true;
					am->ForceHeading(unit->heading);
					am->SetWantedAltitude(unit->model->height);
					am->maxDrift=1;
					//logOutput.Print("cai dist %f %f %f",owner->pos.distance(wantedPos),owner->pos.distance2D(wantedPos),owner->pos.y-wantedPos.y);
					if(owner->pos.SqDistance(wantedPos)<Square(AIRTRANSPORT_DOCKING_RADIUS) && abs(owner->heading-unit->heading)<AIRTRANSPORT_DOCKING_ANGLE && owner->updir.dot(UpVector)>0.995f){
						am->dontCheckCol=false;
						am->dontLand=true;
						owner->script->BeginTransport(unit);
						const int piece = owner->script->QueryTransport(unit);
						((CTransportUnit*)owner)->AttachUnit(unit, piece);
						am->SetWantedAltitude(0);
						FinishCommand();
						return;
					}
				} else {
					inCommand=true;
					StopMove();
					owner->script->TransportPickup(unit);
				}
			}
		} else {
			FinishCommand();
		}
	} else if(c.params.size()==4){		//load in radius
		if(lastCall==gs->frameNum)	//avoid infinite loops
			return;
		lastCall=gs->frameNum;
		float3 pos(c.params[0],c.params[1],c.params[2]);
		float radius=c.params[3];
		CUnit* unit=FindUnitToTransport(pos,radius);
		if(unit && ((CTransportUnit*)owner)->transportCapacityUsed < owner->unitDef->transportCapacity){
			Command c2;
			c2.id=CMD_LOAD_UNITS;
			c2.params.push_back(unit->id);
			c2.options=c.options | INTERNAL_ORDER;
			commandQue.push_front(c2);
			inCommand=false;
			SlowUpdate();
			return;
		} else {
			FinishCommand();
			return;
		}
	}
	isFirstIteration=true;
	startingDropPos = float3(-1,-1,-1);

	return;
}

void CTransportCAI::ExecuteUnloadUnits(Command &c)
{
	//new Methods
	CTransportUnit* transport=(CTransportUnit*)owner;

	switch(unloadType) {
			case UNLOAD_LAND: UnloadUnits_Land(c,transport); break;

			case UNLOAD_DROP:
							if (owner->unitDef->canfly)
								UnloadUnits_Drop(c,transport);
							else
								UnloadUnits_Land(c,transport);
							break;

			case UNLOAD_LANDFLOOD: UnloadUnits_LandFlood(c,transport); break;

			default:UnloadUnits_Land(c,transport); break;
	}
}

void CTransportCAI::ExecuteUnloadUnit(Command &c)
{
	//new methods
	switch (unloadType) {
		case UNLOAD_LAND: UnloadLand(c); break;

		case UNLOAD_DROP:
						if (owner->unitDef->canfly)
							UnloadDrop(c);
						else
							UnloadLand(c);
						break;

		case UNLOAD_LANDFLOOD: UnloadLandFlood(c); break;

		default: UnloadLand(c); break;
	}
}

bool CTransportCAI::CanTransport(CUnit* unit)
{
	CTransportUnit* transport=(CTransportUnit*)owner;

	if (unit->unitDef->cantBeTransported)
		return false;
	if(unit->mass>=100000 || unit->beingBuilt)
		return false;
	// don't transport cloaked enemies
	if (unit->isCloaked && !teamHandler->AlliedTeams(unit->team, owner->team))
		return false;
	if(unit->xsize > owner->unitDef->transportSize*2)
		return false;
	if(unit->xsize < owner->unitDef->minTransportSize*2)
		return false;
	if(unit->mass < owner->unitDef->minTransportMass)
		return false;
	if(!transport->CanTransport(unit))
		return false;
	if(unit->mass+transport->transportMassUsed > owner->unitDef->transportMass)
		return false;

	return true;
}

// FindEmptySpot(pos, max(16.0f, radius), spread, found, u);
bool CTransportCAI::FindEmptySpot(float3 center, float radius, float emptyRadius, float3& found, CUnit* unitToUnload)
{
	if (dynamic_cast<CTAAirMoveType*>(owner->moveType)) {
		// If the command radius is less than the diameter of the unit we wish to drop
		if(radius < emptyRadius*2) {
			// Boundary checking.  If we are too close to the edge of the map, we will get stuck
			// in an infinite loop due to not finding any random positions that match a valid location.
			if	(	(center.x+radius < emptyRadius)
				||	(center.z+radius < emptyRadius)
				||	(center.x-radius >= gs->mapx * SQUARE_SIZE - emptyRadius)
				||	(center.z-radius >= gs->mapy * SQUARE_SIZE - emptyRadius)
				)
				return false;
		}

		// handle air transports differently
		for (int a = 0; a < 100; ++a) {
			float3 delta(1, 0, 1);
			float3 tmp;
			do {
				delta.x = (gs->randFloat() - 0.5f) * 2;
				delta.z = (gs->randFloat() - 0.5f) * 2;
				tmp = center + delta * radius;
			} while (delta.SqLength2D() > 1
					|| tmp.x < emptyRadius || tmp.z < emptyRadius
					|| tmp.x >= gs->mapx * SQUARE_SIZE - emptyRadius
					|| tmp.z >= gs->mapy * SQUARE_SIZE - emptyRadius);

			float3 pos = center + delta * radius;
			pos.y = ground->GetHeight(pos.x, pos.z);

			float unloadPosHeight = ground->GetApproximateHeight(pos.x, pos.z);

			if (unloadPosHeight < (0 - unitToUnload->unitDef->maxWaterDepth))
 				continue;
			if (unloadPosHeight > (0 - unitToUnload->unitDef->minWaterDepth))
				continue;

			// Don't unload anything on slopes
			if (unitToUnload->unitDef->movedata
					&& ground->GetSlope(pos.x, pos.z) > unitToUnload->unitDef->movedata->maxSlope)
				continue;
			if (!qf->GetUnitsExact(pos, emptyRadius + 8).empty())
				continue;

			found = pos;
			return true;
		}
	} else {
		for (float y = std::max(emptyRadius, center.z - radius); y < std::min(float(gs->mapy * SQUARE_SIZE - emptyRadius), center.z + radius); y += SQUARE_SIZE) {
			float dy = y - center.z;
			float rx = radius * radius - dy * dy;

			if (rx <= emptyRadius)
				continue;

			rx = sqrt(rx);

			for (float x = std::max(emptyRadius, center.x - rx); x < std::min(float(gs->mapx * SQUARE_SIZE - emptyRadius), center.x + rx); x += SQUARE_SIZE) {
				float unloadPosHeight = ground->GetApproximateHeight(x, y);

				if (unloadPosHeight < (0 - unitToUnload->unitDef->maxWaterDepth))
 					continue;
				if (unloadPosHeight > (0 - unitToUnload->unitDef->minWaterDepth))
					continue;

				// Don't unload anything on slopes
				if (unitToUnload->unitDef->movedata
						&& ground->GetSlope(x, y) > unitToUnload->unitDef->movedata->maxSlope)
					continue;

				float3 pos(x, ground->GetApproximateHeight(x, y), y);

				if (!qf->GetUnitsExact(pos, emptyRadius + 8).empty())
					continue;

				found = pos;
				return true;
			}
		}
	}
	return false;
}

bool CTransportCAI::SpotIsClear(float3 pos,CUnit* unitToUnload) {
	float unloadPosHeight=ground->GetApproximateHeight(pos.x,pos.z);

	if(unloadPosHeight<(0-unitToUnload->unitDef->maxWaterDepth))
 		return false;
	if(unloadPosHeight>(0-unitToUnload->unitDef->minWaterDepth))
		return false;
	if(ground->GetSlope(pos.x,pos.z) > unitToUnload->unitDef->movedata->maxSlope)
		return false;
	if(!qf->GetUnitsExact(pos,unitToUnload->radius+8).empty())
		return false;

	return true;
}

/** this is a version of SpotIsClear that ignores the tranport and all
	units it carries. */
bool CTransportCAI::SpotIsClearIgnoreSelf(float3 pos,CUnit* unitToUnload) {
	float unloadPosHeight=ground->GetApproximateHeight(pos.x,pos.z);

	if(unloadPosHeight<(0-unitToUnload->unitDef->maxWaterDepth))
 		return false;
	if(unloadPosHeight>(0-unitToUnload->unitDef->minWaterDepth))
		return false;
	if(ground->GetSlope(pos.x,pos.z) > unitToUnload->unitDef->movedata->maxSlope)
		return false;

	vector<CUnit*> units = qf->GetUnitsExact(pos,unitToUnload->radius+8);
	CTransportUnit* me = (CTransportUnit*)owner;
	for (vector<CUnit*>::iterator it = units.begin(); it != units.end(); ++it) {
		// check if the units are in the transport
		bool found = false;
		for (std::list<CTransportUnit::TransportedUnit>::iterator it2 = me->transported.begin();
				it2 != me->transported.end(); ++it2) {
			if (it2->unit == *it) {
				found = true;
				break;
			}
		}
		if (!found && *it != owner)
			return false;
	}

	return true;
}

bool CTransportCAI::FindEmptyDropSpots(float3 startpos, float3 endpos, std::list<float3>& dropSpots) {
	//should only be used by air

	CTransportUnit* transport = (CTransportUnit*)owner;
	//dropSpots.clear();
	float gap = 25.5; //TODO - set tag for this?
	float3 dir = endpos - startpos;
	dir.Normalize();

	float3 nextPos = startpos;
	float3 pos;

	std::list<CTransportUnit::TransportedUnit>::iterator ti = transport->transported.begin();
	dropSpots.push_front(nextPos);

	// first spot
	if (ti!=transport->transported.end()) {
		nextPos += dir*(gap + ti->unit->radius);
		ti++;
	}

	// remaining spots
	if (dynamic_cast<CTAAirMoveType*>(owner->moveType)) {
		while (ti != transport->transported.end() && startpos.SqDistance(nextPos) < startpos.SqDistance(endpos)) {
			nextPos += dir*(ti->unit->radius);
			nextPos.y = ground->GetHeight(nextPos.x, nextPos.z);

			//check landing spot is ok for landing on
			if (!SpotIsClear(nextPos,ti->unit))
				continue;

			dropSpots.push_front(nextPos);
			nextPos += dir*(gap + ti->unit->radius);
			ti++;
		}
		return true;
	}
	return false;
}

bool CTransportCAI::FindEmptyFloodSpots(float3 startpos, float3 endpos, std::list<float3>& dropSpots, std::vector<float3> exitDirs) {
	// TODO: select suitable spots according to directions we are allowed to exit transport from
	return false;
}

void CTransportCAI::UnloadUnits_Land(Command& c, CTransportUnit* transport) {
	if (lastCall == gs->frameNum) {
		// avoid infinite loops
		return;
	}

	lastCall = gs->frameNum;

	if (((CTransportUnit*) owner)->transported.empty()) {
		FinishCommand();
		return;
	}

	CUnit* u = ((CTransportUnit*) owner)->transported.front().unit;
	float3 pos(c.params[0],c.params[1],c.params[2]);
	float radius = c.params[3];
	float spread = u->radius * ((CTransportUnit*) owner)->unitDef->unloadSpread;
	float3 found;

	bool canUnload = FindEmptySpot(pos, std::max(16.0f, radius), spread, found, u);

	if (canUnload) {
		Command c2;
		c2.id = CMD_UNLOAD_UNIT;
		c2.params.push_back(found.x);
		c2.params.push_back(found.y);
		c2.params.push_back(found.z);
		c2.options = c.options | INTERNAL_ORDER;
		commandQue.push_front(c2);
		SlowUpdate();
		return;
	} else {
		FinishCommand();
	}
	return;

}
void CTransportCAI::UnloadUnits_Drop(Command& c, CTransportUnit* transport) {
	//called repeatedly for each unit till units are unloaded
		if(lastCall==gs->frameNum)	//avoid infinite loops
			return;
		lastCall=gs->frameNum;

		if(((CTransportUnit*)owner)->transported.empty() ){
			FinishCommand();
			return;
		}

		float3 pos(c.params[0],c.params[1],c.params[2]);
		float radius=c.params[3];
		bool canUnload = false;

		//at the start of each user command
		if (isFirstIteration )	{
			dropSpots.clear();
			startingDropPos = pos;

			approachVector = startingDropPos-owner->pos;
			approachVector.Normalize();
			canUnload = FindEmptyDropSpots(pos, pos + approachVector * std::max(16.0f,radius), dropSpots);

		} else if (!dropSpots.empty() ) {
			//make sure we check current spot infront of us each unload
			pos = dropSpots.back(); //take last landing pos as new start spot
			canUnload = dropSpots.size() > 0;
		}

		if( canUnload ){
			if(SpotIsClear(dropSpots.back(),((CTransportUnit*)owner)->transported.front().unit)) {
				float3 pos = dropSpots.back();
				Command c2;
				c2.id=CMD_UNLOAD_UNIT;
				c2.params.push_back(pos.x);
				c2.params.push_back(pos.y);
				c2.params.push_back(pos.z);
				c2.options=c.options | INTERNAL_ORDER;
				commandQue.push_front(c2);

				SlowUpdate();
				isFirstIteration = false;
				return;
			} else {
				dropSpots.pop_back();
			}
		} else {

			startingDropPos = float3(-1,-1,-1);
			isFirstIteration=true;
			dropSpots.clear();
			FinishCommand();
		}
}
void CTransportCAI::UnloadUnits_CrashFlood(Command& c, CTransportUnit* transport) {
	//TODO - fly into the ground, doing damage to units at landing pos, then unload.
	//needs heavy modification of TAAirMoveType
}
void CTransportCAI::UnloadUnits_LandFlood(Command& c, CTransportUnit* transport) {
	if(lastCall==gs->frameNum)	//avoid infinite loops
		return;
	lastCall=gs->frameNum;
	if(((CTransportUnit*)owner)->transported.empty()){
	FinishCommand();
		return;
	}
	float3 pos(c.params[0],c.params[1],c.params[2]);
	float radius=c.params[3];
	float3 found;
	//((CTransportUnit*)owner)->transported

	bool canUnload=FindEmptySpot(pos, std::max(16.0f,radius),((CTransportUnit*)owner)->transported.front().unit->radius  * ((CTransportUnit*)owner)->unitDef->unloadSpread,
								found,((CTransportUnit*)owner)->transported.front().unit);
	if(canUnload){

		Command c2;
		c2.id=CMD_UNLOAD_UNIT;
		c2.params.push_back(found.x);
		c2.params.push_back(found.y);
		c2.params.push_back(found.z);
		c2.options=c.options | INTERNAL_ORDER;
		commandQue.push_front(c2);

		if (isFirstIteration )	{
			Command c1;
			c1.id=CMD_MOVE;
			c1.params.push_back(pos.x);
			c1.params.push_back(pos.y);
			c1.params.push_back(pos.z);
			c1.options=c.options | INTERNAL_ORDER;
			commandQue.push_front(c1);
			startingDropPos = pos;
		}

		SlowUpdate();
		return;
	} else {
		FinishCommand();
	}
	return;

}


//
void CTransportCAI::UnloadLand(Command& c) {
	//default unload
	CTransportUnit* transport = (CTransportUnit*)owner;
	if (inCommand) {
			if (!owner->script->IsBusy()) {
				FinishCommand();
			}
	} else {
		const std::list<CTransportUnit::TransportedUnit>& transList =
		  transport->transported;

		if (transList.empty()) {
			FinishCommand();
			return;
		}

		float3 pos(c.params[0], c.params[1], c.params[2]);
		if(goalPos.SqDistance2D(pos) > 400){
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
			CTAAirMoveType* am = dynamic_cast<CTAAirMoveType*>(owner->moveType);
			if (am != NULL) {
				// handle air transports differently
				pos.y = ground->GetHeight(pos.x, pos.z);
				const float3 wantedPos = pos + UpVector * unit->model->height;
				SetGoal(wantedPos, owner->pos);
				am->SetWantedAltitude(unit->model->height);
				am->maxDrift = 1;
				if ((owner->pos.SqDistance(wantedPos) < 64) &&
						(owner->updir.dot(UpVector) > 0.99f)) {
					if (!SpotIsClearIgnoreSelf(wantedPos, unit)) {
						// chosen spot is no longer clear to land, choose a new one
						// if a new spot cannot be found, don't unload at all
						float3 newpos;
						if (FindEmptySpot(wantedPos, std::max(128.f, unit->radius*4),
								unit->radius, newpos, unit)) {
							c.params[0] = newpos.x;
							c.params[1] = newpos.y;
							c.params[2] = newpos.z;
							SetGoal(newpos + UpVector * unit->model->height, owner->pos);
							return;
						}
					} else {
						transport->DetachUnit(unit);
						if (transport->transported.empty()) {
							am->dontLand = false;
							owner->script->EndTransport();
						}
					}
					const float3 fix = owner->pos + owner->frontdir * 20;
					SetGoal(fix, owner->pos);		//move the transport away slightly
					FinishCommand();
				}
			} else {
				inCommand = true;
				StopMove();
				owner->script->TransportDrop(transList.front().unit, pos);
			}
		}
	}
	return;

}
void CTransportCAI::UnloadDrop(Command& c) {

	//fly over and drop unit
	if(inCommand){
		if(!owner->script->IsBusy())
			FinishCommand();
	} else {
		if(((CTransportUnit*)owner)->transported.empty()){
			FinishCommand();
			return;
		}

		float3 pos(c.params[0],c.params[1],c.params[2]); //head towards goal

		//note that taairmovetype must be modified to allow non stop movement through goals for this to work well
		if(goalPos.SqDistance2D(pos)>400){
			SetGoal(pos,owner->pos);
			lastDropPos = pos;
		}

		if(CTAAirMoveType* am=dynamic_cast<CTAAirMoveType*>(owner->moveType)){

			pos.y=ground->GetHeight(pos.x,pos.z);
			CUnit* unit=((CTransportUnit*)owner)->transported.front().unit;
			am->maxDrift=1;

			//if near target or have past it accidentally- drop unit
			if(owner->pos.SqDistance2D(pos) < 1600 || (((pos - owner->pos).Normalize()).SqDistance(owner->frontdir.Normalize()) > 0.25 && owner->pos.SqDistance2D(pos)< (205*205))) {
				am->dontLand=true;
				owner->script->EndTransport(); //test
				((CTransportUnit*)owner)->DetachUnitFromAir(unit,pos);
				dropSpots.pop_back();

				if (dropSpots.empty()) {
					float3 fix = owner->pos+owner->frontdir*200;
					SetGoal(fix,owner->pos);//move the transport away after last drop
				}
				FinishCommand();
			}
		} else {
			inCommand=true;
			StopMove();
			owner->script->TransportDrop(((CTransportUnit*)owner)->transported.front().unit, pos);
		}
	}
}
void CTransportCAI::UnloadCrashFlood(Command& c) {
	//TODO - will require heavy modification of TAAirMoveType.cpp
}
void CTransportCAI::UnloadLandFlood(Command& c) {
	//land, then release all units at once
	CTransportUnit* transport = (CTransportUnit*)owner;
	if (inCommand) {
			if (!owner->script->IsBusy()) {
				FinishCommand();
			}
	} else {
		const std::list<CTransportUnit::TransportedUnit>& transList =
		  transport->transported;

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

		//move to position
		float3 pos(c.params[0], c.params[1], c.params[2]);
		if (isFirstIteration) {
			if(goalPos.SqDistance2D(pos) > 400)
				SetGoal(startingDropPos, owner->pos);
		}

		if (startingDropPos.SqDistance2D(owner->pos) < Square(owner->unitDef->loadingRadius * 0.9f)) {
			//create aircraft movetype instance
			CTAAirMoveType* am = dynamic_cast<CTAAirMoveType*>(owner->moveType);

			if (am != NULL) {
				//lower to ground

				startingDropPos.y = ground->GetHeight(startingDropPos.x,startingDropPos.z);
				const float3 wantedPos = startingDropPos + UpVector * unit->model->height;
				SetGoal(wantedPos, owner->pos);
				am->SetWantedAltitude(1);
				am->maxDrift = 1;
				am->dontLand = false;

				//when on our way down start animations for unloading gear
				if (isFirstIteration) {
					owner->script->StartUnload();
				}
				isFirstIteration = false;

				//once at ground
				if (owner->pos.y - ground->GetHeight(wantedPos.x,wantedPos.z) < 8) {

					//nail it to the ground before it tries jumping up, only to land again...
					am->SetState(am->AIRCRAFT_LANDED);
					//call this so that other animations such as opening doors may be started
					owner->script->TransportDrop(transList.front().unit, pos);

					transport->DetachUnitFromAir(unit,pos);

					FinishCommand();
					if (transport->transported.empty()) {
						am->dontLand = false;
						owner->script->EndTransport();
						am->UpdateLanded();
					}
				}
			} else {

				//land transports
				inCommand = true;
				StopMove();
				owner->script->TransportDrop(transList.front().unit, pos);
				transport->DetachUnitFromAir(unit,pos);
				isFirstIteration = false;
				FinishCommand();
				if (transport->transported.empty())
					owner->script->EndTransport();
			}
		}
	}
	return;
}
CUnit* CTransportCAI::FindUnitToTransport(float3 center, float radius)
{
	CUnit* best=0;
	float bestDist=100000000;
	std::vector<CUnit*> units=qf->GetUnitsExact(center,radius);
	for(std::vector<CUnit*>::iterator ui=units.begin();ui!=units.end();++ui){
		CUnit* unit=(*ui);
		float dist=unit->pos.SqDistance2D(owner->pos);
		if(CanTransport(unit) && dist<bestDist && !unit->toBeTransported &&
				 (unit->losStatus[owner->allyteam] & (LOS_INRADAR|LOS_INLOS))){
			bestDist=dist;
			best=unit;
		}
	}
	return best;
}

int CTransportCAI::GetDefaultCmd(CUnit* pointed, CFeature* feature)
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
//	if(((CTransportUnit*)owner)->transported.empty())
	if (owner->unitDef->canmove) {
		return CMD_MOVE;
	} else {
		return CMD_STOP;
	}
//	else
//		return CMD_UNLOAD_UNITS;
}

void CTransportCAI::DrawCommands(void)
{
	lineDrawer.StartPath(owner->drawMidPos, cmdColors.start);

	if (owner->selfDCountdown != 0) {
		lineDrawer.DrawIconAtLastPos(CMD_SELFD);
	}

	CCommandQueue::iterator ci;
	for(ci=commandQue.begin();ci!=commandQue.end();++ci){
		switch(ci->id){
			case CMD_MOVE:{
				const float3 endPos(ci->params[0],ci->params[1],ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.move);
				break;
			}
			case CMD_FIGHT:{
				if (ci->params.size() >= 3) {
					const float3 endPos(ci->params[0],ci->params[1],ci->params[2]);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.fight);
				}
				break;
			}
			case CMD_PATROL:{
				const float3 endPos(ci->params[0],ci->params[1],ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.patrol);
				break;
			}
			case CMD_ATTACK:{
				if(ci->params.size()==1){
					const CUnit* unit = uh->units[int(ci->params[0])];
					if((unit != NULL) && isTrackable(unit)) {
						const float3 endPos =
							helper->GetUnitErrorPos(unit, owner->allyteam);
						lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.attack);
					}
				} else {
					const float3 endPos(ci->params[0],ci->params[1],ci->params[2]);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.attack);
				}
				break;
			}
			case CMD_GUARD:{
				const CUnit* unit = uh->units[int(ci->params[0])];
				if((unit != NULL) && isTrackable(unit)) {
					const float3 endPos =
						helper->GetUnitErrorPos(unit, owner->allyteam);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.guard);
				}
				break;
			}
			case CMD_LOAD_UNITS:{
				if(ci->params.size()==4){
					const float3 endPos(ci->params[0],ci->params[1],ci->params[2]);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.load);
					lineDrawer.Break(endPos, cmdColors.load);
					glSurfaceCircle(endPos, ci->params[3], 20);
					lineDrawer.RestartSameColor();
				} else {
					const CUnit* unit = uh->units[int(ci->params[0])];
					if((unit != NULL) && isTrackable(unit)) {
						const float3 endPos =
							helper->GetUnitErrorPos(unit, owner->allyteam);
						lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.load);
					}
				}
				break;
			}
			case CMD_UNLOAD_UNITS:{
				if(ci->params.size()==4){
					const float3 endPos(ci->params[0],ci->params[1],ci->params[2]);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.unload);
					lineDrawer.Break(endPos, cmdColors.unload);
					glSurfaceCircle(endPos, ci->params[3], 20);
					lineDrawer.RestartSameColor();
				}
				break;
			}
			case CMD_UNLOAD_UNIT:{
				const float3 endPos(ci->params[0],ci->params[1],ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.unload);
				break;
			}
			case CMD_WAIT:{
				DrawWaitIcon(*ci);
				break;
			}
			case CMD_SELFD:{
				lineDrawer.DrawIconAtLastPos(ci->id);
				break;
			}
			default:
				DrawDefaultCommand(*ci);
				break;
		}
	}
	lineDrawer.FinishPath();
}


void CTransportCAI::FinishCommand(void)
{
	if(CTAAirMoveType* am=dynamic_cast<CTAAirMoveType*>(owner->moveType))
		am->dontCheckCol=false;

	if(toBeTransportedUnitId!=-1){
		if(uh->units[toBeTransportedUnitId])
			uh->units[toBeTransportedUnitId]->toBeTransported=false;
		toBeTransportedUnitId=-1;
	}
	CMobileCAI::FinishCommand();
}

bool CTransportCAI::LoadStillValid(CUnit* unit){
	if(commandQue.size() < 2){
		return false;
	}
	Command cmd = commandQue[1];
	return !(cmd.id == CMD_LOAD_UNITS && cmd.params.size() == 4
		&& unit->pos.SqDistance2D(
		float3(cmd.params[0], cmd.params[1], cmd.params[2])) > Square(cmd.params[3]*2));
}

bool CTransportCAI::AllowedCommand(const Command& c, bool fromSynced)
{
	if(!CMobileCAI::AllowedCommand(c, fromSynced))
		return false;

	switch (c.id) {
		case CMD_UNLOAD_UNIT:
		case CMD_UNLOAD_UNITS: {
			CTransportUnit* transport = (CTransportUnit*) owner;

			// allow unloading empty transports for easier setup of transport bridges
			if (!transport->transported.empty()) {
				CUnit* u = transport->transported.front().unit;
				float3 pos(c.params[0],c.params[1],c.params[2]);
				float radius = c.params[3];
				float spread = u->radius * transport->unitDef->unloadSpread;
				float3 found;
				bool canUnload = FindEmptySpot(pos, std::max(16.0f, radius), spread, found, u);
				if(!canUnload) return false;
			}
			break;
		}
	}

	return true;
}
