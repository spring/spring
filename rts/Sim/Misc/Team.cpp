/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Team.h"

#include "TeamHandler.h"
#include "GlobalSynced.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Map/ReadMap.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/MsgStrings.h"
#include "System/Rectangle.h"
#include "System/creg/STL_List.h"
#include "System/creg/STL_Map.h"
#include "System/creg/STL_Set.h"


CR_BIND_DERIVED(CTeam, TeamBase, (-1))
CR_REG_METADATA(CTeam, (
	CR_MEMBER(teamNum),
	CR_MEMBER(maxUnits),
	CR_MEMBER(isDead),
	CR_MEMBER(gaia),
	CR_MEMBER(origColor),
	CR_MEMBER(units),
	CR_MEMBER(res),
	CR_MEMBER(resStorage),
	CR_MEMBER(resPull),
	CR_MEMBER(resPrevPull),
	CR_MEMBER(resIncome),
	CR_MEMBER(resPrevIncome),
	CR_MEMBER(resExpense),
	CR_MEMBER(resPrevExpense),
	CR_MEMBER(resShare),
	CR_MEMBER(resDelayedShare),
	CR_MEMBER(resSent),
	CR_MEMBER(resPrevSent),
	CR_MEMBER(resReceived),
	CR_MEMBER(resPrevReceived),
	CR_MEMBER(resPrevExcess),
	CR_MEMBER(nextHistoryEntry),
	CR_MEMBER(statHistory),
	CR_MEMBER(currentStats),
	CR_MEMBER(modParams),
	CR_MEMBER(modParamsMap),
	CR_IGNORED(highlight)
))


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTeam::CTeam(int _teamNum):
	teamNum(_teamNum),
	maxUnits(0),
	isDead(false),
	gaia(false),
	resStorage(1000000, 1000000),
	resShare(0.99f, 0.95f),
	nextHistoryEntry(0),
	origColor(0, 0, 0, 0),
	highlight(0.0f)
{
	statHistory.push_back(TeamStatistics());
	currentStats = &statHistory.back();
}

void CTeam::SetDefaultStartPos()
{
	const int allyTeam = teamHandler->AllyTeam(teamNum);
	const std::vector<AllyTeam>& allyStartData = CGameSetup::GetAllyStartingData();

	assert(!allyStartData.empty());
	assert(allyTeam == teamAllyteam);

	const AllyTeam& allyTeamData = allyStartData[allyTeam];
	// pick a spot near the center of our startbox
	const float xmin = (mapDims.mapx * SQUARE_SIZE) * allyTeamData.startRectLeft;
	const float zmin = (mapDims.mapy * SQUARE_SIZE) * allyTeamData.startRectTop;
	const float xmax = (mapDims.mapx * SQUARE_SIZE) * allyTeamData.startRectRight;
	const float zmax = (mapDims.mapy * SQUARE_SIZE) * allyTeamData.startRectBottom;
	const float xcenter = (xmin + xmax) * 0.5f;
	const float zcenter = (zmin + zmax) * 0.5f;

	assert(xcenter >= 0 && xcenter < mapDims.mapx * SQUARE_SIZE);
	assert(zcenter >= 0 && zcenter < mapDims.mapy * SQUARE_SIZE);

	startPos.x = (teamNum - teamHandler->ActiveTeams()) * 4 * SQUARE_SIZE + xcenter;
	startPos.z = (teamNum - teamHandler->ActiveTeams()) * 4 * SQUARE_SIZE + zcenter;
}

void CTeam::ClampStartPosInStartBox(float3* pos) const
{
	const int allyTeam = teamHandler->AllyTeam(teamNum);
	const std::vector<AllyTeam>& allyStartData = CGameSetup::GetAllyStartingData();
	const AllyTeam& allyTeamData = allyStartData[allyTeam];
	const SRectangle rect(
		allyTeamData.startRectLeft   * mapDims.mapx * SQUARE_SIZE,
		allyTeamData.startRectTop    * mapDims.mapy * SQUARE_SIZE,
		allyTeamData.startRectRight  * mapDims.mapx * SQUARE_SIZE,
		allyTeamData.startRectBottom * mapDims.mapy * SQUARE_SIZE
	);

	int2 ipos(pos->x, pos->z);
	rect.ClampPos(&ipos);
	pos->x = ipos.x;
	pos->z = ipos.y;
}


bool CTeam::UseMetal(float amount)
{
	if (res.metal >= amount) {
		res.metal -= amount;
		resExpense.metal += amount;
		return true;
	}
	return false;
}

bool CTeam::UseEnergy(float amount)
{
	if (res.energy >= amount) {
		res.energy -= amount;
		resExpense.energy += amount;
		return true;
	}
	return false;
}



void CTeam::AddMetal(float amount, bool useIncomeMultiplier)
{
	if (useIncomeMultiplier) { amount *= GetIncomeMultiplier(); }
	res.metal += amount;
	resIncome.metal += amount;
	if (res.metal > resStorage.metal) {
		resDelayedShare.metal += (res.metal - resStorage.metal);
		res.metal = resStorage.metal;
	}
}

void CTeam::AddEnergy(float amount, bool useIncomeMultiplier)
{
	if (useIncomeMultiplier) { amount *= GetIncomeMultiplier(); }
	res.energy += amount;
	resIncome.energy += amount;
	if (res.energy > resStorage.energy) {
		resDelayedShare.energy += (res.energy - resStorage.energy);
		res.energy = resStorage.energy;
	}
}


void CTeam::AddResources(SResourcePack amount, bool useIncomeMultiplier)
{
	if (useIncomeMultiplier) { amount *= GetIncomeMultiplier(); }
	res += amount;
	resIncome += amount;
	for (int i = 0; i < SResourcePack::MAX_RESOURCES; ++i) {
		if (res[i] > resStorage[i]) {
			resDelayedShare[i] += (res[i] - resStorage[i]);
			res[i] = resStorage[i];
		}
	}
}

bool CTeam::UseResources(const SResourcePack& amount)
{
	if (res >= amount) {
		res -= amount;
		resExpense += amount;
		return true;
	}
	return false;
}


void CTeam::GiveEverythingTo(const unsigned toTeam)
{
	CTeam* target = teamHandler->Team(toTeam);

	if (!target) {
		LOG_L(L_WARNING, "Team %i does not exist, can't give units", toTeam);
		return;
	}

	if (eventHandler.AllowResourceTransfer(teamNum, toTeam, "m", res.metal)) {
		target->res.metal += res.metal;
		res.metal = 0;
	}
	if (eventHandler.AllowResourceTransfer(teamNum, toTeam, "e", res.energy)) {
		target->res.energy += res.energy;
		res.energy = 0;
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
		// this message is not relayed to clients, it's only for the server
		clientNet->Send(CBaseNetProtocol::Get().SendTeamDied(gu->myPlayerNum, teamNum));
		// if not a normal death, no need (or use) to send AI state changes
		KillAIs();
	}

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

	if (!HasLeader()) {
		SetLeader(playerNum);
	}

	playerHandler->Player(playerNum)->JoinTeam(teamNum);
	playerHandler->Player(playerNum)->SetControlledTeams();

	isDead = false;
}

void CTeam::KillAIs()
{
	for (const auto& id: skirmishAIHandler.GetSkirmishAIsInTeam(teamNum, gu->myPlayerNum)) {
		skirmishAIHandler.SetLocalSkirmishAIDieing(id, 2 /* = team died */);
	}
}



void CTeam::ResetResourceState()
{
	// reset all state variables that were
	// potentially modified during the last
	// <TEAM_SLOWUPDATE_RATE> frames
	resPrevPull.metal     = resPull.metal;     resPull.metal     = 0.0f;
	resPrevIncome.metal   = resIncome.metal;   resIncome.metal   = 0.0f;
	resPrevExpense.metal  = resExpense.metal;  resExpense.metal  = 0.0f;
	resPrevPull.energy    = resPull.energy;    resPull.energy    = 0.0f;
	resPrevIncome.energy  = resIncome.energy;  resIncome.energy  = 0.0f;
	resPrevExpense.energy = resExpense.energy; resExpense.energy = 0.0f;

	// reset the sharing accumulators
	resPrevSent.metal = resSent.metal; resSent.metal = 0.0f;
	resPrevReceived.metal = resReceived.metal; resReceived.metal = 0.0f;
	resPrevSent.energy = resSent.energy; resSent.energy = 0.0f;
	resPrevReceived.energy = resReceived.energy; resReceived.energy = 0.0f;
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

			eShare += std::max(0.0f, (team->resStorage.energy * 0.99f) - team->res.energy);
			mShare += std::max(0.0f, (team->resStorage.metal  * 0.99f) - team->res.metal);
		}
	}


	currentStats->metalProduced  += resPrevIncome.metal;
	currentStats->energyProduced += resPrevIncome.energy;
	currentStats->metalUsed  += resPrevExpense.metal;
	currentStats->energyUsed += resPrevExpense.energy;

	res.metal  += resDelayedShare.metal;  resDelayedShare.metal  = 0.0f;
	res.energy += resDelayedShare.energy; resDelayedShare.energy = 0.0f;


	// calculate how much we can share in total (any and all excess resources)
	const float eExcess = std::max(0.0f, res.energy - (resStorage.energy * resShare.energy));
	const float mExcess = std::max(0.0f, res.metal  - (resStorage.metal  * resShare.metal));

	float de = 0.0f, dm = 0.0f;
	if (eShare > 0.0f) { de = std::min(1.0f, eExcess / eShare); }
	if (mShare > 0.0f) { dm = std::min(1.0f, mExcess / mShare); }

	// now evenly distribute our excess resources among allied teams
	for (int a = 0; a < teamHandler->ActiveTeams(); ++a) {
		if ((a != teamNum) && (teamHandler->AllyTeam(teamNum) == teamHandler->AllyTeam(a))) {
			CTeam* team = teamHandler->Team(a);
			if (team->isDead)
				continue;

			const float edif = std::max(0.0f, (team->resStorage.energy * 0.99f) - team->res.energy) * de;
			const float mdif = std::max(0.0f, (team->resStorage.metal * 0.99f) - team->res.metal) * dm;

			res.energy     -= edif; team->res.energy         += edif;
			resSent.energy += edif; team->resReceived.energy += edif;
			res.metal      -= mdif; team->res.metal          += mdif;
			resSent.metal  += mdif; team->resReceived.metal  += mdif;

			currentStats->energySent += edif; team->currentStats->energyReceived += edif;
			currentStats->metalSent  += mdif; team->currentStats->metalReceived  += mdif;
		}
	}

	// clamp resource levels to storage capacity
	if (res.metal > resStorage.metal) {
		resPrevExcess.metal = (res.metal - resStorage.metal);
		currentStats->metalExcess += resPrevExcess.metal;
		res.metal = resStorage.metal;
	} else {
		resPrevExcess.metal = 0;
	}
	if (res.energy > resStorage.energy) {
		resPrevExcess.energy = (res.energy - resStorage.energy);
		currentStats->energyExcess += resPrevExcess.energy;
		res.energy = resStorage.energy;
	} else {
		resPrevExcess.energy = 0;
	}

	// make sure the stats update is always in a SlowUpdate
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
	if (HasLeader()) {
		const CPlayer* leadPlayer = playerHandler->Player(leader);

		if (leadPlayer->team != this->teamNum) {
			const CTeam*   realLeadPlayerTeam = teamHandler->Team(leadPlayer->team);
			const CPlayer* realLeadPlayer     = NULL;

			if (realLeadPlayerTeam->HasLeader()) {
				realLeadPlayer = playerHandler->Player(realLeadPlayerTeam->GetLeader());
				s = realLeadPlayer->name;
			} else {
				s = "N/A"; // weird
			}
		} else {
			s = leadPlayer->name;
		}

		for (const auto& aiId: skirmishAIHandler.GetSkirmishAIsInTeam(this->teamNum)) {
			const SkirmishAIData* aiData = skirmishAIHandler.GetSkirmishAI(aiId);
			const std::string prefix = "AI: " + aiData->shortName + " " + aiData->version + " ";
			const std::string pstfix = "(nick: " + aiData->name + ")";
			s += ", " + prefix + pstfix;
		}
	} else {
		s = UncontrolledPlayerName;
	}

	return s;
}
