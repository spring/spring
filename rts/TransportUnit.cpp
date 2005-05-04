#include "stdafx.h"
#include ".\transportunit.h"
#include "3doparser.h"
#include "taairmovetype.h"
#include "commandai.h"
//#include "mmgr.h"

CTransportUnit::CTransportUnit(const float3 &pos,int team,UnitDef* unitDef)
: CUnit(pos,team,unitDef),
	transportCapacityUsed(0)
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
			transported.erase(ti);
			transportCapacityUsed-=ti->unit->xsize/2;
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
	unit->UnBlock();
	if(CTAAirMoveType* am=dynamic_cast<CTAAirMoveType*>(moveType))
		unit->moveType->useHeading=false;	
	TransportedUnit tu;
	tu.unit=unit;
	tu.piece=piece;
	transported.push_back(tu);
	transportCapacityUsed+=unit->xsize/2;
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
			Command c;
			c.id=CMD_STOP;
			c.options=0;
			unit->commandAI->GiveCommand(c);
			unit->Block();
			transported.erase(ti);
			transportCapacityUsed-=unit->xsize/2;
			break;
		}
	}
}
