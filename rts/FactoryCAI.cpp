#include "stdafx.h"
#include ".\factorycai.h"
#include "Factory.h"
#include "group.h"
#include "selectedunits.h"
#include "mygl.h"
#include "unithandler.h"
#include "unitloader.h"
#include "unitdefhandler.h"
#include "team.h"
//#include "mmgr.h"

CFactoryCAI::CFactoryCAI(CUnit* owner)
: CCommandAI(owner),
	building(false)
{
	CommandDescription c;
	c.id=CMD_MOVE;
	c.type=CMDTYPE_ICON_MAP;
	c.name="Move";
	c.key='M';
	possibleCommands.push_back(c);
	c.id=CMD_PATROL;
	c.type=CMDTYPE_ICON_MAP;
	c.name="Patrol";
	c.key='P';
	possibleCommands.push_back(c);

	CFactory* fac=(CFactory*)owner;

	map<int,string>::iterator bi;
	for(bi=fac->unitDef->buildOptions.begin();bi!=fac->unitDef->buildOptions.end();++bi){
		string name=bi->second;
		UnitDef* ud= unitDefHandler->GetUnitByName(name);
		CommandDescription c;
		c.id=-ud->id;								//build options are always negative
		c.type=CMDTYPE_ICON;
		c.name=name;

		char tmp[500];
		sprintf(tmp,"\nHealth %.0f\nCost %.0f Build time %.0f",ud->health,ud->metalCost,ud->buildTime);
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
	map<int,BuildOption>::iterator boi;
	if((boi=buildOptions.find(c.id))==buildOptions.end()){		//not a build order so que it to built units
		if(nonQueingCommands.find(c.id)!=nonQueingCommands.end()){
			CCommandAI::GiveCommand(c);
			return;
		}

		if(!(c.options & SHIFT_KEY)){
			newUnitCommands.clear();
		}
		if(c.id!=CMD_STOP)
			newUnitCommands.push_back(c);

		return;
	}
	BuildOption &bo=boi->second;
	if(!unitLoader.CanBuildUnit(bo.fullName,owner->team))
		return;
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
					boi->second.numQued--;
					UpdateIconName(c.id,boi->second);
					FinishCommand();
				}
			} else {
				if(uh->maxUnits>gs->teams[owner->team]->units.size()){
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
			if(unitLoader.CanBuildUnit(bo.fullName,owner->team)){
				char t[32];
				itoa(bo.numQued,t,10);
				pci->name=bo.name;
				pci->params.clear();
				if(bo.numQued)
					pci->params.push_back(t);
			}else{
				pci->name="Need tech";
			}
			break;
		}
	}
	selectedUnits.PossibleCommandChange(owner);
}

void CFactoryCAI::DrawCommands(void)
{
	float3 pos=owner->midPos;
	glColor4f(1,1,1,0.4);
	glBegin(GL_LINE_STRIP);
	glVertexf3(pos);
	deque<Command>::iterator ci;
	for(ci=newUnitCommands.begin();ci!=newUnitCommands.end();++ci){
		bool draw=false;
		switch(ci->id){
		case CMD_MOVE:
			pos=float3(ci->params[0],ci->params[1]+3,ci->params[2]);
			glColor4f(0.5,1,0.5,0.4);
			draw=true;
			break;
		case CMD_PATROL:
			pos=float3(ci->params[0],ci->params[1]+3,ci->params[2]);
			glColor4f(0.5,0.5,1,0.4);
			draw=true;
			break;
		case CMD_ATTACK:
			if(ci->params.size()==1){
				if(uh->units[int(ci->params[0])]!=0)
					pos=uh->units[int(ci->params[0])]->pos;
			} else {
				pos=float3(ci->params[0],ci->params[1]+3,ci->params[2]);
			}
			glColor4f(1,0.5,0.5,0.4);
			draw=true;
			break;
		case CMD_GUARD:
			if(uh->units[int(ci->params[0])]!=0)
				pos=uh->units[int(ci->params[0])]->pos;
			glColor4f(0.3,0.3,1,0.4);
			draw=true;
			break;
		}
		if(draw){
			glVertexf3(pos);	
		}
	}
	glEnd();
}
