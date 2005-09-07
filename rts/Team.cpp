// Team.cpp: implementation of the CTeam class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Team.h"
#include "InfoConsole.h"
#include "Player.h"
#include "Unit.h"
#include "UnitHandler.h"
#include "UnitDef.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTeam::CTeam()
: active(false),
	metal(200000),
	energy(900000),
	metalIncome(0),
	energyIncome(0),
	metalUpkeep(0),
	energyUpkeep(0),
	metalExpense(0),
	energyExpense(0),
	oldMetalIncome(0),
	oldEnergyIncome(0),
	oldMetalUpkeep(0),
	oldEnergyUpkeep(0),
	oldMetalExpense(0),
	oldEnergyExpense(0),
	metalStorage(1000000),
	energyStorage(1000000),
	metalShare(0.99f),
	energyShare(0.95f),
	tempMetalIncome(0),
	colorNum(0),
	side("arm"),
	startPos(100,100,100),
	handicap(1),
	leader(0),
	isDead(false),
	lastStatSave(0),
	numCommanders(0)
{
	memset(&currentStats,0,sizeof(currentStats));
	statHistory.push_back(currentStats);
}

CTeam::~CTeam()
{

}

bool CTeam::UseMetal(float amount)
{
	if(metal-oldMetalUpkeep*10>=amount){
		metal-=amount;
		metalExpense+=amount;
		return true;
	}
	return false;
}

bool CTeam::UseEnergy(float amount)
{
	if(energy-oldEnergyUpkeep*10>=amount){
		energy-=amount;
		energyExpense+=amount;
		return true;
	}
	return false;
}

bool CTeam::UseMetalUpkeep(float amount)
{
	if(metal>=amount){
		metal-=amount;
		metalExpense+=amount;
		metalUpkeep+=amount;
		return true;
	}
	return false;
}

bool CTeam::UseEnergyUpkeep(float amount)
{
	if(energy>=amount){
		energy-=amount;
		energyExpense+=amount;
		energyUpkeep+=amount;
		return true;
	}
	return false;
}

void CTeam::AddMetal(float amount)
{
	amount*=handicap;
	metal+=amount;
	metalIncome+=amount;
	if(metal>metalStorage){
		currentStats.metalExcess+=metal-metalStorage;
		metal=metalStorage;
	}
}

void CTeam::AddEnergy(float amount)
{
	amount*=handicap;
	energy+=amount;
	energyIncome+=amount;
	if(energy>energyStorage){
		currentStats.energyExcess+=energy-energyStorage;
		energy=energyStorage;
	}
}

void CTeam::Update()
{
	currentStats.metalProduced+=metalIncome;
	currentStats.energyProduced+=energyIncome;
	currentStats.metalUsed+=metalUpkeep+metalExpense;
	currentStats.energyUsed+=energyUpkeep+energyExpense;

	oldMetalIncome=metalIncome;
	metalIncome=0;
	oldEnergyIncome=energyIncome;
	energyIncome=0;
	oldMetalUpkeep=metalUpkeep;
	metalUpkeep=0;
	oldEnergyUpkeep=energyUpkeep;
	energyUpkeep=0;
	oldMetalExpense=metalExpense;
	metalExpense=0;
	oldEnergyExpense=energyExpense;
	energyExpense=0;

	float eShare=0,mShare=0;
	for(int a=0;a<gs->activeTeams;++a){
		if(a!=teamNum && gs->allies[gs->team2allyteam[teamNum]][gs->team2allyteam[a]]){
			eShare+=max(0.0,gs->teams[a]->energyStorage*0.9-gs->teams[a]->energy);
			mShare+=max(0.0,gs->teams[a]->metalStorage*0.9-gs->teams[a]->metal);
		}
	}
	float eExcess=max(0.0,energy-energyStorage*energyShare);
	float mExcess=max(0.0,metal-metalStorage*metalShare);
	
	float de=0,dm=0;
	if(eShare>0)
		de=min(1.0f,eExcess/eShare);
	if(mShare>0)
		dm=min(1.0f,mExcess/mShare);

	for(int a=0;a<gs->activeTeams;++a){
		if(a!=teamNum && gs->allies[gs->team2allyteam[teamNum]][gs->team2allyteam[a]]){
			float edif=max(0.0,gs->teams[a]->energyStorage*0.9-gs->teams[a]->energy)*de;
			gs->teams[a]->energy+=edif;
			energy-=edif;
			currentStats.energySent+=edif;
			gs->teams[a]->currentStats.energyReceived+=edif;
			float mdif=max(0.0,gs->teams[a]->metalStorage*0.9-gs->teams[a]->metal)*dm;
			gs->teams[a]->metal+=mdif;
			metal-=mdif;
			currentStats.metalSent+=edif;
			gs->teams[a]->currentStats.metalReceived+=edif;
		}
	}

	if(lastStatSave+480<gs->frameNum){		//save every 16th second
		lastStatSave+=480;
		statHistory.push_back(currentStats);
	}
}

void CTeam::AddUnit(CUnit* unit,AddType type)
{
	units.insert(unit);
	switch(type){
	case AddBuilt:
		currentStats.unitsProduced++;
		break;
	case AddGiven:
		currentStats.unitsReceived++;
		break;
	case AddCaptured:
		currentStats.unitsCaptured++;
		break;
	}
	if(unit->unitDef->isCommander)
		numCommanders++;
}

void CTeam::RemoveUnit(CUnit* unit,RemoveType type)
{
	units.erase(unit);
	switch(type){
	case RemoveDied:
		currentStats.unitsDied++;
		break;
	case RemoveGiven:
		currentStats.unitsSent++;
		break;
	case RemoveCaptured:
		currentStats.unitsOutCaptured++;
		break;
	}

	if(units.empty()){
		info->AddLine("Team%i(%s) is no more",teamNum,gs->players[leader]->playerName.c_str());
		isDead=true;
		for(int a=0;a<MAX_PLAYERS;++a){
			if(gs->players[a]->active && gs->players[a]->team==teamNum){
				gs->players[a]->spectator=true;
				if(a==gu->myPlayerNum)
					gu->spectating=true;
			}
		} 
	}
}

void CTeam::CommanderDied(CUnit* commander)
{
	numCommanders--;
	if(gs->gameMode==1 && numCommanders<=0){
		for(list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui){
			if((*ui)->team==teamNum)
				(*ui)->KillUnit(true,false);
		}
	}
}
