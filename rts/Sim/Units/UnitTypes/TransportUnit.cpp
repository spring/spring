#include "StdAfx.h"
#include "TransportUnit.h"
#include "Lua/LuaCallInHandler.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Sim/MoveTypes/TAAirMoveType.h"
#include "Sim/MoveTypes/groundmovetype.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Misc/LosHandler.h"
#include "Game/SelectedUnits.h"
#include "LogOutput.h"
#include "mmgr.h"


CR_BIND_DERIVED(CTransportUnit, CUnit, );


CTransportUnit::CTransportUnit()
:	transportCapacityUsed(0),
  transportMassUsed(0)
{
}


CTransportUnit::~CTransportUnit()
{
}


void CTransportUnit::Update()
{
	CUnit::Update();
	for(list<TransportedUnit>::iterator ti=transported.begin();ti!=transported.end();++ti){
		float3 relPos;
		if(ti->piece>=0){
			relPos=localmodel->GetPiecePos(max(0,ti->piece));
		} else {
			relPos=float3(0,-1000,0);
		}
		float3 pos = this->pos + frontdir * relPos.z + updir * relPos.y + rightdir * relPos.x;
//		pos.y-=ti->unit->radius;
		ti->unit->pos=pos;
		ti->unit->midPos=ti->unit->pos+ti->unit->frontdir*ti->unit->relMidPos.z + ti->unit->updir*ti->unit->relMidPos.y + ti->unit->rightdir*ti->unit->relMidPos.x;		
		if(CTAAirMoveType* am=dynamic_cast<CTAAirMoveType*>(moveType)){
			if(unitDef->holdSteady){
				ti->unit->heading=heading;
				ti->unit->updir=updir;
				ti->unit->frontdir=frontdir;
				ti->unit->rightdir=rightdir;
			}
		}
	}
}


void CTransportUnit::DependentDied(CObject* o)
{
	for(list<TransportedUnit>::iterator ti=transported.begin();ti!=transported.end();++ti){
		if(ti->unit==o){
			transportCapacityUsed-=ti->size;
			transportMassUsed-=ti->mass;
			transported.erase(ti);
			break;
		}
	}

	CUnit::DependentDied(o);
}


void CTransportUnit::KillUnit(bool selfDestruct,bool reclaimed, CUnit *attacker)
{
	list<TransportedUnit>::iterator ti;
	for (ti = transported.begin(); ti != transported.end(); ++ti) {
		ti->unit->transporter = 0;
		ti->unit->DeleteDeathDependence(this);
		if (!unitDef->releaseHeld) {
			if (!selfDestruct) {
				ti->unit->DoDamage(DamageArray()*1000000,0,ZeroVector);	//dont want it to leave a corpse
			}
			ti->unit->KillUnit(selfDestruct,reclaimed,attacker);
		}
		else {
			ti->unit->stunned = false;
			if (CGroundMoveType* mt = dynamic_cast<CGroundMoveType*>(ti->unit->moveType)) {
				mt->StartSkidding();
				mt->StartFlying();
			}
			ti->unit->speed = speed;

			luaCallIns.UnitUnloaded(ti->unit, this);
		}
	}

	CUnit::KillUnit(selfDestruct,reclaimed,attacker);
}


bool CTransportUnit::CanTransport (CUnit *unit)
{
	if (unit->transporter)
		return false;

	CTransportUnit* u=this;
	while (u) {
		if (u == unit) 
			return false;
		u = u->transporter;
	}
	return true;
}


void CTransportUnit::AttachUnit(CUnit* unit, int piece)
{
	DetachUnit(unit);

	if (!CanTransport(unit))
		return;

	AddDeathDependence(unit);
	unit->AddDeathDependence (this);
	unit->transporter = this;
	unit->toBeTransported=false;
	if (!unitDef->isfireplatform) {
		unit->stunned=true;	//make sure unit doesnt fire etc in transport
		selectedUnits.RemoveUnit(unit);
	}
	unit->UnBlock();
	loshandler->FreeInstance(unit->los);
	unit->los=0;
	if (CTAAirMoveType* am=dynamic_cast<CTAAirMoveType*>(moveType)) {
		unit->moveType->useHeading=false;	
	}
	TransportedUnit tu;
	tu.unit=unit;
	tu.piece=piece;
	tu.size=unit->xsize/2;
	tu.mass=unit->mass;
	transportCapacityUsed+=tu.size;
	transportMassUsed+=tu.mass;
	transported.push_back(tu);

	unit->CalculateTerrainType();
	unit->UpdateTerrainType();

	luaCallIns.UnitLoaded(unit, this);
}

void CTransportUnit::DetachUnit(CUnit* unit)
{
	if (unit->transporter != this) {
		return;
	}

	list<TransportedUnit>::iterator ti;
	for (ti = transported.begin(); ti != transported.end(); ++ti) {
		if (ti->unit == unit) {
			this->DeleteDeathDependence(unit);
			unit->DeleteDeathDependence(this);
			unit->transporter = 0;
			if (CTAAirMoveType* am = dynamic_cast<CTAAirMoveType*>(moveType)) {
				unit->moveType->useHeading = true;
			}
			unit->stunned = false; // de-stun in case it isfireplatform=0
			unit->Block();
			loshandler->MoveUnit(unit, false);
			unit->moveType->LeaveTransport();
			const CCommandQueue& queue = unit->commandAI->commandQue;
			if (queue.empty() || (queue.front().id != CMD_WAIT)) {
				Command c;
				c.id=CMD_STOP;
				unit->commandAI->GiveCommand(c);
			}
			transportCapacityUsed -= ti->size;
			transportMassUsed -= ti->mass;
			transported.erase(ti);

			unit->CalculateTerrainType();
			unit->UpdateTerrainType();

			luaCallIns.UnitUnloaded(unit, this);

			break;
		}
	}
}
