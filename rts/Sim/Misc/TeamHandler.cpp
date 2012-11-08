/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TeamHandler.h"

#include <cstring>

#include "Game/GameSetup.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/Util.h"


CR_BIND(CTeamHandler, );

CR_REG_METADATA(CTeamHandler, (
	CR_MEMBER(gaiaTeamID),
	CR_MEMBER(gaiaAllyTeamID),
	CR_MEMBER(teams),
	//CR_MEMBER(allyTeams),
	CR_RESERVED(64)
));


CTeamHandler* teamHandler = NULL;


CTeamHandler::CTeamHandler():
	gaiaTeamID(-1),
	gaiaAllyTeamID(-1)
{
}


CTeamHandler::~CTeamHandler()
{
	for (std::vector<CTeam*>::iterator it = teams.begin(); it != teams.end(); ++it)
		delete *it;
}


void CTeamHandler::LoadFromSetup(const CGameSetup* setup)
{
	assert(!setup->teamStartingData.empty());
	assert(setup->teamStartingData.size() <= MAX_TEAMS);
	assert(setup->allyStartingData.size() <= MAX_TEAMS);

	teams.reserve(setup->teamStartingData.size() + 1); // +1 for Gaia
	teams.resize(setup->teamStartingData.size());
	allyTeams = setup->allyStartingData;

	for (size_t i = 0; i < teams.size(); ++i) {
		// TODO: this loop body could use some more refactoring
		CTeam* team = new CTeam();
		teams[i] = team;
		*team = setup->teamStartingData[i];
		team->teamNum = i;
		team->maxUnits = std::min(setup->maxUnits, int(MAX_UNITS / teams.size()));

		assert(team->teamAllyteam >=                0);
		assert(team->teamAllyteam <  allyTeams.size());
	}

	if (gs->useLuaGaia) {
		// Gaia adjustments
		gaiaTeamID = static_cast<int>(teams.size());
		gaiaAllyTeamID = static_cast<int>(allyTeams.size());

		// Setup the gaia team
		CTeam* gaia = new CTeam();
		gaia->color[0] = 255;
		gaia->color[1] = 255;
		gaia->color[2] = 255;
		gaia->color[3] = 255;
		gaia->gaia = true;
		gaia->teamNum = gaiaTeamID;
		gaia->maxUnits = MAX_UNITS - (teams.size() * teams[0]->maxUnits);
		gaia->StartposMessage(ZeroVector);
		gaia->teamAllyteam = gaiaAllyTeamID;
		teams.push_back(gaia);

		assert((((teams.size() - 1) * teams[0]->maxUnits) + gaia->maxUnits) == MAX_UNITS);

		for (std::vector< ::AllyTeam >::iterator it = allyTeams.begin(); it != allyTeams.end(); ++it) {
			it->allies.push_back(false); // enemy to everyone
		}

		::AllyTeam allyteam;
		allyteam.allies.resize(allyTeams.size() + 1, false); // everyones enemy
		allyteam.allies[gaiaAllyTeamID] = true; // peace with itself
		allyTeams.push_back(allyteam);
	}
}

void CTeamHandler::GameFrame(int frameNum)
{
	if ((frameNum % TEAM_SLOWUPDATE_RATE) == 0) {
		for (int a = 0; a < ActiveTeams(); ++a) {
			teams[a]->ResetResourceState();
		}
		for (int a = 0; a < ActiveTeams(); ++a) {
			teams[a]->SlowUpdate();
		}
	}
}
