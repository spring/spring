/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "TeamHighlight.h"
#include "Sim/Misc/TeamHandler.h"
#include "Game/PlayerHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/ConfigHandler.h"
#include "System/GlobalUnsynced.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include <limits.h>

bool CTeamHighlight::highlight = false;
std::map<int, int> CTeamHighlight::oldColors;

void CTeamHighlight::Enable(unsigned currentTime) {
	if(highlight) {
		for(int i=0; i < teamHandler->ActiveTeams(); ++i) {
			CTeam *t = teamHandler->Team(i);
			if(t->highlight > 0.0f) {
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

void CTeamHighlight::Disable() {
	if(oldColors.size() > 0) {
		for(std::map<int,int>::iterator i = oldColors.begin(); i != oldColors.end(); ++i) {
			CTeam *t = teamHandler->Team((*i).first);
			*(int *)t->color = (*i).second;
		}
		oldColors.clear();
	}
}

void CTeamHighlight::Update(int frameNum) {
	if (frameNum & (TEAM_SLOWUPDATE_RATE-1))
		return;

	bool hl = false;
	if((gc->teamHighlight == 1 && !gu->spectatingFullView) || gc->teamHighlight == 2) {
		int maxhl = 1000 * (gc->networkTimeout + 1);
		for(int ti = 0; ti < teamHandler->ActiveTeams(); ++ti) {
			CTeam *t = teamHandler->Team(ti);
			float teamhighlight = 0.0f;
			if(!t->isDead && !t->units.empty() && teamHandler->AlliedTeams(gu->myTeam, ti) && 
				!t->gaia && skirmishAIHandler.GetSkirmishAIsInTeam(ti).empty()) {
					int minPing = INT_MAX;
					bool hasPlayers = false;
					for(int pi = 0; pi < playerHandler->ActivePlayers(); ++pi) {
						CPlayer *p = playerHandler->Player(pi);
						if (p->active && !p->spectator && (p->team == ti)) {
							hasPlayers = true;
							if(p->ping != PATHING_FLAG && p->ping >= 0) {
								int ping = (int)(((p->ping) * 1000) / (GAME_SPEED * gs->speedFactor));
								if(ping < minPing)
									minPing = ping;
							}
						}
					}
					if(!hasPlayers || t->leader < 0)
						teamhighlight = 1.0f;
					else if(minPing != INT_MAX && minPing > 1000)
						teamhighlight = (float)std::max(0, std::min(minPing, maxhl)) / (float)maxhl;
					if(teamhighlight > 0.0f)
						hl = true;
			}
			*(volatile float *)&t->highlight = teamhighlight;
		}
	}
	*(volatile bool *)&highlight = hl;
}
