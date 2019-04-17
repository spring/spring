/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EndGameBox.h"

#include "MouseHandler.h"
#include "Game/Game.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
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
CEndGameBox* CEndGameBox::endGameBox = nullptr;

CEndGameBox::CEndGameBox(const std::vector<unsigned char>& winningAllyTeams)
	: winners(winningAllyTeams)
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

	CBitmap bm;
	if (!bm.Load("bitmaps/graphPaper.bmp"))
		bm.AllocDummy(SColor(255, 255, 255, 255));

	graphTex = bm.CreateTexture();
}

CEndGameBox::~CEndGameBox()
{
	if (graphTex != 0)
		glDeleteTextures(1, &graphTex);

	endGameBox = nullptr;
}


bool CEndGameBox::MousePress(int x, int y, int button)
{
	if (!enabled)
		return false;

	float mx = MouseX(x);
	float my = MouseY(y);

	if (!InBox(mx, my, box))
		return false;

	moveBox = true;

	if (InBox(mx, my, box + exitBox))
		moveBox = false;

	if (InBox(mx, my, box + playerBox))
		moveBox = false;

	if (InBox(mx, my, box + sumBox))
		moveBox = false;

	if (InBox(mx, my, box + difBox))
		moveBox = false;

	const float bxmin = box.x1 + 0.01f ;
	const float bxmax = box.x1 + 0.12f ;
	const float bymin = box.y1 + 0.571f - (stats.size() * 0.02f);
	const float bymax = box.y1 + 0.57f ;

	if (dispMode > 0 &&  mx > bxmin && mx < bxmax && my > bymin && my < bymax)
		moveBox = false;

	return true;
}

void CEndGameBox::MouseMove(int x, int y, int dx, int dy, int button)
{
	if (!enabled)
		return;

	if (moveBox) {
		box.x1 += MouseMoveX(dx);
		box.x2 += MouseMoveX(dx);
		box.y1 += MouseMoveY(dy);
		box.y2 += MouseMoveY(dy);
	}
}

void CEndGameBox::MouseRelease(int x, int y, int button)
{
	if (!enabled)
		return;

	const float mx = MouseX(x);
	const float my = MouseY(y);

	if (InBox(mx, my, box + exitBox)) {
		delete this;
		gu->globalQuit = true;
		return;
	}

	if (InBox(mx, my, box + playerBox))
		dispMode = 0;

	if (InBox(mx, my, box + sumBox))
		dispMode = 1;

	if (InBox(mx, my, box + difBox))
		dispMode = 2;


	if (dispMode <= 0)
		return;

	const float bxmin = box.x1 + 0.01f ;
	const float bxmax = box.x1 + 0.12f ;
	const float bymin = box.y1 + 0.571f - (stats.size() * 0.02f);
	const float bymax = box.y1 + 0.57f ;

	if (mx > bxmin && mx < bxmax && my > bymin && my < bymax) {
		const int sel = (int) math::floor(-(my - box.y1 - 0.57f) * 50);

		if (button == 1) {
			stat1 = sel;
			stat2 = -1;
		} else {
			stat2 = sel;
		}
	}
}

bool CEndGameBox::IsAbove(int x, int y)
{
	if (!enabled)
		return false;

	return (InBox(MouseX(x), MouseY(y), box));
}



void CEndGameBox::Draw()
{
	if (!enabled)
		return;

	const float mx = MouseX(mouse->lastx);
	const float my = MouseY(mouse->lasty);

	GL::RenderDataBufferC* bufferC = GL::GetRenderBufferC();
	GL::RenderDataBufferT* bufferT = GL::GetRenderBufferT();

	Shader::IProgramObject* shaderC = bufferC->GetShader();
	Shader::IProgramObject* shaderT = bufferT->GetShader();


	glAttribStatePtr->EnableBlendMask();

	{
		// Large Box
		gleDrawQuadC(box, SColor{0.2f, 0.2f, 0.2f, guiAlpha}, bufferC);

		switch (dispMode) {
			case 0: {
				gleDrawQuadC(box + playerBox, SColor{0.2f, 0.2f, 0.7f, guiAlpha}, bufferC);
			} break;
			case 1: {
				gleDrawQuadC(box + sumBox, SColor{0.2f, 0.2f, 0.7f, guiAlpha}, bufferC);
			} break;
			default: {
				gleDrawQuadC(box + difBox, SColor{0.2f, 0.2f, 0.7f, guiAlpha}, bufferC);
			} break;
		}

		if (InBox(mx, my, box + exitBox))
			gleDrawQuadC(box + exitBox, SColor{0.7f, 0.2f, 0.2f, guiAlpha}, bufferC);

		if (InBox(mx, my, box + playerBox))
			gleDrawQuadC(box + playerBox, SColor{0.7f, 0.2f, 0.2f, guiAlpha}, bufferC);

		if (InBox(mx, my, box + sumBox))
			gleDrawQuadC(box + sumBox, SColor{0.7f, 0.2f, 0.2f, guiAlpha}, bufferC);

		if (InBox(mx, my, box + difBox))
			gleDrawQuadC(box + difBox, SColor{0.7f, 0.2f, 0.2f, guiAlpha}, bufferC);

	}
	{
		// draw boxes
		shaderC->Enable();
		shaderC->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
		shaderC->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));
		bufferC->Submit(GL_TRIANGLES);
		shaderC->Disable();
	}


	font->SetTextColor(1.0f, 1.0f, 1.0f, 0.8f);
	font->glPrint(box.x1 +   exitBox.x1 + 0.025f, box.y1 +   exitBox.y1 + 0.005f, 1.0f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Exit");
	font->glPrint(box.x1 + playerBox.x1 + 0.015f, box.y1 + playerBox.y1 + 0.005f, 0.7f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Player stats");
	font->glPrint(box.x1 +    sumBox.x1 + 0.015f, box.y1 +    sumBox.y1 + 0.005f, 0.7f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Team stats");
	font->glPrint(box.x1 +    difBox.x1 + 0.015f, box.y1 +    difBox.y1 + 0.005f, 0.7f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Team delta stats");

	if (winners.empty()) {
		font->glPrint(box.x1 + 0.25f, box.y1 + 0.65f, 1.0f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Game result was undecided");
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

			font->glPrint(box.x1 + 0.25f, box.y1 + 0.65f, 1.0f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, winnersText.str());
		} else {
			winnersText.str("");
			winnersText << "Game Over! Your ally-team ";
			winnersText << (playedAndWon? "won!": "lost!");
			font->glPrint(box.x1 + 0.25f, box.y1 + 0.65f, 1.0f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, winnersText.str());
		}
	}

	if (gs->PreSimFrame()) {
		font->DrawBufferedGL4();
		return;
	}


	if (dispMode == 0) {
		float xpos = 0.01f;
		float ypos = 0.5f;

		const char* headers[] = {"Name", "MC/m", "MP/m", "KP/m", "Cmds/m", "ACS"};
		char values[6][100];

		for (const auto& header: headers) {
			font->glPrint(box.x1 + xpos, box.y1 + 0.55f, 0.8f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, header);
			xpos += 0.1f;
		}

		for (int a = 0; a < playerHandler.ActivePlayers(); ++a) {
			const CPlayer* p = playerHandler.Player(a);
			const PlayerStatistics& pStats = p->currentStats;

			SNPRINTF(values[0], 100, "%s", p->name.c_str());

			if (game->totalGameTime > 0) {
				SNPRINTF(values[1], 100, "%i", int(pStats.mouseClicks * 60 / game->totalGameTime));
				SNPRINTF(values[2], 100, "%i", int(pStats.mousePixels * 60 / game->totalGameTime));
				SNPRINTF(values[3], 100, "%i", int(pStats.keyPresses  * 60 / game->totalGameTime));
				SNPRINTF(values[4], 100, "%i", int(pStats.numCommands * 60 / game->totalGameTime));
			} else {
				for (int i = 1; i < 5; i++)
					SNPRINTF(values[i], 100, "%i", 0);
			}

			SNPRINTF(values[5], 100, "%i", (pStats.numCommands != 0)? (pStats.unitCommands / pStats.numCommands): 0);

			xpos = 0.01f;
			for (const auto& value: values) {
				font->glPrint(box.x1 + xpos, box.y1 + ypos, 0.8f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, value);
				xpos += 0.1f;
			}

			ypos -= 0.02f;
		}
	} else {
		if (stats.empty())
			FillTeamStats();


		bufferT->SafeAppend({{box.x1 + 0.15f, box.y1 + 0.08f, 0.0f}, 0.0f, 0.0f}); // tl
		bufferT->SafeAppend({{box.x1 + 0.69f, box.y1 + 0.08f, 0.0f}, 4.0f, 0.0f}); // tr
		bufferT->SafeAppend({{box.x1 + 0.69f, box.y1 + 0.62f, 0.0f}, 4.0f, 4.0f}); // br

		bufferT->SafeAppend({{box.x1 + 0.69f, box.y1 + 0.62f, 0.0f}, 4.0f, 4.0f}); // br
		bufferT->SafeAppend({{box.x1 + 0.15f, box.y1 + 0.62f, 0.0f}, 0.0f, 4.0f}); // bl
		bufferT->SafeAppend({{box.x1 + 0.15f, box.y1 + 0.08f, 0.0f}, 0.0f, 0.0f}); // tl

		glBindTexture(GL_TEXTURE_2D, graphTex);
		shaderT->Enable();
		shaderT->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
		shaderT->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));
		bufferT->Submit(GL_TRIANGLES);
		shaderT->Disable();
		glBindTexture(GL_TEXTURE_2D, 0);

		const float bxmin = box.x1 + 0.01f ;
		const float bxmax = box.x1 + 0.12f ;
		const float bymin = box.y1 + 0.571f - (stats.size() * 0.02f);
		const float bymax = box.y1 + 0.57f ;

		if (mx > bxmin && mx < bxmax && my > bymin && my < bymax) {
			const float sel = math::floor(50.0f * -(my - box.y1 - 0.57f));

			bufferC->SafeAppend({{box.x1 + 0.01f, box.y1 + 0.55f - (sel * 0.02f)         , 0.0f}, {0.7f, 0.2f, 0.2f, guiAlpha}}); // tl
			bufferC->SafeAppend({{box.x1 + 0.01f, box.y1 + 0.55f - (sel * 0.02f) + 0.02f , 0.0f}, {0.7f, 0.2f, 0.2f, guiAlpha}}); // bl
			bufferC->SafeAppend({{box.x1 + 0.12f, box.y1 + 0.55f - (sel * 0.02f) + 0.02f , 0.0f}, {0.7f, 0.2f, 0.2f, guiAlpha}}); // br

			bufferC->SafeAppend({{box.x1 + 0.12f, box.y1 + 0.55f - (sel * 0.02f) + 0.02f , 0.0f}, {0.7f, 0.2f, 0.2f, guiAlpha}}); // br
			bufferC->SafeAppend({{box.x1 + 0.12f, box.y1 + 0.55f - (sel * 0.02f)         , 0.0f}, {0.7f, 0.2f, 0.2f, guiAlpha}}); // tr
			bufferC->SafeAppend({{box.x1 + 0.01f, box.y1 + 0.55f - (sel * 0.02f)         , 0.0f}, {0.7f, 0.2f, 0.2f, guiAlpha}}); // tl
			shaderC->Enable();
			bufferC->Submit(GL_TRIANGLES);
			shaderC->Disable();
		}



		float ypos = 0.55f;
		float maxy = 1.0f;

		for (const auto& stat: stats) {
			font->glPrint(box.x1 + 0.01f, box.y1 + ypos, 0.8f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, stat.name);
			ypos -= 0.02f;
		}

		if (dispMode == 1) {
			maxy = std::max(stats[stat1].max,    (stat2 != -1) ? stats[stat2].max    : 0);
		} else {
			maxy = std::max(stats[stat1].maxdif, (stat2 != -1) ? stats[stat2].maxdif : 0) / TeamStatistics::statsPeriod;
		}

		const size_t numPoints = stats[0].values[0].size();

		const float scalex = 0.54f / std::max(1.0f, numPoints - 1.0f);
		const float scaley = 0.54f / maxy;

		for (int a = 0; a < 5; ++a) {
			const int secs = int(a * 0.25f * (numPoints - 1) * TeamStatistics::statsPeriod) % 60;
			const int mins = int(a * 0.25f * (numPoints    ) * TeamStatistics::statsPeriod) / 60;

			font->glPrint(box.x1 + 0.12f, box.y1 + 0.07f + (a * 0.135f), 0.8f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, FloatToSmallString(maxy * 0.25f * a));
			font->glFormat(box.x1 + 0.135f + (a * 0.135f), box.y1 + 0.057f, 0.8f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, "%02i:%02i", mins, secs);
		}

		font->glPrint(box.x1 + 0.55f, box.y1 + 0.65f, 0.8f, FONT_SCALE | FONT_NORM | FONT_BUFFERED,                 stats[stat1].name     );
		font->glPrint(box.x1 + 0.55f, box.y1 + 0.63f, 0.8f, FONT_SCALE | FONT_NORM | FONT_BUFFERED, (stat2 != -1) ? stats[stat2].name : "");



		bufferC->SafeAppend({{box.x1 + 0.50f, box.y1 + 0.66f, 0.0f}, {1.0f, 1.0f, 1.0f, 0.8f}});
		bufferC->SafeAppend({{box.x1 + 0.55f, box.y1 + 0.66f, 0.0f}, {1.0f, 1.0f, 1.0f, 0.8f}});

		// no more stippling, minor sacrifice
		// glLineStipple(3, 0x5555);
		bufferC->SafeAppend({{box.x1 + 0.50f, box.y1 + 0.64f, 0.0f}, {1.0f, 1.0f, 1.0f, 0.8f}});
		bufferC->SafeAppend({{box.x1 + 0.55f, box.y1 + 0.64f, 0.0f}, {1.0f, 1.0f, 1.0f, 0.8f}});


		for (int teamNum = 0; teamNum < teamHandler.ActiveTeams(); teamNum++) {
			const CTeam* team = teamHandler.Team(teamNum);

			if (team->gaia)
				continue;

			{
				const std::vector<float>& statValues = stats[stat1].values[teamNum];

				for (size_t a = 0, n = numPoints - 1; a < n; ++a) {
					float v0 = 0.0f;
					float v1 = 0.0f;

					if (dispMode == 1) {
						v0 = statValues[a    ];
						v1 = statValues[a + 1];
					} else if (a > 0) {
						// deltas
						v0 = (statValues[a    ] - statValues[a - 1]) / TeamStatistics::statsPeriod;
						v1 = (statValues[a + 1] - statValues[a    ]) / TeamStatistics::statsPeriod;
					}

					bufferC->SafeAppend({{box.x1 + 0.15f + (a    ) * scalex, box.y1 + 0.08f + v0 * scaley, 0.0f}, team->color});
					bufferC->SafeAppend({{box.x1 + 0.15f + (a + 1) * scalex, box.y1 + 0.08f + v1 * scaley, 0.0f}, team->color});
				}
			}

			if (stat2 != -1) {
				const std::vector<float>& statValues = stats[stat2].values[teamNum];

				// ditto
				// glLineStipple(3, 0x5555);

				for (size_t a = 0, n = numPoints - 1; a < n; ++a) {
					float v0 = 0.0f;
					float v1 = 0.0f;

					if (dispMode == 1) {
						v0 = statValues[a    ];
						v1 = statValues[a + 1];
					} else if (a > 0) {
						v0 = (statValues[a    ] - statValues[a - 1]) / TeamStatistics::statsPeriod;
						v1 = (statValues[a + 1] - statValues[a    ]) / TeamStatistics::statsPeriod;
					}

					bufferC->SafeAppend({{box.x1 + 0.15f + (a    ) * scalex, box.y1 + 0.08f + v0 * scaley, 0.0f}, team->color});
					bufferC->SafeAppend({{box.x1 + 0.15f + (a + 1) * scalex, box.y1 + 0.08f + v1 * scaley, 0.0f}, team->color});
				}
			}
		}

		// draw all lines
		shaderC->Enable();
		bufferC->Submit(GL_LINES);
		shaderC->Disable();
	}

	font->DrawBufferedGL4();
}

std::string CEndGameBox::GetTooltip(int x, int y)
{
	if (!enabled)
		return "";

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
	stats.clear();
	stats.reserve(23);

	stats.emplace_back("");
	stats.emplace_back("Metal used");
	stats.emplace_back("Energy used");
	stats.emplace_back("Metal produced");
	stats.emplace_back("Energy produced");

	stats.emplace_back("Metal excess");
	stats.emplace_back("Energy excess");

	stats.emplace_back("Metal received");
	stats.emplace_back("Energy received");

	stats.emplace_back("Metal sent");
	stats.emplace_back("Energy sent");

	stats.emplace_back("Metal stored");
	stats.emplace_back("Energy stored");

	stats.emplace_back("Active Units");
	stats.emplace_back("Units killed");

	stats.emplace_back("Units produced");
	stats.emplace_back("Units died");

	stats.emplace_back("Units received");
	stats.emplace_back("Units sent");
	stats.emplace_back("Units captured");
	stats.emplace_back("Units stolen");

	stats.emplace_back("Damage Dealt");
	stats.emplace_back("Damage Received");

	for (int team = 0; team < teamHandler.ActiveTeams(); team++) {
		const CTeam* pteam = teamHandler.Team(team);

		if (pteam->gaia)
			continue;

		for (const auto& si: pteam->statHistory) {
			stats[ 0].AddStat(team, 0);

			stats[ 1].AddStat(team, si.metalUsed);
			stats[ 2].AddStat(team, si.energyUsed);
			stats[ 3].AddStat(team, si.metalProduced);
			stats[ 4].AddStat(team, si.energyProduced);

			stats[ 5].AddStat(team, si.metalExcess);
			stats[ 6].AddStat(team, si.energyExcess);

			stats[ 7].AddStat(team, si.metalReceived);
			stats[ 8].AddStat(team, si.energyReceived);

			stats[ 9].AddStat(team, si.metalSent);
			stats[10].AddStat(team, si.energySent);

			stats[11].AddStat(team, si.metalProduced + si.metalReceived - (si.metalUsed + si.metalSent+si.metalExcess) );
			stats[12].AddStat(team, si.energyProduced + si.energyReceived - (si.energyUsed + si.energySent+si.energyExcess) );

			stats[13].AddStat(team, si.unitsProduced + si.unitsReceived + si.unitsCaptured - (si.unitsDied + si.unitsSent + si.unitsOutCaptured));
			stats[14].AddStat(team, si.unitsKilled);

			stats[15].AddStat(team, si.unitsProduced);
			stats[16].AddStat(team, si.unitsDied);

			stats[17].AddStat(team, si.unitsReceived);
			stats[18].AddStat(team, si.unitsSent);
			stats[19].AddStat(team, si.unitsCaptured);
			stats[20].AddStat(team, si.unitsOutCaptured);

			stats[21].AddStat(team, si.damageDealt);
			stats[22].AddStat(team, si.damageReceived);
		}
	}
}
