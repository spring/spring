#include "StdAfx.h"
#include "EndGameBox.h"
#include "MouseHandler.h"
#include "Rendering/GL/myGL.h"
#include "Game/Team.h"
#include "Game/Player.h"
#include "Rendering/glFont.h"
#include "Net.h"
#include "Game/SelectedUnits.h"

extern bool globalQuit;

CEndGameBox::CEndGameBox(void)
{
	box.x1 = 0.14f;
	box.y1 = 0.1f;
	box.x2 = 0.86f;
	box.y2 = 0.8f;

	exitBox.x1=0.31;
	exitBox.y1=0.02;
	exitBox.x2=0.41;
	exitBox.y2=0.06;
}

CEndGameBox::~CEndGameBox(void)
{
}

bool CEndGameBox::MousePress(int x, int y, int button)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;
	if(InBox(mx,my,box)){
		moveBox=true;
		if(InBox(mx,my,box+exitBox))
			moveBox=false;
		return true;
	}
	return false;
}

void CEndGameBox::MouseMove(int x, int y, int dx,int dy, int button)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;
	if(moveBox){
		box.x1+=float(dx)/gu->screenx;
		box.x2+=float(dx)/gu->screenx;
		box.y1-=float(dy)/gu->screeny;
		box.y2-=float(dy)/gu->screeny;
	}
}

void CEndGameBox::MouseRelease(int x, int y, int button)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;

	if(InBox(mx,my,box+exitBox)){
		delete this;
		globalQuit=true;
		return;
	}
}

bool CEndGameBox::IsAbove(int x, int y)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;
	if(InBox(mx,my,box))
		return true;
	return false;
}

void CEndGameBox::Draw()
{
	float mx=float(mouse->lastx)/gu->screenx;
	float my=(gu->screeny-float(mouse->lasty))/gu->screeny;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	// Large Box
	glColor4f(0.2f,0.2f,0.2f,0.4f);
	DrawBox(box);

	// exit Box on mouse over
	if(InBox(mx,my,box+exitBox)){
		glColor4f(0.7f,0.2f,0.2f,0.4f);
		DrawBox(box+exitBox);
	}
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,0.8);
	font->glPrintAt(box.x1+exitBox.x1+0.025,box.y1+exitBox.y1+0.005,1,"Exit");

	if(gs->Team(gu->myTeam)->isDead){
		font->glPrintAt(box.x1+0.25,box.y1+0.65,1,"You lost the game");
	} else {
		font->glPrintAt(box.x1+0.25,box.y1+0.65,1,"You won the game");
	}
}

std::string CEndGameBox::GetTooltip(int x,int y)
{
	return "No tooltip defined";
}
