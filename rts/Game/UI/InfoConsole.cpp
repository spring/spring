/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Rendering/GL/myGL.h"
#include <fstream>

#include "mmgr.h"

#include "InfoConsole.h"
#include "GuiHandler.h"
#include "Rendering/glFont.h"

#ifdef WIN32
#  include "Platform/Win/win32.h"
#endif

#include "Sync/SyncTracer.h"
#include "ConfigHandler.h"
#include "InputReceiver.h"

#define border 7

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

const size_t CInfoConsole::maxRawLines   = 1024;
const size_t CInfoConsole::maxLastMsgPos = 10;

CInfoConsole::CInfoConsole() :
	  fontScale(1.0f)
	, disabled(false)
	, lastMsgIter(lastMsgPositions.begin())
	, newLines(0)
	, rawId(0)
	, lastTime(0)
{
	data.clear();

	wantLogInformationPrefix = configHandler->Get("DisplayDebugPrefixConsole", false);
	lifetime = configHandler->Get("InfoMessageTime", 400);

	const std::string geo = configHandler->GetString("InfoConsoleGeometry",
                                                  "0.26 0.96 0.41 0.205");
	const int vars = sscanf(geo.c_str(), "%f %f %f %f",
	                        &xpos, &ypos, &width, &height);
	if (vars != 4) {
		xpos = 0.26f;
		ypos = 0.96f;
		width = 0.41f;
		height = 0.205f;
	}

	fontSize = fontScale * smallFont->GetSize();

	logOutput.AddSubscriber(this);
}

CInfoConsole::~CInfoConsole()
{
	logOutput.RemoveSubscriber(this);
}

void CInfoConsole::Draw()
{
	if (disabled) return;
	if (!smallFont) return;

	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex); //is this really needed?

	if(!data.empty() && (guihandler && !guihandler->GetOutlineFonts())){
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

	const float fontHeight = fontSize * smallFont->GetLineHeight() * globalRendering->pixelY;

	float curX = xpos + border * globalRendering->pixelX;
	float curY = ypos - border * globalRendering->pixelY;

	smallFont->Begin();
	smallFont->SetColors(); // default
	int fontOptions = FONT_NORM;
	if (guihandler && guihandler->GetOutlineFonts())
		fontOptions |= FONT_OUTLINE;

	std::deque<InfoLine>::iterator ili;
	for (ili = data.begin(); ili != data.end(); ++ili) {
		curY -= fontHeight;
		smallFont->glPrint(curX, curY, fontSize, fontOptions, ili->text);
	}

	smallFont->End();
}


void CInfoConsole::Update()
{
	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);
	if (lastTime > 0) {
		lastTime--;
	}
	if (!data.empty()) {
		data.begin()->time--;
		if (data[0].time <= 0) {
			data.pop_front();
		}
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


void CInfoConsole::NotifyLogMsg(const CLogSubsystem& subsystem, const std::string& text)
{
	if (!smallFont) return;

	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);

	RawLine rl(text, &subsystem, rawId);
	rawId++;
	rawData.push_back(rl);
	if (rawData.size() > maxRawLines) {
		rawData.pop_front();
	}
	if (newLines < maxRawLines) {
		newLines++;
	}

	const float maxWidth  = width * globalRendering->viewSizeX - border * 2;
	const float maxHeight = height * globalRendering->viewSizeY - border * 2;
	const unsigned int numLines = math::floor(maxHeight / (fontSize * smallFont->GetLineHeight()));

	std::list<std::string> lines = smallFont->Wrap(text,fontSize,maxWidth);

	std::list<std::string>::iterator il;
	for (il = lines.begin(); il != lines.end(); ++il) {
		//! add the line to the console
		InfoLine l;
		data.push_back(l);
		data.back().text = *il;
		data.back().time = lifetime - lastTime;
		lastTime = lifetime;
	}

	for (size_t i = data.size(); i > numLines; i--) {
		data[1].time += data[0].time;
		data.pop_front();
	}
}


void CInfoConsole::SetLastMsgPos(const float3& pos)
{
	if (lastMsgPositions.size() < maxLastMsgPos) {
		lastMsgPositions.push_front(pos);
	} else {
		lastMsgPositions.push_front(pos);
		lastMsgPositions.pop_back();
	}

	//! reset the iterator when a new msg comes in
	lastMsgIter = lastMsgPositions.begin();
}

const float3& CInfoConsole::GetMsgPos()
{
	if (lastMsgPositions.empty()) {
		return ZeroVector;
	}

	// advance the position
	const float3& p = *(lastMsgIter++);

	// wrap around
	if (lastMsgIter == lastMsgPositions.end()) {
		lastMsgIter = lastMsgPositions.begin();
	}

	return p;
}
