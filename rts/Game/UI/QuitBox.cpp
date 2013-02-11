/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "QuitBox.h"

#include "MouseHandler.h"
#include "Game/Player.h"
#include "Game/PlayerHandler.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/Log/ILog.h"
#include "System/NetProtocol.h"
#include "System/TimeUtil.h"
#include "System/FileSystem/FileSystem.h"
#include "System/LoadSave/LoadSaveHandler.h"
#include "System/MsgStrings.h"

#include <SDL_keysym.h>


#define MAX_QUIT_TEAMS (teamHandler->ActiveTeams() - 1)

#undef CreateDirectory

CQuitBox::CQuitBox()
{
	box.x1 = 0.34f;
	box.y1 = 0.18f;
	box.x2 = 0.66f;
	box.y2 = 0.78f;

	resignBox.x1=0.02f;
	resignBox.y1=0.53f;
	resignBox.x2=0.30f;
	resignBox.y2=0.57f;

	saveBox.x1=0.02f;
	saveBox.y1=0.49f;
	saveBox.x2=0.30f;
	saveBox.y2=0.53f;

	giveAwayBox.x1=0.02f;
	giveAwayBox.y1=0.44f;
	giveAwayBox.x2=0.30f;
	giveAwayBox.y2=0.48f;

	teamBox.x1=0.02f;
	teamBox.y1=0.11f;
	teamBox.x2=0.30f;
	teamBox.y2=0.43f;

	scrollbarBox.x1 = 0.28f;
	scrollbarBox.y1 = 0.11f;
	scrollbarBox.x2 = 0.30f;
	scrollbarBox.y2 = 0.43f;

	scrollBox.x1 = 0.28f;
	scrollBox.y1 = 0.40f;
	scrollBox.x2 = 0.30f;
	scrollBox.y2 = 0.43f;

	quitBox.x1=0.02f;
	quitBox.y1=0.06f;
	quitBox.x2=0.30f;
	quitBox.y2=0.10f;

	cancelBox.x1=0.02f;
	cancelBox.y1=0.02f;
	cancelBox.x2=0.30f;
	cancelBox.y2=0.06f;

	startTeam = 0;
	numTeamsDisp = (int)((teamBox.y2 - teamBox.y1) / 0.025f);
	scrolling = false;
	scrollGrab = 0.0f;
	hasScroll = MAX_QUIT_TEAMS > numTeamsDisp;

	moveBox=false;
	noAlliesLeft=true;
	shareTeam=0;
	// if we have alive allies left, set the shareteam to an undead ally.
	for(int team=0;team<teamHandler->ActiveTeams();++team){
		if (teamHandler->Team(team)->gaia) continue;
		if(team!=gu->myTeam && !teamHandler->Team(team)->isDead)
		{
			if(shareTeam==gu->myTeam || teamHandler->Team(shareTeam)->isDead)
				shareTeam=team;
			if(teamHandler->Ally(gu->myAllyTeam, teamHandler->AllyTeam(team))){
				noAlliesLeft=false;
				shareTeam=team;
				break;
			}
		}
	}
}

CQuitBox::~CQuitBox()
{
}

void CQuitBox::Draw()
{
	float mx=MouseX(mouse->lastx);
	float my=MouseY(mouse->lasty);

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	// Large Box
	glColor4f(0.2f,0.2f,0.2f,guiAlpha);
	DrawBox(box);

	// resign Box on mouse over
	if(InBox(mx,my,box+resignBox)){
		glColor4f(0.7f,0.2f,0.2f,guiAlpha);
		DrawBox(box+resignBox);
	}
	// save Box on mouse over
	if(InBox(mx,my,box+saveBox)){
		glColor4f(0.7f,0.2f,0.2f,guiAlpha);
		DrawBox(box+saveBox);
	}
	// give away Box on mouse over
	if(InBox(mx,my,box+giveAwayBox)){
		glColor4f(0.7f,0.2f,0.2f,guiAlpha);
		DrawBox(box+giveAwayBox);
	}
	// cancel Box on mouse over
	if(InBox(mx,my,box+cancelBox)){
		glColor4f(0.7f,0.2f,0.2f,guiAlpha);
		DrawBox(box+cancelBox);
	}
	// quit Box on mouse over
	if(InBox(mx,my,box+quitBox)){
		glColor4f(0.7f,0.2f,0.2f,guiAlpha);
		DrawBox(box+quitBox);
	}


	glColor4f(0.2f,0.2f,0.2f,guiAlpha);
	DrawBox(box+teamBox);

	if (hasScroll) {
		glColor4f(0.1f, 0.1f, 0.1f, guiAlpha);
		DrawBox(box + scrollbarBox);

		float sz = scrollbarBox.y2 - scrollbarBox.y1;
		float tsz = sz / (float)(MAX_QUIT_TEAMS);
		float psz = tsz * (float)numTeamsDisp;

		scrollBox.y2 = scrollbarBox.y2 - startTeam * tsz;
		scrollBox.y1 = scrollBox.y2 - psz;

		glColor4f(0.8f, 0.8f, 0.8f, guiAlpha);
		DrawBox(box + scrollBox);
	}

	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,0.4f,0.8f);
	font->glPrint(box.x1+0.045f,box.y1+0.58f,0.7f, FONT_VCENTER | FONT_SCALE | FONT_NORM,"Do you want to ...");
	glColor4f(1,1,1,0.8f);
	font->glPrint(box.x1 + resignBox.x1     + 0.025f,
	                box.y1 + (resignBox.y1 + resignBox.y2)/2, 1, FONT_VCENTER | FONT_SCALE | FONT_NORM, "Resign");
	font->glPrint(box.x1 + saveBox.x1   + 0.025f,
	                box.y1 + (saveBox.y1 + saveBox.y2)/2, 1, FONT_VCENTER | FONT_SCALE | FONT_NORM, "Save");
	font->glPrint(box.x1 + giveAwayBox.x1   + 0.025f,
	                box.y1 + (giveAwayBox.y1 + giveAwayBox.y2)/2, 1, FONT_VCENTER | FONT_SCALE | FONT_NORM, "Give everything to ...");
	font->glPrint(box.x1 + cancelBox.x1     + 0.025f,
	                box.y1 + (cancelBox.y1 + cancelBox.y2)/2, 1, FONT_VCENTER | FONT_SCALE | FONT_NORM, "Cancel");
	font->glPrint(box.x1 + quitBox.x1       + 0.025f,
	                box.y1 + (quitBox.y1 + quitBox.y2)/2, 1, FONT_VCENTER | FONT_SCALE | FONT_NORM, "Quit");

	int teamPos = 0;
	for(int team = startTeam; team < MAX_QUIT_TEAMS && teamPos < numTeamsDisp; ++team, ++teamPos) {
		int actualTeam=team;
		if (team >= gu->myTeam) {
			actualTeam++;
		}
		if (teamHandler->Team(actualTeam)->gaia) continue;

		if (shareTeam == actualTeam) {
			glColor4f(1,1,1,0.8f);
		} else {
			glColor4f(1,1,1,0.4f);
		}

		std::string teamName;
		if (teamHandler->Team(actualTeam)->leader >= 0)
			teamName = playerHandler->Player(teamHandler->Team(actualTeam)->leader)->name;
		else
			teamName = UncontrolledPlayerName;

		std::string ally, dead;
		if (teamHandler->Ally(gu->myAllyTeam, teamHandler->AllyTeam(actualTeam))) {
			ally = " <Ally>)";
		} else {
			ally = " <Enemy>";
		}
		if(teamHandler->Team(actualTeam)->isDead) {
			dead = " <Dead>";
		}
		if (actualTeam == teamHandler->GaiaTeamID()) {
			teamName = "Gaia";
			ally   = " <Gaia>";
		}
		font->glFormat(box.x1 + teamBox.x1 + 0.002f,
		                box.y1 + teamBox.y2 - 0.025f - teamPos * 0.025f, 0.7f,  FONT_SCALE | FONT_NORM,
		                "Team%i (%s)%s%s", actualTeam,
		                teamName.c_str(), ally.c_str(), dead.c_str());
	}
}

bool CQuitBox::IsAbove(int x, int y)
{
	float mx=MouseX(x);
	float my=MouseY(y);
	if(InBox(mx,my,box))
		return true;
	return false;
}

std::string CQuitBox::GetTooltip(int x, int y)
{
	float mx=MouseX(x);
	float my=MouseY(y);

	if(InBox(mx,my,box+resignBox))
		return "Resign the match, remain in the game";
	if(InBox(mx,my,box+saveBox))
		return "Save the current game state to a file \nfor later reload";
	if(InBox(mx,my,box+giveAwayBox))
		return "Give away all units and resources \nto the team specified below";
	if (hasScroll && InBox(mx, my, box + scrollBox))
		return "Scroll the team list here";
	if (hasScroll && InBox(mx, my, box + scrollbarBox))
		return "Scroll the team list here";
	if(InBox(mx,my,box+teamBox))
		return "Select which team recieves everything";
	if(InBox(mx,my,box+cancelBox))
		return "Return to the game";
	if(InBox(mx,my,box+quitBox))
		return "Forget about the other players and quit";
	if(InBox(mx,my,box))
		return " ";
	return "";
}

bool CQuitBox::MousePress(int x, int y, int button)
{
	float mx=MouseX(x);
	float my=MouseY(y);
	if(InBox(mx,my,box)){
		moveBox=true;
		if(InBox(mx,my,box+resignBox) || InBox(mx,my,box+saveBox) || InBox(mx,my,box+giveAwayBox) || InBox(mx,my,box+teamBox) || InBox(mx,my,box+cancelBox) || InBox(mx,my,box+quitBox) ||
				InBox(mx, my, box + scrollbarBox) || InBox(mx, my, box + scrollBox))
			moveBox=false;
		if (hasScroll && InBox(mx, my, box + scrollBox)) {
			scrolling = true;
			scrollGrab = (box + scrollBox).y2 - my;
		}
		else if (hasScroll && InBox(mx, my, box + scrollbarBox)) {
			if(my < (box + scrollBox).y1)
				*(volatile int *)&startTeam = startTeam + std::min(MAX_QUIT_TEAMS - numTeamsDisp - startTeam, numTeamsDisp);
			if(my > (box + scrollBox).y2)
				*(volatile int *)&startTeam = startTeam - std::min(startTeam, numTeamsDisp);
		}
		else if(InBox(mx,my,box+teamBox)){
			int team = startTeam + (int)((box.y1 + teamBox.y2 - my) / 0.025f);
			if(team>=gu->myTeam)
				team++;
			if(teamHandler->IsValidTeam(team) && !teamHandler->Team(team)->isDead){
				// we don't want to give everything to the enemy if there are allies left
				if(noAlliesLeft || (!noAlliesLeft && teamHandler->Ally(gu->myAllyTeam, teamHandler->AllyTeam(team)))){
					shareTeam=team;
				}
			}
		}
		return true;
	}
	return false;
}

void CQuitBox::MouseRelease(int x,int y,int button)
{
	float mx=MouseX(x);
	float my=MouseY(y);

	scrolling = false;
	scrollGrab = 0.0f;

	if(InBox(mx,my,box+resignBox)
	   || (InBox(mx,my,box+saveBox) && !teamHandler->Team(gu->myTeam)->isDead)
	   || (InBox(mx,my,box+giveAwayBox) && !teamHandler->Team(shareTeam)->isDead && !teamHandler->Team(gu->myTeam)->isDead)) {
		// give away all units (and resources)
		if(InBox(mx,my,box+giveAwayBox) && !playerHandler->Player(gu->myPlayerNum)->spectator) {
			net->Send(CBaseNetProtocol::Get().SendGiveAwayEverything(gu->myPlayerNum, shareTeam, playerHandler->Player(gu->myPlayerNum)->team));
		}
		// resign, so self-d all units
		if (InBox(mx,my,box+resignBox) && !playerHandler->Player(gu->myPlayerNum)->spectator) {
			net->Send(CBaseNetProtocol::Get().SendResign(gu->myPlayerNum));
		}
		// save current game state
		if (InBox(mx,my,box+saveBox)) {
			if (FileSystem::CreateDirectory("Saves")) {
				std::string timeStr = CTimeUtil::GetCurrentTimeStr();
				std::string saveFileName(timeStr + "_" + modInfo.filename + "_" + gameSetup->mapName);
				saveFileName = "Saves/" + saveFileName + ".ssf";
				if (!FileSystem::FileExists(saveFileName)) {
					LOG("Saving game to %s", saveFileName.c_str());
					ILoadSaveHandler* ls = ILoadSaveHandler::Create();
					ls->mapName = gameSetup->mapName;
					ls->modName = modInfo.filename;
					ls->SaveGame(saveFileName);
					delete ls;
				} else {
					LOG_L(L_ERROR, "File %s already exists, game NOT saved!",
							saveFileName.c_str());
				}
			}
		}
	}
	else if (InBox(mx, my, box + quitBox)) {
		LOG("User exited");
		gu->globalQuit = true;
	}
	// if we're still in the game, remove the resign box
	if(InBox(mx,my,box+resignBox) || InBox(mx,my,box+saveBox) || InBox(mx,my,box+giveAwayBox) || InBox(mx,my,box+cancelBox) || InBox(mx,my,box+quitBox)){
		delete this;
		return;
	}
	moveBox=false;
}

void CQuitBox::MouseMove(int x, int y, int dx,int dy, int button)
{
	float mx=MouseX(x);
	float my=MouseY(y);

	if(scrolling) {
		float scr = (box+scrollbarBox).y2 - (my + scrollGrab);
		float sz = scrollbarBox.y2 - scrollbarBox.y1;
		float tsz = sz / (float)MAX_QUIT_TEAMS;

		*(volatile int *)&startTeam = std::max(0, std::min((int)(scr / tsz + 0.5), MAX_QUIT_TEAMS - numTeamsDisp));
		return;
	}
	if(moveBox){
		box.x1+=MouseMoveX(dx);
		box.x2+=MouseMoveX(dx);
		box.y1+=MouseMoveY(dy);
		box.y2+=MouseMoveY(dy);
	}
	if(!(hasScroll && (InBox(mx, my, box + scrollBox) || InBox(mx, my, box + scrollbarBox))) && InBox(mx,my,box+teamBox)){
		int team = startTeam + (int)((box.y1 + teamBox.y2 - my) / 0.025f);
		if(team>=gu->myTeam)
			team++;
		if(teamHandler->IsValidTeam(team) && !teamHandler->Team(team)->isDead){
			// we don't want to give everything to the enemy if there are allies left
			if(noAlliesLeft || (!noAlliesLeft && teamHandler->Ally(gu->myAllyTeam, teamHandler->AllyTeam(team)))){
				shareTeam=team;
			}
		}
	}
}


bool CQuitBox::KeyPressed(unsigned short key, bool isRepeat)
{
	if (key == SDLK_ESCAPE) {
		delete this;
		return true;
	}
	return false;
}
