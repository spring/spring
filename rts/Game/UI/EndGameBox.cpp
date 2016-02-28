/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EndGameBox.h"

#include "MouseHandler.h"
#include "Game/Game.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/TeamStatistics.h"
#include "System/Exceptions.h"

#include <cstdio>
#include <sstream>

using std::sprintf;


static std::string FloatToSmallString(float num, float mul = 1) {

	char c[50];

	if (num == 0) {
		sprintf(c, "0");
	} else if (math::fabs(num) < 10 * mul) {
		sprintf(c, "%.1f",  num);
	} else if (math::fabs(num) < 10000 * mul) {
		sprintf(c, "%.0f",  num);
	} else if (math::fabs(num) < 10000000 * mul) {
		sprintf(c, "%.0fk", num / 1000);
	} else {
		sprintf(c, "%.0fM", num / 1000000);
	}

	return c;
}


bool CEndGameBox::enabled = true;
CEndGameBox* CEndGameBox::endGameBox = NULL;

CEndGameBox::CEndGameBox(const std::vector<unsigned char>& winningAllyTeams)
	: CInputReceiver()
	, moveBox(false)
	, dispMode(0)
	, stat1(1)
	, stat2(-1)
	, winners(winningAllyTeams)
	, graphTex(0)
{
	endGameBox = this;
	box.x1 = 0.14f;
	box.y1 = 0.1f;
	box.x2 = 0.86f;
	box.y2 = 0.8f;

	exitBox.x1 = 0.31f;
	exitBox.y1 = 0.02f;
	exitBox.x2 = 0.41f;
	exitBox.y2 = 0.06f;

	playerBox.x1 = 0.05f;
	playerBox.y1 = 0.62f;
	playerBox.x2 = 0.15f;
	playerBox.y2 = 0.65f;

	sumBox.x1 = 0.16f;
	sumBox.y1 = 0.62f;
	sumBox.x2 = 0.26f;
	sumBox.y2 = 0.65f;

	difBox.x1 = 0.27f;
	difBox.y1 = 0.62f;
	difBox.x2 = 0.38f;
	difBox.y2 = 0.65f;

	if (!bm.Load("bitmaps/graphPaper.bmp")) {
		throw content_error("Could not load bitmaps/graphPaper.bmp");
	}
}

CEndGameBox::~CEndGameBox()
{
	if (graphTex) {
		glDeleteTextures(1,&graphTex);
	}
	endGameBox = NULL;
}

bool CEndGameBox::MousePress(int x, int y, int button)
{
	if (!enabled) {
		return false;
	}

	float mx = MouseX(x);
	float my = MouseY(y);
	if (InBox(mx, my, box)) {
		moveBox = true;
		if (InBox(mx, my, box + exitBox)) {
			moveBox = false;
		}
		if (InBox(mx, my, box + playerBox)) {
			moveBox = false;
		}
		if (InBox(mx, my, box + sumBox)) {
			moveBox = false;
		}
		if (InBox(mx, my, box + difBox)) {
			moveBox = false;
		}
		if (dispMode>0 && mx>box.x1+0.01f && mx<box.x1+0.12f && my<box.y1+0.57f && my>box.y1+0.571f-stats.size()*0.02f) {
			moveBox = false;
		}
		return true;
	}

	return false;
}

void CEndGameBox::MouseMove(int x, int y, int dx, int dy, int button)
{
	if (!enabled) {
		return;
	}

	if (moveBox) {
		box.x1 += MouseMoveX(dx);
		box.x2 += MouseMoveX(dx);
		box.y1 += MouseMoveY(dy);
		box.y2 += MouseMoveY(dy);
	}
}

void CEndGameBox::MouseRelease(int x, int y, int button)
{
	if (!enabled) {
		return;
	}

	float mx = MouseX(x);
	float my = MouseY(y);

	if (InBox(mx, my, box + exitBox)) {
		delete this;
		gu->globalQuit = true;
		return;
	}

	if (InBox(mx, my, box + playerBox)) {
		dispMode = 0;
	}
	if (InBox(mx, my, box + sumBox)) {
		dispMode = 1;
	}
	if (InBox(mx, my, box + difBox)) {
		dispMode = 2;
	}

	if (dispMode > 0 ) {
		if ((mx > (box.x1 + 0.01f)) && (mx < (box.x1 + 0.12f)) &&
		    (my < (box.y1 + 0.57f)) && (my > (box.y1 + 0.571f - stats.size()*0.02f))) {
			int sel = (int) math::floor(-(my - box.y1 - 0.57f) * 50);

			if (button == 1) {
				stat1 = sel;
				stat2 = -1;
			} else {
				stat2 = sel;
			}
		}
	}

}

bool CEndGameBox::IsAbove(int x, int y)
{
	if (!enabled) {
		return false;
	}

	const float mx = MouseX(x);
	const float my = MouseY(y);
	return (InBox(mx, my, box));
}

void CEndGameBox::Draw()
{
	if (!graphTex) {
		graphTex = bm.CreateTexture();
	}

	if (!enabled) {
		return;
	}

	float mx = MouseX(mouse->lastx);
	float my = MouseY(mouse->lasty);

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	// Large Box
	glColor4f(0.2f, 0.2f, 0.2f, guiAlpha);
	DrawBox(box);

	glColor4f(0.2f, 0.2f, 0.7f, guiAlpha);
	if (dispMode == 0) {
		DrawBox(box + playerBox);
	} else if (dispMode == 1) {
		DrawBox(box + sumBox);
	} else {
		DrawBox(box + difBox);
	}

	if (InBox(mx, my, box+exitBox)) {
		glColor4f(0.7f, 0.2f, 0.2f, guiAlpha);
		DrawBox(box + exitBox);
	}
	if (InBox(mx,my,box+playerBox)) {
		glColor4f(0.7f, 0.2f, 0.2f, guiAlpha);
		DrawBox(box + playerBox);
	}
	if (InBox(mx,my,box+sumBox)) {
		glColor4f(0.7f, 0.2f, 0.2f, guiAlpha);
		DrawBox(box + sumBox);
	}
	if (InBox(mx,my,box+difBox)) {
		glColor4f(0.7f, 0.2f, 0.2f, guiAlpha);
		DrawBox(box + difBox);
	}

	glEnable(GL_TEXTURE_2D);
	glColor4f(1, 1, 1, 0.8f);
	font->glPrint(box.x1 + exitBox.x1   + 0.025f, box.y1 + exitBox.y1   + 0.005f, 1.0f, FONT_SCALE | FONT_NORM, "Exit");
	font->glPrint(box.x1 + playerBox.x1 + 0.015f, box.y1 + playerBox.y1 + 0.005f, 0.7f, FONT_SCALE | FONT_NORM, "Player stats");
	font->glPrint(box.x1 + sumBox.x1    + 0.015f, box.y1 + sumBox.y1    + 0.005f, 0.7f, FONT_SCALE | FONT_NORM, "Team stats");
	font->glPrint(box.x1 + difBox.x1    + 0.015f, box.y1 + difBox.y1    + 0.005f, 0.7f, FONT_SCALE | FONT_NORM, "Team delta stats");

	if (winners.empty()) {
		font->glPrint(box.x1 + 0.25f, box.y1 + 0.65f, 1.0f, FONT_SCALE | FONT_NORM, "Game result was undecided");
	} else {
		std::stringstream winnersText;
		std::stringstream winnersList;

		// myPlayingAllyTeam is >= 0 iff we ever joined a team
		const bool neverPlayed = (gu->myPlayingAllyTeam < 0);
		      bool playedAndWon = false;

		for (unsigned int i = 0; i < winners.size(); i++) {
			const int winnerAllyTeam = winners[i];

			if (!neverPlayed && winnerAllyTeam == gu->myPlayingAllyTeam) {
				// we actually played and won!
				playedAndWon = true; break;
			}

			winnersList << (((i > 0)? ((i < (winners.size() - 1))? ", ": " and "): ""));
			winnersList << winnerAllyTeam;
		}

		if (neverPlayed) {
			winnersText << "Game Over! Ally-team(s) ";
			winnersText << winnersList.str() << " won!";

			font->glPrint(box.x1 + 0.25f, box.y1 + 0.65f, 1.0f, FONT_SCALE | FONT_NORM, (winnersText.str()).c_str());
		} else {
			winnersText.str("");
			winnersText << "Game Over! Your ally-team ";
			winnersText << (playedAndWon? "won!": "lost!");
			font->glPrint(box.x1 + 0.25f, box.y1 + 0.65f, 1.0f, FONT_SCALE | FONT_NORM, (winnersText.str()).c_str());
		}
	}

	if (gs->PreSimFrame())
		return;

	if (dispMode == 0) {
		float xpos = 0.01f;

		std::string headers[] = {"Name", "MC/m", "MP/m", "KP/m", "Cmds/m", "ACS"};

		for (int a = 0; a < 6; ++a) {
			font->glPrint(box.x1 + xpos, box.y1 + 0.55f, 0.8f, FONT_SCALE | FONT_NORM,headers[a].c_str());
			xpos += 0.1f;
		}

		float ypos = 0.5f;
		for (int a = 0; a < playerHandler->ActivePlayers(); ++a) {
			const CPlayer* p = playerHandler->Player(a);
			const PlayerStatistics& pStats = p->currentStats;
			char values[6][100];

			SNPRINTF(values[0], 100, "%s", p->name.c_str());
			if (game->totalGameTime>0){ //prevent div zero
				SNPRINTF(values[1], 100, "%i", int(pStats.mouseClicks * 60 / game->totalGameTime));
				SNPRINTF(values[2], 100, "%i", int(pStats.mousePixels * 60 / game->totalGameTime));
				SNPRINTF(values[3], 100, "%i", int(pStats.keyPresses  * 60 / game->totalGameTime));
				SNPRINTF(values[4], 100, "%i", int(pStats.numCommands * 60 / game->totalGameTime));
			}else{
				for(int i=1; i<5; i++)
					SNPRINTF(values[i], 100, "%i", 0);
			}
			SNPRINTF(values[5], 100, "%i",
				(pStats.numCommands != 0)?
				(pStats.unitCommands / pStats.numCommands):
				(0));

			float xpos = 0.01f;
			for (int a = 0; a < 6; ++a) {
				font->glPrint(box.x1 + xpos, box.y1 + ypos, 0.8f, FONT_SCALE | FONT_NORM, values[a]);
				xpos += 0.1f;
			}

			ypos -= 0.02f;
		}
	} else {
		if (stats.empty()) {
			FillTeamStats();
		}

		glBindTexture(GL_TEXTURE_2D, graphTex);
		CVertexArray* va=GetVertexArray();
		va->Initialize();

		va->AddVertexT(float3(box.x1+0.15f, box.y1+0.08f, 0), 0, 0);
		va->AddVertexT(float3(box.x1+0.69f, box.y1+0.08f, 0), 4, 0);
		va->AddVertexT(float3(box.x1+0.69f, box.y1+0.62f, 0), 4, 4);
		va->AddVertexT(float3(box.x1+0.15f, box.y1+0.62f, 0), 0, 4);

		va->DrawArrayT(GL_QUADS);

		if ((mx > box.x1 + 0.01f) && (mx < box.x1 + 0.12f) &&
		    (my < box.y1 + 0.57f) && (my > box.y1 + 0.571f - (stats.size() * 0.02f))) {
			const int sel = (int) math::floor(50 * -(my - box.y1 - 0.57f));

			glColor4f(0.7f, 0.2f, 0.2f, guiAlpha);
			glDisable(GL_TEXTURE_2D);
			CVertexArray* va = GetVertexArray();
			va->Initialize();

			va->AddVertex0(float3(box.x1 + 0.01f, box.y1 + 0.55f - (sel * 0.02f)         , 0));
			va->AddVertex0(float3(box.x1 + 0.01f, box.y1 + 0.55f - (sel * 0.02f) + 0.02f , 0));
			va->AddVertex0(float3(box.x1 + 0.12f, box.y1 + 0.55f - (sel * 0.02f) + 0.02f , 0));
			va->AddVertex0(float3(box.x1 + 0.12f, box.y1 + 0.55f - (sel * 0.02f)         , 0));

			va->DrawArray0(GL_QUADS);
			glEnable(GL_TEXTURE_2D);
			glColor4f(1, 1, 1, 0.8f);
		}
		float ypos = 0.55f;
		for (size_t a = 0; a < stats.size(); ++a) {
			font->glPrint(box.x1 + 0.01f, box.y1 + ypos, 0.8f, FONT_SCALE | FONT_NORM, stats[a].name);
			ypos -= 0.02f;
		}
		float maxy = 1;

		if (dispMode == 1) {
			maxy = std::max(stats[stat1].max,    (stat2 != -1) ? stats[stat2].max    : 0);
		} else {
			maxy = std::max(stats[stat1].maxdif, (stat2 != -1) ? stats[stat2].maxdif : 0) / TeamStatistics::statsPeriod;
		}

		int numPoints = stats[0].values[0].size();

		for (int a = 0; a < 5; ++a) {
			font->glPrint(box.x1 + 0.12f, box.y1 + 0.07f + (a * 0.135f), 0.8f, FONT_SCALE | FONT_NORM,
			                FloatToSmallString(maxy * 0.25f * a));
			font->glFormat(box.x1 + 0.135f + (a * 0.135f), box.y1 + 0.057f, 0.8f, FONT_SCALE | FONT_NORM, "%02i:%02i",
			                (int) (a * 0.25f * numPoints * TeamStatistics::statsPeriod / 60),
			                (int) (a * 0.25f * (numPoints - 1) * TeamStatistics::statsPeriod) % 60);
		}

		font->glPrint(box.x1 + 0.55f, box.y1 + 0.65f, 0.8f, FONT_SCALE | FONT_NORM, stats[stat1].name);
		font->glPrint(box.x1 + 0.55f, box.y1 + 0.63f, 0.8f, FONT_SCALE | FONT_NORM, (stat2 != -1) ? stats[stat2].name : "");

		glDisable(GL_TEXTURE_2D);
		glBegin(GL_LINES);
				glVertex3f(box.x1+0.50f, box.y1+0.66f, 0);
				glVertex3f(box.x1+0.55f, box.y1+0.66f, 0);
		glEnd();

		glLineStipple(3, 0x5555);
		glEnable(GL_LINE_STIPPLE);
		glBegin(GL_LINES);
				glVertex3f(box.x1 + 0.50f, box.y1 + 0.64f, 0.0f);
				glVertex3f(box.x1 + 0.55f, box.y1 + 0.64f, 0.0f);
		glEnd();
		glDisable(GL_LINE_STIPPLE);

		const float scalex = 0.54f / std::max(1.0f, numPoints - 1.0f);
		const float scaley = 0.54f / maxy;

		for (int team = 0; team < teamHandler->ActiveTeams(); team++) {
			const CTeam* pteam = teamHandler->Team(team);

			if (pteam->gaia) {
				continue;
			}

			glColor4ubv(pteam->color);

			glBegin(GL_LINE_STRIP);
			for (int a = 0; a < numPoints; ++a) {
				float value = 0.0f;

				if (dispMode == 1) {
					value = stats[stat1].values[team][a];
				} else if (a > 0) {
					value = (stats[stat1].values[team][a] - stats[stat1].values[team][a - 1]) / TeamStatistics::statsPeriod;
				}

				glVertex3f(box.x1 + 0.15f + a * scalex, box.y1 + 0.08f + value * scaley, 0.0f);
			}
			glEnd();

			if (stat2 != -1) {
				glLineStipple(3, 0x5555);
				glEnable(GL_LINE_STIPPLE);

				glBegin(GL_LINE_STRIP);
				for (int a = 0; a < numPoints; ++a) {
					float value = 0;
					if (dispMode == 1) {
						value = stats[stat2].values[team][a];
					} else if (a > 0) {
						value = (stats[stat2].values[team][a]-stats[stat2].values[team][a-1]) / TeamStatistics::statsPeriod;
					}

					glVertex3f(box.x1+0.15f+a*scalex, box.y1+0.08f+value*scaley, 0);
				}
				glEnd();

				glDisable(GL_LINE_STIPPLE);
			}
		}
	}
}

std::string CEndGameBox::GetTooltip(int x, int y)
{
	if (!enabled) {
		return "";
	}

	const float mx = MouseX(x);

	if (dispMode == 0) {
		if ((mx > box.x1 + 0.02f) && (mx < box.x1 + 0.1f * 6)) {
			static const std::string tips[] = {
				"Player Name",
				"Mouse clicks per minute",
				"Mouse movement in pixels per minute",
				"Keyboard presses per minute",
				"Unit commands per minute",
				"Average command size (units affected per command)"
			};

			return tips[int((mx - box.x1 - 0.01f) * 10)];
		}
	}

	return "No tooltip defined";
}

void CEndGameBox::FillTeamStats()
{
	stats.push_back(Stat(""));
	stats.push_back(Stat("Metal used"));
	stats.push_back(Stat("Energy used"));
	stats.push_back(Stat("Metal produced"));
	stats.push_back(Stat("Energy produced"));

	stats.push_back(Stat("Metal excess"));
	stats.push_back(Stat("Energy excess"));

	stats.push_back(Stat("Metal received"));
	stats.push_back(Stat("Energy received"));

	stats.push_back(Stat("Metal sent"));
	stats.push_back(Stat("Energy sent"));

	stats.push_back(Stat("Metal stored"));
	stats.push_back(Stat("Energy stored"));

	stats.push_back(Stat("Active Units"));
	stats.push_back(Stat("Units killed"));

	stats.push_back(Stat("Units produced"));
	stats.push_back(Stat("Units died"));

	stats.push_back(Stat("Units received"));
	stats.push_back(Stat("Units sent"));
	stats.push_back(Stat("Units captured"));
	stats.push_back(Stat("Units stolen"));

	stats.push_back(Stat("Damage Dealt"));
	stats.push_back(Stat("Damage Received"));

	for (int team = 0; team < teamHandler->ActiveTeams(); team++) {
		const CTeam* pteam = teamHandler->Team(team);

		if (pteam->gaia) {
			continue;
		}

		for (std::list<TeamStatistics>::const_iterator si = pteam->statHistory.begin(); si != pteam->statHistory.end(); ++si) {
			stats[0].AddStat(team, 0);

			stats[1].AddStat(team, si->metalUsed);
			stats[2].AddStat(team, si->energyUsed);
			stats[3].AddStat(team, si->metalProduced);
			stats[4].AddStat(team, si->energyProduced);

			stats[5].AddStat(team, si->metalExcess);
			stats[6].AddStat(team, si->energyExcess);

			stats[7].AddStat(team, si->metalReceived);
			stats[8].AddStat(team, si->energyReceived);

			stats[9].AddStat(team, si->metalSent);
			stats[10].AddStat(team, si->energySent);

			stats[11].AddStat(team, si->metalProduced+si->metalReceived - (si->metalUsed + si->metalSent+si->metalExcess) );
			stats[12].AddStat(team, si->energyProduced+si->energyReceived - (si->energyUsed + si->energySent+si->energyExcess) );

			stats[13].AddStat(team, si->unitsProduced+si->unitsReceived + si->unitsCaptured - (si->unitsDied + si->unitsSent + si->unitsOutCaptured) );
			stats[14].AddStat(team, si->unitsKilled);

			stats[15].AddStat(team, si->unitsProduced);
			stats[16].AddStat(team, si->unitsDied);

			stats[17].AddStat(team, si->unitsReceived);
			stats[18].AddStat(team, si->unitsSent);
			stats[19].AddStat(team, si->unitsCaptured);
			stats[20].AddStat(team, si->unitsOutCaptured);

			stats[21].AddStat(team, si->damageDealt);
			stats[22].AddStat(team, si->damageReceived);
		}
	}
}
