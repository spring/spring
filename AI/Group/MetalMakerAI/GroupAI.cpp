// GroupAI.cpp: implementation of the CGroupAI class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GroupAI.h"
#include "ExternalAI/IGroupAiCallback.h"
#include "ExternalAI/IAICallback.h"
#include "Sim/Units/UnitDef.h"
#include <vector>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace std{
	void _xlen(){};
}

CGroupAI::CGroupAI()
{
	lastUpdate=0;
}

CGroupAI::~CGroupAI()
{
	for(map<int,UnitInfo*>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui)
		delete ui->second;
	myUnits.clear();
}

void CGroupAI::InitAi(IGroupAICallback* callback)
{
	this->callback=callback;
	aicb=callback->GetAICallback();
}

bool CGroupAI::AddUnit(int unit)
{
	const UnitDef* ud=aicb->GetUnitDef(unit);
	if(!(ud->energyUpkeep>0.0f && (ud->makesMetal>0.0f || ud->extractsMetal>0.0f))){
		aicb->SendTextMsg("Can only use metal makers",0);
		return false;
	}
	UnitInfo* info=new UnitInfo;

	const std::vector<CommandDescription>* cd=aicb->GetUnitCommands(unit);
	for(std::vector<CommandDescription>::const_iterator cdi=cd->begin();cdi!=cd->end();++cdi){
		if(cdi->id==CMD_ONOFF){
			int on=atoi(cdi->params[0].c_str());
			if(on)
				info->turnedOn=true;
			else
				info->turnedOn=false;
			break;
		}
	}
	info->energyUse=ud->energyUpkeep;
	myUnits[unit]=info;

	return true;
}

void CGroupAI::RemoveUnit(int unit)
{
	delete myUnits[unit];
	myUnits.erase(unit);
}

void CGroupAI::GiveCommand(Command* c)
{
}

int CGroupAI::GetDefaultCmd(int unitid)
{
	return CMD_STOP;
}

void CGroupAI::Update()
{
	int frameNum=aicb->GetCurrentFrame();
	if(lastUpdate<=frameNum-32){
		lastUpdate=frameNum;

		float energy=aicb->GetEnergy();
		float estore=aicb->GetEnergyStorage();
		float dif=energy-lastEnergy;
		lastEnergy=energy;

		if(energy<estore*0.3){
			float needed=-dif+5;		//how much energy we need to save to turn positive
			for(map<int,UnitInfo*>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui){		//find makers to turn off
				if(needed<0)
					break;
				if(ui->second->turnedOn){
					needed-=ui->second->energyUse;
					Command c;
					c.id=CMD_ONOFF;
					c.params.push_back(0);
					aicb->GiveOrder(ui->first,&c);
					ui->second->turnedOn=false;
				}
			}
		} else if(energy>estore*0.7){
			float needed=dif+5;		//how much energy we need to start using to turn negative
			for(map<int,UnitInfo*>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui){		//find makers to turn on
				if(needed<0)
					break;
				if(!ui->second->turnedOn){
					needed-=ui->second->energyUse;
					Command c;
					c.id=CMD_ONOFF;
					c.params.push_back(1);
					aicb->GiveOrder(ui->first,&c);
					ui->second->turnedOn=true;
				}
			}
		}
	}
}
