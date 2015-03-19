/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TeamHighlight.h"

#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/Config/ConfigHandler.h"
#include "System/GlobalConfig.h"

#include <climits>


bool CTeamHighlight::highlight = false;
std::map<int, int> CTeamHighlight::oldColors;

void CTeamHighlight::Enable(unsigned currentTime) {

	if (highlight) {
		for (int i = 0; i < teamHandler->ActiveTeams(); ++i) {
			CTeam* t = teamHandler->Team(i);
			if (t->highlight > 0.0f) {
				oldColors[i] = *(int *)t->color;
				float s = (float)(currentTime & 255) * 4.0f / 255.0f;
				int c =(int)(255.0f * ((s > 2.0f) ? 3.0f - s : s - 1.0f));
				c *= t->highlight;
				for (int n = 0; n < 3; ++n) {
					t->color[n] = std::max(0, std::min(t->color[n] + c , 255));
				}
			}
		}
	}
}

void CTeamHighlight::Disable() {

	if (!oldColors.empty()) {
		for (std::map<int, int>::iterator i = oldColors.begin(); i != oldColors.end(); ++i) {
			CTeam* team = teamHandler->Team((*i).first);
			*(int*)team->color = (*i).second;
		}
		oldColors.clear();
	}
}

void CTeamHighlight::Update(int frameNum) {
	if ((frameNum % TEAM_SLOWUPDATE_RATE))
		return;

	bool hl = false;
	if ((globalConfig->teamHighlight == HIGHLIGHT_PLAYERS && !gu->spectatingFullView) || globalConfig->teamHighlight == HIGHLIGHT_ALL) {
		const int maxhl = 1000 * (globalConfig->networkTimeout + 1);

		for (int ti = 0; ti < teamHandler->ActiveTeams(); ++ti) {
			CTeam* t = teamHandler->Team(ti);
			float teamhighlight = 0.0f;

			if (t->gaia) { continue; }
			if (t->isDead) { continue; }
			if (t->units.empty()) { continue; }
			if (!skirmishAIHandler.GetSkirmishAIsInTeam(ti).empty()) { continue; }
			if (!gu->spectatingFullView && !teamHandler->AlliedTeams(gu->myTeam, ti)) { continue; }

			int minPing = INT_MAX;
			bool hasPlayers = false;

			for (int pi = 0; pi < playerHandler->ActivePlayers(); ++pi) {
				CPlayer* p = playerHandler->Player(pi);

				if (!p->active) { continue; }
				if (p->spectator) { continue; }
				if ((p->team != ti)) { continue; }

				hasPlayers = true;

				if (p->ping != PATHING_FLAG && p->ping >= 0) {
					minPing = std::min(p->ping, minPing);
				}
			}
			if (!hasPlayers || !t->HasLeader()) {
				teamhighlight = 1.0f;
			} else if (minPing != INT_MAX && minPing > 1000) {
				teamhighlight = std::max(0, std::min(minPing, maxhl)) / float(maxhl);
			}
			if (teamhighlight > 0.0f) {
				hl = true;
			}

			*(volatile float *)&t->highlight = teamhighlight;
		}
	}

	*(volatile bool *)&highlight = hl;
}
