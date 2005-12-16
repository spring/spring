#include "StdAfx.h"
#include "ShareBox.h"
#include "MouseHandler.h"
#include "myGL.h"
#include "Team.h"
#include "Player.h"
#include "glFont.h"
#include "Net.h"
#include "SelectedUnits.h"

int CShareBox::lastShareTeam=0;

CShareBox::CShareBox(void)
{
	box.x1 = 0.34f;
	box.y1 = 0.25f;
	box.x2 = 0.66f;
	box.y2 = 0.75f;

	okBox.x1=0.22;
	okBox.y1=0.02;
	okBox.x2=0.30;
	okBox.y2=0.06;

	applyBox.x1=0.12;
	applyBox.y1=0.02;
	applyBox.x2=0.20;
	applyBox.y2=0.06;

	cancelBox.x1=0.02;
	cancelBox.y1=0.02;
	cancelBox.x2=0.10;
	cancelBox.y2=0.06;

	teamBox.x1=0.02;
	teamBox.y1=0.25;
	teamBox.x2=0.30;
	teamBox.y2=0.48;

	unitBox.x1=0.01;
	unitBox.y1=0.07;
	unitBox.x2=0.05;
	unitBox.y2=0.12;

	energyBox.x1=0.02;
	energyBox.y1=0.145;
	energyBox.x2=0.30;
	energyBox.y2=0.155;

	metalBox.x1=0.02;
	metalBox.y1=0.205;
	metalBox.x2=0.30;
	metalBox.y2=0.215;

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

	GLfloat x1,y1,x2,y2,x;
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	// Large Box
	glColor4f(0.2f,0.2f,0.2f,0.4f);
	DrawBox(box);

	// ok Box on mouse over
	if(InBox(mx,my,box+okBox)){
		glColor4f(0.7f,0.2f,0.2f,0.4f);
		DrawBox(box+okBox);
	}

	// apply Box on mouse over
	if(InBox(mx,my,box+applyBox)){
		glColor4f(0.7f,0.2f,0.2f,0.4f);
		DrawBox(box+applyBox);
	}

	// cancel Box on mouse over
	if(InBox(mx,my,box+cancelBox)){
		glColor4f(0.7f,0.2f,0.2f,0.4f);
		DrawBox(box+cancelBox);
	}

	glColor4f(0.2f,0.2f,0.2f,0.4f);
	DrawBox(box+teamBox);

	if(InBox(mx,my,box+unitBox))
		glColor4f(0.7f,0.2f,0.2f,0.4f);
	else
		glColor4f(0.2f,0.2f,0.2f,0.4f);
	DrawBox(box+unitBox);

	glColor4f(0.8f,0.8f,0.9f,0.7f);
	DrawBox(box+metalBox);

	glColor4f(0.9f,0.9f,0.2f,0.7f);
	DrawBox(box+energyBox);

	//draw share indicators in metal/energy bars
	glColor4f(0.9f,0.2f,0.2f,0.7f);
	ContainerBox metalShareBox;
	metalShareBox.x1=metalBox.x1+metalShare*(metalBox.x2-metalBox.x1)-0.005;
	metalShareBox.x2=metalShareBox.x1+0.01;
	metalShareBox.y1=metalBox.y1-0.005;
	metalShareBox.y2=metalBox.y2+0.005;
	DrawBox(box+metalShareBox);
	ContainerBox energyShareBox;
	energyShareBox.x1=energyBox.x1+energyShare*(energyBox.x2-energyBox.x1)-0.005;
	energyShareBox.x2=energyShareBox.x1+0.01;
	energyShareBox.y1=energyBox.y1-0.005;
	energyShareBox.y2=energyBox.y2+0.005;
	DrawBox(box+energyShareBox);

	//show that share units is selected
	if(shareUnits){
//		DrawBox(box+unitBox);
		glLineWidth(3);
		glBegin(GL_LINE_STRIP);
		glVertex2f(box.x1+unitBox.x1+0.01,box.y1+unitBox.y1+0.025);
		glVertex2f(box.x1+unitBox.x1+0.02,box.y1+unitBox.y1+0.01);
		glVertex2f(box.x1+unitBox.x1+0.03,box.y1+unitBox.y1+0.04);
		glEnd();
		glLineWidth(1);
	}

	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,0.8);
	font->glPrintAt(box.x1+okBox.x1+0.025,box.y1+okBox.y1+0.005,1,"Ok");
	font->glPrintAt(box.x1+applyBox.x1+0.025,box.y1+applyBox.y1+0.005,1,"Apply");
	font->glPrintAt(box.x1+cancelBox.x1+0.005,box.y1+cancelBox.y1+0.005,1,"Cancel");

	font->glPrintAt(box.x1+0.06,box.y1+0.085,0.7,"Share selected units");

	glColor4f(1,1,0.4,0.8f);
	font->glPrintAt(box.x1+0.01,box.y1+0.16,0.7f,"Share Energy");

	glColor4f(1,1,1,0.8f);
	font->glPrintAt(box.x1+0.25f,box.y1+0.12,0.7f,"%.0f",gs->Team(gu->myTeam)->energy);
	font->glPrintAt(box.x1+0.14f,box.y1+0.12,0.7f,"%.0f",gs->Team(gu->myTeam)->energy*energyShare);

	glColor4f(0.8,0.8,0.9,0.8f);
	font->glPrintAt(box.x1+0.01,box.y1+0.22,0.7f,"Share Metal");

	glColor4f(1,1,1,0.8f);
	font->glPrintAt(box.x1+0.25f,box.y1+0.18,0.7f,"%.0f",gs->Team(gu->myTeam)->metal);
	font->glPrintAt(box.x1+0.14f,box.y1+0.18,0.7f,"%.0f",gs->Team(gu->myTeam)->metal*metalShare);

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
		netbuf[0]=NETMSG_SHARE;
		netbuf[1]=gu->myPlayerNum;
		netbuf[2]=shareTeam;
		netbuf[3]=shareUnits;
		*(float*)&netbuf[4]=metalShare*gs->Team(gu->myTeam)->metal;
		*(float*)&netbuf[8]=energyShare*gs->Team(gu->myTeam)->energy;
		net->SendData(netbuf,12);
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
		int team=(int)((box.y1+teamBox.y2-my)/0.025);
		if(team>=gu->myTeam)
			team++;
		if(team<gs->activeTeams && !gs->Team(team)->isDead)
			shareTeam=team;
	}
}
