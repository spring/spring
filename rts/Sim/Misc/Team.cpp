/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "Team.h"
#include "TeamHandler.h"

#include "GlobalSynced.h"
#include "LogOutput.h"
#include "Game/PlayerHandler.h"
#include "Game/GameSetup.h"
#include "Game/Messages.h"
#include "GlobalUnsynced.h"
#include "Game/UI/LuaUI.h"
#include "Lua/LuaRules.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDef.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "EventHandler.h"
#include "GlobalUnsynced.h"
#include "creg/STL_List.h"
#include "creg/STL_Map.h"
#include "creg/STL_Set.h"
#include "NetProtocol.h"

CR_BIND(CTeam,);
CR_REG_METADATA(CTeam, (
// from CTeamBase
				CR_MEMBER(leader),
				CR_MEMBER(color),
				CR_MEMBER(handicap),
				CR_MEMBER(side),
				CR_MEMBER(startPos),
				CR_MEMBER(teamStartNum),
				CR_MEMBER(teamAllyteam),
//				CR_MEMBER(customValues),
// from CTeam
				CR_MEMBER(teamNum),
				CR_MEMBER(isDead),
				CR_MEMBER(gaia),
				CR_MEMBER(lineageRoot),
				CR_MEMBER(origColor),
				CR_MEMBER(units),
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
				//CR_MEMBER(currentStats),
				CR_MEMBER(nextHistoryEntry),
				CR_MEMBER(numCommanders),
				//CR_MEMBER(statHistory),
				CR_MEMBER(modParams),
				CR_MEMBER(modParamsMap),
				CR_RESERVED(64)
				));


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTeam::CTeam()
: isDead(false),
  gaia(false),
  lineageRoot(-1),
  metal(0),
  energy(0),
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
  nextHistoryEntry(0),
  numCommanders(0),
  highlight(0.0f)
{
	statHistory.push_back(TeamStatistics());
	currentStats = &statHistory.back();
}


CTeam::~CTeam()
{
}


bool CTeam::UseMetal(float amount)
{
	if ((metal - (prevMetalUpkeep * 10)) >= amount) {
		metal -= amount;
		metalExpense += amount;
		return true;
	}
	return false;
}


bool CTeam::UseEnergy(float amount)
{
	if ((energy - (prevEnergyUpkeep * 10)) >= amount) {
		energy -= amount;
		energyExpense += amount;
		return true;
	}
	return false;
}


bool CTeam::UseMetalUpkeep(float amount)
{
	if (metal >= amount) {
		metal -= amount;
		metalExpense += amount;
		metalUpkeep += amount;
		return true;
	}
	return false;
}


bool CTeam::UseEnergyUpkeep(float amount)
{
	if (energy >= amount) {
		energy -= amount;
		energyExpense += amount;
		energyUpkeep += amount;
		return true;
	}
	return false;
}


void CTeam::AddMetal(float amount, bool hc)
{
	if (hc) { amount *= handicap; }
	metal += amount;
	metalIncome += amount;
	if (metal > metalStorage) {
		delayedMetalShare += metal - metalStorage;
		metal = metalStorage;
	}
}


void CTeam::AddEnergy(float amount, bool hc)
{
	if (hc) { amount *= handicap; }
	energy += amount;
	energyIncome += amount;
	if (energy > energyStorage) {
		delayedEnergyShare += energy - energyStorage;
		energy = energyStorage;
	}
}

void CTeam::GiveEverythingTo(const unsigned toTeam)
{
	CTeam* target = teamHandler->Team(toTeam);

	if (!target) {
		logOutput.Print("Team %i does not exist, can't give units", toTeam);
		return;
	}

	if (!luaRules || luaRules->AllowResourceTransfer(teamNum, toTeam, "m", metal)) {
		target->metal += metal;
		metal = 0;
	}
	if (!luaRules || luaRules->AllowResourceTransfer(teamNum, toTeam, "e", energy)) {
		target->energy += energy;
		energy = 0;
	}

	for (CUnitSet::iterator ui = units.begin(); ui != units.end(); ) {
		// must pass the normal checks, isDead, unit count restrictions, luaRules, etc...
		CUnitSet::iterator next = ui; ++next;
		(*ui)->ChangeTeam(toTeam, CUnit::ChangeGiven);
		ui = next;
	}

	Died();
}


void CTeam::Died()
{
	if (isDead)
		return;

	if (leader >= 0) {
		logOutput.Print(CMessages::Tr("Team%i(%s) is no more").c_str(),
		                teamNum, playerHandler->Player(leader)->name.c_str());
	} else {
		logOutput.Print(CMessages::Tr("Team%i is no more").c_str(), teamNum);
	}
	isDead = true;

	CSkirmishAIHandler::ids_t localTeamAIs = skirmishAIHandler.GetSkirmishAIsInTeam(teamNum, gu->myPlayerNum);
	for (CSkirmishAIHandler::ids_t::const_iterator ai = localTeamAIs.begin(); ai != localTeamAIs.end(); ++ai) {
		skirmishAIHandler.SetLocalSkirmishAIDieing(*ai, 2 /* = team died */);
	}

	// this message is not relayed to clients, it's only for the server
	net->Send(CBaseNetProtocol::Get().SendTeamDied(gu->myPlayerNum, teamNum));

	for (int a = 0; a < playerHandler->ActivePlayers(); ++a) {
		if (playerHandler->Player(a)->team == teamNum) {
			playerHandler->Player(a)->StartSpectating();
		}
	}

	CLuaUI::UpdateTeams();
	CPlayer::UpdateControlledTeams();
	eventHandler.TeamDied(teamNum);
}

void CTeam::StartposMessage(const float3& pos)
{
	startPos = pos;
}

CTeam& CTeam::operator=(const TeamBase& base)
{
	TeamBase::operator=(base);
	return *this;
}

void CTeam::ResetFrameVariables()
{
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
}

void CTeam::SlowUpdate()
{
	currentStats->metalProduced  += prevMetalIncome;
	currentStats->energyProduced += prevEnergyIncome;
	currentStats->metalUsed  += prevMetalUpkeep + prevMetalExpense;
	currentStats->energyUsed += prevEnergyUpkeep + prevEnergyExpense;

	float eShare = 0.0f, mShare = 0.0f;
	for (int a = 0; a < teamHandler->ActiveTeams(); ++a) {
		if ((a != teamNum) && (teamHandler->AllyTeam(teamNum) == teamHandler->AllyTeam(a))) {
			CTeam* team = teamHandler->Team(a);
			if (team->isDead)
				continue;

			eShare += std::max(0.0f, (team->energyStorage * 0.99f) - team->energy);
			mShare += std::max(0.0f, (team->metalStorage  * 0.99f) - team->metal);
		}
	}

	metal += delayedMetalShare;
	energy += delayedEnergyShare;
	delayedMetalShare = 0;
	delayedEnergyShare = 0;

	const float eExcess = std::max(0.0f, energy - (energyStorage * energyShare));
	const float mExcess = std::max(0.0f, metal  - (metalStorage  * metalShare));

	float de = 0.0f, dm = 0.0f;
	if (eShare > 0.0f) {
		de = std::min(1.0f, eExcess/eShare);
	}
	if (mShare > 0.0f) {
		dm = std::min(1.0f, mExcess/mShare);
	}
	for (int a = 0; a < teamHandler->ActiveTeams(); ++a) {
		if ((a != teamNum) && (teamHandler->AllyTeam(teamNum) == teamHandler->AllyTeam(a))) {
			CTeam* team = teamHandler->Team(a);
			if (team->isDead)
				continue;

			const float edif = std::max(0.0f, (team->energyStorage * 0.99f) - team->energy) * de;
			energy -= edif;
			energySent += edif;
			currentStats->energySent += edif;
			team->energy += edif;
			team->energyReceived += edif;
			team->currentStats->energyReceived += edif;

			const float mdif = std::max(0.0f, (team->metalStorage * 0.99f) - team->metal) * dm;
			metal -= mdif;
			metalSent += mdif;
			currentStats->metalSent += mdif;
			team->metal += mdif;
			team->metalReceived += mdif;
			team->currentStats->metalReceived += mdif;
		}
	}

	if (metal > metalStorage) {
		currentStats->metalExcess += (metal - metalStorage);
		metal = metalStorage;
	}
	if (energy > energyStorage) {
		currentStats->energyExcess += (energy - energyStorage);
		energy = energyStorage;
	}

	//! make sure the stats update is always in a SlowUpdate
	assert(((TeamStatistics::statsPeriod * GAME_SPEED) % TEAM_SLOWUPDATE_RATE) == 0);

	const int statsFrames = TeamStatistics::statsPeriod * GAME_SPEED;
	if (nextHistoryEntry <= gs->frameNum) {
		currentStats->frame = gs->frameNum;
		statHistory.push_back(*currentStats);
		currentStats = &statHistory.back();

		nextHistoryEntry = gs->frameNum + statsFrames;
		currentStats->frame = nextHistoryEntry;
	}

	/* Kill the player on 'com dies = game ends' games.  This can't be done in
	CTeam::CommanderDied anymore, because that function is called in
	CUnit::ChangeTeam(), hence it'd cause a random amount of the shared units
	to be killed if the commander is among them. Also, ".take" would kill all
	units once it transfered the commander. */
	if (gameSetup->gameMode == GameMode::ComEnd && numCommanders<=0 && !gaia){
		for(std::list<CUnit*>::iterator ui=uh->activeUnits.begin();ui!=uh->activeUnits.end();++ui){
			if ((*ui)->team==teamNum && !(*ui)->unitDef->isCommander) {
				(*ui)->KillUnit(true, false, NULL);
			}
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
			currentStats->unitsProduced++;
			break;
		}
		case AddGiven: {
			currentStats->unitsReceived++;
			break;
		}
		case AddCaptured: {
			currentStats->unitsCaptured++;
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
			currentStats->unitsDied++;
			break;
		}
		case RemoveGiven: {
			currentStats->unitsSent++;
			break;
		}
		case RemoveCaptured: {
			currentStats->unitsOutCaptured++;
			break;
		}
	}

	if (units.empty() && !gaia) {
		Died();
	}
}


void CTeam::CommanderDied(CUnit* commander)
{
	assert(commander->unitDef->isCommander);
	--numCommanders;
}


void CTeam::LeftLineage(CUnit* unit)
{
	if (gameSetup->gameMode == GameMode::Lineage && unit->id == this->lineageRoot) {
		for (std::list<CUnit*>::iterator ui = uh->activeUnits.begin(); ui != uh->activeUnits.end(); ++ui) {
			if ((*ui)->lineage == this->teamNum) {
				(*ui)->KillUnit(true, false, NULL);
			}
		}
	}
}



std::string CTeam::GetControllerName() const {
	std::string s;

	// "Joe, AI: ABCAI 0.1 (nick: Killer), AI: DEFAI 1.2 (nick: Slayer), ..."
	if (this->leader >= 0) {
		const CPlayer* leadPlayer = playerHandler->Player(this->leader);

		if (leadPlayer->team != this->teamNum) {
			const CTeam*   realLeadPlayerTeam = teamHandler->Team(leadPlayer->team);
			const CPlayer* realLeadPlayer     = NULL;

			if (realLeadPlayerTeam->leader >= 0) {
				realLeadPlayer = playerHandler->Player(realLeadPlayerTeam->leader);
				s = realLeadPlayer->name;
			} else {
				s = "N/A"; // weird
			}
		} else {
			s = leadPlayer->name;
		}

		const CSkirmishAIHandler::ids_t& teamAIs = skirmishAIHandler.GetSkirmishAIsInTeam(this->teamNum);
		const int numTeamAIs = teamAIs.size();

		if (numTeamAIs > 0) {
			s += ", ";
		}

		int i = 0;
		for (CSkirmishAIHandler::ids_t::const_iterator ai = teamAIs.begin(); ai != teamAIs.end(); ++ai) {
			const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(*ai);
			const std::string prefix = "AI: " + aiData->shortName + " " + aiData->version + " ";
			const std::string pstfix = "(nick: " + aiData->name + ")";

			s += (prefix + pstfix);
			i += 1;

			if (i < numTeamAIs) {
				s += ", ";
			}
		}
	} else {
		s = "Uncontrolled";
	}

	return s;
}
