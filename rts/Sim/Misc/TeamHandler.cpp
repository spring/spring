/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "TeamHandler.h"

#include <cstring>

#include "Game/GameSetup.h"
#include "Lua/LuaGaia.h"
#include "Sim/Misc/GlobalConstants.h"
#include "mmgr.h"
#include "Util.h"
#include "LogOutput.h"
#include "GlobalUnsynced.h"
#include "Platform/errorhandler.h"
#include "Game/PlayerHandler.h"
#include "Sim/Misc/GlobalSynced.h"

CR_BIND(CTeamHandler, );

CR_REG_METADATA(CTeamHandler, (
	CR_MEMBER(gaiaTeamID),
	CR_MEMBER(gaiaAllyTeamID),
	CR_MEMBER(teams),
	//CR_MEMBER(allyTeams),
	CR_RESERVED(64)
));


CTeamHandler* teamHandler;


CTeamHandler::CTeamHandler():
	gaiaTeamID(-1),
	gaiaAllyTeamID(-1),
	highlight(false)
{
}


CTeamHandler::~CTeamHandler()
{
	for (std::vector<CTeam*>::iterator it = teams.begin(); it != teams.end(); ++it)
		delete *it;
}


void CTeamHandler::LoadFromSetup(const CGameSetup* setup)
{
	const bool useLuaGaia = CLuaGaia::SetConfigString(setup->luaGaiaStr);

	assert(setup->teamStartingData.size() <= MAX_TEAMS);
	teams.resize(setup->teamStartingData.size());

	for (size_t i = 0; i < teams.size(); ++i) {
		// TODO: this loop body could use some more refactoring
		CTeam* team = new CTeam();
		teams[i] = team;
		*team = setup->teamStartingData[i];
		team->teamNum = i;
		SetAllyTeam(i, team->teamAllyteam);
	}

	allyTeams = setup->allyStartingData;
	assert(setup->allyStartingData.size() <= MAX_TEAMS);
	if (useLuaGaia) {
		// Gaia adjustments
		gaiaTeamID = static_cast<int>(teams.size());
		gaiaAllyTeamID = static_cast<int>(allyTeams.size());

		// Setup the gaia team
		CTeam& team = *(new CTeam());
		team.color[0] = 255;
		team.color[1] = 255;
		team.color[2] = 255;
		team.color[3] = 255;
		team.gaia = true;
		team.teamNum = gaiaTeamID;
		team.StartposMessage(float3(0.0, 0.0, 0.0));
		team.teamAllyteam = gaiaAllyTeamID;
		teams.push_back(&team);

		for (std::vector< ::AllyTeam >::iterator it = allyTeams.begin(); it != allyTeams.end(); ++it)
		{
			it->allies.push_back(false); // enemy to everyone
		}
		::AllyTeam allyteam;
		allyteam.allies.resize(allyTeams.size()+1,false); // everyones enemy
		allyteam.allies[gaiaTeamID] = true; // peace with itself
		allyTeams.push_back(allyteam);
	}
}

void CTeamHandler::GameFrame(int frameNum)
{
	if (!(frameNum & (TEAM_SLOWUPDATE_RATE-1))) {
		for (int a = 0; a < ActiveTeams(); ++a) {
			Team(a)->ResetFrameVariables();
		}
		for (int a = 0; a < ActiveTeams(); ++a) {
			Team(a)->SlowUpdate();
		}
		UpdateHighlight();
	}
}

void CTeamHandler::EnableHighlight(unsigned currentTime) {
	if(highlight) {
		for(int i=0; i < ActiveTeams(); ++i) {
			CTeam *t = Team(i);
			if(!t->isDead && t->highlight > 0.0f) {
				oldColors[i] = *(int *)t->color;
				float s = (float)(currentTime & 255) * 4.0f / 255.0f;
				int c =(int)(255.0f * ((s > 2.0f) ? 3.0f - s : s - 1.0f));
				c *= t->highlight;
				for(int n = 0; n < 3; ++n)
					t->color[n] = std::max(0, std::min(t->color[n] + c , 255));
			}
		}
	}
}

void CTeamHandler::DisableHighlight() {
	if(oldColors.size() > 0) {
		for(std::map<int,int>::iterator i = oldColors.begin(); i != oldColors.end(); ++i) {
			CTeam *t = Team((*i).first);
			*(int *)t->color = (*i).second;
		}
		oldColors.clear();
	}
}

void CTeamHandler::UpdateHighlight() {
	bool hl = false;
	for(int ti = 0; ti < ActiveTeams(); ++ti) {
		CTeam *t = Team(ti);
		int minPing = INT_MAX;
		for(int pi = 0; pi < playerHandler->ActivePlayers(); ++pi) {
			CPlayer *p = playerHandler->Player(pi);
			if (p->active && !p->spectator && (p->team == ti) && p->ping != PATHING_FLAG && p->ping >= 0) {
				int ping = (int)(((p->ping) * 1000) / (GAME_SPEED * gs->speedFactor));
				if(ping < minPing)
					minPing = ping;
			}
		}
		float teamhighlight = 0.0f;
		if(!t->isDead && ((minPing != INT_MAX && minPing > 1000) || t->leader < 0)) {
			int maxhl = 1000 * (gu->networkTimeout + 1);
			teamhighlight = (t->leader < 0) ? 1.0f : (float)std::max(0, std::min(minPing, maxhl)) / (float)maxhl;
			hl = true;
		}
		*(volatile float *)&t->highlight = teamhighlight;
	}
	*(volatile bool *)&highlight = hl;
}
