/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "ResourceBar.h"
#include "MouseHandler.h"
#include "GuiHandler.h"
#include "Rendering/GL/myGL.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Rendering/glFont.h"
#include "NetProtocol.h"
#include "TimeProfiler.h"

CResourceBar* resourceBar=0;


CResourceBar::CResourceBar(void) : disabled(false)
{
	box.x1 = 0.26f;
	box.y1 = 0.97f;
	box.x2 = 1;
	box.y2 = 1;

	metalBox.x1 = 0.09f;
	metalBox.y1 = 0.01f;
	metalBox.x2 = (box.x2 - box.x1) / 2.0f - 0.03f;
	metalBox.y2 = metalBox.y1 + (box.y2 - box.y1);		// extend to the very top of the screen

	energyBox.x1 = 0.48f;
	energyBox.y1 = 0.01f;
	energyBox.x2 = box.x2 - 0.03f - box.x1;
	energyBox.y2 = energyBox.y1 + (box.y2 - box.y1);		// extend to the very top of the screen
}


CResourceBar::~CResourceBar(void)
{
}


static std::string FloatToSmallString(float num, float mul = 1) {
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
	if (disabled) {
		return;
	}

	const CTeam* myTeam = teamHandler->Team(gu->myTeam);

	GLfloat x1,y1,x2,y2,x;

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	glBegin(GL_QUADS);

	// Box
	glColor4f(0.2f,0.2f,0.2f,guiAlpha);
	glVertex2f(box.x1, box.y1);
	glVertex2f(box.x1, box.y2);
	glVertex2f(box.x2, box.y2);
	glVertex2f(box.x2, box.y1);

	//layout metal in box
	GLfloat metalx = box.x1+.01f;
	GLfloat metaly = box.y1+.004f;

	GLfloat metalbarx1 = metalx+.08f;
	GLfloat metalbarx2 = box.x1+(box.x2-box.x1)/2.0f-.03f;

	// metal layout
	GLfloat metalbarlen = metalbarx2-metalbarx1;
	x1 = metalbarx1;
	y1 = metaly+.014f;
	x2 = metalbarx2;
	y2 = metaly+.020f;
	x = (1.0f * myTeam->metal / myTeam->metalStorage) * metalbarlen;

	//metal draw
	glColor4f(0.8f,0.8f,0.8f,0.2f);
	glVertex2f(x1, y1);
	glVertex2f(x1, y2);
	glVertex2f(x2, y2);
	glVertex2f(x2, y1);

	glColor4f(1.0f,1.0f,1.0f,1.0f);
	glVertex2f(x1, y1);
	glVertex2f(x1, y2);
	glVertex2f(x1+x, y2);
	glVertex2f(x1+x, y1);

	x = myTeam->metalShare*metalbarlen;
	glColor4f(0.9f,0.2f,0.2f,0.7f);
	glVertex2f(x1+x+0.003f, y1-0.003f);
	glVertex2f(x1+x+0.003f, y2+0.003f);
	glVertex2f(x1+x-0.003f, y2+0.003f);
	glVertex2f(x1+x-0.003f, y1-0.003f);

	// Energy

	//layout energy in box
	GLfloat energyx = box.x1 + 0.4f;
	GLfloat energyy = box.y1 + 0.004f;

	GLfloat energybarx1 = energyx + 0.08f;
	GLfloat energybarx2 = box.x2 - 0.03f;

	//energy layout
	GLfloat energybarlen = energybarx2-energybarx1;
	x1 = energybarx1;
	y1 = energyy + 0.014f;
	x2 = energybarx2;
	y2 = energyy + 0.020f;
	x = (1.0f * myTeam->energy / myTeam->energyStorage) * energybarlen;

	//energy draw
	glColor4f(0.8f,0.8f,0.2f,0.2f);
	glVertex2f(x1, y1);
	glVertex2f(x1, y2);
	glVertex2f(x2, y2);
	glVertex2f(x2, y1);

	glColor4f(1.0f,1.0f,0.2f,1.0f);
	glVertex2f(x1, y1);
	glVertex2f(x1, y2);
	glVertex2f(x1+x, y2);
	glVertex2f(x1+x, y1);

	x=myTeam->energyShare*energybarlen;
	glColor4f(0.9f,0.2f,0.2f,0.7f);
	glVertex2f(x1+x+0.003f, y1-0.003f);
	glVertex2f(x1+x+0.003f, y2+0.003f);
	glVertex2f(x1+x-0.003f, y2+0.003f);
	glVertex2f(x1+x-0.003f, y1-0.003f);
	glEnd();


	smallFont->Begin();

	const float headerFontSize = (box.y2 - box.y1) * 0.7 * globalRendering->viewSizeY;
	const float labelsFontSize = (box.y2 - box.y1) * 0.5 * globalRendering->viewSizeY;
	const int fontOptions = (guihandler && guihandler->GetOutlineFonts()) ? FONT_OUTLINE | FONT_NORM : FONT_NORM;

	smallFont->SetTextColor(0.8f,0.8f,1,0.8f);
	smallFont->glPrint(metalx - 0.004f, (box.y1+box.y2)*0.5, headerFontSize, FONT_VCENTER | fontOptions, "Metal");

	smallFont->SetTextColor(1,1,0.4f,0.8f);
	smallFont->glPrint(energyx - 0.018f, (box.y1+box.y2)*0.5, headerFontSize, FONT_VCENTER | fontOptions, "Energy");

	smallFont->SetTextColor(1.0f, 0.3f, 0.3f, 1.0f); // Expenses
	smallFont->glFormat(metalx + 0.044f, box.y1, labelsFontSize, FONT_DESCENDER | fontOptions, "-%s(-%s)",
	                FloatToSmallString(fabs(myTeam->prevMetalPull)).c_str(),
	                FloatToSmallString(fabs(myTeam->metalSent)).c_str());
	smallFont->glFormat(energyx + 0.044f, box.y1, labelsFontSize, FONT_DESCENDER | fontOptions, "-%s(-%s)",
	                FloatToSmallString(fabs(myTeam->prevEnergyPull)).c_str(),
	                FloatToSmallString(fabs(myTeam->energySent)).c_str());

	smallFont->SetTextColor(0.4f, 1.0f, 0.4f, 0.95f); // Income
	smallFont->glFormat(metalx + 0.044f, box.y2-2*globalRendering->pixelY, labelsFontSize, FONT_ASCENDER | fontOptions, "+%s",
	                FloatToSmallString(myTeam->prevMetalIncome).c_str());
	                // FloatToSmallString(myTeam->metalReceived).c_str());
	smallFont->glFormat(energyx + 0.044f, box.y2-2*globalRendering->pixelY, labelsFontSize, FONT_ASCENDER | fontOptions, "+%s",
	                FloatToSmallString(myTeam->prevEnergyIncome).c_str());
	                // FloatToSmallString(myTeam->energyReceived).c_str());

	smallFont->SetTextColor(1,1,1,0.8f);
	smallFont->glPrint(energybarx2 - 0.01f, energyy - 0.005f, labelsFontSize, fontOptions,
	                FloatToSmallString(myTeam->energyStorage));
	smallFont->glPrint(energybarx1 + energybarlen / 2.0f, energyy - 0.005f, labelsFontSize, fontOptions,
	                FloatToSmallString(myTeam->energy));

	smallFont->glPrint(metalbarx2 - 0.01f, metaly - 0.005f, labelsFontSize, fontOptions,
	                FloatToSmallString(myTeam->metalStorage));
	smallFont->glPrint(metalbarx1 + metalbarlen / 2.0f, metaly - 0.005f, labelsFontSize, fontOptions,
	                FloatToSmallString(myTeam->metal));

	smallFont->End();
}


bool CResourceBar::IsAbove(int x, int y)
{
	if (disabled) {
		return false;
	}

	const float mx=MouseX(x);
	const float my=MouseY(y);
	if(InBox(mx,my,box))
		return true;
	return false;
}


std::string CResourceBar::GetTooltip(int x, int y)
{
	const float mx=MouseX(x);

	if (mx < (box.x1 + 0.36f)) {
		return "Shows your stored metal as well as\n"
		       "income(\xff\x40\xff\x40green\xff\xff\xff\xff) and expenditures (\xff\xff\x20\x20red\xff\xff\xff\xff).\n"
		       "Click in the bar to select your\n"
		       "AutoShareLevel.";
	}
	return "Shows your stored energy as well as\n"
	       "income(\xff\x40\xff\x40green\xff\xff\xff\xff) and expenditures (\xff\xff\x20\x20red\xff\xff\xff\xff).\n"
	       "Click in the bar to select your\n"
	       "AutoShareLevel.";
}


bool CResourceBar::MousePress(int x, int y, int button)
{
	if (disabled) {
		return false;
	}

	const float mx=MouseX(x);
	const float my=MouseY(y);

	if(InBox(mx,my,box)){
		moveBox=true;
		if(!gu->spectating){
			if(InBox(mx,my,box+metalBox)){
				moveBox = false;
				float metalShare = std::max(0.f, std::min(1.f,(mx-(box.x1+metalBox.x1))/(metalBox.x2-metalBox.x1)));
				net->Send(CBaseNetProtocol::Get().SendSetShare(gu->myPlayerNum, gu->myTeam, metalShare, teamHandler->Team(gu->myTeam)->energyShare));
			}
			if(InBox(mx,my,box+energyBox)){
				moveBox = false;
				float energyShare = std::max(0.f, std::min(1.f,(mx-(box.x1+energyBox.x1))/(energyBox.x2-energyBox.x1)));
				net->Send(CBaseNetProtocol::Get().SendSetShare(gu->myPlayerNum, gu->myTeam, teamHandler->Team(gu->myTeam)->metalShare, energyShare));
			}
		}
		return true;
	}
	return false;
}


void CResourceBar::MouseMove(int x, int y, int dx,int dy, int button)
{
	if (disabled) {
		return;
	}

	if(moveBox){
		box.x1+=float(dx)/globalRendering->viewSizeX;
		box.x2+=float(dx)/globalRendering->viewSizeX;
		box.y1-=float(dy)/globalRendering->viewSizeY;
		box.y2-=float(dy)/globalRendering->viewSizeY;
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
