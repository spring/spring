// Team.cpp: implementation of the CTeam class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Messages.h"
#include "Team.h"
#include "LogOutput.h"
#include "Player.h"
#include "Game/UI/LuaUI.h"
#include "Game/GameServer.h"
#include "Lua/LuaCallInHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDef.h"
#include "creg/STL_List.h"
#include "creg/STL_Map.h"
#include "creg/STL_Set.h"
#include "ExternalAI/GlobalAI.h"
#include "ExternalAI/GlobalAIHandler.h"
#include "mmgr.h"

CR_BIND(CTeam,);

CR_REG_METADATA(CTeam, (
				CR_MEMBER(teamNum),
				CR_RESERVED(1),
				CR_MEMBER(isDead),
				CR_MEMBER(gaia),
				CR_MEMBER(color),
				CR_MEMBER(leader),
				CR_MEMBER(lineageRoot),
				CR_MEMBER(handicap),
				CR_MEMBER(side),
				CR_MEMBER(luaAI),
				CR_MEMBER(units),
				CR_MEMBER(startPos),
				CR_MEMBER(metal),
				CR_MEMBER(energy),
				CR_MEMBER(metalPull),
				CR_MEMBER(prevMetalPull),
				CR_MEMBER(metalIncome),
				CR_MEMBER(prevMetalIncome),
				CR_MEMBER(metalExpense),
				CR_MEMBER(prevMetalExpense),
				CR_MEMBER(metalUpkeep),
				CR_MEMBER(prevMetalUpkeep),
				CR_MEMBER(energyPull),
				CR_MEMBER(prevEnergyPull),
				CR_MEMBER(energyIncome),
				CR_MEMBER(prevEnergyIncome),
				CR_MEMBER(energyExpense),
				CR_MEMBER(prevEnergyExpense),
				CR_MEMBER(energyUpkeep),
				CR_MEMBER(prevEnergyUpkeep),
				CR_MEMBER(metalStorage),
				CR_MEMBER(energyStorage),
				CR_MEMBER(metalShare),
				CR_MEMBER(energyShare),
				CR_MEMBER(delayedMetalShare),
				CR_MEMBER(delayedEnergyShare),
				CR_MEMBER(metalSent),
				CR_MEMBER(metalReceived),
				CR_MEMBER(energySent),
				CR_MEMBER(energyReceived),
				CR_MEMBER(currentStats),
				CR_MEMBER(lastStatSave),
				CR_MEMBER(numCommanders),
				CR_MEMBER(statHistory),
				CR_MEMBER(modParams),
				CR_MEMBER(modParamsMap),
				CR_RESERVED(64)
				));

CR_BIND(CTeam::Statistics,);

CR_REG_METADATA_SUB(CTeam, Statistics, (
					CR_MEMBER(metalUsed),
					CR_MEMBER(energyUsed),
					CR_MEMBER(metalProduced),
					CR_MEMBER(energyProduced),
					CR_MEMBER(metalExcess),
					CR_MEMBER(energyExcess),
					CR_MEMBER(metalReceived),
					CR_MEMBER(energyReceived),
					CR_MEMBER(metalSent),
					CR_MEMBER(energySent),
					CR_MEMBER(damageDealt),
					CR_MEMBER(damageReceived),
					CR_MEMBER(unitsProduced),
					CR_MEMBER(unitsDied),
					CR_MEMBER(unitsReceived),
					CR_MEMBER(unitsSent),
					CR_MEMBER(unitsCaptured),
					CR_MEMBER(unitsOutCaptured),
					CR_MEMBER(unitsKilled),
					CR_RESERVED(16)
					));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTeam::CTeam()
: 	gaia(false),
	metal(200000),
	energy(900000),
	metalPull(0),     prevMetalPull(0),
	metalIncome(0),   prevMetalIncome(0),
	metalExpense(0),  prevMetalExpense(0),
	metalUpkeep(0),   prevMetalUpkeep(0),
	energyPull(0),    prevEnergyPull(0),
	energyIncome(0),  prevEnergyIncome(0),
	energyExpense(0), prevEnergyExpense(0),
	energyUpkeep(0),  prevEnergyUpkeep(0),
	metalStorage(1000000),
	energyStorage(1000000),
	metalShare(0.99f),
	energyShare(0.95f),
	delayedMetalShare(0),
	delayedEnergyShare(0),
	metalSent(0),
	metalReceived(0),
	energySent(0),
	energyReceived(0),
	side("arm"),
	luaAI(""),
	startPos(100,100,100),
	handicap(1),
	leader(0),
	lineageRoot(-1),
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
	if(metal-prevMetalUpkeep*10>=amount){
		metal-=amount;
		metalExpense+=amount;
		return true;
	}
	return false;
}

bool CTeam::UseEnergy(float amount)
{
	if(energy-prevEnergyUpkeep*10>=amount){
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
		delayedMetalShare += metal - metalStorage;
		metal=metalStorage;
	}
}

void CTeam::AddEnergy(float amount)
{
	amount*=handicap;
	energy+=amount;
	energyIncome+=amount;
	if(energy>energyStorage){
		delayedEnergyShare += energy - energyStorage;
		energy=energyStorage;
	}
}

void CTeam::SlowUpdate()
{
	currentStats.metalProduced  += metalIncome;
	currentStats.energyProduced += energyIncome;
	currentStats.metalUsed  += metalUpkeep + metalExpense;
	currentStats.energyUsed += energyUpkeep + energyExpense;

	prevMetalPull     = metalPull;
	prevMetalIncome   = metalIncome;
	prevMetalExpense  = metalExpense;
	prevMetalUpkeep   = metalUpkeep;
	prevEnergyPull    = energyPull;
	prevEnergyIncome  = energyIncome;
	prevEnergyExpense = energyExpense;
	prevEnergyUpkeep  = energyUpkeep;

	metalPull = 0;
	metalIncome = 0;
	metalExpense = 0;
	metalUpkeep = 0;
	energyPull = 0;
	energyIncome = 0;
	energyExpense = 0;
	energyUpkeep = 0;

	metalSent = 0;
	energySent = 0;
	metalReceived = 0;
	energyReceived = 0;

	float eShare=0,mShare=0;
	for(int a=0; a < gs->activeTeams; ++a){
		if((a != teamNum) && gs->AlliedTeams(teamNum,a)){
			CTeam* team = gs->Team(a);
			eShare += max(0.0f, (team->energyStorage * 0.99f) - team->energy);
			mShare += max(0.0f, (team->metalStorage * 0.99f) - team->metal);
		}
	}

	metal += delayedMetalShare;
	energy += delayedEnergyShare;
	delayedMetalShare = 0;
	delayedEnergyShare = 0;

	const float eExcess = max(0.0f, energy - (energyStorage * energyShare));
	const float mExcess = max(0.0f, metal  - (metalStorage  * metalShare));

	float de=0,dm=0;
	if(eShare>0)
		de=min(1.0f,eExcess/eShare);
	if(mShare>0)
		dm=min(1.0f,mExcess/mShare);

	for(int a=0; a < gs->activeTeams; ++a){
		if((a != teamNum) && gs->AlliedTeams(teamNum,a)){
			CTeam* team = gs->Team(a);

			const float edif = max(0.0f, (team->energyStorage * 0.99f) - team->energy) * de;
			energy -= edif;
			energySent += edif;
			currentStats.energySent += edif;
			team->energy += edif;
			team->energyReceived += edif;
			team->currentStats.energyReceived += edif;

			const float mdif = max(0.0f, (team->metalStorage * 0.99f) - team->metal) * dm;
			metal -= mdif;
			metalSent += mdif;
			currentStats.metalSent += mdif;
			team->metal += mdif;
			team->metalReceived += mdif;
			team->currentStats.metalReceived += mdif;
		}
	}

	if (metal > metalStorage) {
		currentStats.metalExcess+=metal-metalStorage;
		metal = metalStorage;
	}
	if (energy > energyStorage) {
		currentStats.energyExcess+=energy-energyStorage;
		energy = energyStorage;
	}

	const int statsFrames = (statsPeriod * GAME_SPEED);
	if ((lastStatSave + statsFrames) < gs->frameNum) {
		lastStatSave += statsFrames;
		statHistory.push_back(currentStats);
	}

	/* Kill the player on 'com dies = game ends' games.  This can't be done in
	CTeam::CommanderDied anymore, because that function is called in
	CUnit::ChangeTeam(), hence it'd cause a random amount of the shared units
	to be killed if the commander is among them. Also, ".take" would kill all
	units once it transfered the commander. */
	if(gs->gameMode==1 && numCommanders<=0 && !gaia){
		for(list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui){
			if((*ui)->team==teamNum && !(*ui)->unitDef->isCommander)
				(*ui)->KillUnit(true,false,0);
		}
		// Set to 1 to prevent above loop from being done every update.
		numCommanders = 1;
	}
}

void CTeam::AddUnit(CUnit* unit,AddType type)
{
	units.insert(unit);
	switch (type) {
		case AddBuilt: {
			currentStats.unitsProduced++;
			break;
		}
		case AddGiven: {
			currentStats.unitsReceived++;
			break;
		}
		case AddCaptured: {
			currentStats.unitsCaptured++;
			break;
		}	
	}
	if (unit->unitDef->isCommander) {
		numCommanders++;
	}
}

void CTeam::RemoveUnit(CUnit* unit,RemoveType type)
{
	units.erase(unit);
	switch (type) {
		case RemoveDied: {
			currentStats.unitsDied++;
			break;
		}
		case RemoveGiven: {
			currentStats.unitsSent++;
			break;
		}
		case RemoveCaptured: {
			currentStats.unitsOutCaptured++;
			break;
		}
	}

	if(units.empty() && !gaia){
		logOutput.Print(CMessages::Tr("Team%i(%s) is no more").c_str(), teamNum, gs->players[leader]->playerName.c_str());
		isDead=true;
		luaCallIns.TeamDied(teamNum);
		for(int a=0;a<MAX_PLAYERS;++a){
			if(gs->players[a]->active && gs->players[a]->team==teamNum){
				gs->players[a]->spectator=true;
				if(a==gu->myPlayerNum){
					gu->spectating           = true;
					gu->spectatingFullView   = true;
					gu->spectatingFullSelect = true;
					CLuaUI::UpdateTeams();
				}
				if (gameServer)	{
					gameServer->PlayerDefeated(a);
				}
			}
		}
		if (globalAI->ais[teamNum]) {
			delete globalAI->ais[teamNum];
			globalAI->ais[teamNum] = 0;
		}
	}
}

void CTeam::CommanderDied(CUnit* commander)
{
	assert(commander->unitDef->isCommander);
	--numCommanders;
}

void CTeam::LeftLineage(CUnit* unit)
{
	if (gs->gameMode == 2 && unit->id == this->lineageRoot) {
		for(list<CUnit*>::iterator ui = uh->activeUnits.begin(); ui != uh->activeUnits.end(); ++ui) {
			if ((*ui)->lineage == this->teamNum)
				(*ui)->KillUnit(true, false, 0);
		}
	}
}
