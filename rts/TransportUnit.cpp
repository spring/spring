#include "StdAfx.h"
#include "TransportUnit.h"
#include "3DOParser.h"
#include "TAAirMoveType.h"
#include "CommandAI.h"
#include "UnitDef.h"
//#include "mmgr.h"

CTransportUnit::CTransportUnit(const float3 &pos,int team,UnitDef* unitDef)
: CUnit(pos,team,unitDef),
	transportCapacityUsed(0),
	transportMassUsed(0)
{
}

CTransportUnit::~CTransportUnit(void)
{
}

void CTransportUnit::Update(void)
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
			ti->unit->heading=heading;
			ti->unit->frontdir=frontdir;
			ti->unit->rightdir=rightdir;
			ti->unit->updir=updir;
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

void CTransportUnit::KillUnit(bool selfDestruct,bool reclaimed)
{
	for(list<TransportedUnit>::iterator ti=transported.begin();ti!=transported.end();++ti){
		ti->unit->inTransport=false;
		if(!selfDestruct)
			ti->unit->DoDamage(DamageArray()*1000000,0,ZeroVector);	//dont want it to leave a corpse
		ti->unit->KillUnit(selfDestruct,reclaimed);
	}
	CUnit::KillUnit(selfDestruct,reclaimed);
}

void CTransportUnit::AttachUnit(CUnit* unit, int piece)
{
	DetachUnit(unit);
	if(unit->inTransport)
		return;
	AddDeathDependence(unit);
	unit->inTransport=true;
	unit->toBeTransported=false;
	if (unit->unitDef->stunnedCargo)
		unit->stunned=true;	//make sure unit doesnt fire etc in transport
	unit->UnBlock();
	if(CTAAirMoveType* am=dynamic_cast<CTAAirMoveType*>(moveType))
		unit->moveType->useHeading=false;	
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
}

void CTransportUnit::DetachUnit(CUnit* unit)
{
	if(!unit->inTransport)
		return;

	for(list<TransportedUnit>::iterator ti=transported.begin();ti!=transported.end();++ti){
		if(ti->unit==unit){
			this->DeleteDeathDependence(unit);
			unit->inTransport=false;
			if(CTAAirMoveType* am=dynamic_cast<CTAAirMoveType*>(moveType))
				unit->moveType->useHeading=true;
			if(unit->unitDef->stunnedCargo) 
				unit->stunned=false;
			unit->Block();
			unit->moveType->LeaveTransport();
			Command c;
			c.id=CMD_STOP;
			unit->commandAI->GiveCommand(c);
			transportCapacityUsed-=ti->size;
			transportMassUsed-=ti->mass;
			transported.erase(ti);

			unit->CalculateTerrainType();
			unit->UpdateTerrainType();

			break;
		}
	}
}
