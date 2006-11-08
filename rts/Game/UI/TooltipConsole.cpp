#include "StdAfx.h"
#include "TooltipConsole.h"
#include "MouseHandler.h"
#include "OutlineFont.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/glFont.h"
#include "System/Platform/ConfigHandler.h"
#include "mmgr.h"


CTooltipConsole* tooltip = 0;


CTooltipConsole::CTooltipConsole(void) : disabled(false)
{
	const std::string geo = configHandler.GetString("TooltipGeometry",
	                                                "0.0 0.0 0.41 0.1");
	const int vars = sscanf(geo.c_str(), "%f %f %f %f", &x, &y, &w, &h);
	if (vars != 4) {
		x = 0.00f;
		y = 0.00f;
		w = 0.41f;
		h = 0.10f;
	}

	outFont = !!configHandler.GetInt("TooltipOutlineFont", 0);
}


CTooltipConsole::~CTooltipConsole(void)
{
}


// FIXME -- duplication
static string StripColorCodes(const string& text)
{
	std::string nocolor;
	const int len = (int)text.size();
	for (int i = 0; i < len; i++) {
		if ((unsigned char)text[i] == 255) {
			i = i + 3;
		} else {
			nocolor += text[i];
		}
	}
	return nocolor;
}


void CTooltipConsole::Draw(void)
{
	if (disabled) {
		return;
	}
	
	const std::string s = mouse->GetCurrentTooltip();

	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	if (!outFont) {
		glColor4f(0.2f, 0.2f, 0.2f, CInputReceiver::guiAlpha);
		glRectf(x, y, (x + w), (y + h));
	}

	const float yScale = 0.015f;
	const float xScale = (yScale / gu->aspectRatio) * 1.2f;
	const float xPixel  = 1.0f / (xScale * (float)gu->screenx);
	const float yPixel  = 1.0f / (yScale * (float)gu->screeny);
	
	glTranslatef(x + 0.01f, y + 0.08f, 0.0f);
	glScalef(xScale, yScale, 1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 0.8f);

	glEnable(GL_TEXTURE_2D);

	unsigned int p = 0;
	while (p < s.size()) {
		std::string s2;
		int pos = 0;
		for (int a = 0; a < 420; ++a) {
			s2 += s[p];
			if ((s[p++] == '\n') || (p >= s.size())) {
				break;
			}
		}
		if (!outFont) {
			font->glPrintColor("%s", s2.c_str());
		} else {
			const float color[4] = { 1.0f, 1.0f, 1.0f, 0.0f };
			outlineFont.print(xPixel, yPixel, color, StripColorCodes(s2).c_str());
			font->glPrintColor("%s", s2.c_str());
		}
		glTranslatef(0.0f, -1.2f, 0.0f);
	}

	glPopMatrix();
}


bool CTooltipConsole::IsAbove(int x,int y)
{
	if (disabled) {
		return false;
	}
	
	const float mx = MouseX(x);
	const float my = MouseY(y);
	
	return ((mx > (x + 0.01f)) && (mx < (x + w)) &&
	        (my > (y + 0.01f)) && (my < (y + h)));
}
