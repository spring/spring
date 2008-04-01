#include "StdAfx.h"
// InfoConsole.cpp: implementation of the CInfoConsole class.
//
//////////////////////////////////////////////////////////////////////

#include "InfoConsole.h"
#include <fstream>
#include "OutlineFont.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/myGL.h"

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

const int CInfoConsole::maxRawLines = 1024;


CInfoConsole::CInfoConsole()
: lastMsgPos(0,0,0), disabled(false), newLines(0), rawId(0)
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

	if(!data.empty() && !outlineFont.IsEnabled()){
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.2f, 0.2f, 0.2f, CInputReceiver::guiAlpha);

		glBegin(GL_TRIANGLE_STRIP);
			glVertex3f(xpos,ypos,0);
			glVertex3f(xpos+width,ypos,0);
			glVertex3f(xpos,ypos-height,0);
			glVertex3f(xpos+width,ypos-height,0);
		glEnd();
	}

	const float fontScale = 0.6f;
	const float fontHeight = fontScale * font->GetHeight();

	float curX = xpos + 0.01f;
	float curY = ypos - 0.026f;

	if (!outlineFont.IsEnabled()) {
		glColor4f(1,1,1,1);

		std::deque<InfoLine>::iterator ili;
		for (ili = data.begin(); ili != data.end(); ili++) {
			font->glPrintAt(curX, curY, fontScale, ili->text.c_str());
			curY -= fontHeight;
		}
	}
	else {
		const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

		std::deque<InfoLine>::iterator ili;
		for (ili = data.begin(); ili != data.end(); ili++) {
			font->glPrintOutlinedAt(curX, curY, fontScale, ili->text.c_str(), white);
			curY -= fontHeight;
		}
	}
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


int CInfoConsole::GetRawLines(std::deque<RawLine>& lines)
{
	int tmp;
	{
		boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);
		lines = rawData;
		tmp = newLines;
		newLines = 0;
	}
	return tmp;
}


void CInfoConsole::GetNewRawLines(std::vector<RawLine>& lines)
{
	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);
	const int count = (int)rawData.size();
	const int start = count - newLines;
	for (int i = start; i < count; i++) {
		lines.push_back(rawData[i]);
	}
	newLines = 0;
}


void CInfoConsole::NotifyLogMsg(int zone, const char *text)
{
	PUSH_CODE_MODE;
	ENTER_MIXED;
	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);

	RawLine rl;
	rl.text = text;
	rl.zone = zone;
	rl.time = 0; // filled in later
	rl.id = rawId;
	rawId++;
	rawData.push_back(rl);
	if (rawData.size() > maxRawLines) {
		rawData.pop_front();
	}
	if (newLines < maxRawLines) {
		newLines++;
	}

	float maxWidth = 25.0f;
	int pos=0, line_start=0;

	while (text[pos]) {
		// iterate through text until maxWidth width is reached
		char temp[120];
		float w = 0.0f;
		for (;text[pos] && pos-line_start < sizeof(temp) - 1 && w <= maxWidth;pos++) {
			w += font->CalcCharWidth(text[pos]);
			temp[pos-line_start] = text[pos];
		}
		temp[pos-line_start] = 0;

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
