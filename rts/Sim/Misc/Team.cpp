/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Team.h"

#include "TeamHandler.h"
#include "GlobalSynced.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/Player.h"
#include "Game/PlayerHandler.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Messages.h"
#include "Lua/LuaRules.h"
#include "Lua/LuaUI.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDef.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/NetProtocol.h"
#include "System/MsgStrings.h"
#include "System/Rectangle.h"
#include "System/creg/STL_List.h"
#include "System/creg/STL_Map.h"
#include "System/creg/STL_Set.h"

CR_BIND_DERIVED(CTeam, TeamBase, (-1));
CR_REG_METADATA(CTeam, (
	CR_MEMBER(teamNum),
	CR_MEMBER(maxUnits),
	CR_MEMBER(isDead),
	CR_MEMBER(gaia),
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
	CR_MEMBER(energyPull),
	CR_MEMBER(prevEnergyPull),
	CR_MEMBER(energyIncome),
	CR_MEMBER(prevEnergyIncome),
	CR_MEMBER(energyExpense),
	CR_MEMBER(prevEnergyExpense),
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
	CR_MEMBER(prevMetalSent),
	CR_MEMBER(prevMetalReceived),
	CR_MEMBER(prevMetalExcess),
	CR_MEMBER(prevEnergySent),
	CR_MEMBER(prevEnergyReceived),
	CR_MEMBER(prevEnergyExcess),
	CR_MEMBER(nextHistoryEntry),
	CR_MEMBER(statHistory),
	CR_MEMBER(currentStats),
	CR_MEMBER(modParams),
	CR_MEMBER(modParamsMap),
	CR_IGNORED(highlight)
));


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTeam::CTeam(int _teamNum):
	teamNum(_teamNum),
	maxUnits(0),
	isDead(false),
	gaia(false),
	metal(0.0f),
	energy(0.0f),
	metalPull(0.0f),     prevMetalPull(0.0f),
	metalIncome(0.0f),   prevMetalIncome(0.0f),
	metalExpense(0.0f),  prevMetalExpense(0.0f),
	energyPull(0.0f),    prevEnergyPull(0.0f),
	energyIncome(0.0f),  prevEnergyIncome(0.0f),
	energyExpense(0.0f), prevEnergyExpense(0.0f),
	metalStorage(1000000),
	energyStorage(1000000),
	metalShare(0.99f),
	energyShare(0.95f),
	delayedMetalShare(0.0f),
	delayedEnergyShare(0.0f),
	metalSent(0.0f),      prevMetalSent(0.0f),
	metalReceived(0.0f),  prevMetalReceived(0.0f),
	energySent(0.0f),     prevEnergySent(0.0f),
	energyReceived(0.0f), prevEnergyReceived(0.0f),
	prevMetalExcess(0.0f),
	prevEnergyExcess(0.0f),
	nextHistoryEntry(0),
	highlight(0.0f)
{
	origColor[0] = 0;
	origColor[1] = 0;
	origColor[2] = 0;
	origColor[3] = 0;

	statHistory.push_back(TeamStatistics());
	currentStats = &statHistory.back();
}


void CTeam::ClampStartPosInStartBox(float3* pos) const
{
	const int allyTeam = teamHandler->AllyTeam(teamNum);
	if (allyTeam < 0 || allyTeam >= gameSetup->allyStartingData.size()) {
		LOG_L(L_ERROR, "%s: invalid AllyStartingData (team %d)", __FUNCTION__, teamNum);
		return;
	}
	const AllyTeam& at = gameSetup->allyStartingData[allyTeam];
	const SRectangle rect(
		at.startRectLeft   * gs->mapx * SQUARE_SIZE,
		at.startRectTop    * gs->mapy * SQUARE_SIZE,
		at.startRectRight  * gs->mapx * SQUARE_SIZE,
		at.startRectBottom * gs->mapy * SQUARE_SIZE
	);

	int2 ipos(pos->x, pos->z);
	rect.ClampPos(&ipos);
	pos->x = ipos.x;
	pos->z = ipos.y;
}


bool CTeam::UseMetal(float amount)
{
	if (metal >= amount) {
		metal -= amount;
		metalExpense += amount;
		return true;
	}
	return false;
}

bool CTeam::UseEnergy(float amount)
{
	if (energy >= amount) {
		energy -= amount;
		energyExpense += amount;
		return true;
	}
	return false;
}



void CTeam::AddMetal(float amount, bool useIncomeMultiplier)
{
	if (useIncomeMultiplier) { amount *= GetIncomeMultiplier(); }
	metal += amount;
	metalIncome += amount;
	if (metal > metalStorage) {
		delayedMetalShare += (metal - metalStorage);
		metal = metalStorage;
	}
}

void CTeam::AddEnergy(float amount, bool useIncomeMultiplier)
{
	if (useIncomeMultiplier) { amount *= GetIncomeMultiplier(); }
	energy += amount;
	energyIncome += amount;
	if (energy > energyStorage) {
		delayedEnergyShare += (energy - energyStorage);
		energy = energyStorage;
	}
}



void CTeam::GiveEverythingTo(const unsigned toTeam)
{
	CTeam* target = teamHandler->Team(toTeam);

	if (!target) {
		LOG_L(L_WARNING, "Team %i does not exist, can't give units", toTeam);
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
}


void CTeam::Died(bool normalDeath)
{
	if (isDead)
		return;

	if (normalDeath) {
		if (leader >= 0) {
			const CPlayer* leadPlayer = playerHandler->Player(leader);
			const char* leaderName = leadPlayer->name.c_str();
			LOG(CMessages::Tr("Team %i (lead by %s) is no more").c_str(), teamNum, leaderName);
		} else {
			LOG(CMessages::Tr("Team %i is no more").c_str(), teamNum);
		}

		// this message is not relayed to clients, it's only for the server
		net->Send(CBaseNetProtocol::Get().SendTeamDied(gu->myPlayerNum, teamNum));
	}

	KillAIs();

	// demote all players in _this_ team to spectators
	for (int a = 0; a < playerHandler->ActivePlayers(); ++a) {
		if (playerHandler->Player(a)->team == teamNum) {
			playerHandler->Player(a)->StartSpectating();
			playerHandler->Player(a)->SetControlledTeams();
		}
	}

	// increase per-team unit-limit for each remaining team in _our_ allyteam
	teamHandler->UpdateTeamUnitLimitsPreDeath(teamNum);
	eventHandler.TeamDied(teamNum);

	isDead = true;
}

void CTeam::AddPlayer(int playerNum)
{
	// note: does it matter if this team was already dead?
	// (besides needing to restore its original unit-limit)
	if (isDead) {
		teamHandler->UpdateTeamUnitLimitsPreSpawn(teamNum);
	}

	if (leader == -1) {
		leader = playerNum;
	}

	playerHandler->Player(playerNum)->JoinTeam(teamNum);
	playerHandler->Player(playerNum)->SetControlledTeams();

	isDead = false;
}

void CTeam::KillAIs()
{
	const CSkirmishAIHandler::ids_t& localTeamAIs = skirmishAIHandler.GetSkirmishAIsInTeam(teamNum, gu->myPlayerNum);

	for (CSkirmishAIHandler::ids_t::const_iterator ai = localTeamAIs.begin(); ai != localTeamAIs.end(); ++ai) {
		skirmishAIHandler.SetLocalSkirmishAIDieing(*ai, 2 /* = team died */);
	}
}



void CTeam::ResetResourceState()
{
	// reset all state variables that were
	// potentially modified during the last
	// <TEAM_SLOWUPDATE_RATE> frames
	prevMetalPull     = metalPull;     metalPull     = 0.0f;
	prevMetalIncome   = metalIncome;   metalIncome   = 0.0f;
	prevMetalExpense  = metalExpense;  metalExpense  = 0.0f;
	prevEnergyPull    = energyPull;    energyPull    = 0.0f;
	prevEnergyIncome  = energyIncome;  energyIncome  = 0.0f;
	prevEnergyExpense = energyExpense; energyExpense = 0.0f;

	// reset the sharing accumulators
	prevMetalSent = metalSent; metalSent = 0.0f;
	prevMetalReceived = metalReceived; metalReceived = 0.0f;
	prevEnergySent = energySent; energySent = 0.0f;
	prevEnergyReceived = energyReceived; energyReceived = 0.0f;
}

void CTeam::SlowUpdate()
{
	float eShare = 0.0f, mShare = 0.0f;

	// calculate the total amount of resources that all
	// (allied) teams can collectively receive through
	// sharing
	for (int a = 0; a < teamHandler->ActiveTeams(); ++a) {
		CTeam* team = teamHandler->Team(a);

		if ((a != teamNum) && (teamHandler->AllyTeam(teamNum) == teamHandler->AllyTeam(a))) {
			if (team->isDead)
				continue;

			eShare += std::max(0.0f, (team->energyStorage * 0.99f) - team->energy);
			mShare += std::max(0.0f, (team->metalStorage  * 0.99f) - team->metal);
		}
	}


	currentStats->metalProduced  += prevMetalIncome;
	currentStats->energyProduced += prevEnergyIncome;
	currentStats->metalUsed  += prevMetalExpense;
	currentStats->energyUsed += prevEnergyExpense;

	metal  += delayedMetalShare;  delayedMetalShare  = 0.0f;
	energy += delayedEnergyShare; delayedEnergyShare = 0.0f;


	// calculate how much we can share in total (any and all excess resources)
	const float eExcess = std::max(0.0f, energy - (energyStorage * energyShare));
	const float mExcess = std::max(0.0f, metal  - (metalStorage  * metalShare));

	float de = 0.0f, dm = 0.0f;
	if (eShare > 0.0f) { de = std::min(1.0f, eExcess / eShare); }
	if (mShare > 0.0f) { dm = std::min(1.0f, mExcess / mShare); }

	// now evenly distribute our excess resources among allied teams
	for (int a = 0; a < teamHandler->ActiveTeams(); ++a) {
		if ((a != teamNum) && (teamHandler->AllyTeam(teamNum) == teamHandler->AllyTeam(a))) {
			CTeam* team = teamHandler->Team(a);
			if (team->isDead)
				continue;

			const float edif = std::max(0.0f, (team->energyStorage * 0.99f) - team->energy) * de;
			const float mdif = std::max(0.0f, (team->metalStorage * 0.99f) - team->metal) * dm;

			energy     -= edif; team->energy         += edif;
			energySent += edif; team->energyReceived += edif;
			metal      -= mdif; team->metal          += mdif;
			metalSent  += mdif; team->metalReceived  += mdif;

			currentStats->energySent += edif; team->currentStats->energyReceived += edif;
			currentStats->metalSent  += mdif; team->currentStats->metalReceived  += mdif;
		}
	}

	// clamp resource levels to storage capacity
	if (metal > metalStorage) {
		prevMetalExcess = (metal - metalStorage);
		currentStats->metalExcess += prevMetalExcess;
		metal = metalStorage;
	} else {
		prevMetalExcess = 0;
	}
	if (energy > energyStorage) {
		prevEnergyExcess = (energy - energyStorage);
		currentStats->energyExcess += prevEnergyExcess;
		energy = energyStorage;
	} else {
		prevEnergyExcess = 0;
	}

	//! make sure the stats update is always in a SlowUpdate
	assert(((TeamStatistics::statsPeriod * GAME_SPEED) % TEAM_SLOWUPDATE_RATE) == 0);

	if (nextHistoryEntry <= gs->frameNum) {
		currentStats->frame = gs->frameNum;
		statHistory.push_back(*currentStats);
		currentStats = &statHistory.back();

		nextHistoryEntry = gs->frameNum + (TeamStatistics::statsPeriod * GAME_SPEED);
		currentStats->frame = nextHistoryEntry;
	}
}


void CTeam::AddUnit(CUnit* unit, AddType type)
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
}


void CTeam::RemoveUnit(CUnit* unit, RemoveType type)
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
		s = UncontrolledPlayerName;
	}

	return s;
}
