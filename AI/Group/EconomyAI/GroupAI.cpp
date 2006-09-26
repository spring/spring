// GroupAI.cpp: implementation of the CGroupAI class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GroupAI.h"
#include "ExternalAI/IGroupAiCallback.h"
#include "ExternalAI/IAICallback.h"
#include "Sim/Units/UnitDef.h"

#define CMD_SET_AREA	 	150
#define CMD_DUMMY			170

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroupAI::CGroupAI()
{
	totalBuildSpeed = 0;
	currentBuilder	= 0;
	BOchanged		= false;
	unitRemoved		= false;
	newBuildTaskNeeded = false;
	newBuildTaskFrame = 0;

	CommandDescription cd;
	cd.id=CMD_SET_AREA;
	cd.type=CMDTYPE_ICON_AREA;
	cd.name="Set area";
	cd.action="repair";
	cd.hotkey="a";
	cd.tooltip="Set area: define an area where the Economy AI can build";
	commands.push_back(cd);
}

CGroupAI::~CGroupAI()
{
	for(map<string,BOInfo*>::iterator boi=allBO.begin();boi!=allBO.end();++boi)
		delete boi->second;
	allBO.clear();
	myUnits.clear();
	delete helper;
}

void CGroupAI::InitAi(IGroupAICallback* callback)
{
	this->callback=callback;
	aicb=callback->GetAICallback();
	
	helper = new CHelper(aicb);

	tidalStrength	= aicb->GetTidalStrength();
	avgWind			= (aicb->GetMinWind() + aicb->GetMaxWind())/2;
	avgMetal		= helper->metalMap->AverageMetalPerSpot;
}

bool CGroupAI::AddUnit(int unit)
{
	const UnitDef* ud=aicb->GetUnitDef(unit);
	totalBuildSpeed += ud->buildSpeed;
	myUnits[unit] = ud->buildSpeed;
	AddBuildOptions(ud);
	SetUnitGuarding(unit);
	return true;
}

void CGroupAI::RemoveUnit(int unit)
{
	unitRemoved = true;
	totalBuildSpeed -= myUnits[unit];
	myUnits.erase(unit);
	if(unit==currentBuilder && !myUnits.empty())
		FindNewBuildTask();
}

void CGroupAI::FindNewBuildTask()
{
	if(!newBuildTaskNeeded)
	{
		newBuildTaskFrame = aicb->GetCurrentFrame();
		newBuildTaskNeeded = true;
		return;
	}
	if(newBuildTaskNeeded && aicb->GetCurrentFrame() > newBuildTaskFrame + 60)
		newBuildTaskNeeded = false;

	CalculateIdealME();
	CalculateCurrentME();

	// if there's a unit removed from our group, we have to reset all build options
	if(unitRemoved)
	{
		unitRemoved = false;
		// clear all build options
		for(map<string,BOInfo*>::iterator boi=allBO.begin();boi!=allBO.end();++boi)
			delete boi->second;
		allBO.clear();

		// insert all build options
		for(map<int,float>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui)
		{
			const UnitDef* ud=aicb->GetUnitDef(ui->first);
			if(ud==0)
				continue;
			AddBuildOptions(ud);
		}
	}

	// if the build options have changed, we have to sort them again
	if(BOchanged)
	{
		BOchanged = false;
		bestMetal.clear();
		bestEnergy.clear();
		for(map<string,BOInfo*>::const_iterator boi=allBO.begin();boi!=allBO.end();++boi)
		{
			BOInfo* info = boi->second;
			if(info->mp > 0) bestMetal.push_back(info);
			if(info->ep > 0) bestEnergy.push_back(info);
		}
		sort(bestMetal.begin(),bestMetal.end(),compareMetal());
		sort(bestEnergy.begin(),bestEnergy.end(),compareEnergy());
	}

	// do we want to build energy or metal?
	vector<BOInfo*>* bestBO = (currentME > idealME) ? &bestEnergy : &bestMetal;

	// find a unit to build
	for(vector<BOInfo*>::iterator boi=bestBO->begin();boi!=bestBO->end();++boi)
	{
		string name = (*boi)->name;
		// check if we have enough resource to build it
		int buildFrames = (int) (*boi)->buildTime / max(1,totalBuildSpeed);
		float metalEnd	= aicb->GetMetal() + buildFrames * aicb->GetMetalIncome();
		float energyEnd	= aicb->GetEnergy() + buildFrames * aicb->GetEnergyIncome();
		if(metalEnd < (*boi)->metalCost || energyEnd < (*boi)->energyCost)
			continue;
		// find a builder
		pair<int,int> commandPair;
		map<int,float>::iterator ui;
		for(ui=myUnits.begin();ui!=myUnits.end();++ui)
		{
			commandPair = helper->BuildNameToId(name,ui->first);
			if(commandPair.first!=0)
				break;
		}
		currentBuilder = ui->first;
		Command c;
		c.id = commandPair.first;
		// find a build position if it isn't build in a factory
		if(commandPair.second!=CMDTYPE_ICON)
		{
			float3	pos		= helper->FindBuildPos(name,(*boi)->isMex, (*boi)->isGeo,currentBuilder);
			if(pos==helper->errorPos)
				continue;
			c.params.push_back(pos.x);
			c.params.push_back(pos.y);
			c.params.push_back(pos.z);
		}
		aicb->GiveOrder(currentBuilder,&c);
		// set all units to guard the current builder
		for(ui=myUnits.begin();ui!=myUnits.end();++ui)
		{
			if(ui->first!=currentBuilder)
				SetUnitGuarding(ui->first);
		}
		return;
	}
	// if we got this far there's nothing to build
	helper->SendTxt("Nothing to build");
	if(myUnits.find(currentBuilder)!=myUnits.end())
	{
		aicb->SetLastMsgPos(aicb->GetUnitPos(currentBuilder));
	}
}

void CGroupAI::AddBuildOptions(const UnitDef* unitDef)
{
	if(unitDef->buildOptions.empty())
		return;
	for(map<int,string>::const_iterator boi=unitDef->buildOptions.begin();boi!=unitDef->buildOptions.end();++boi)
	{
		if(allBO.find(boi->second)!=allBO.end())
			continue;

		BOInfo* info = new BOInfo;
		const UnitDef* ud=aicb->GetUnitDef(boi->second.c_str());
		info->name			= boi->second;
		info->energyCost	= ud->energyCost;
		info->metalCost		= ud->metalCost;
		info->buildTime		= ud->buildTime; // this is always 1 or bigger
		info->totalCost		= max(1,(info->energyCost * helper->mmkrME) + info->metalCost);
		info->isMex			= (ud->type=="MetalExtractor") ? true : false;
		info->isGeo			= (ud->needGeo) ? true : false;

		info->mp = ud->extractsMetal*avgMetal + ud->metalMake + ud->makesMetal - ud->metalUpkeep;
		info->ep = ud->energyMake - ud->energyUpkeep + ud->tidalGenerator*tidalStrength + min(ud->windGenerator,avgWind);
		info->me = info->mp / max(ud->energyUpkeep,1);
		info->em = info->ep / max(ud->metalUpkeep,1);

		allBO[info->name] = info;

		BOchanged = true;
	}
	return;
}

void CGroupAI::CalculateIdealME()
{
	float totalMetalCost	= 0.0f;
	float totalEnergyCost	= 0.0f;

	set<string> currentBO;

	int size = aicb->GetFriendlyUnits(helper->friendlyUnits);
	for(int i=0;i<size;i++)
	{
		const UnitDef* ud=aicb->GetUnitDef(helper->friendlyUnits[i]);
		if(ud==0)
			continue;
		for(map<int,string>::const_iterator boi=ud->buildOptions.begin(); boi!=ud->buildOptions.end(); ++boi)
		{
			string name = boi->second;
			if(currentBO.find(name)!=currentBO.end())
				continue;
			const UnitDef* ud2=aicb->GetUnitDef(name.c_str());
			if(ud2->type=="Factory") // don't take factories into account, as they tend to be more expensive
				continue;
			currentBO.insert(name);
			totalMetalCost	+= ud2->metalCost;
			totalEnergyCost	+= ud2->energyCost;
		}
	}
	if(totalMetalCost < 1)
		totalMetalCost = 1;
	if(totalEnergyCost < 1)
		totalEnergyCost = 1;
	idealME = totalMetalCost / totalEnergyCost;
}

void CGroupAI::CalculateCurrentME()
{
	float totalEnergyUpkeep	= 0.0f;
	float totalMetalUpkeep	= 0.0f;
	// parse through every unit to get the total energy and metal upkeep
	int size = aicb->GetFriendlyUnits(helper->friendlyUnits);
	for(int i=0;i<size;i++)
	{
		int unit = helper->friendlyUnits[i];
		const UnitDef* ud=aicb->GetUnitDef(unit);
		if(ud==0)
			continue;
		if(ud->energyUpkeep > 0)	totalEnergyUpkeep	+= ud->energyUpkeep;
		if(ud->metalUpkeep > 0)		totalMetalUpkeep	+= ud->metalUpkeep;
	}
	float effMetal	= aicb->GetMetalIncome() - totalMetalUpkeep;
	float effEnergy = aicb->GetEnergyIncome() - totalEnergyUpkeep;
	if(effMetal < 1)
		effMetal = 1;
	if(effEnergy < 1)
		effEnergy = 1;
	currentME = effMetal / effEnergy;
}

void CGroupAI::GiveCommand(Command* c)
{
	if(c->id==CMD_SET_AREA && c->params.size()==4)
	{
		float3 pos = float3(c->params[0],c->params[1],c->params[2]);
		bool reset = (c->options& SHIFT_KEY) ? false : true;
		helper->NewLocation(pos,c->params[3],reset);

		if(currentBuilder==0 || (currentBuilder!=0 && aicb->GetCurrentUnitCommands(currentBuilder)->empty())) 
			FindNewBuildTask();
	}
}

const vector<CommandDescription>& CGroupAI::GetPossibleCommands()
{
	return commands;
}

int CGroupAI::GetDefaultCmd(int unit)
{
	return CMD_SET_AREA;
}

void CGroupAI::CommandFinished(int unit,int type)
{
	// check if we have just built a metalmaker
	string name = helper->BuildIdToName(type,unit);
	if(name!="")
	{
		const UnitDef* ud=aicb->GetUnitDef(name.c_str());
		if(ud!=0)
		{
			if(ud->isMetalMaker)
			{
				helper->AssignMetalMakerAI();
			}
		}
	}
	if(unit==currentBuilder && aicb->GetCurrentUnitCommands(unit)->empty())
		FindNewBuildTask();
}


void CGroupAI::Update()
{
	if(newBuildTaskNeeded && aicb->GetCurrentFrame() > newBuildTaskFrame + 60)
		FindNewBuildTask();

	if(callback->IsSelected())
		helper->DrawBuildArea();
}

void CGroupAI::SetUnitGuarding(int unit)
{
	if(myUnits.find(currentBuilder)!=myUnits.end())
	{
		Command c;
		c.id = CMD_GUARD;
		c.params.push_back(currentBuilder);
		aicb->GiveOrder(unit, &c);
	}
}