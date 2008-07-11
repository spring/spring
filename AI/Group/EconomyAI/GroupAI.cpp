// GroupAI.cpp: implementation of the CGroupAI class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <cassert>
#include "GroupAI.h"
#include "ExternalAI/IGroupAiCallback.h"
#include "ExternalAI/IAICallback.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/CommandAI/CommandQueue.h"
#include "creg/STL_Deque.h"
#include "creg/STL_Map.h"
#include "creg/Serializer.h"
#include "creg/cregex.h"

#define CMD_SET_AREA	 	150
#define CMD_START			160
#define CMD_SET_PERCENTAGE	170

CR_BIND(CGroupAI, );

CR_REG_METADATA(CGroupAI,(
				CR_MEMBER(idealME),
				CR_MEMBER(currentME),

				CR_MEMBER(myUnits),
				CR_MEMBER(currentBuilder),
				CR_MEMBER(totalBuildSpeed),

				CR_MEMBER(helper),
				//CR_MEMBER(boHandler),

				CR_MEMBER(unitRemoved),

				CR_MEMBER(newBuildTaskNeeded),
				CR_MEMBER(newBuildTaskFrame),

				//CR_MEMBER(initialized),

				CR_MEMBER(maxResourcePercentage),
				CR_MEMBER(totalMMenergyUpkeep),
				CR_POSTLOAD(PostLoad)
				));

int Instances = 0;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGroupAI::CGroupAI()
{
	totalBuildSpeed = 0;
	currentBuilder	= 0;
	unitRemoved		= false;
	newBuildTaskNeeded = false;
	newBuildTaskFrame = 0;
	maxResourcePercentage = 1.0f;
	initialized		= false;
	if (!(Instances++)) creg::System::InitializeClasses ();
}

CGroupAI::~CGroupAI()
{
	myUnits.clear();
	if(initialized)
	{
		delete helper;
		delete boHandler;
	}
	if (!(--Instances)) creg::System::FreeClasses ();
}

void CGroupAI::PostLoad()
{
	helper->aicb = aicb;
	helper->PostLoad();
	boHandler	= new CBoHandler(aicb,helper->mmkrME,helper->metalMap->AverageMetalPerSpot,helper->maxPartitionRadius);

	for (map<int,float>::iterator i=myUnits.begin();i!=myUnits.end();i++) {
		const UnitDef* ud=aicb->GetUnitDef(i->first);
		boHandler->AddBuildOptions(ud);
	}

	initialized = true;
}

void CGroupAI::InitAi(IGroupAICallback* callback)
{
	this->callback=callback;
	aicb=callback->GetAICallback();
	
	helper		= new CHelper(aicb,this);
	boHandler	= new CBoHandler(aicb,helper->mmkrME,helper->metalMap->AverageMetalPerSpot,helper->maxPartitionRadius);

	initialized = true;
}

bool CGroupAI::AddUnit(int unit)
{
	const UnitDef* ud=aicb->GetUnitDef(unit);
	totalBuildSpeed += ud->buildSpeed;
	myUnits[unit] = ud->buildSpeed;
	boHandler->AddBuildOptions(ud);
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

void CGroupAI::GiveCommand(Command* c)
{
	if(c->id==CMD_SET_AREA && c->params.size()==4)
	{
		float3 pos = float3(c->params[0],c->params[1],c->params[2]);
		if(!(c->options& SHIFT_KEY))
			helper->ResetLocations();
		helper->NewLocation(pos,c->params[3]);
		if(currentBuilder==0 || (ValidCurrentBuilder() && aicb->GetCurrentUnitCommands(currentBuilder)->empty())) 
			FindNewBuildTask();
	}
	if(c->id==CMD_START)
	{
		FindNewBuildTask();
	}
	if(c->id==CMD_STOP)
	{
		Command c;
		c.id = CMD_STOP;
		for(map<int,float>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui)
			aicb->GiveOrder(ui->first, &c);
		helper->ResetLocations();
	}
	if(c->id==CMD_SET_PERCENTAGE)
	{
		maxResourcePercentage = 1.0f - 0.25 * c->params[0];
	}
}

const vector<CommandDescription>& CGroupAI::GetPossibleCommands()
{
	commands.clear();

	CommandDescription cd;

	cd.id=CMD_SET_AREA;
	cd.type=CMDTYPE_ICON_AREA;
	cd.name="Set area";
	cd.action="repair";
	cd.tooltip="Set area: define an area where the Economy AI can build";
	commands.push_back(cd);

	cd.params.clear();
	cd.id=CMD_STOP;
	cd.type=CMDTYPE_ICON;
	cd.name="Stop";
	cd.action="stop";
	cd.tooltip="Stop all units and remove all buildings sites";
	commands.push_back(cd);

	cd.params.clear();
	cd.id=CMD_START;
	cd.type=CMDTYPE_ICON;
	cd.name="Start";
	cd.action="onoff";
	cd.tooltip="Begin building resources on the current building sites";
	commands.push_back(cd);

	cd.params.clear();
	cd.id=CMD_SET_PERCENTAGE;
	cd.type=CMDTYPE_ICON_MODE;
	cd.name="Max resource usage";
	cd.action="reclaim";
	int stateInt = int((1.0f - maxResourcePercentage) / 0.25f);
	char stateChar[1];
	sprintf(stateChar,"%i",stateInt);
	cd.params.push_back(stateChar);
	cd.params.push_back("100%");
	cd.params.push_back("75%");
	cd.params.push_back("50%");
	cd.params.push_back("25%");
	cd.tooltip="Maximum percentage of available resources that may be used";
	commands.push_back(cd);
	return commands;
}

int CGroupAI::GetDefaultCmd(int unit)
{
	return CMD_SET_AREA;
}

void CGroupAI::CommandFinished(int unit,int type)
{
	// check if we have just built something
	string name = helper->BuildIdToName(type,unit);
	if(name!="")
	{
		// did we built a metalmaker? then assign the metalmaker AI to it
		const UnitDef* ud=aicb->GetUnitDef(name.c_str());
		if(ud!=0)
		{
			if(ud->isMetalMaker)
			{
				helper->AssignMetalMakerAI();
			}
		}
		// if the unit is the currentBuilder, we have to find a new build task
		if(unit==currentBuilder && aicb->GetCurrentUnitCommands(unit)->empty())
		{
			// wait 2 seconds before actually finding a new build task
			// (because some buildings take a few seconds to get online, e.g. solars)
			newBuildTaskFrame = aicb->GetCurrentFrame();
			newBuildTaskNeeded = true;
		}
	}
}


void CGroupAI::Update()
{
	if(newBuildTaskNeeded && aicb->GetCurrentFrame() > newBuildTaskFrame + 60)
	{
		FindNewBuildTask();
	}
}

void CGroupAI::DrawCommands()
{
	if(callback->IsSelected())
		helper->DrawBuildArea();
}

void CGroupAI::FindNewBuildTask()
{
	newBuildTaskNeeded = false;

	// if there's a unit removed from our group, we have to reset all build options
	if(unitRemoved)
	{
		unitRemoved = false;
		boHandler->ClearBuildOptions();
		// insert all build options
		for(map<int,float>::iterator ui=myUnits.begin();ui!=myUnits.end();++ui)
		{
			const UnitDef* ud=aicb->GetUnitDef(ui->first);
			if(ud==0)
				continue;
			boHandler->AddBuildOptions(ud);
		}
	}

	boHandler->SortBuildOptions();

	CalculateCurrentME();
	// do we want to build energy or metal?
	bool buildMetal = false; // true: build metal, false: build energy
	// check if we're wasting metal
	if(aicb->GetEnergyIncome() > (1.1 * aicb->GetEnergyUsage()) && (1.2 * aicb->GetEnergy()) > aicb->GetEnergyStorage())
	{
		buildMetal = true;
	}
	// check if we're wasting energy
	else if(aicb->GetMetalIncome() > (1.1 * aicb->GetMetalUsage()) && (1.2 * aicb->GetMetal()) > aicb->GetMetalStorage())
	{
		buildMetal = false;
	}
	else
	{
		CalculateIdealME();
		if(currentME < idealME)
			buildMetal = true;
	}
	vector<BOInfo*>* bestBO = buildMetal ? &(boHandler->bestMetal) : &(boHandler->bestEnergy);

	// find a unit to build
	for(vector<BOInfo*>::iterator boi=bestBO->begin();boi!=bestBO->end();++boi)
	{
		string name = (*boi)->name;

		// check if we have enough resources to build it
		int buildFrames = (int) (*boi)->buildTime / max(1.0f,totalBuildSpeed);
		float metalEnd	= maxResourcePercentage * (aicb->GetMetal() + buildFrames * (aicb->GetMetalIncome() - aicb->GetMetalUsage()));
		float energyEnd	= maxResourcePercentage * (aicb->GetEnergy() + buildFrames * (aicb->GetEnergyIncome() - aicb->GetEnergyUsage() + totalMMenergyUpkeep));
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
			float3 pos = helper->FindBuildPos(name,(*boi)->isMex, (*boi)->isGeo,(*boi)->spacing,currentBuilder);
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
	if(ValidCurrentBuilder())
	{
		aicb->SetLastMsgPos(aicb->GetUnitPos(currentBuilder));
	}
}

void CGroupAI::CalculateIdealME()
{
	float totalMetalCost	= 0.0f;
	float totalEnergyCost	= 0.0f;

	set<string> currentBO;

	int size = aicb->GetFriendlyUnits(&helper->friendlyUnits.front());
	for(int i=0;i<size;i++)
	{
		if(helper->myTeam != aicb->GetUnitTeam(helper->friendlyUnits[i]))
			continue;
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
	totalMMenergyUpkeep = 0.0f;
	// parse through every unit to get the total energy and metal upkeep
	int size = aicb->GetFriendlyUnits(&helper->friendlyUnits.front());
	for(int i=0;i<size;i++)
	{
		int unit = helper->friendlyUnits[i];
		if(helper->myTeam != aicb->GetUnitTeam(unit))
			continue;
		const UnitDef* ud=aicb->GetUnitDef(unit);
		if(ud==0)
			continue;
		if(ud->energyUpkeep > 0)
		{
			if(ud->isMetalMaker && aicb->IsUnitActivated(unit))
			{
				totalMMenergyUpkeep += ud->energyUpkeep;
			}
			totalEnergyUpkeep	+= ud->energyUpkeep;
		}
		if(ud->metalUpkeep > 0)
		{
			totalMetalUpkeep	+= ud->metalUpkeep;
		}
	}
	float effMetal	= aicb->GetMetalIncome() - totalMetalUpkeep;
	float effEnergy = aicb->GetEnergyIncome() - totalEnergyUpkeep;
	if(effMetal < 1)
		effMetal = 1;
	if(effEnergy < 1)
		effEnergy = 1;
	currentME = effMetal / effEnergy;
}

void CGroupAI::SetUnitGuarding(int unit)
{
	if(ValidCurrentBuilder())
	{
		Command c;
		c.id = CMD_GUARD;
		c.params.push_back(currentBuilder);
		aicb->GiveOrder(unit, &c);
	}
}

bool CGroupAI::ValidCurrentBuilder()
{
	if(myUnits.find(currentBuilder)!=myUnits.end())
		return true;
	else
		return false;
}

CREX_REG_STATE_COLLECTOR(EconomyAI,CGroupAI);

void CGroupAI::Load(IGroupAICallback* callback,std::istream *ifs)
{
	this->callback=callback;
	aicb=callback->GetAICallback();
	CREX_SC_LOAD(EconomyAI,ifs);
}

void CGroupAI::Save(std::ostream *ofs)
{
	CREX_SC_SAVE(EconomyAI,ofs);
}
