#include "StdAfx.h"
#include "ShareBox.h"
#include "MouseHandler.h"
#include "Rendering/GL/myGL.h"
#include "Game/Team.h"
#include "Game/Player.h"
#include "Rendering/glFont.h"
#include "Net.h"
#include "Game/SelectedUnits.h"

int CShareBox::lastShareTeam=0;

CShareBox::CShareBox(void)
{
	box.x1 = 0.34f;
	box.y1 = 0.25f;
	box.x2 = 0.66f;
	box.y2 = 0.75f;

	okBox.x1=0.22f;
	okBox.y1=0.02f;
	okBox.x2=0.30f;
	okBox.y2=0.06f;

	applyBox.x1=0.12f;
	applyBox.y1=0.02f;
	applyBox.x2=0.20f;
	applyBox.y2=0.06f;

	cancelBox.x1=0.02f;
	cancelBox.y1=0.02f;
	cancelBox.x2=0.10f;
	cancelBox.y2=0.06f;

	teamBox.x1=0.02f;
	teamBox.y1=0.25f;
	teamBox.x2=0.30f;
	teamBox.y2=0.48f;

	unitBox.x1=0.01f;
	unitBox.y1=0.07f;
	unitBox.x2=0.05f;
	unitBox.y2=0.12f;

	energyBox.x1=0.02f;
	energyBox.y1=0.145f;
	energyBox.x2=0.30f;
	energyBox.y2=0.155f;

	metalBox.x1=0.02f;
	metalBox.y1=0.205f;
	metalBox.x2=0.30f;
	metalBox.y2=0.215f;

	metalShare=0;
	energyShare=0;
	shareUnits=false;
	moveBox=false;

	shareTeam=lastShareTeam;
	if(gu->myTeam==0 && shareTeam==0)
		shareTeam++;
}

CShareBox::~CShareBox(void)
{
}

void CShareBox::Draw(void)
{
	float mx=float(mouse->lastx)/gu->screenx;
	float my=(gu->screeny-float(mouse->lasty))/gu->screeny;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	// Large Box
	glColor4f(0.2f,0.2f,0.2f,guiAlpha);
	DrawBox(box);

	// ok Box on mouse over
	if(InBox(mx,my,box+okBox)){
		glColor4f(0.7f,0.2f,0.2f,guiAlpha);
		DrawBox(box+okBox);
	}

	// apply Box on mouse over
	if(InBox(mx,my,box+applyBox)){
		glColor4f(0.7f,0.2f,0.2f,guiAlpha);
		DrawBox(box+applyBox);
	}

	// cancel Box on mouse over
	if(InBox(mx,my,box+cancelBox)){
		glColor4f(0.7f,0.2f,0.2f,guiAlpha);
		DrawBox(box+cancelBox);
	}

	glColor4f(0.2f,0.2f,0.2f,guiAlpha);
	DrawBox(box+teamBox);

	if(InBox(mx,my,box+unitBox))
		glColor4f(0.7f,0.2f,0.2f,guiAlpha);
	else
		glColor4f(0.2f,0.2f,0.2f,guiAlpha);
	DrawBox(box+unitBox);

	glColor4f(0.8f,0.8f,0.9f,0.7f);
	DrawBox(box+metalBox);

	glColor4f(0.9f,0.9f,0.2f,0.7f);
	DrawBox(box+energyBox);

	//draw share indicators in metal/energy bars
	glColor4f(0.9f,0.2f,0.2f,0.7f);
	ContainerBox metalShareBox;
	metalShareBox.x1=metalBox.x1+metalShare*(metalBox.x2-metalBox.x1)-0.005f;
	metalShareBox.x2=metalShareBox.x1+0.01f;
	metalShareBox.y1=metalBox.y1-0.005f;
	metalShareBox.y2=metalBox.y2+0.005f;
	DrawBox(box+metalShareBox);
	ContainerBox energyShareBox;
	energyShareBox.x1=energyBox.x1+energyShare*(energyBox.x2-energyBox.x1)-0.005f;
	energyShareBox.x2=energyShareBox.x1+0.01f;
	energyShareBox.y1=energyBox.y1-0.005f;
	energyShareBox.y2=energyBox.y2+0.005f;
	DrawBox(box+energyShareBox);

	//show that share units is selected
	if(shareUnits){
//		DrawBox(box+unitBox);
		glLineWidth(3);
		glBegin(GL_LINE_STRIP);
		glVertex2f(box.x1+unitBox.x1+0.01f,box.y1+unitBox.y1+0.025f);
		glVertex2f(box.x1+unitBox.x1+0.02f,box.y1+unitBox.y1+0.01f);
		glVertex2f(box.x1+unitBox.x1+0.03f,box.y1+unitBox.y1+0.04f);
		glEnd();
		glLineWidth(1);
	}

	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,0.8f);
	font->glPrintAt(box.x1+okBox.x1+0.025f,box.y1+okBox.y1+0.005f,1,"Ok");
	font->glPrintAt(box.x1+applyBox.x1+0.025f,box.y1+applyBox.y1+0.005f,1,"Apply");
	font->glPrintAt(box.x1+cancelBox.x1+0.005f,box.y1+cancelBox.y1+0.005f,1,"Cancel");

	font->glPrintAt(box.x1+0.06f,box.y1+0.085f,0.7f,"Share selected units");

	glColor4f(1,1,0.4f,0.8f);
	font->glPrintAt(box.x1+0.01f,box.y1+0.16f,0.7f,"Share Energy");

	glColor4f(1,1,1,0.8f);
	font->glPrintAt(box.x1+0.25f,box.y1+0.12f,0.7f,"%.0f",gs->Team(gu->myTeam)->energy);
	font->glPrintAt(box.x1+0.14f,box.y1+0.12f,0.7f,"%.0f",gs->Team(gu->myTeam)->energy*energyShare);

	glColor4f(0.8f,0.8f,0.9f,0.8f);
	font->glPrintAt(box.x1+0.01f,box.y1+0.22f,0.7f,"Share Metal");

	glColor4f(1,1,1,0.8f);
	font->glPrintAt(box.x1+0.25f,box.y1+0.18f,0.7f,"%.0f",gs->Team(gu->myTeam)->metal);
	font->glPrintAt(box.x1+0.14f,box.y1+0.18f,0.7f,"%.0f",gs->Team(gu->myTeam)->metal*metalShare);

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
		font->glPrintAt(box.x1+teamBox.x1+0.002f,box.y1+teamBox.y2-0.025f-team*0.025f,0.7f,"Team%i (%s)%s%s",actualTeam,gs->players[gs->Team(actualTeam)->leader]->playerName.c_str(),ally.c_str(),dead.c_str());
	}
}

bool CShareBox::IsAbove(int x, int y)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;
	if(InBox(mx,my,box))
		return true;
	return false;
}

std::string CShareBox::GetTooltip(int x, int y)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;

	if(InBox(mx,my,box+okBox))
		return "Shares the selected stuff and close dialog";
	if(InBox(mx,my,box+applyBox))
		return "Shares the selected stuff";
	if(InBox(mx,my,box+cancelBox))
		return "Close this dialog without sharing";
	if(InBox(mx,my,box+unitBox))
		return "Toggles if you want to share your\ncurrently selected units";
	if(InBox(mx,my,box+metalBox))
		return "Click here to select how much metal to share";
	if(InBox(mx,my,box+energyBox))
		return "Click here to select how much energy to share";
	if(InBox(mx,my,box+teamBox))
		return "Select which team to share to";

	if(InBox(mx,my,box))
		return " ";
	return "";
}

bool CShareBox::MousePress(int x, int y, int button)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;
	if(InBox(mx,my,box)){
		moveBox=true;
		if(InBox(mx,my,box+okBox) || InBox(mx,my,box+applyBox) || InBox(mx,my,box+cancelBox) || InBox(mx,my,box+unitBox) || InBox(mx,my,box+metalBox) || InBox(mx,my,box+energyBox) || InBox(mx,my,box+teamBox))
			moveBox=false;
		if(InBox(mx,my,box+metalBox)){
			metalMove=true;
			metalShare=max(0.f,min(1.f,(mx-box.x1-metalBox.x1)/(metalBox.x2-metalBox.x1)));
		}
		if(InBox(mx,my,box+energyBox)){
			energyMove=true;
			energyShare=max(0.f,min(1.f,(mx-box.x1-energyBox.x1)/(energyBox.x2-energyBox.x1)));
		}
		if(InBox(mx,my,box+teamBox)){
			int team=(int)((box.y1+teamBox.y2-my)/0.025f);
			if(team>=gu->myTeam)
				team++;
			if(team<gs->activeTeams && !gs->Team(team)->isDead)
				shareTeam=team;
		}
		return true;
	}
	return false;
}

void CShareBox::MouseRelease(int x,int y,int button)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;

	if(InBox(mx,my,box+unitBox)){
		shareUnits=!shareUnits;
	}
	if(InBox(mx,my,box+okBox) || InBox(mx,my,box+applyBox) && !gs->Team(shareTeam)->isDead && !gs->Team(gu->myTeam)->isDead){
		if(shareUnits){
			Command c;
			c.id=CMD_STOP;
			selectedUnits.GiveCommand(c,false);		//make sure the units are stopped and that the selection is transmitted
		}
		net->SendData<unsigned char, unsigned char, unsigned char, float, float>(
				NETMSG_SHARE, gu->myPlayerNum, shareTeam, shareUnits,
				metalShare*gs->Team(gu->myTeam)->metal, energyShare*gs->Team(gu->myTeam)->energy);
		if(shareUnits)
			selectedUnits.ClearSelected();
		lastShareTeam=shareTeam;
	}
	if(InBox(mx,my,box+okBox) || InBox(mx,my,box+cancelBox)){
		delete this;
		return;
	}
	moveBox=false;
	metalMove=false;
	energyMove=false;
}

void CShareBox::MouseMove(int x, int y, int dx,int dy, int button)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;
	if(moveBox){
		box.x1+=float(dx)/gu->screenx;
		box.x2+=float(dx)/gu->screenx;
		box.y1-=float(dy)/gu->screeny;
		box.y2-=float(dy)/gu->screeny;
	}
	if(metalMove){
		metalShare=max(0.f,min(1.f,(mx-box.x1-metalBox.x1)/(metalBox.x2-metalBox.x1)));
	}
	if(energyMove){
		energyShare=max(0.f,min(1.f,(mx-box.x1-energyBox.x1)/(energyBox.x2-energyBox.x1)));
	}
	if(InBox(mx,my,box+teamBox)){
		int team=(int)((box.y1+teamBox.y2-my)/0.025f);
		if(team>=gu->myTeam)
			team++;
		if(team<gs->activeTeams && !gs->Team(team)->isDead)
			shareTeam=team;
	}
}

bool CShareBox::KeyPressed(unsigned short key)
{
	if (key == 27) { // escape
		delete this;
		return true;
	}
	return false;
}

