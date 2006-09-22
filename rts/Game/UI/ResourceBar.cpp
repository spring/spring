#include "StdAfx.h"
#include "ResourceBar.h"
#include "MouseHandler.h"
#include "Rendering/GL/myGL.h"
#include "Game/Team.h"
#include "Rendering/glFont.h"
#include "Net.h"
#include "mmgr.h"


CResourceBar* resourceBar=0;


CResourceBar::CResourceBar(void)
{
	box.x1 = 0.26f;
	box.y1 = 0.97f;
	box.x2 = 1;
	box.y2 = 1;

	metalBox.x1 = 0.09f;
	metalBox.y1 = 0.01f;
	metalBox.x2 = (box.x2 - box.x1) / 2.0f - 0.03f;
	metalBox.y2 = 0.024f;

	energyBox.x1 = 0.45f;
	energyBox.y1 = 0.01f;
	energyBox.x2 = box.x2 - 0.03f - box.x1;
	energyBox.y2 = 0.024f;
}


CResourceBar::~CResourceBar(void)
{
}


static string FloatToSmallString(float num,float mul=1){
	char c[50];

	if(num==0)
		return "0";
	if(fabs(num)<10*mul){
		sprintf(c,"%.1f",num);
	} else if(fabs(num)<10000*mul){
		sprintf(c,"%.0f",num);
	} else if(fabs(num)<10000000*mul){
		sprintf(c,"%.0fk",num/1000);
	} else {
		sprintf(c,"%.0fM",num/1000000);
	}
	return c;
};


void CResourceBar::Draw(void)
{
	const float mx=float(mouse->lastx - gu->screenxPos) / gu->screenx;
	const float my=(gu->screeny - float(mouse->lasty)) / gu->screeny;

	const CTeam* myTeam = gs->Team(gu->myTeam);

	GLfloat x1,y1,x2,y2,x;

	// Box
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glColor4f(0.2f,0.2f,0.2f,guiAlpha);

	glBegin(GL_QUADS);
	glVertex2f(box.x1, box.y1);
	glVertex2f(box.x1, box.y2);
	glVertex2f(box.x2, box.y2);
	glVertex2f(box.x2, box.y1);
	glVertex2f(box.x1, box.y1);
	glEnd();

	//layout metal in box
	GLfloat metalx = box.x1+.01f;
	GLfloat metaly = box.y1+.002f;

	GLfloat metalbarx1 = metalx+.08f;
	GLfloat metalbarx2 = box.x1+(box.x2-box.x1)/2.0f-.03f;

	// metal layout
	GLfloat metalbarlen = metalbarx2-metalbarx1;
	x1 = metalbarx1;
	y1 = metaly+.014f;
	x2 = metalbarx2;
	y2 = metaly+.020f;
	x = (1.0f * myTeam->metal / myTeam->metalStorage) * metalbarlen;
	
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,0.8f);
	font->glPrintAt(metalx, metaly + 0.005f, 0.7f, "");
	glDisable(GL_TEXTURE_2D);

	//metal draw
	glColor4f(0.8f,0.8f,0.8f,0.2f);
	glBegin(GL_QUADS);
	glVertex2f(x1, y1);
	glVertex2f(x1, y2);
	glVertex2f(x2, y2);
	glVertex2f(x2, y1);
	glEnd();

	glColor4f(1.0f,1.0f,1.0f,1.0f);
	glBegin(GL_QUADS);
	glVertex2f(x1, y1);
	glVertex2f(x1, y2);
	glVertex2f(x1+x, y2);
	glVertex2f(x1+x, y1);
	glEnd();
	
	x = myTeam->metalShare*metalbarlen;
	glColor4f(0.9f,0.2f,0.2f,0.7f);
	glBegin(GL_QUADS);
	glVertex2f(x1+x+0.003f, y1-0.003f);
	glVertex2f(x1+x+0.003f, y2+0.003f);
	glVertex2f(x1+x-0.003f, y2+0.003f);
	glVertex2f(x1+x-0.003f, y1-0.003f);
	glEnd();

	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,0.8f);
	
	font->glPrintAt(metalx - 0.004f, metaly + 0.005f, 0.7f, "Metal");

	font->glPrintAt(metalbarx2 - 0.01f, metaly, 0.5f, "%s",
	                FloatToSmallString(myTeam->metalStorage).c_str());
	font->glPrintAt(metalbarx1 + metalbarlen / 2.0f, metaly /*+.02f*/, 0.5f, "%s",
	                FloatToSmallString(myTeam->metal).c_str());

	glColor4f(1.0f, 0.4f, 0.4f, 1.0f); // Expenses
	font->glPrintAt(metalx + 0.044f, metaly - 0.002f, 0.5f, "-%s(-%s)",
	                FloatToSmallString(fabs(myTeam->prevMetalPull)).c_str(),
	                FloatToSmallString(fabs(myTeam->metalSent)).c_str());

	glColor4f(0.6f, 1.0f, 0.6f, 0.95f); // Income
	font->glPrintAt(metalx + 0.044f, metaly + 0.01f, 0.5f, "+%s",
	                FloatToSmallString(myTeam->prevMetalIncome).c_str());
	                // FloatToSmallString(myTeam->metalReceived).c_str());

	// Energy
	glDisable(GL_TEXTURE_2D);

	//layout energy in box
	GLfloat energyx = box.x1 + 0.4f;
	GLfloat energyy = box.y1 + 0.002f;

	GLfloat energybarx1 = energyx + 0.08f;
	GLfloat energybarx2 = box.x2 - 0.03f;

	//energy layout
	GLfloat energybarlen = energybarx2-energybarx1;
	x1 = energybarx1;
	y1 = energyy + 0.014f;
	x2 = energybarx2;
	y2 = energyy + 0.020f;
	x = (1.0f * myTeam->energy / myTeam->energyStorage) * energybarlen;
	
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,0.8f);
	font->glPrintAt(energyx, energyy + 0.005f, 0.7f, "");
	glDisable(GL_TEXTURE_2D);

	//energy draw
	glColor4f(0.8f,0.8f,0.2f,0.2f);
	glBegin(GL_QUADS);
	glVertex2f(x1, y1);
	glVertex2f(x1, y2);
	glVertex2f(x2, y2);
	glVertex2f(x2, y1);
	glEnd();

	glColor4f(1.0f,1.0f,0.2f,1.0f);
	glBegin(GL_QUADS);
	glVertex2f(x1, y1);
	glVertex2f(x1, y2);
	glVertex2f(x1+x, y2);
	glVertex2f(x1+x, y1);
	glEnd();

	x=myTeam->energyShare*energybarlen;
	glColor4f(0.9f,0.2f,0.2f,0.7f);
	glBegin(GL_QUADS);
	glVertex2f(x1+x+0.003f, y1-0.003f);
	glVertex2f(x1+x+0.003f, y2+0.003f);
	glVertex2f(x1+x-0.003f, y2+0.003f);
	glVertex2f(x1+x-0.003f, y1-0.003f);
	glEnd();

	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,0.4f,0.8f);
	
	font->glPrintAt(energyx - 0.018f, energyy + 0.005f, 0.7f, "Energy");

	glColor4f(1,1,1,0.8f);
	font->glPrintAt(energybarx2 - .001f, energyy, 0.5f, "%s",
	                FloatToSmallString(myTeam->energyStorage).c_str());
	font->glPrintAt(energybarx1 + energybarlen / 2.0f, energyy /*+.02f*/, 0.5f, "%s",
	                FloatToSmallString(myTeam->energy).c_str());

	glColor4f(1.0f,.4f,.4f,1.0f); // Expenses
	font->glPrintAt(energyx + 0.044f, energyy - 0.002f, 0.5f, "-%s(-%s)",
	                FloatToSmallString(fabs(myTeam->prevEnergyPull)).c_str(),
	                FloatToSmallString(fabs(myTeam->energySent)).c_str());

	glColor4f(.6f,1.0f,.6f,.95f); // Income
	font->glPrintAt(energyx + 0.044f, energyy + 0.01f, 0.5f, "+%s",
	                FloatToSmallString(myTeam->prevEnergyIncome).c_str());
	                // FloatToSmallString(myTeam->energyReceived).c_str());

	glDisable(GL_TEXTURE_2D);

	glLoadIdentity();
}


bool CResourceBar::IsAbove(int x, int y)
{
	const float mx=float(x - gu->screenxPos) / gu->screenx;
	const float my=(gu->screeny - float(y)) / gu->screeny;
	if(InBox(mx,my,box))
		return true;
	return false;
}


std::string CResourceBar::GetTooltip(int x, int y)
{
	const float mx=float(x-gu->screenxPos)/gu->screenx;
	const float my=(gu->screeny-float(y))/gu->screeny;

	if (mx < (box.x1 + 0.36f)) {
		return "Shows your stored metal as well as\n"
		       "income(green) and expidentures (red)\n"
		       "Click in the bar to select your\n"
		       "auto share level";
	}
	return "Shows your stored energy as well as\n"
	       "income(green) and expidentures (red)\n"
	       "Click in the bar to select your\n"
	       "auto share level";
}


bool CResourceBar::MousePress(int x, int y, int button)
{
	const float mx=float(x - gu->screenxPos) / gu->screenx;
	const float my=(gu->screeny - float(y)) / gu->screeny;


	if(InBox(mx,my,box)){
		moveBox=true;
		if(!gu->spectating){
			if(InBox(mx,my,box+metalBox)){
				moveBox=false;
				float metalShare=max(0.f,min(1.f,(mx-(box.x1+metalBox.x1))/(metalBox.x2-metalBox.x1)));
				net->SendData<unsigned char, float, float>(
						NETMSG_SETSHARE, gu->myTeam, metalShare, gs->Team(gu->myTeam)->energyShare);
			}
			if(InBox(mx,my,box+energyBox)){
				moveBox=false;
				float energyShare=max(0.f,min(1.f,(mx-(box.x1+energyBox.x1))/(energyBox.x2-energyBox.x1)));
				net->SendData<unsigned char, float, float>(
						NETMSG_SETSHARE, gu->myTeam, gs->Team(gu->myTeam)->metalShare, energyShare);
			}
		}
		return true;
	}
	return false;
}


void CResourceBar::MouseMove(int x, int y, int dx,int dy, int button)
{
	if(moveBox){
		box.x1+=float(dx)/gu->screenx;
		box.x2+=float(dx)/gu->screenx;
		box.y1-=float(dy)/gu->screeny;
		box.y2-=float(dy)/gu->screeny;
		if (box.x1 < 0.0f) {
			box.x1 = 0.0f;
			box.x2 = 0.74f;
		}
		if (box.x2 > 1.0f) {
			box.x1 = 0.26f;
			box.x2 = 1.00f;
		}
		if (box.y1 < 0.0f) {
			box.y1 = 0.00f;
			box.y2 = 0.03f;
		}
		if (box.y2 > 1.0f) {
			box.y1 = 0.97f;
			box.y2 = 1.00f;
		}
	}
}
