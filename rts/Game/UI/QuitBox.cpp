#include "StdAfx.h"
#include "QuitBox.h"
#include "MouseHandler.h"
#include "Rendering/GL/myGL.h"
#include "Game/Team.h"
#include "Game/Player.h"
#include "Rendering/glFont.h"
#include "Net.h"
#include "Game/SelectedUnits.h"
#include "InfoConsole.h"
#include "Sim/Units/Unit.h"

extern bool globalQuit;
int CQuitBox::lastShareTeam=0;

CQuitBox::CQuitBox(void)
{
	box.x1 = 0.34f;
	box.y1 = 0.25f;
	box.x2 = 0.66f;
	box.y2 = 0.75f;

	resignQuitBox.x1=0.02;
	resignQuitBox.y1=0.42;
	resignQuitBox.x2=0.30;
	resignQuitBox.y2=0.46;

	resignBox.x1=0.02;
	resignBox.y1=0.38;
	resignBox.x2=0.30;
	resignBox.y2=0.42;

	giveAwayBox.x1=0.02;
	giveAwayBox.y1=0.34;
	giveAwayBox.x2=0.30;
	giveAwayBox.y2=0.38;

	teamBox.x1=0.02;
	teamBox.y1=0.11;
	teamBox.x2=0.30;
	teamBox.y2=0.33;

	quitBox.x1=0.02;
	quitBox.y1=0.06;
	quitBox.x2=0.30;
	quitBox.y2=0.10;

	cancelBox.x1=0.02;
	cancelBox.y1=0.02;
	cancelBox.x2=0.30;
	cancelBox.y2=0.06;


	moveBox=false;

	shareTeam=lastShareTeam;
	if(gu->myTeam==0 && shareTeam==0)
		shareTeam++;
}

CQuitBox::~CQuitBox(void)
{
}

void CQuitBox::Draw(void)
{
	float mx=float(mouse->lastx)/gu->screenx;
	float my=(gu->screeny-float(mouse->lasty))/gu->screeny;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	// Large Box
	glColor4f(0.2f,0.2f,0.2f,0.4f);
	DrawBox(box);

	// resign and quit Box on mouse over
	if(InBox(mx,my,box+resignQuitBox)){
		glColor4f(0.7f,0.2f,0.2f,0.4f);
		DrawBox(box+resignQuitBox);
	}

	// resign Box on mouse over
	if(InBox(mx,my,box+resignBox)){
		glColor4f(0.7f,0.2f,0.2f,0.4f);
		DrawBox(box+resignBox);
	}

	// give away Box on mouse over
	if(InBox(mx,my,box+giveAwayBox)){
		glColor4f(0.7f,0.2f,0.2f,0.4f);
		DrawBox(box+giveAwayBox);
	}
	// cancel Box on mouse over
	if(InBox(mx,my,box+cancelBox)){
		glColor4f(0.7f,0.2f,0.2f,0.4f);
		DrawBox(box+cancelBox);
	}
	// quit Box on mouse over
	if(InBox(mx,my,box+quitBox)){
		glColor4f(0.7f,0.2f,0.2f,0.4f);
		DrawBox(box+quitBox);
	}


	glColor4f(0.2f,0.2f,0.2f,0.4f);
	DrawBox(box+teamBox);


	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,0.4,0.8f);
	font->glPrintAt(box.x1+0.045,box.y1+0.47,0.7,"Do you want to ...");
	glColor4f(1,1,1,0.8);
	font->glPrintAt(box.x1+resignQuitBox.x1+0.025,box.y1+resignQuitBox.y1+0.005,1,"Resign and quit");
	font->glPrintAt(box.x1+resignBox.x1+0.025,box.y1+resignBox.y1+0.005,1,"Resign");
	font->glPrintAt(box.x1+giveAwayBox.x1+0.025,box.y1+giveAwayBox.y1+0.005,1,"Give everything to ...");
	font->glPrintAt(box.x1+cancelBox.x1+0.025,box.y1+cancelBox.y1+0.005,1,"Cancel");
	font->glPrintAt(box.x1+quitBox.x1+0.025,box.y1+quitBox.y1+0.005,1,"Quit");

	for(int team=0;team<gs->activeTeams-1;++team){
		int actualTeam=team;
		if(team>=gu->myTeam)
			actualTeam++;

		if(shareTeam==actualTeam)
			glColor4f(1,1,1,0.8f);
		else
			glColor4f(1,1,1,0.4f);
		string ally,dead;
		if(gs->Ally(gu->myAllyTeam, gs->AllyTeam(actualTeam)))
			ally="(Ally)";
		if(gs->Team(actualTeam)->isDead)
			dead="(Dead)";
		font->glPrintAt(box.x1+teamBox.x1+0.002f,box.y1+teamBox.y2-0.025-team*0.025,0.7f,"Team%i (%s)%s%s",actualTeam,gs->players[gs->Team(actualTeam)->leader]->playerName.c_str(),ally.c_str(),dead.c_str());
	}
}

bool CQuitBox::IsAbove(int x, int y)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;
	if(InBox(mx,my,box))
		return true;
	return false;
}

std::string CQuitBox::GetTooltip(int x, int y)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;

	if(InBox(mx,my,box+resignQuitBox))
		return "Resign the match, and quit the game. Units will self-destruct";
	if(InBox(mx,my,box+resignBox))
		return "Resign the match, remain in the game. Units will self-destruct";
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
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;
	if(InBox(mx,my,box)){
		moveBox=true;
		if(InBox(mx,my,box+resignQuitBox) || InBox(mx,my,box+resignBox) || InBox(mx,my,box+giveAwayBox) || InBox(mx,my,box+teamBox) || InBox(mx,my,box+cancelBox) || InBox(mx,my,box+quitBox))
			moveBox=false;
		if(InBox(mx,my,box+teamBox)){
			int team=(int)((box.y1+teamBox.y2-my)/0.025);
			if(team>=gu->myTeam)
				team++;
			if(team<gs->activeTeams && !gs->Team(team)->isDead)
				shareTeam=team;
		}
		return true;
	}
	return false;
}

void CQuitBox::MouseRelease(int x,int y,int button)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;

	if(InBox(mx,my,box+resignQuitBox) || InBox(mx,my,box+resignBox) || InBox(mx,my,box+giveAwayBox) && !gs->Team(shareTeam)->isDead && !gs->Team(gu->myTeam)->isDead){
		set<CUnit*>* tu=&gs->Team(gu->myTeam)->units;
		// give away all units (and resources)
		if(InBox(mx,my,box+giveAwayBox)){
			//select all units
			selectedUnits.ClearSelected();
			for(set<CUnit*>::iterator ui=tu->begin();ui!=tu->end();++ui){
				selectedUnits.AddUnit(*ui);
			}
			//make sure the units are stopped and that the selection is transmitted
			Command c;
			c.id=CMD_STOP;
			selectedUnits.GiveCommand(c,false);
			net->SendData<unsigned char, unsigned char, unsigned char, float, float>(
				NETMSG_SHARE, gu->myPlayerNum, shareTeam, true,
				gs->Team(gu->myTeam)->metal, gs->Team(gu->myTeam)->energy);
			selectedUnits.ClearSelected();
			lastShareTeam=shareTeam;
			// inform other users of the giving away of units
			char givenAwayMsg[200];
			sprintf(givenAwayMsg,"%s gave everything to %s.",
				gs->players[gu->myPlayerNum]->playerName.c_str(),
				gs->players[gs->Team(shareTeam)->leader]->playerName.c_str());
			net->SendSTLData<unsigned char, std::string>(NETMSG_CHAT, gu->myPlayerNum, givenAwayMsg);
		}
		// resign, so self-d all units
		if(InBox(mx,my,box+resignQuitBox) || InBox(mx,my,box+resignBox))
			for(set<CUnit*>::iterator ui=tu->begin();ui!=tu->end();++ui){
				(*ui)->KillUnit(true,false,0);
			}
		// (resign and) quit, so leave the game
		if(InBox(mx,my,box+resignQuitBox)){
			info->AddLine("User exited");
			globalQuit=true;
		}
	}
	else if(InBox(mx,my,box+quitBox))
	{
		info->AddLine("User exited");
		globalQuit=true;
	}
	// if we're still in the game, remove the resign box
	if(InBox(mx,my,box+resignQuitBox) || InBox(mx,my,box+resignBox) || InBox(mx,my,box+giveAwayBox) || InBox(mx,my,box+cancelBox)){
		delete this;
		return;
	}
	moveBox=false;
}

void CQuitBox::MouseMove(int x, int y, int dx,int dy, int button)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;
	if(moveBox){
		box.x1+=float(dx)/gu->screenx;
		box.x2+=float(dx)/gu->screenx;
		box.y1-=float(dy)/gu->screeny;
		box.y2-=float(dy)/gu->screeny;
	}
	if(InBox(mx,my,box+teamBox)){
		int team=(int)((box.y1+teamBox.y2-my)/0.025);
		if(team>=gu->myTeam)
			team++;
		if(team<gs->activeTeams && !gs->Team(team)->isDead)
			shareTeam=team;
	}
}

