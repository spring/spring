#include "StdAfx.h"
#include "TransportCAI.h"
#include "LineDrawer.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitTypes/TransportUnit.h"
#include "Map/Ground.h"
#include "Sim/Misc/QuadField.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/CursorIcons.h"
#include "LogOutput.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "Game/GameHelper.h"
#include "Sim/MoveTypes/TAAirMoveType.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "mmgr.h"

static void ScriptCallback(int retCode,void* p1,void* p2)
{
	((CTransportCAI*)p1)->ScriptReady();
}

CTransportCAI::CTransportCAI(CUnit* owner)
: CMobileCAI(owner),
	lastCall(0),
	scriptReady(false),
	toBeTransportedUnitId(-1)
{
	CommandDescription c;
	c.id=CMD_LOAD_UNITS;
	c.action="loadunits";
	c.type=CMDTYPE_ICON_UNIT_OR_AREA;
	c.name="Load units";
	c.hotkey="l";
	c.tooltip="Sets the transport to load a unit or units within an area";
	possibleCommands.push_back(c);

	c.id=CMD_UNLOAD_UNITS;
	c.action="unloadunits";
	c.type=CMDTYPE_ICON_AREA;
	c.name="Unload units";
	c.hotkey="u";
	c.tooltip="Sets the transport to unload units in an area";
	possibleCommands.push_back(c);
}

CTransportCAI::~CTransportCAI(void)
{
	if(toBeTransportedUnitId!=-1){
		if(uh->units[toBeTransportedUnitId])
			uh->units[toBeTransportedUnitId]->toBeTransported=false;
		toBeTransportedUnitId=-1;
	}
}

#define PACKXZ(x,z) (((int)(x) << 16)+((int)(z) & 0xffff))

void CTransportCAI::SlowUpdate(void)
{
	if(commandQue.empty()){
		CMobileCAI::SlowUpdate();
		return;
	}

	CTransportUnit* transport=(CTransportUnit*)owner;
	Command& c=commandQue.front();
	switch(c.id){
	case CMD_LOAD_UNITS:
		if(c.params.size()==1){		//load single unit
			if(transport->transportCapacityUsed >= owner->unitDef->transportCapacity){
				FinishCommand();
				return;
			}
			if(inCommand){
				if(!owner->cob->busy)
					FinishCommand();
				return;
			}
			CUnit* unit=uh->units[(int)c.params[0]];
			if(unit && CanTransport(unit) && UpdateTargetLostTimer(int(c.params[0]))){
				toBeTransportedUnitId=unit->id;
				unit->toBeTransported=true;
				if(unit->mass+transport->transportMassUsed > owner->unitDef->transportMass){
					FinishCommand();
					return;
				}
				if(goalPos.distance2D(unit->pos)>10){
					float3 fix = unit->pos;
					SetGoal(fix,owner->pos,64);
				}
				if(unit->pos.distance2D(owner->pos)<owner->unitDef->loadingRadius*0.9f){
					if(CTAAirMoveType* am=dynamic_cast<CTAAirMoveType*>(owner->moveType)){		//handle air transports differently
						float3 wantedPos=unit->pos+UpVector*unit->model->height;
						SetGoal(wantedPos,owner->pos);
						am->dontCheckCol=true;
						am->ForceHeading(unit->heading);
						am->SetWantedAltitude(unit->model->height);
						am->maxDrift=1;
						//logOutput.Print("cai dist %f %f %f",owner->pos.distance(wantedPos),owner->pos.distance2D(wantedPos),owner->pos.y-wantedPos.y);
						if(owner->pos.distance(wantedPos)<4 && abs(owner->heading-unit->heading)<50 && owner->updir.dot(UpVector)>0.995f){
							am->dontCheckCol=false;
							am->dontLand=true;
							std::vector<int> args;
							args.push_back((int)(unit->model->height*65536));
							owner->cob->Call("BeginTransport",args);
							std::vector<int> args2;
							args2.push_back(0);
							args2.push_back((int)(unit->model->height*65536));
							owner->cob->Call("QueryTransport",args2);
							((CTransportUnit*)owner)->AttachUnit(unit,args2[0]);
							am->SetWantedAltitude(0);
							FinishCommand();
							return;
						}
					} else {
						inCommand=true;
						scriptReady=false;
						StopMove();
						std::vector<int> args;
						args.push_back(unit->id);
						owner->cob->Call("TransportPickup",args,ScriptCallback,this,0);
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
		break;
	case CMD_UNLOAD_UNITS:{
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
		bool canUnload=FindEmptySpot(pos,max(16.0f,radius),((CTransportUnit*)owner)->transported.front().unit->radius,found);
		if(canUnload){
			Command c2;
			c2.id=CMD_UNLOAD_UNIT;
			c2.params.push_back(found.x);
			c2.params.push_back(found.y);
			c2.params.push_back(found.z);
			c2.options=c.options | INTERNAL_ORDER;
			commandQue.push_front(c2);
			SlowUpdate();
			return;
		} else {
			FinishCommand();
		}
		break;}
	case CMD_UNLOAD_UNIT:
		if(inCommand){
			if(!owner->cob->busy)
//			if(scriptReady)
				FinishCommand();
		} else {
			if(((CTransportUnit*)owner)->transported.empty()){
				FinishCommand();
				return;
			}
			float3 pos(c.params[0],c.params[1],c.params[2]);
			if(goalPos.distance2D(pos)>20){
				SetGoal(pos,owner->pos);
			}
			if(pos.distance2D(owner->pos)<owner->unitDef->loadingRadius*0.9f){
				if(CTAAirMoveType* am=dynamic_cast<CTAAirMoveType*>(owner->moveType)){		//handle air transports differently
					pos.y=ground->GetHeight(pos.x,pos.z);
					CUnit* unit=((CTransportUnit*)owner)->transported.front().unit;
					float3 wantedPos=pos+UpVector*unit->model->height;
					SetGoal(wantedPos,owner->pos);
					am->SetWantedAltitude(unit->model->height);
					am->maxDrift=1;
					if(owner->pos.distance(wantedPos)<8 && owner->updir.dot(UpVector)>0.99f){
						am->dontLand=false;
						owner->cob->Call("EndTransport");
						((CTransportUnit*)owner)->DetachUnit(unit);
						float3 fix = owner->pos+owner->frontdir*20;
						SetGoal(fix,owner->pos);		//move the transport away slightly
						FinishCommand();
					}
				} else {
					inCommand=true;
					scriptReady=false;
					StopMove();
					std::vector<int> args;
					args.push_back(((CTransportUnit*)owner)->transported.front().unit->id);
					args.push_back(PACKXZ(pos.x, pos.z));
					owner->cob->Call("TransportDrop",args,ScriptCallback,this,0);
				}
			}
		}
		break;
	default:
		CMobileCAI::SlowUpdate();
		break;
	}
}

void CTransportCAI::ScriptReady(void)
{
	scriptReady=true;
}

bool CTransportCAI::CanTransport(CUnit* unit)
{
	CTransportUnit* transport=(CTransportUnit*)owner;

	if(unit->mass>=100000 || unit->beingBuilt)
		return false;
	if(unit->unitDef->canhover || unit->unitDef->floater || unit->unitDef->canfly)
		return false;
	if(unit->xsize > owner->unitDef->transportSize*2)
		return false;
	if(!transport->CanTransport(unit))
		return false;
	if(unit->mass+transport->transportMassUsed > owner->unitDef->transportMass)
		return false;

	return true;
}

bool CTransportCAI::FindEmptySpot(float3 center, float radius,float emptyRadius, float3& found)
{
//	std::vector<CUnit*> units=qf->GetUnitsExact(center,radius);
	if(CTAAirMoveType* am=dynamic_cast<CTAAirMoveType*>(owner->moveType)){		//handle air transports differently
		for(int a=0;a<100;++a){
			float3 delta(1,0,1);
			while(delta.SqLength2D()>1){
				delta.x=(gs->randFloat()-0.5f)*2;
				delta.z=(gs->randFloat()-0.5f)*2;
			}
			float3 pos=center+delta*radius;
			pos.y=ground->GetHeight(pos.x,pos.z);

			if(ground->GetApproximateHeight(pos.x,pos.z)<-5)
				continue;
			if(ground->GetSlope(pos.x,pos.z)>0.05f)
				continue;
			if(!qf->GetUnitsExact(pos,emptyRadius+8).empty())
				continue;
			found=pos;
			return true;
		}
	} else {		
		for(float y=max(0.0f,center.z-radius);y<min(float(gs->mapx*SQUARE_SIZE),center.z+radius);y+=SQUARE_SIZE){
			float dy=y-center.z;
			float rx=radius*radius-dy*dy;
			if(rx<=0)
				continue;
			rx=sqrt(rx);
			for(float x=max(0.0f,center.x-rx);x<min(float(gs->mapx*SQUARE_SIZE),center.x+rx);x+=SQUARE_SIZE){
				if(ground->GetApproximateHeight(x,y)<-5)
					continue;
				if(ground->GetSlope(x,y)>0.05f)
					continue;
				float3 pos(x,ground->GetApproximateHeight(x,y),y);
				if(!qf->GetUnitsExact(pos,emptyRadius+8).empty())
					continue;
				found=pos;
				return true;
			}
		}
	}
	return false;
}

CUnit* CTransportCAI::FindUnitToTransport(float3 center, float radius)
{
	CUnit* best=0;
	float bestDist=100000000;
	std::vector<CUnit*> units=qf->GetUnitsExact(center,radius);
	for(std::vector<CUnit*>::iterator ui=units.begin();ui!=units.end();++ui){
		CUnit* unit=(*ui);
		float dist=unit->pos.distance2D(owner->pos);
		if(CanTransport(unit) && dist<bestDist && !unit->toBeTransported){
			bestDist=dist;
			best=unit;
		}
	}
	return best;
}

int CTransportCAI::GetDefaultCmd(CUnit* pointed, CFeature* feature)
{
	if (pointed) {
		if (!gs->Ally(gu->myAllyTeam, pointed->allyteam)) {
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
	lineDrawer.StartPath(owner->midPos, cmdColors.start);

	deque<Command>::iterator ci;
	for(ci=commandQue.begin();ci!=commandQue.end();++ci){
		switch(ci->id){
			case CMD_WAIT:{
				lineDrawer.DrawIconAtLastPos(ci->id);
				break;
			}
			case CMD_MOVE:{
				const float3 endPos(ci->params[0],ci->params[1],ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.move);
				break;
			}
			case CMD_FIGHT:{
				const float3 endPos(ci->params[0],ci->params[1],ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.fight);
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
