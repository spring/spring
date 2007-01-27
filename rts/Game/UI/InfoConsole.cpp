#include "StdAfx.h"
// InfoConsole.cpp: implementation of the CInfoConsole class.
//
//////////////////////////////////////////////////////////////////////

#include "InfoConsole.h"
#include "OutlineFont.h"
#include "GuiHandler.h"
#include "Rendering/GL/myGL.h"
#include <fstream>
#include "Rendering/glFont.h"

#ifdef WIN32
#include "Platform/Win/win32.h"
#endif
 
#include "Sync/SyncTracer.h"
#include "Platform/ConfigHandler.h"
#include "InputReceiver.h"

#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CInfoConsole::CInfoConsole()
: lastMsgPos(0,0,0), disabled(false)
{
	data.clear();

	lastTime=0;
	lifetime     = configHandler.GetInt("InfoMessageTime", 400);
	verboseLevel = configHandler.GetInt("VerboseLevel", 0);

	const std::string geo = configHandler.GetString("InfoConsoleGeometry",
                                                  "0.26 0.96 0.41 0.205");
	const int vars = sscanf(geo.c_str(), "%f %f %f %f",
	                        &xpos, &ypos, &width, &height);
	if (vars != 4) {
		xpos = 0.26f;
		ypos = 0.96f;
		width = 0.41f;
		height = 0.205f;
	}

	numLines = 8;

	logOutput.AddSubscriber(this);
}

CInfoConsole::~CInfoConsole()
{
	logOutput.RemoveSubscriber(this);
}

void CInfoConsole::Draw()
{
	if (disabled) {
		return;
	}

	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);

	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glColor4f(0.2f, 0.2f, 0.2f, CInputReceiver::guiAlpha);

	if(!data.empty() && !outlineFont.IsEnabled()){
		glBegin(GL_TRIANGLE_STRIP);
			glVertex3f(xpos,ypos,0);
			glVertex3f(xpos+width,ypos,0);
			glVertex3f(xpos,ypos-height,0);
			glVertex3f(xpos+width,ypos-height,0);
		glEnd();
	}

	const float xScale = 0.015f;
	const float yScale = 0.020f;
	
	glTranslatef(xpos + 0.01f, ypos - 0.026f, 0.0f);
	glScalef(xScale, yScale, 1.0f);

	glEnable(GL_TEXTURE_2D);

	if (!outlineFont.IsEnabled()) {
		glColor4f(1,1,1,1);

		std::deque<InfoLine>::iterator ili;
		for (ili = data.begin(); ili != data.end(); ili++) {
			glPushMatrix();
			font->glPrintRaw(ili->text.c_str());
			glPopMatrix();
			glTranslatef(0.0f, -1.2f, 0.0f);
		}
	}
	else {
		const float xPixel = 1.0f / (xScale * (float)gu->viewSizeX);
		const float yPixel = 1.0f / (yScale * (float)gu->viewSizeY);
		const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

		std::deque<InfoLine>::iterator ili;
		for (ili = data.begin(); ili != data.end(); ili++) {
			outlineFont.print(xPixel, yPixel, white, ili->text.c_str());
			glTranslatef(0.0f, -1.2f, 0.0f);
		}
	}

	glPopMatrix();
}


void CInfoConsole::Update()
{
	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);
	if(lastTime>0)
		lastTime--;
	if(!data.empty()){
		data.begin()->time--;
		if(data[0].time<=0)
			data.pop_front();
	}
}


void CInfoConsole::NotifyLogMsg(int priority, const char *text)
{
	if (priority > verboseLevel) {
		return;
	}
	
	PUSH_CODE_MODE;
	ENTER_MIXED;
	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);

	if (guihandler) {
		guihandler->AddConsoleLine(text, priority);
	}

	float maxWidth = 25.0f;
	int pos=0, line_start=0;

	while (text[pos]) {
		// iterate through text until maxWidth width is reached
		char temp[120];
		float w = 0.0f;
		for (;text[pos] && pos-line_start < sizeof(temp) - 1 && w <= maxWidth;pos++) {
			w += font->CalcCharWidth (text[pos]);
			temp[pos-line_start] = text[pos];
		}

		// if needed, find a breaking position
		if (w > maxWidth) {
			int break_pos = pos-line_start;
			while (break_pos >= 0 && temp[break_pos] != ' ')
				break_pos --;

			if (break_pos <= 0) break_pos = pos-line_start;
			line_start += break_pos + (temp[break_pos] == ' ' ? 1 : 0);
			pos = line_start;
			temp[break_pos] = 0;
		} else {
			temp[pos-line_start] = 0;
			line_start = pos;
		}

		// add the line to the console
		InfoLine l;
		if((int)data.size()>(numLines-1)){
			data[1].time+=data[0].time;
			data.pop_front();
		}
		data.push_back(l);
		data.back().text=temp;
		data.back().time=lifetime-lastTime;
		lastTime=lifetime;
	}
	
	POP_CODE_MODE;
}


void CInfoConsole::SetLastMsgPos(const float3& pos)
{
	lastMsgPos=pos;
}
