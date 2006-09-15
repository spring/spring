#include "StdAfx.h"
#include "FactoryCAI.h"
#include "LineDrawer.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "ExternalAI/Group.h"
#include "Game/SelectedUnits.h"
#include "Game/UI/CommandColors.h"
#include "Game/UI/CursorIcons.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitLoader.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Game/Team.h"
#include "mmgr.h"

CFactoryCAI::CFactoryCAI(CUnit* owner)
: CCommandAI(owner),
	building(false)
{
	CommandDescription c;
	c.id=CMD_MOVE;
	c.action="move";
	c.type=CMDTYPE_ICON_MAP;
	c.name="Move";
	c.hotkey="m";
	c.tooltip="Move: Order ready built units to move to a position";
	possibleCommands.push_back(c);

	c.id=CMD_PATROL;
	c.action="patrol";
	c.type=CMDTYPE_ICON_MAP;
	c.name="Patrol";
	c.hotkey="p";
	c.tooltip="Patrol: Order ready built units to patrol to one or more waypoints";
	possibleCommands.push_back(c);

	c.id = CMD_FIGHT;
	c.action="fight";
	c.type = CMDTYPE_ICON_MAP;
	c.name = "Fight";
	c.hotkey = "f";
	c.tooltip = "Fight: Order ready built units to take action while moving to a position";
	possibleCommands.push_back(c);

	c.id=CMD_GUARD;
	c.action="guard";
	c.type=CMDTYPE_ICON_UNIT;
	c.name="Guard";
	c.hotkey="g";
	c.tooltip="Guard: Order ready built units to guard another unit and attack units attacking it";
	possibleCommands.push_back(c);

	CFactory* fac=(CFactory*)owner;

	map<int,string>::iterator bi;
	for(bi=fac->unitDef->buildOptions.begin();bi!=fac->unitDef->buildOptions.end();++bi){
		string name=bi->second;
		UnitDef* ud= unitDefHandler->GetUnitByName(name);
		CommandDescription c;
		c.id=-ud->id; //build options are always negative
		c.action="buildunit_" + StringToLower(ud->name);
		c.type=CMDTYPE_ICON;
		c.name=name;

		char tmp[500];
		sprintf(tmp,"\nHealth %.0f\nMetal cost %.0f\nEnergy cost %.0f Build time %.0f",ud->health,ud->metalCost,ud->energyCost,ud->buildTime);
		c.tooltip=string("Build: ")+ud->humanName + " " + ud->tooltip+tmp;

		possibleCommands.push_back(c);
		BuildOption bo;
		bo.name=name;
		bo.fullName=name;
		bo.numQued=0;
		buildOptions[c.id]=bo;
	}
}

CFactoryCAI::~CFactoryCAI()
{

}

void CFactoryCAI::GiveCommand(Command& c)
{
	if (c.id==CMD_SET_WANTED_MAX_SPEED) {
	  return;
	}
	map<int,BuildOption>::iterator boi;
	if((boi=buildOptions.find(c.id))==buildOptions.end()){		//not a build order so que it to built units
		if(nonQueingCommands.find(c.id)!=nonQueingCommands.end()){
			CCommandAI::GiveCommand(c);
			return;
		}

		if(!(c.options & SHIFT_KEY)){
			newUnitCommands.clear();
		}
		if(c.id!=CMD_STOP){
			std::deque<Command>::iterator ci = GetCancelQueued(c);
			if(ci == this->newUnitCommands.end()){
				newUnitCommands.push_back(c);
			} else {
				this->newUnitCommands.erase(ci);
			}
		}
		return;
	}
	BuildOption &bo=boi->second;

	int numItems=1;
	if(c.options& SHIFT_KEY)
		numItems*=5;
	if(c.options & CONTROL_KEY)
		numItems*=20;

	if(c.options & RIGHT_MOUSE_KEY){
		bo.numQued-=numItems;
		if(bo.numQued<0)
			bo.numQued=0;

		int numToErase=numItems;
		if(c.options & ALT_KEY){
			for(unsigned int cmdNum=0;cmdNum<commandQue.size() && numToErase;++cmdNum){
				if(commandQue[cmdNum].id==c.id){
					commandQue[cmdNum].id=CMD_STOP;
					numToErase--;
				}
			}
		} else {
			for(int cmdNum=commandQue.size()-1;cmdNum!=-1 && numToErase;--cmdNum){
				if(commandQue[cmdNum].id==c.id){
					commandQue[cmdNum].id=CMD_STOP;
					numToErase--;
				}
			}
		}
		UpdateIconName(c.id,bo);
		SlowUpdate();

	} else {
		if(c.options & ALT_KEY){
			for(int a=0;a<numItems;++a){
				commandQue.push_front(c);
			}
			building=false;
			CFactory* fac=(CFactory*)owner;
			fac->StopBuild();
		} else {
			for(int a=0;a<numItems;++a){
				commandQue.push_back(c);
			}
		}
		bo.numQued+=numItems;
		UpdateIconName(c.id,bo);

		SlowUpdate();
	}
}

void CFactoryCAI::SlowUpdate()
{
	if(commandQue.empty() || owner->beingBuilt)
		return;

	CFactory* fac=(CFactory*)owner;

	unsigned int oldSize;
	do{
		Command& c=commandQue.front();
		oldSize=commandQue.size();
		map<int,BuildOption>::iterator boi;
		if((boi=buildOptions.find(c.id))!=buildOptions.end()){
			if(building){
				if(!fac->curBuild && !fac->quedBuild){
					building=false;
					if(owner->group)
						owner->group->CommandFinished(owner->id,commandQue.front().id);
					if(!repeatOrders)
						boi->second.numQued--;
					UpdateIconName(c.id,boi->second);
					FinishCommand();
				}
			} else {
				if(uh->maxUnits>gs->Team(owner->team)->units.size()){
					fac->StartBuild(boi->second.fullName);
					building=true;
				}
			}
		} else {
			switch(c.id){
			case CMD_STOP:
				building=false;
				fac->StopBuild();
				commandQue.pop_front();
				break;
			default:
				CCommandAI::SlowUpdate();
				return;
			}
		}
	}while(oldSize!=commandQue.size() && !commandQue.empty());

	return;
}

int CFactoryCAI::GetDefaultCmd(CUnit *pointed,CFeature* feature)
{
	return CMD_MOVE;
}

void CFactoryCAI::UpdateIconName(int id,BuildOption& bo)
{
	vector<CommandDescription>::iterator pci;
	for(pci=possibleCommands.begin();pci!=possibleCommands.end();++pci){
		if(pci->id==id){
			char t[32];
			SNPRINTF(t,10,"%d",bo.numQued);
			pci->name=bo.name;
			pci->params.clear();
			if(bo.numQued)
				pci->params.push_back(t);
			break;
		}
	}
	selectedUnits.PossibleCommandChange(owner);
}


void CFactoryCAI::DrawCommands(void)
{
	lineDrawer.StartPath(owner->midPos, cmdColors.start);

	deque<Command>::iterator ci;
	for(ci=newUnitCommands.begin();ci!=newUnitCommands.end();++ci){
		switch(ci->id){
			case CMD_MOVE:{
				const float3 endPos(ci->params[0],ci->params[1]+3,ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.move);
				break;
			}
			case CMD_FIGHT:{
				const float3 endPos(ci->params[0],ci->params[1]+3,ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.fight);
				break;
			}
			case CMD_PATROL:{
				const float3 endPos(ci->params[0],ci->params[1]+3,ci->params[2]);
				lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.patrol);
				break;
			}
			case CMD_ATTACK:{
				if(ci->params.size()==1){
					if(uh->units[int(ci->params[0])]!=0){
						const float3 endPos = uh->units[int(ci->params[0])]->pos;
						lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.attack);
					}
				} else {
					const float3 endPos(ci->params[0],ci->params[1]+3,ci->params[2]);
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.attack);
				}
				break;
			}
			case CMD_GUARD:{
				if(uh->units[int(ci->params[0])]!=0){
					const float3 endPos = uh->units[int(ci->params[0])]->pos;
					lineDrawer.DrawLineAndIcon(ci->id, endPos, cmdColors.guard);
				}
				break;
			}
		}
	}
	lineDrawer.FinishPath();
}


/**
* @brief Finds the queued command that would be canceled by the Command c
* @return An iterator located at the command, or commandQue.end() if no such queued command exsists
**/
std::deque<Command>::iterator CFactoryCAI::GetCancelQueued(Command &c){
	if(!newUnitCommands.empty()){
		std::deque<Command>::iterator ci=newUnitCommands.end();
		do{
			--ci;			//iterate from the end and dont check the current order
			if((ci->id==c.id || (c.id<0 && ci->id<0)) && ci->params.size()==c.params.size()){
				if(c.params.size()==1){			//we assume the param is a unit of feature id
					if(ci->params[0]==c.params[0]){
						return ci;
					}
				} else if(c.params.size()>=3){		//we assume this means that the first 3 makes a position
					float3 cpos(c.params[0],c.params[1],c.params[2]);
					float3 cipos(ci->params[0],ci->params[1],ci->params[2]);

					if((cpos-cipos).SqLength2D()<17*17){
						return ci;
					}
				}
			}
		}while(ci!=newUnitCommands.begin());
	}
	return newUnitCommands.end();
}
