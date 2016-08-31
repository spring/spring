/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ResourceBar.h"
#include "GuiHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Fonts/glFont.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/TimeProfiler.h"
#include "System/myMath.h"

CResourceBar* resourceBar = NULL;


CResourceBar::CResourceBar()
	: moveBox(false)
	, enabled(true)
{
	box.x1 = 0.26f;
	box.y1 = 0.97f;
	box.x2 = 1;
	box.y2 = 1;

	metalBox.x1 = 0.09f;
	metalBox.y1 = 0.01f;
	metalBox.x2 = (box.x2 - box.x1) / 2.0f - 0.03f;
	metalBox.y2 = metalBox.y1 + (box.y2 - box.y1);   // extend to the very top of the screen

	energyBox.x1 = 0.48f;
	energyBox.y1 = 0.01f;
	energyBox.x2 = box.x2 - 0.03f - box.x1;
	energyBox.y2 = energyBox.y1 + (box.y2 - box.y1); // extend to the very top of the screen
}


CResourceBar::~CResourceBar()
{
}


static std::string FloatToSmallString(float num, float mul = 1) {

	char c[50];

	if (num == 0)
		sprintf(c, "0");
	if (       math::fabs(num) < (10       * mul)) {
		sprintf(c, "%.1f",  num);
	} else if (math::fabs(num) < (10000    * mul)) {
		sprintf(c, "%.0f",  num);
	} else if (math::fabs(num) < (10000000 * mul)) {
		sprintf(c, "%.0fk", num / 1000);
	} else {
		sprintf(c, "%.0fM", num / 1000000);
	}

	return c;
}


void CResourceBar::Draw()
{
	if (!enabled) {
		return;
	}

	const CTeam* myTeam = teamHandler->Team(gu->myTeam);

	GLfloat x1, y1, x2, y2, x;

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

	// layout metal in box
	GLfloat metalx = box.x1 + .01f;
	GLfloat metaly = box.y1 + .004f;

	GLfloat metalbarx1 = metalx + .08f;
	GLfloat metalbarx2 = box.x1 + (box.x2 - box.x1) / 2.0f - .03f;

	// metal layout
	GLfloat metalbarlen = metalbarx2 - metalbarx1;
	x1 = metalbarx1;
	y1 = metaly + .014f;
	x2 = metalbarx2;
	y2 = metaly + .020f;

	if (myTeam->resStorage.metal != 0.0f)
		x = (myTeam->res.metal / myTeam->resStorage.metal) * metalbarlen;
	else
		x = 0.0f;

	// metal draw
	glColor4f(0.8f, 0.8f, 0.8f, 0.2f);
	glVertex2f(x1, y1);
	glVertex2f(x1, y2);
	glVertex2f(x2, y2);
	glVertex2f(x2, y1);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glVertex2f(x1,     y1);
	glVertex2f(x1,     y2);
	glVertex2f(x1 + x, y2);
	glVertex2f(x1 + x, y1);

	x = myTeam->resShare.metal * metalbarlen;
	glColor4f(0.9f, 0.2f, 0.2f, 0.7f);
	glVertex2f(x1 + x + 0.003f, y1 - 0.003f);
	glVertex2f(x1 + x + 0.003f, y2 + 0.003f);
	glVertex2f(x1 + x - 0.003f, y2 + 0.003f);
	glVertex2f(x1 + x - 0.003f, y1 - 0.003f);

	// Energy

	// layout energy in box
	GLfloat energyx = box.x1 + 0.4f;
	GLfloat energyy = box.y1 + 0.004f;

	GLfloat energybarx1 = energyx + 0.08f;
	GLfloat energybarx2 = box.x2 - 0.03f;

	// energy layout
	GLfloat energybarlen = energybarx2 - energybarx1;
	x1 = energybarx1;
	y1 = energyy + 0.014f;
	x2 = energybarx2;
	y2 = energyy + 0.020f;

	if (myTeam->resStorage.energy != 0.0f) {
		x = (myTeam->res.energy / myTeam->resStorage.energy) * energybarlen;
	}

	// energy draw
	glColor4f(0.8f, 0.8f, 0.2f, 0.2f);
	glVertex2f(x1, y1);
	glVertex2f(x1, y2);
	glVertex2f(x2, y2);
	glVertex2f(x2, y1);

	glColor4f(1.0f, 1.0f, 0.2f, 1.0f);
	glVertex2f(x1,     y1);
	glVertex2f(x1,     y2);
	glVertex2f(x1 + x, y2);
	glVertex2f(x1 + x, y1);

	x = myTeam->resShare.energy * energybarlen;
	glColor4f(0.9f, 0.2f, 0.2f, 0.7f);
	glVertex2f(x1 + x + 0.003f, y1 - 0.003f);
	glVertex2f(x1 + x + 0.003f, y2 + 0.003f);
	glVertex2f(x1 + x - 0.003f, y2 + 0.003f);
	glVertex2f(x1 + x - 0.003f, y1 - 0.003f);
	glEnd();


	smallFont->Begin();

	const float headerFontSize = (box.y2 - box.y1) * 0.7 * globalRendering->viewSizeY;
	const float labelsFontSize = (box.y2 - box.y1) * 0.5 * globalRendering->viewSizeY;
	const int fontOptions = (guihandler && guihandler->GetOutlineFonts()) ? FONT_OUTLINE | FONT_NORM : FONT_NORM;

	smallFont->SetTextColor(0.8f, 0.8f, 1, 0.8f);
	smallFont->glPrint(metalx - 0.004f,  (box.y1 + box.y2) * 0.5, headerFontSize, FONT_VCENTER | fontOptions, "Metal");

	smallFont->SetTextColor(1, 1, 0.4f, 0.8f);
	smallFont->glPrint(energyx - 0.018f, (box.y1 + box.y2) * 0.5, headerFontSize, FONT_VCENTER | fontOptions, "Energy");

	smallFont->SetTextColor(1.0f, 0.3f, 0.3f, 1.0f); // Expenses
	smallFont->glFormat(metalx  + 0.044f, box.y1, labelsFontSize, FONT_DESCENDER | fontOptions, "-%s(-%s)",
			FloatToSmallString(math::fabs(myTeam->resPrevPull.metal)).c_str(),
			FloatToSmallString(math::fabs(myTeam->resSent.metal)).c_str());
	smallFont->glFormat(energyx + 0.044f, box.y1, labelsFontSize, FONT_DESCENDER | fontOptions, "-%s(-%s)",
			FloatToSmallString(math::fabs(myTeam->resPrevPull.energy)).c_str(),
			FloatToSmallString(math::fabs(myTeam->resSent.energy)).c_str());

	smallFont->SetTextColor(0.4f, 1.0f, 0.4f, 0.95f); // Income
	smallFont->glFormat(metalx  + 0.044f, box.y2 - 2*globalRendering->pixelY, labelsFontSize, FONT_ASCENDER | fontOptions, "+%s",
			FloatToSmallString(myTeam->resPrevIncome.metal).c_str());
			// FloatToSmallString(myTeam->metalReceived).c_str());
	smallFont->glFormat(energyx + 0.044f, box.y2 - 2*globalRendering->pixelY, labelsFontSize, FONT_ASCENDER | fontOptions, "+%s",
			FloatToSmallString(myTeam->resPrevIncome.energy).c_str());
			// FloatToSmallString(myTeam->energyReceived).c_str());

	smallFont->SetTextColor(1, 1, 1, 0.8f);
	smallFont->glPrint(energybarx2 - 0.01f, energyy - 0.005f, labelsFontSize, fontOptions,
			FloatToSmallString(myTeam->resStorage.energy));
	smallFont->glPrint(energybarx1 + energybarlen / 2.0f, energyy - 0.005f, labelsFontSize, fontOptions,
			FloatToSmallString(myTeam->res.energy));

	smallFont->glPrint(metalbarx2 - 0.01f, metaly - 0.005f, labelsFontSize, fontOptions,
			FloatToSmallString(myTeam->resStorage.metal));
	smallFont->glPrint(metalbarx1 + metalbarlen / 2.0f, metaly - 0.005f, labelsFontSize, fontOptions,
			FloatToSmallString(myTeam->res.metal));

	smallFont->End();
}


bool CResourceBar::IsAbove(int x, int y)
{
	if (!enabled) {
		return false;
	}

	const float mx = MouseX(x);
	const float my = MouseY(y);
	return InBox(mx, my, box);
}


std::string CResourceBar::GetTooltip(int x, int y)
{
	const float mx = MouseX(x);

	std::string resourceName;
	if (mx < (box.x1 + 0.36f)) {
		resourceName = "metal";
	} else {
		resourceName = "energy";
	}

	return "Shows your stored " + resourceName + " as well as\n"
	       "income(\xff\x40\xff\x40green\xff\xff\xff\xff) and expenditures (\xff\xff\x20\x20red\xff\xff\xff\xff).\n"
	       "Click in the bar to select your\n"
	       "AutoShareLevel.";
}


bool CResourceBar::MousePress(int x, int y, int button)
{
	if (!enabled) {
		return false;
	}

	const float mx = MouseX(x);
	const float my = MouseY(y);

	if (InBox(mx, my, box)) {
		moveBox = true;
		if (!gu->spectating) {
			if (InBox(mx, my, box + metalBox)) {
				moveBox = false;
				const float metalShare = Clamp((mx - (box.x1 + metalBox.x1)) / (metalBox.x2 - metalBox.x1), 0.f, 1.f);
				clientNet->Send(CBaseNetProtocol::Get().SendSetShare(gu->myPlayerNum, gu->myTeam, metalShare, teamHandler->Team(gu->myTeam)->resShare.energy));
			}
			if (InBox(mx, my, box + energyBox)) {
				moveBox = false;
				const float energyShare = Clamp((mx - (box.x1 + energyBox.x1)) / (energyBox.x2 - energyBox.x1), 0.f, 1.f);
				clientNet->Send(CBaseNetProtocol::Get().SendSetShare(gu->myPlayerNum, gu->myTeam, teamHandler->Team(gu->myTeam)->resShare.metal, energyShare));
			}
		}
		return true;
	}
	return false;
}


void CResourceBar::MouseMove(int x, int y, int dx, int dy, int button)
{
	if (!enabled) {
		return;
	}

	if (moveBox) {
		box.x1 += float(dx) / globalRendering->viewSizeX;
		box.x2 += float(dx) / globalRendering->viewSizeX;
		box.y1 -= float(dy) / globalRendering->viewSizeY;
		box.y2 -= float(dy) / globalRendering->viewSizeY;
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
