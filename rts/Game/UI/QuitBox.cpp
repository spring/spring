#include "StdAfx.h"
#include "Game/Team.h"
#include "LogOutput.h"
#include "MouseHandler.h"
#include "NetProtocol.h"
#include "QuitBox.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/myGL.h"
#include <SDL_keysym.h>

extern bool globalQuit;

CQuitBox::CQuitBox(void)
{
	box.x1 = 0.34f;
	box.y1 = 0.18f;
	box.x2 = 0.66f;
	box.y2 = 0.78f;

	resignBox.x1=0.02f;
	resignBox.y1=0.53f;
	resignBox.x2=0.30f;
	resignBox.y2=0.57f;

	giveAwayBox.x1=0.02f;
	giveAwayBox.y1=0.49f;
	giveAwayBox.x2=0.30f;
	giveAwayBox.y2=0.53f;

	teamBox.x1=0.02f;
	teamBox.y1=0.11f;
	teamBox.x2=0.30f;
	teamBox.y2=0.48f;

	quitBox.x1=0.02f;
	quitBox.y1=0.06f;
	quitBox.x2=0.30f;
	quitBox.y2=0.10f;

	cancelBox.x1=0.02f;
	cancelBox.y1=0.02f;
	cancelBox.x2=0.30f;
	cancelBox.y2=0.06f;


	moveBox=false;
	noAlliesLeft=true;
	shareTeam=0;
	// if we have alive allies left, set the shareteam to an undead ally.
	for(int team=0;team<gs->activeTeams;++team){
		if (gs->Team(team)->gaia) continue;
		if(team!=gu->myTeam && !gs->Team(team)->isDead)
		{
			if(shareTeam==gu->myTeam || gs->Team(shareTeam)->isDead)
				shareTeam=team;
			if(gs->Ally(gu->myAllyTeam, gs->AllyTeam(team))){
				noAlliesLeft=false;
				shareTeam=team;
				break;
			}
		}
	}
}

CQuitBox::~CQuitBox(void)
{
}

void CQuitBox::Draw(void)
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


	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,0.4f,0.8f);
	font->glPrintAt(box.x1+0.045f,box.y1+0.58f,0.7f,"Do you want to ...");
	glColor4f(1,1,1,0.8f);
	font->glPrintAt(box.x1 + resignBox.x1     + 0.025f,
	                box.y1 + resignBox.y1     + 0.005f, 1, "Resign");
	font->glPrintAt(box.x1 + giveAwayBox.x1   + 0.025f,
	                box.y1 + giveAwayBox.y1   + 0.005f, 1, "Give everything to ...");
	font->glPrintAt(box.x1 + cancelBox.x1     + 0.025f,
	                box.y1 + cancelBox.y1     + 0.005f, 1, "Cancel");
	font->glPrintAt(box.x1 + quitBox.x1       + 0.025f,
	                box.y1 + quitBox.y1       + 0.005f, 1, "Quit");

	for(int team=0;team<gs->activeTeams-1;++team){
		int actualTeam=team;
		if (team >= gu->myTeam) {
			actualTeam++;
		}
		if (gs->Team(actualTeam)->gaia) continue;

		if (shareTeam == actualTeam) {
			glColor4f(1,1,1,0.8f);
		} else {
			glColor4f(1,1,1,0.4f);
		}

		std::string teamName;
		if (gs->Team(actualTeam)->leader >= 0)
			teamName = gs->players[gs->Team(actualTeam)->leader]->name;
		else
			teamName = "uncontrolled";

		std::string ally, dead;
		if (gs->Ally(gu->myAllyTeam, gs->AllyTeam(actualTeam))) {
			ally = " <Ally>)";
		} else {
			ally = " <Enemy>";
		}
		if(gs->Team(actualTeam)->isDead) {
			dead = " <Dead>";
		}
		if (actualTeam == gs->gaiaTeamID) {
			teamName = "Gaia";
			ally   = " <Gaia>";
		}
		font->glFormatAt(box.x1 + teamBox.x1 + 0.002f,
		                box.y1 + teamBox.y2 - 0.025f - team * 0.025f, 0.7f,
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
		return "Resign the match, remain in the game.";
	if(InBox(mx,my,box+giveAwayBox))
		return "Give away all units and resources \nto the team specified below";
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
		if(InBox(mx,my,box+resignBox) || InBox(mx,my,box+giveAwayBox) || InBox(mx,my,box+teamBox) || InBox(mx,my,box+cancelBox) || InBox(mx,my,box+quitBox))
			moveBox=false;
		if(InBox(mx,my,box+teamBox)){
			int team=(int)((box.y1+teamBox.y2-my)/0.025f);
			if(team>=gu->myTeam)
				team++;
			if(team<gs->activeTeams && !gs->Team(team)->isDead){
				// we don't want to give everything to the enemy if there are allies left
				if(noAlliesLeft || (!noAlliesLeft && gs->Ally(gu->myAllyTeam, gs->AllyTeam(team)))){
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

	if(InBox(mx,my,box+resignBox) || InBox(mx,my,box+giveAwayBox) && !gs->Team(shareTeam)->isDead && !gs->Team(gu->myTeam)->isDead){
		// give away all units (and resources)
		if(InBox(mx,my,box+giveAwayBox)) {
			net->Send(CBaseNetProtocol::Get().SendGiveAwayEverything(gu->myPlayerNum, shareTeam));
			// inform other users of the giving away of units
			char givenAwayMsg[200];
			sprintf(givenAwayMsg,"%s gave everything to %s.",
				gs->players[gu->myPlayerNum]->name.c_str(),
				gs->players[gs->Team(shareTeam)->leader]->name.c_str());
			net->Send(CBaseNetProtocol::Get().SendSystemMessage(gu->myPlayerNum, givenAwayMsg));
		}
		// resign, so self-d all units
		if (InBox(mx,my,box+resignBox)) {
			net->Send(CBaseNetProtocol::Get().SendResign(gu->myPlayerNum));
		}
	}
	else if(InBox(mx,my,box+quitBox))
	{
		logOutput.Print("User exited");
		globalQuit=true;
	}
	// if we're still in the game, remove the resign box
	if(InBox(mx,my,box+resignBox) || InBox(mx,my,box+giveAwayBox) || InBox(mx,my,box+cancelBox)){
		delete this;
		return;
	}
	moveBox=false;
}

void CQuitBox::MouseMove(int x, int y, int dx,int dy, int button)
{
	float mx=MouseX(x);
	float my=MouseY(y);
	if(moveBox){
		box.x1+=MouseMoveX(dx);
		box.x2+=MouseMoveX(dx);
		box.y1+=MouseMoveY(dy);
		box.y2+=MouseMoveY(dy);
	}
	if(InBox(mx,my,box+teamBox)){
		int team=(int)((box.y1+teamBox.y2-my)/0.025f);
		if(team>=gu->myTeam)
			team++;
		if(team<gs->activeTeams && !gs->Team(team)->isDead){
			// we don't want to give everything to the enemy if there are allies left
			if(noAlliesLeft || (!noAlliesLeft && gs->Ally(gu->myAllyTeam, gs->AllyTeam(team)))){
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
