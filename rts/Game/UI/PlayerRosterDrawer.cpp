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
#include "System/SpringMath.h"

/******************************************************************************/


void CPlayerRosterDrawer::Draw()
{
	if (playerRoster.GetSortType() == PlayerRoster::Disabled)
		return;

	const unsigned currentTime = spring_now().toSecsi();

	const char* prefix = "";
	const char* formats[3] = {
		"\xff%c%c%c \tNu\tm   \tUser name   \tCPU  \tPing",
		"\xff%c%c%c%c \t%i \t%s   \t\xff%c%c%c%s   \t\xff%c%c%c%.0f%%  \t\xff%c%c%c%dms",
		"\xff%c%c%c%c \t%i \t%s   \t\xff%c%c%c%s   \t%s-%d  \t%d",
	};
	char buf[128];

	static std::string chart;
	const std::vector<int>& playerIndices = playerRoster.GetIndices(true);

	SNPRINTF(buf, sizeof(buf), formats[0], 255, 255, 63);
	chart.clear();
	chart.append(buf);

	for (const int playerIndex : playerIndices) {
		const CPlayer* p = playerHandler.Player(playerIndex);

		if (!p->active)
			continue;

		unsigned char teamColor[3] = {255, 255, 255};
		unsigned char allyColor[3] = {255, 255, 255};

		const char sep = (gu->spectating && !p->spectator && (gu->myTeam == p->team)) ? '-' : ' ';

		if (p->ping != PATHING_FLAG || !gs->PreSimFrame()) {
			if (p->spectator)
				prefix = "S";
			else {
				const unsigned char* bColor = teamHandler.Team(p->team)->color;

				teamColor[0] = std::max((unsigned char)1, bColor[0]);
				teamColor[1] = std::max((unsigned char)1, bColor[1]);
				teamColor[2] = std::max((unsigned char)1, bColor[2]);

				if (gu->myAllyTeam == teamHandler.AllyTeam(p->team)) {
					allyColor[0] = allyColor[2] = 1;
					prefix = "A";	// same AllyTeam
				}
				else if (teamHandler.AlliedTeams(gu->myTeam, p->team)) {
					allyColor[0] = allyColor[1] = 1;
					prefix = "E+";	// different AllyTeams, but are allied
				}
				else {
					allyColor[1] = allyColor[2] = 1;
					prefix = "E";	// no alliance at all
				}
			}

			const bool cpuConstRed = !p->spectator && (p->cpuUsage > 0.75f) && (gs->speedFactor < gs->wantedSpeedFactor * 0.99f) && (currentTime & 128);
			const bool pingConstRed = !p->spectator && (globalConfig.reconnectTimeout > 0) && (p->ping > 1000 * globalConfig.reconnectTimeout) && (currentTime & 128);

			const float4 cpuColor(cpuConstRed ? 0.5f : Clamp(p->cpuUsage * 2.0f / 0.75f, 0.01f, 1.0f), Clamp((1.0f - p->cpuUsage / 0.75f) * 2.0f, 0.01f, 1.0f), 0.01f, 1.0f);
			const float4 pingColor(pingConstRed ? 0.5f : Clamp((p->ping - 250) / 375.0f, 0.01f, 1.0f), Clamp((1000 - p->ping) / 375.0f, 0.01f, 1.0f), 0.01f, 1.0f);

			SNPRINTF(
				buf, sizeof(buf),
				formats[1],
				allyColor[0], allyColor[1], allyColor[2],
				sep,
				p->team,
				prefix,
				teamColor[0], teamColor[1], teamColor[2],
				p->name.c_str(),
				(unsigned char)(cpuColor[0] * 255.0f),
				(unsigned char)(cpuColor[1] * 255.0f),
				(unsigned char)(cpuColor[2] * 255.0f),
				p->cpuUsage * 100.0f,
				(unsigned char)(pingColor[0] * 255.0f),
				(unsigned char)(pingColor[1] * 255.0f),
				(unsigned char)(pingColor[2] * 255.0f),
				p->ping
			);
		} else {
			SNPRINTF(
				buf, sizeof(buf),
				formats[2],
				allyColor[0], allyColor[1], allyColor[2],
				sep,
				p->team,
				"",
				teamColor[0], teamColor[1], teamColor[2],
				p->name.c_str(),
				(((int) p->cpuUsage) & 0x1)? "PC": "BO",
				 ((int) p->cpuUsage) & 0xFE,
				(((int) p->cpuUsage) >> 8) * 1000
			);
		}

		chart.append("\n");
		chart.append(buf);
	}

	const int fontFlags = FONT_RIGHT | FONT_BOTTOM | FONT_SCALE | FONT_NORM | (guihandler->GetOutlineFonts() * FONT_OUTLINE) | FONT_BUFFERED;

	smallFont->SetColors();
	smallFont->glPrintTable(1.0f - 5.0f * globalRendering->pixelX, 0.00f + 5.0f * globalRendering->pixelY, 1.0f, fontFlags, chart);
}

/******************************************************************************/
