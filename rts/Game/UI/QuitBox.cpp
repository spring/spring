/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "QuitBox.h"

#include <cmath>

#include "MouseHandler.h"
#include "Game/Game.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/glExtra.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/Log/ILog.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/TimeUtil.h"
#include "System/MsgStrings.h"

#include <SDL_keycode.h>

#define MAX_QUIT_TEAMS (teamHandler.ActiveTeams() - 1)

#undef CreateDirectory

CQuitBox::CQuitBox()
{
	box.x1 = 0.34f;
	box.y1 = 0.18f;
	box.x2 = 0.66f;
	box.y2 = 0.84f;

	// sorted by y-value (bottom to top)
	cancelBox.x1 = 0.02f;
	cancelBox.y1 = 0.02f;
	cancelBox.x2 = 0.30f;
	cancelBox.y2 = 0.06f;

	quitBox.x1 = 0.02f;
	quitBox.y1 = cancelBox.y2;
	quitBox.x2 = 0.30f;
	quitBox.y2 = 0.10f;

	menuBox.x1 = 0.02f;
	menuBox.y1 = quitBox.y2;
	menuBox.x2 = 0.30f;
	menuBox.y2 = 0.14f;

	teamBox.x1 = 0.02f;
	teamBox.y1 = menuBox.y2 + 0.01f;
	teamBox.x2 = 0.30f;
	teamBox.y2 = 0.43f;

	giveAwayBox.x1 = 0.02f;
	giveAwayBox.y1 = teamBox.y2 + 0.01f;
	giveAwayBox.x2 = 0.30f;
	giveAwayBox.y2 = 0.48f;

	saveBox.x1 = 0.02f;
	saveBox.y1 = giveAwayBox.y2;
	saveBox.x2 = 0.30f;
	saveBox.y2 = 0.53f;

	resignBox.x1 = 0.02f;
	resignBox.y1 = saveBox.y2;
	resignBox.x2 = 0.30f;
	resignBox.y2 = 0.57f;


	scrollBox.x1 = 0.28f;
	scrollBox.y1 = 0.40f;
	scrollBox.x2 = 0.30f;
	scrollBox.y2 = 0.43f;

	scrollbarBox.x1 = 0.28f;
	scrollbarBox.y1 = 0.11f;
	scrollbarBox.x2 = 0.30f;
	scrollbarBox.y2 = 0.43f;


	startTeam = 0;
	numTeamsDisp = (int)((teamBox.y2 - teamBox.y1) / 0.025f);
	scrolling = false;
	scrollGrab = 0.0f;
	hasScroll = MAX_QUIT_TEAMS > numTeamsDisp;

	moveBox = false;
	noAlliesLeft = true;
	shareTeam = 0;

	// if we have alive allies left, set the shareteam to an undead ally.
	for (int team = 0; team < teamHandler.ActiveTeams(); ++team) {
		if (team == gu->myTeam)
			continue;
		if (teamHandler.Team(team)->gaia)
			continue;
		if (teamHandler.Team(team)->isDead)
			continue;

		if (shareTeam == gu->myTeam || teamHandler.Team(shareTeam)->isDead)
			shareTeam = team;
		if (teamHandler.Ally(gu->myAllyTeam, teamHandler.AllyTeam(team))) {
			noAlliesLeft = false;
			shareTeam = team;
			break;
		}
	}
}

void CQuitBox::Draw()
{
	const float mx = MouseX(mouse->lastx);
	const float my = MouseY(mouse->lasty);

	const TRectangle<float> boxes[] = {resignBox, saveBox, giveAwayBox, cancelBox, menuBox, quitBox};

	GL::RenderDataBufferC* buffer = GL::GetRenderBufferC();
	Shader::IProgramObject* shader = buffer->GetShader();

	glAttribStatePtr->EnableBlendMask();

	{
		// draw the background box
		gleDrawQuadC(box, SColor{0.2f, 0.2f, 0.2f, guiAlpha}, buffer);

		// draw the sub-box we are in
		for (size_t i = 0, n = sizeof(boxes) / sizeof(boxes[0]); i < n; i++) {
			if (InBox(mx, my, box + boxes[i])) {
				gleDrawQuadC(box + boxes[i], SColor{0.7f, 0.2f, 0.2f, guiAlpha}, buffer);
				break;
			}
		}

		gleDrawQuadC(box + teamBox, SColor{0.2f, 0.2f, 0.2f, guiAlpha}, buffer);
	}

	if (hasScroll) {
		gleDrawQuadC(box + scrollbarBox, SColor{0.1f, 0.1f, 0.1f, guiAlpha}, buffer);

		const float  sz = scrollbarBox.y2 - scrollbarBox.y1;
		const float tsz =  sz / float(MAX_QUIT_TEAMS);
		const float psz = tsz * float(numTeamsDisp);

		scrollBox.y2 = scrollbarBox.y2 - startTeam * tsz;
		scrollBox.y1 = scrollBox.y2 - psz;

		gleDrawQuadC(box + scrollBox, SColor{0.8f, 0.8f, 0.8f, guiAlpha}, buffer);
	}

	{
		shader->Enable();
		shader->SetUniformMatrix4x4<float>("u_movi_mat", false, CMatrix44f::Identity());
		shader->SetUniformMatrix4x4<float>("u_proj_mat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));
		buffer->Submit(GL_TRIANGLES);
		shader->Disable();
	}


	font->SetTextColor(1.0f, 1.0f, 0.4f, 0.8f);
	font->glPrint(box.x1 + 0.045f, box.y1 + 0.58f, 0.7f, FONT_VCENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Do you want to ...");
	font->SetTextColor(1.0f, 1.0f, 1.0f, 0.8f);

	font->glPrint(box.x1 +   resignBox.x1 + 0.025f, box.y1 + (  resignBox.y1 +   resignBox.y2) / 2, 1, FONT_VCENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Resign");
	font->glPrint(box.x1 +     saveBox.x1 + 0.025f, box.y1 + (    saveBox.y1 +     saveBox.y2) / 2, 1, FONT_VCENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Save");
	font->glPrint(box.x1 + giveAwayBox.x1 + 0.025f, box.y1 + (giveAwayBox.y1 + giveAwayBox.y2) / 2, 1, FONT_VCENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Give everything to ...");
	font->glPrint(box.x1 +   cancelBox.x1 + 0.025f, box.y1 + (  cancelBox.y1 +   cancelBox.y2) / 2, 1, FONT_VCENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Cancel");
	font->glPrint(box.x1 +     menuBox.x1 + 0.025f, box.y1 + (    menuBox.y1 +     menuBox.y2) / 2, 1, FONT_VCENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Quit To Menu");
	font->glPrint(box.x1 +     quitBox.x1 + 0.025f, box.y1 + (    quitBox.y1 +     quitBox.y2) / 2, 1, FONT_VCENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, "Quit To System");

	for (int teamNum = startTeam, teamPos = 0; teamNum < MAX_QUIT_TEAMS && teamPos < numTeamsDisp; ++teamNum, ++teamPos) {
		const int actualTeamNum = teamNum + int(teamNum >= gu->myTeam);

		const CTeam* team = teamHandler.Team(actualTeamNum);

		if (team->gaia)
			continue;

		const char* name = team->GetControllerName();
		const char* ally = "";
		const char* dead = "";

		if (teamHandler.Ally(gu->myAllyTeam, teamHandler.AllyTeam(actualTeamNum))) {
			ally = " <Ally>";
		} else {
			ally = " <Enemy>";
		}
		if (team->isDead)
			dead = " <Dead>";

		if (actualTeamNum == teamHandler.GaiaTeamID()) {
			name = "Gaia";
			ally = " <Gaia>";
		}

		font->SetTextColor(1.0f, 1.0f, 1.0f, 0.4f + 0.4f * (shareTeam == actualTeamNum));
		font->glFormat(
			box.x1 + teamBox.x1 + 0.002f,
			box.y1 + teamBox.y2 - 0.025f - teamPos * 0.025f,
			0.7f,
			FONT_SCALE | FONT_NORM | FONT_BUFFERED,
			"Team %02i (%s)%s%s",
			actualTeamNum,
			name,
			ally,
			dead
		);
	}

	font->DrawBufferedGL4();
}

bool CQuitBox::IsAbove(int x, int y)
{
	return (InBox(MouseX(x), MouseY(y), box));
}

std::string CQuitBox::GetTooltip(int x, int y)
{
	const float mx = MouseX(x);
	const float my = MouseY(y);

	const TRectangle<float> boxes[] = {
		resignBox, saveBox, giveAwayBox, scrollBox, scrollbarBox, teamBox, cancelBox, menuBox, quitBox
	};
	const char* toolTips[] = {
		"Resign the match; remain in the game",
		"Save the current game state to a file \nfor later reload",
		"Give away all units and resources \nto the team specified below",
		"Scroll the team list here",
		"Scroll the team list here",
		"Select which team receives everything",
		"Return to the game",
		"Return to menu; quit the game",
		"Return to system; quit the game",
	};

	const char* toolTip = "";

	for (size_t i = 0, n = sizeof(boxes) / sizeof(boxes[0]); i < n; i++) {
		if (!InBox(mx, my, box + boxes[i]))
			continue;

		toolTip = toolTips[i];
		break;
	}

	return toolTip;
}

bool CQuitBox::MousePress(int x, int y, int button)
{
	const float mx = MouseX(x);
	const float my = MouseY(y);

	const TRectangle<float> boxes[] = {resignBox, saveBox, giveAwayBox, teamBox, cancelBox, menuBox, quitBox, scrollbarBox, scrollBox};

	if (!InBox(mx, my, box))
		return false;

	moveBox = true;

	for (size_t i = 0, n = sizeof(boxes) / sizeof(boxes[0]); i < n; i++) {
		if (!InBox(mx, my, box + boxes[i]))
			continue;

		moveBox = false;
		break;
	}

	if (hasScroll && InBox(mx, my, box + scrollBox)) {
		scrolling = true;
		scrollGrab = (box + scrollBox).y2 - my;
		return true;
	}
	if (hasScroll && InBox(mx, my, box + scrollbarBox)) {
		if (my < (box + scrollBox).y1)
			*(volatile int*) &startTeam = startTeam + std::min(MAX_QUIT_TEAMS - numTeamsDisp - startTeam, numTeamsDisp);
		if (my > (box + scrollBox).y2)
			*(volatile int*) &startTeam = startTeam - std::min(startTeam, numTeamsDisp);

		return true;
	}

	if (!InBox(mx, my, box + teamBox))
		return true;

	const int teamIdx = startTeam + (box.y1 + teamBox.y2 - my) / 0.025f;
	const int teamNum = teamIdx + (teamIdx >= gu->myTeam);

	if (teamHandler.IsValidTeam(teamNum) && !teamHandler.Team(teamNum)->isDead) {
		// we don't want to give everything to the enemy if there are allies left
		if (noAlliesLeft || (!noAlliesLeft && teamHandler.Ally(gu->myAllyTeam, teamHandler.AllyTeam(teamNum))))
			shareTeam = teamNum;
	}

	return true;
}

void CQuitBox::MouseRelease(int x, int y, int button)
{
	const float mx = MouseX(x);
	const float my = MouseY(y);

	scrolling = false;
	scrollGrab = 0.0f;

	const CTeam* localTeam = teamHandler.Team(gu->myTeam);
	const CTeam* recipTeam = teamHandler.Team(shareTeam);
	const CPlayer* localPlayer = playerHandler.Player(gu->myPlayerNum);

	const bool resign = InBox(mx, my, box + resignBox);
	const bool   save = InBox(mx, my, box + saveBox);
	const bool   give = InBox(mx, my, box + giveAwayBox);

	if (resign || (save && !localTeam->isDead) || (give && !recipTeam->isDead && !localTeam->isDead)) {
		// give away all units (and resources)
		if (give && !localPlayer->spectator)
			clientNet->Send(CBaseNetProtocol::Get().SendGiveAwayEverything(gu->myPlayerNum, shareTeam, localPlayer->team));

		// resign, so self-d all units
		if (resign && !localPlayer->spectator)
			clientNet->Send(CBaseNetProtocol::Get().SendResign(gu->myPlayerNum));

		// save current game state
		if (save) {
			const std::string currTimeStr = std::move(CTimeUtil::GetCurrentTimeStr());
			const std::string saveFileName = currTimeStr + "_" + modInfo.filename + "_" + gameSetup->mapName;

			game->Save("Saves/" + saveFileName + ".ssf", "");
		}
	}
	else if (InBox(mx, my, box + menuBox)) {
		LOG("[QuitBox] user exited to menu");

		// signal SpringApp
		gameSetup->reloadScript = "";
		gu->globalReload = true;
	}
	else if (InBox(mx, my, box + quitBox)) {
		LOG("[QuitBox] user exited to system");
		gu->globalQuit = true;
	}

	// if we're still in the game, remove the QuitBox
	const TRectangle<float> boxes[] = {resignBox, saveBox, giveAwayBox, cancelBox, menuBox, quitBox};

	for (size_t i = 0, n = sizeof(boxes) / sizeof(boxes[0]); i < n; i++) {
		if (!InBox(mx, my, box + boxes[i]))
			continue;

		delete this;
		return;
	}

	moveBox = false;
}

void CQuitBox::MouseMove(int x, int y, int dx, int dy, int button)
{
	const float mx = MouseX(x);
	const float my = MouseY(y);

	if (scrolling) {
		const float scr = (box + scrollbarBox).y2 - (my + scrollGrab);
		const float sz = scrollbarBox.y2 - scrollbarBox.y1;
		const float tsz = sz / float(MAX_QUIT_TEAMS);

		// ??
		*(volatile int*) &startTeam = std::max(0, std::min((int)std::lround(scr / tsz), MAX_QUIT_TEAMS - numTeamsDisp));
		return;
	}

	if (moveBox) {
		box.x1 += MouseMoveX(dx);
		box.x2 += MouseMoveX(dx);
		box.y1 += MouseMoveY(dy);
		box.y2 += MouseMoveY(dy);
	}

	if (hasScroll)
		return;
	if (!InBox(mx, my, box + teamBox))
		return;
	if (InBox(mx, my, box + scrollBox) || InBox(mx, my, box + scrollbarBox))
		return;

	int team = startTeam + (int)((box.y1 + teamBox.y2 - my) / 0.025f);

	if (team >= gu->myTeam)
		team++;

	if (teamHandler.IsValidTeam(team) && !teamHandler.Team(team)->isDead) {
		// we don't want to give everything to the enemy if there are allies left
		if (noAlliesLeft || (!noAlliesLeft && teamHandler.Ally(gu->myAllyTeam, teamHandler.AllyTeam(team)))) {
			shareTeam=team;
		}
	}
}


bool CQuitBox::KeyPressed(int key, bool isRepeat)
{
	if (key == SDLK_ESCAPE) {
		delete this;
		return true;
	}

	return false;
}
