/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ShareBox.h"

#include "MouseHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/Player.h"
#include "Game/PlayerHandler.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/NetProtocol.h"
#include "System/MsgStrings.h"

#include <SDL_keysym.h>


#define MAX_SHARE_TEAMS (teamHandler->ActiveTeams() - 1)
int CShareBox::lastShareTeam = 0;

CShareBox::CShareBox()
{
	box.x1 = 0.34f;
	box.y1 = 0.18f;
	box.x2 = 0.66f;
	box.y2 = 0.82f;

	okBox.x1 = 0.22f;
	okBox.y1 = 0.02f;
	okBox.x2 = 0.30f;
	okBox.y2 = 0.06f;

	applyBox.x1 = 0.12f;
	applyBox.y1 = 0.02f;
	applyBox.x2 = 0.20f;
	applyBox.y2 = 0.06f;

	cancelBox.x1 = 0.02f;
	cancelBox.y1 = 0.02f;
	cancelBox.x2 = 0.10f;
	cancelBox.y2 = 0.06f;

	teamBox.x1 = 0.02f;
	teamBox.y1 = 0.25f;
	teamBox.x2 = 0.30f;
	teamBox.y2 = 0.62f;

	scrollbarBox.x1 = 0.28f;
	scrollbarBox.y1 = 0.25f;
	scrollbarBox.x2 = 0.30f;
	scrollbarBox.y2 = 0.62f;

	scrollBox.x1 = 0.28f;
	scrollBox.y1 = 0.59f;
	scrollBox.x2 = 0.30f;
	scrollBox.y2 = 0.62f;

	unitBox.x1 = 0.01f;
	unitBox.y1 = 0.07f;
	unitBox.x2 = 0.05f;
	unitBox.y2 = 0.12f;

	energyBox.x1 = 0.02f;
	energyBox.y1 = 0.145f;
	energyBox.x2 = 0.30f;
	energyBox.y2 = 0.155f;

	metalBox.x1 = 0.02f;
	metalBox.y1 = 0.205f;
	metalBox.x2 = 0.30f;
	metalBox.y2 = 0.215f;

	metalShare  = 0.0f;
	energyShare = 0.0f;
	shareUnits  = false;
	moveBox     = false;
	metalMove   = false;
	energyMove  = false;

	// find a default team to share to that is not gu->myTeam and is not dead.
	shareTeam = lastShareTeam;

	startTeam = 0;
	numTeamsDisp = (int)((teamBox.y2 - teamBox.y1) / 0.025f);
	scrolling = false;
	scrollGrab = 0.0f;
	hasScroll = MAX_SHARE_TEAMS > numTeamsDisp;

	while (shareTeam == gu->myTeam || teamHandler->Team(shareTeam)->isDead) {
		++shareTeam;

		// wrap around
		if (shareTeam >= teamHandler->ActiveTeams()) {
			shareTeam = 0;
		}

		// we're back at the start, so there are no teams alive...
		// (except possibly gu->myTeam)
		if (shareTeam == lastShareTeam) {
			shareTeam = -1;
			break;
		}
	}
}

CShareBox::~CShareBox()
{
}

void CShareBox::Draw()
{
	const float alpha = std::max(guiAlpha, 0.4f);

	const float mx = MouseX(mouse->lastx);
	const float my = MouseY(mouse->lasty);

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	// Large Box
	glColor4f(0.2f, 0.2f, 0.2f, alpha);
	DrawBox(box);

	// ok Box on mouse over
	if (InBox(mx, my, box + okBox)) {
		glColor4f(0.7f, 0.2f, 0.2f, alpha);
		DrawBox(box + okBox);
	}

	// apply Box on mouse over
	if (InBox(mx, my, box + applyBox)) {
		glColor4f(0.7f, 0.2f, 0.2f, alpha);
		DrawBox(box + applyBox);
	}

	// cancel Box on mouse over
	if (InBox(mx, my, box + cancelBox)) {
		glColor4f(0.7f,0.2f,0.2f,alpha);
		DrawBox(box + cancelBox);
	}

	glColor4f(0.2f, 0.2f, 0.2f, alpha);
	DrawBox(box + teamBox);

	if (hasScroll) {
		glColor4f(0.1f, 0.1f, 0.1f, alpha);
		DrawBox(box + scrollbarBox);

		float sz = scrollbarBox.y2 - scrollbarBox.y1;
		float tsz = sz / (float)(MAX_SHARE_TEAMS);
		float psz = tsz * (float)numTeamsDisp;

		scrollBox.y2 = scrollbarBox.y2 - startTeam * tsz;
		scrollBox.y1 = scrollBox.y2 - psz;

		glColor4f(0.8f, 0.8f, 0.8f, alpha);
		DrawBox(box + scrollBox);
	}

	if (InBox(mx, my, box + unitBox)) {
		glColor4f(0.7f, 0.2f, 0.2f, alpha);
	} else {
		glColor4f(0.2f, 0.2f, 0.2f, alpha);
	}
	
	DrawBox(box + unitBox);

	glColor4f(0.8f, 0.8f, 0.9f, 0.7f);
	DrawBox(box + metalBox);

	glColor4f(0.9f, 0.9f, 0.2f, 0.7f);
	DrawBox(box + energyBox);

	//draw share indicators in metal/energy bars
	glColor4f(0.9f, 0.2f, 0.2f, 0.7f);
	ContainerBox metalShareBox;
	metalShareBox.x1 = metalBox.x1 + metalShare * (metalBox.x2 - metalBox.x1) - 0.005f;
	metalShareBox.x2 = metalShareBox.x1 + 0.01f;
	metalShareBox.y1 = metalBox.y1 - 0.005f;
	metalShareBox.y2 = metalBox.y2 + 0.005f;
	DrawBox(box + metalShareBox);
	ContainerBox energyShareBox;
	energyShareBox.x1 = energyBox.x1 + energyShare * (energyBox.x2 - energyBox.x1) - 0.005f;
	energyShareBox.x2 = energyShareBox.x1 + 0.01f;
	energyShareBox.y1 = energyBox.y1 - 0.005f;
	energyShareBox.y2 = energyBox.y2 + 0.005f;
	DrawBox(box + energyShareBox);

	// show that share units is selected
	if (shareUnits) {
		//DrawBox(box + unitBox);
		glLineWidth(3);
		glBegin(GL_LINE_STRIP);
			glVertex2f(box.x1 + unitBox.x1 + 0.01f, box.y1 + unitBox.y1 + 0.025f);
			glVertex2f(box.x1 + unitBox.x1 + 0.02f, box.y1 + unitBox.y1 + 0.01f);
			glVertex2f(box.x1 + unitBox.x1 + 0.03f, box.y1 + unitBox.y1 + 0.04f);
		glEnd();
		glLineWidth(1);
	}

	font->Begin();

	font->glPrint(box.x1 + (okBox.x1 + okBox.x2) * 0.5f, box.y1 + (okBox.y1 + okBox.y2) * 0.5f, 1, FONT_CENTER | FONT_VCENTER | FONT_SCALE | FONT_NORM, "Ok");
	font->glPrint(box.x1 + (applyBox.x1 + applyBox.x2) * 0.5f, box.y1 + (applyBox.y1 + applyBox.y2) * 0.5f, 1, FONT_CENTER | FONT_VCENTER | FONT_SCALE | FONT_NORM, "Apply");
	font->glPrint(box.x1 + (cancelBox.x1 + cancelBox.x2) * 0.5f, box.y1 + (cancelBox.y1 + cancelBox.y2) * 0.5f, 1, FONT_CENTER | FONT_VCENTER | FONT_SCALE | FONT_NORM, "Cancel");

	font->glPrint(box.x1 + 0.06f, box.y1 + 0.085f, 0.7f, FONT_SCALE | FONT_NORM, "Share selected units");

	font->SetTextColor(1, 1, 0.4f, 0.8f);
	font->glPrint(box.x1 + 0.01f, box.y1 + 0.16f, 0.7f, FONT_SCALE | FONT_NORM, "Share Energy");

	font->SetTextColor(1, 1, 1, 0.8f);
	font->glFormat(box.x1 + 0.25f, box.y1 + 0.12f, 0.7f, FONT_SCALE | FONT_NORM, "%.0f", float(teamHandler->Team(gu->myTeam)->energy));
	font->glFormat(box.x1 + 0.14f, box.y1 + 0.12f, 0.7f, FONT_SCALE | FONT_NORM, "%.0f", teamHandler->Team(gu->myTeam)->energy * energyShare);

	font->SetTextColor(0.8f, 0.8f, 0.9f, 0.8f);
	font->glPrint(box.x1 + 0.01f, box.y1 + 0.22f, 0.7f, FONT_SCALE | FONT_NORM, "Share Metal");

	font->SetTextColor(1, 1, 1, 0.8f);
	font->glFormat(box.x1 + 0.25f, box.y1 + 0.18f, 0.7f, FONT_SCALE | FONT_NORM, "%.0f", float(teamHandler->Team(gu->myTeam)->metal));
	font->glFormat(box.x1 + 0.14f, box.y1 + 0.18f, 0.7f, FONT_SCALE | FONT_NORM, "%.0f", teamHandler->Team(gu->myTeam)->metal * metalShare);

	int teamPos = 0;
	for(int team = startTeam; team < MAX_SHARE_TEAMS && teamPos < numTeamsDisp; ++team, ++teamPos) {
		int actualTeam = team;
		if (team >= gu->myTeam) {
			actualTeam++;
		}
		const float alpha = (shareTeam == actualTeam) ? 0.8f : 0.4f;
		std::string teamName;

		if (teamHandler->Team(actualTeam)->leader >= 0) {
			teamName = playerHandler->Player(teamHandler->Team(actualTeam)->leader)->name;
		} else {
			teamName = UncontrolledPlayerName;
		}

		std::string ally, dead;
		if (teamHandler->Ally(gu->myAllyTeam, teamHandler->AllyTeam(actualTeam))) {
			font->SetTextColor(0.5f, 1.0f, 0.5f, alpha);
			ally = " <Ally>";
		} else {
			font->SetTextColor(1.0f, 0.5f, 0.5f, alpha);
			ally = " <Enemy>";
		}
		if (teamHandler->Team(actualTeam)->isDead) {
			font->SetTextColor(0.5f, 0.5f, 1.0f, alpha);
			dead = " <Dead>";
		}
		if (actualTeam == teamHandler->GaiaTeamID()) {
			font->SetTextColor(0.8f, 0.8f, 0.8f, alpha);
			teamName = "Gaia";
			ally   = " <Gaia>";
		}
		font->glFormat(box.x1 + teamBox.x1 + 0.002f,
		                box.y1 + teamBox.y2 - 0.025f - teamPos * 0.025f,
		                0.7f, FONT_SCALE | FONT_NORM, "Team%i (%s)%s%s", actualTeam,
		                teamName.c_str(), ally.c_str(), dead.c_str());
	}

	font->End();

	glEnable(GL_TEXTURE_2D);
}

bool CShareBox::IsAbove(int x, int y)
{
	const float mx = MouseX(x);
	const float my = MouseY(y);

	return InBox(mx, my, box);
}

std::string CShareBox::GetTooltip(int x, int y)
{
	const float mx = MouseX(x);
	const float my = MouseY(y);

	if (InBox(mx, my, box + okBox)) {
		return "Shares the selected stuff and close dialog";
	} else if (InBox(mx, my, box + applyBox)) {
		return "Shares the selected stuff";
	} else if (InBox(mx, my, box + cancelBox)) {
		return "Close this dialog without sharing";
	} else if (InBox(mx, my, box + unitBox)) {
		return "Toggles if you want to share your\ncurrently selected units";
	} else if (InBox(mx, my, box + metalBox)) {
		return "Click here to select how much metal to share";
	} else if (InBox(mx, my, box + energyBox)) {
		return "Click here to select how much energy to share";
	} else if (hasScroll && InBox(mx, my, box + scrollBox)) {
		return "Scroll the team list here";
	} else if (hasScroll && InBox(mx, my, box + scrollbarBox)) {
		return "Scroll the team list here";
	} else if (InBox(mx, my, box + teamBox)) {
		return "Select which team to share to";
	} else if (InBox(mx, my, box)) {
		return " ";
	} else {
		return "";
	}
}

bool CShareBox::MousePress(int x, int y, int button)
{
	const float mx = MouseX(x);
	const float my = MouseY(y);

	if (InBox(mx, my, box)) {
		moveBox = true;
		if (InBox(mx, my, box + okBox) ||
				InBox(mx, my, box + applyBox) ||
				InBox(mx, my, box + cancelBox) ||
				InBox(mx, my, box + unitBox) ||
				InBox(mx, my, box + metalBox) ||
				InBox(mx, my, box + energyBox) ||
				InBox(mx, my, box + teamBox) ||
				InBox(mx, my, box + scrollbarBox) ||
				InBox(mx, my, box + scrollBox)) {
			moveBox = false;
		}
		if (InBox(mx, my, box + metalBox)) {
			metalMove = true;
			metalShare = std::max(0.f, std::min(1.f, (mx-box.x1-metalBox.x1)/(metalBox.x2-metalBox.x1)));
		}
		if (InBox(mx, my, box + energyBox)) {
			energyMove = true;
			energyShare = std::max(0.f, std::min(1.f, (mx-box.x1-energyBox.x1)/(energyBox.x2-energyBox.x1)));
		}
		if (hasScroll && InBox(mx, my, box + scrollBox)) {
			scrolling = true;
			scrollGrab = (box + scrollBox).y2 - my;
		}
		else if (hasScroll && InBox(mx, my, box + scrollbarBox)) {
			if(my < (box + scrollBox).y1)
				*(volatile int *)&startTeam = startTeam + std::min(MAX_SHARE_TEAMS - numTeamsDisp - startTeam, numTeamsDisp);
			if(my > (box + scrollBox).y2)
				*(volatile int *)&startTeam = startTeam - std::min(startTeam, numTeamsDisp);
		}
		else if (InBox(mx, my, box + teamBox)) {
			int team = startTeam + (int)((box.y1 + teamBox.y2-my)/0.025f);
			if (team >= gu->myTeam) {
				team++;
			}
			if (team < teamHandler->ActiveTeams() && !teamHandler->Team(team)->isDead) {
				shareTeam = team;
			}
		}
		return true;
	}
	return false;
}

void CShareBox::MouseRelease(int x, int y, int button)
{
	const float mx = MouseX(x);
	const float my = MouseY(y);
	scrolling = false;
	scrollGrab = 0.0f;

	if (InBox(mx, my, box + unitBox)) {
		shareUnits = !shareUnits;
	}
	if ((InBox(mx, my, box + okBox) || InBox(mx, my, box + applyBox)) &&
			 shareTeam != -1 && !teamHandler->Team(shareTeam)->isDead && !teamHandler->Team(gu->myTeam)->isDead) {
		if (shareUnits) {
			Command c(CMD_STOP);
			// make sure the units are stopped and that the selection is transmitted
			selectedUnitsHandler.GiveCommand(c, false);
		}
		net->Send(CBaseNetProtocol::Get().SendShare(gu->myPlayerNum, shareTeam, shareUnits, metalShare * teamHandler->Team(gu->myTeam)->metal, energyShare * teamHandler->Team(gu->myTeam)->energy));
		if (shareUnits) {
			selectedUnitsHandler.ClearSelected();
		}
		lastShareTeam = shareTeam;
	}
	if (InBox(mx, my, box + okBox) || InBox(mx, my, box + cancelBox)) {
		delete this;
		return;
	}
	moveBox    = false;
	metalMove  = false;
	energyMove = false;
}

void CShareBox::MouseMove(int x, int y, int dx, int dy, int button)
{
	const float mx = MouseX(x);
	const float my = MouseY(y);

	if(scrolling) {
		float scr = (box+scrollbarBox).y2 - (my + scrollGrab);
		float sz = scrollbarBox.y2 - scrollbarBox.y1;
		float tsz = sz / (float)MAX_SHARE_TEAMS;

		*(volatile int *)&startTeam = std::max(0, std::min((int)(scr / tsz + 0.5), MAX_SHARE_TEAMS - numTeamsDisp));
		return;
	}

	if (moveBox) {
		box.x1 += MouseMoveX(dx);
		box.x2 += MouseMoveX(dx);
		box.y1 += MouseMoveY(dy);
		box.y2 += MouseMoveY(dy);
	}
	if (metalMove) {
		metalShare  = std::max(0.0f, std::min(1.0f, (mx - box.x1 - metalBox.x1)  / (metalBox.x2 - metalBox.x1)));
	}
	if (energyMove) {
		energyShare = std::max(0.0f, std::min(1.0f, (mx - box.x1 - energyBox.x1) / (energyBox.x2 - energyBox.x1)));
	}
	if (!(hasScroll && (InBox(mx, my, box + scrollBox) || InBox(mx, my, box + scrollbarBox))) && InBox(mx, my, box + teamBox)) {
		int team = startTeam + (int) ((box.y1 + teamBox.y2 - my) / 0.025f);
		if (team >= gu->myTeam) {
			team++;
		}
		if (team < teamHandler->ActiveTeams() && !teamHandler->Team(team)->isDead) {
			shareTeam = team;
		}
	}
}

bool CShareBox::KeyPressed(unsigned short key, bool isRepeat)
{
	if (key == SDLK_ESCAPE) {
		delete this;
		return true;
	}
	return false;
}




