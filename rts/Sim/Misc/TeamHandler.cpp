/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TeamHandler.h"

#include <cstring>

#include "Game/GameSetup.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"


CR_BIND(CTeamHandler, )

CR_REG_METADATA(CTeamHandler, (
	CR_MEMBER(gaiaTeamID),
	CR_MEMBER(gaiaAllyTeamID),
	CR_MEMBER(teams),
	CR_MEMBER(allyTeams)
))


// needs to be available as early as PreGame
CTeamHandler teamHandler;


void CTeamHandler::LoadFromSetup(const CGameSetup* setup)
{
	// must start from a blank slate
	assert(teams.empty());
	assert(allyTeams.empty());

	assert(!setup->GetTeamStartingDataCont().empty());
	assert(setup->GetTeamStartingDataCont().size() <= MAX_TEAMS);
	assert(setup->GetAllyStartingDataCont().size() <= MAX_TEAMS);

	teams.reserve(setup->GetTeamStartingDataCont().size() + 1); // +1 for Gaia
	teams.resize(setup->GetTeamStartingDataCont().size());

	allyTeams = setup->GetAllyStartingDataCont();

	const int numTeams = teams.size() + int(gs->useLuaGaia);
	const int maxUnitsPerTeam = std::min(setup->maxUnitsPerTeam, int(MAX_UNITS / numTeams));

	for (size_t i = 0; i < teams.size(); ++i) {
		teams[i].teamNum = i;
		teams[i] = setup->GetTeamStartingDataCont()[i];

		// all non-Gaia teams (within one allyteam) get and maintain the same unit-limit
		// (because of this it would be better treated as a pool owned by class AllyTeam)
		teams[i].SetMaxUnits(maxUnitsPerTeam);

		assert(teams[i].teamAllyteam >=                0);
		assert(teams[i].teamAllyteam <  allyTeams.size());
	}

	if (gs->useLuaGaia) {
		// Gaia adjustments
		gaiaTeamID = static_cast<int>(teams.size());
		gaiaAllyTeamID = static_cast<int>(allyTeams.size());

		// setup the Gaia team
		teams.emplace_back();
		CTeam& gaia = teams.back();

		gaia.teamNum = gaiaTeamID;
		gaia.color[0] = 255;
		gaia.color[1] = 255;
		gaia.color[2] = 255;
		gaia.color[3] = 255;
		gaia.gaia = true;
		gaia.SetMaxUnits(MAX_UNITS - ((teams.size() - 1) * teams[0].GetMaxUnits()));
		gaia.SetStartPos(ZeroVector);
		gaia.teamAllyteam = gaiaAllyTeamID;

		assert((((teams.size() - 1) * teams[0].GetMaxUnits()) + gaia.GetMaxUnits()) == MAX_UNITS);

		// make every existing AT an enemy of Gaia
		for (::AllyTeam& allyTeam: allyTeams) {
			allyTeam.allies.push_back(false);
		}


		allyTeams.emplace_back();
		::AllyTeam& allyteam = allyTeams.back();

		allyteam.allies.resize(allyTeams.size() + 1, false); // make Gaia every AT's enemy
		allyteam.allies[gaiaAllyTeamID] = true; // set Gaia to be at peace with itself
	}
}

void CTeamHandler::SetDefaultStartPositions(const CGameSetup* setup)
{
	if (setup->startPosType != CGameSetup::StartPos_ChooseInGame)
		return;

	for (int a = 0; a < ActiveTeams(); ++a) {
		CTeam& team = teams[a];

		if (team.gaia)
			continue;
		if (team.HasValidStartPos())
			continue;

		// if the player did not choose a start position (eg. if
		// the game was force-started by the host before sending
		// any), silently generate one for him
		// TODO: notify Lua of this also?
		team.SetDefaultStartPos();
	}
}

void CTeamHandler::GameFrame(int frameNum)
{
	if ((frameNum % TEAM_SLOWUPDATE_RATE) != 0)
		return;

	for (int a = 0; a < ActiveTeams(); ++a) {
		teams[a].ResetResourceState();
	}
	for (int a = 0; a < ActiveTeams(); ++a) {
		teams[a].SlowUpdate();
	}
}



unsigned int CTeamHandler::GetNumTeamsInAllyTeam(unsigned int allyTeamNum, bool countDeadTeams) const
{
	unsigned int numTeams = 0;

	for (unsigned int teamNum = 0; teamNum < teamHandler.ActiveTeams(); teamNum++) {
		if (AllyTeam(teamNum) != allyTeamNum)
			continue;
		if (teams[teamNum].isDead && !countDeadTeams)
			continue;

		numTeams += 1;
	}

	return numTeams;
}

void CTeamHandler::UpdateTeamUnitLimitsPreSpawn(int liveTeamNum)
{
	CTeam* liveTeam = &teams[liveTeamNum];
	CTeam* tempTeam = nullptr;

	// will be set to true immediately after we return
	assert(liveTeam->isDead);

	// Gaia team is alone in its allyteam and cannot really spawn
	if (liveTeamNum == gaiaTeamID)
		return;

	// count the number of remaining (non-dead) teams in <liveTeam>'s allyteam, EXCLUDING us
	const unsigned int numRemainingActiveTeams = GetNumTeamsInAllyTeam(AllyTeam(liveTeamNum), false);

	if (numRemainingActiveTeams == 0) {
		// set default since we are the only team in our allyteam now
		liveTeam->SetMaxUnits(std::min(gameSetup->maxUnitsPerTeam, int(MAX_UNITS / teams.size())));
		return;
	}

	// reduce the limit of each team in our allyteam uniformly
	for (unsigned int tempTeamNum = 0; tempTeamNum < teams.size(); tempTeamNum++) {
		if (tempTeamNum == liveTeamNum)
			continue;
		if (AllyTeam(tempTeamNum) != AllyTeam(liveTeamNum))
			continue;
		if (Team(tempTeamNum)->isDead)
			continue;

		tempTeam = &teams[tempTeamNum];
		tempTeam->SetMaxUnits((tempTeam->GetMaxUnits() * numRemainingActiveTeams) / (numRemainingActiveTeams + 1));
	}

	assert(tempTeam != nullptr);
	liveTeam->SetMaxUnits(tempTeam->GetMaxUnits());
}

void CTeamHandler::UpdateTeamUnitLimitsPreDeath(int deadTeamNum)
{
	CTeam* deadTeam = &teams[deadTeamNum];
	CTeam* tempTeam = nullptr;

	// will be set to false immediately after we return
	assert(!deadTeam->isDead);

	// Gaia team is alone in its allyteam and cannot really die
	if (deadTeamNum == gaiaTeamID)
		return;

	// count the number of remaining (non-dead) teams in <deadTeam>'s allyteam, INCLUDING us
	const unsigned int numRemainingActiveTeams = GetNumTeamsInAllyTeam(AllyTeam(deadTeamNum), false);

	if (numRemainingActiveTeams == 1) {
		// no more zombie unit production after death
		deadTeam->SetMaxUnits(0);
		return;
	}

	// redistribute <deadTeam>'s unit-limit over those teams uniformly
	//
	// assume an allyteam of 5 teams, each with a unit-limit of 500
	// whenever one of these teams dies, its limit is redistributed
	// as follows (integer division means some units are lost):
	//
	//   5 x  500 = 2500 --> 4 x ( 500 + ( 500/4)= 125 =  625)=2500
	//   4 x  625 = 2500 --> 3 x ( 625 + ( 625/3)= 208 =  833)=2499
	//   3 x  833 = 2499 --> 2 x ( 833 + ( 833/2)= 416 = 1249)=2498
	//   2 x 1249 = 2498 --> 1 x (1249 + (1249/1)=1249 = 2498)=2498
	for (unsigned int tempTeamNum = 0; tempTeamNum < teams.size(); tempTeamNum++) {
		if (tempTeamNum == deadTeamNum)
			continue;
		if (AllyTeam(tempTeamNum) != AllyTeam(deadTeamNum))
			continue;
		if (teams[tempTeamNum].isDead)
			continue;

		assert(teams[tempTeamNum].GetMaxUnits() == deadTeam->GetMaxUnits());

		tempTeam = &teams[tempTeamNum];
		tempTeam->SetMaxUnits((tempTeam->GetMaxUnits() * numRemainingActiveTeams) / (numRemainingActiveTeams - 1));
	}

	assert(tempTeam != nullptr);
	deadTeam->SetMaxUnits(0);
}

