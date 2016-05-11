/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "PlayerRosterDrawer.h"

#include "PlayerRoster.h"
#include "Game/GlobalUnsynced.h"
#include "Game/UI/GuiHandler.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/GlobalConfig.h"
#include "System/Misc/SpringTime.h"

/******************************************************************************/


void CPlayerRosterDrawer::Draw()
{
	if (playerRoster.GetSortType() == PlayerRoster::Disabled)
		return;

	const unsigned currentTime = spring_now().toSecsi();

	static std::string chart; chart = "";
	static std::string prefix;
	char buf[128];

	int count;
	const std::vector<int>& indices = playerRoster.GetIndices(&count, true);

	SNPRINTF(buf, sizeof(buf), "\xff%c%c%c \tNu\tm   \tUser name   \tCPU  \tPing", 255, 255, 63);
	chart += buf;

	for (int a = 0; a < count; ++a) {
		const CPlayer* p = playerHandler->Player(indices[a]);
		unsigned char color[3] = {255, 255, 255};
		unsigned char allycolor[3] = {255, 255, 255};

		if (p->ping != PATHING_FLAG || !gs->PreSimFrame()) {
			if (p->spectator)
				prefix = "S";
			else {
				const unsigned char* bColor = teamHandler->Team(p->team)->color;
				color[0] = std::max((unsigned char)1, bColor[0]);
				color[1] = std::max((unsigned char)1, bColor[1]);
				color[2] = std::max((unsigned char)1, bColor[2]);
				if (gu->myAllyTeam == teamHandler->AllyTeam(p->team)) {
					allycolor[0] = allycolor[2] = 1;
					prefix = "A";	// same AllyTeam
				}
				else if (teamHandler->AlliedTeams(gu->myTeam, p->team)) {
					allycolor[0] = allycolor[1] = 1;
					prefix = "E+";	// different AllyTeams, but are allied
				}
				else {
					allycolor[1] = allycolor[2] = 1;
					prefix = "E";	//no alliance at all
				}
			}
			float4 cpucolor(!p->spectator && p->cpuUsage > 0.75f && gs->speedFactor < gs->wantedSpeedFactor * 0.99f &&
				(currentTime & 128) ? 0.5f : std::max(0.01f, std::min(1.0f, p->cpuUsage * 2.0f / 0.75f)),
					std::min(1.0f, std::max(0.01f, (1.0f - p->cpuUsage / 0.75f) * 2.0f)), 0.01f, 1.0f);
			float4 pingcolor(!p->spectator && globalConfig->reconnectTimeout > 0 && p->ping > 1000 * globalConfig->reconnectTimeout &&
					(currentTime & 128) ? 0.5f : std::max(0.01f, std::min(1.0f, (p->ping - 250) / 375.0f)),
					std::min(1.0f, std::max(0.01f, (1000 - p->ping) / 375.0f)), 0.01f, 1.0f);
			SNPRINTF(buf, sizeof(buf), "\xff%c%c%c%c \t%i \t%s   \t\xff%c%c%c%s   \t\xff%c%c%c%.0f%%  \t\xff%c%c%c%dms",
					allycolor[0], allycolor[1], allycolor[2], (gu->spectating && !p->spectator && (gu->myTeam == p->team)) ? '-' : ' ',
					p->team, prefix.c_str(), color[0], color[1], color[2], p->name.c_str(),
					(unsigned char)(cpucolor[0] * 255.0f), (unsigned char)(cpucolor[1] * 255.0f), (unsigned char)(cpucolor[2] * 255.0f),
					p->cpuUsage * 100.0f,
					(unsigned char)(pingcolor[0] * 255.0f), (unsigned char)(pingcolor[1] * 255.0f), (unsigned char)(pingcolor[2] * 255.0f),
					p->ping);
		}
		else {
			prefix = "";
			SNPRINTF(buf, sizeof(buf), "\xff%c%c%c%c \t%i \t%s   \t\xff%c%c%c%s   \t%s-%d  \t%d",
					allycolor[0], allycolor[1], allycolor[2], (gu->spectating && !p->spectator && (gu->myTeam == p->team)) ? '-' : ' ',
					p->team, prefix.c_str(), color[0], color[1], color[2], p->name.c_str(), (((int)p->cpuUsage) & 0x1)?"PC":"BO",
					((int)p->cpuUsage) & 0xFE, (((int)p->cpuUsage)>>8)*1000);
		}
		chart += "\n";
		chart += buf;
	}

	int font_options = FONT_RIGHT | FONT_BOTTOM | FONT_SCALE | FONT_NORM;
	if (guihandler->GetOutlineFonts())
		font_options |= FONT_OUTLINE;

	smallFont->SetColors();
	smallFont->glPrintTable(1.0f - 5 * globalRendering->pixelX, 0.00f + 5 * globalRendering->pixelY, 1.0f, font_options, chart);
}

/******************************************************************************/
