/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/myGL.h"


#include "InfoConsole.h"
#include "InputReceiver.h"
#include "GuiHandler.h"
#include "Rendering/glFont.h"
#include "System/EventHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/LogSinkHandler.h"

#include <fstream>

#define border 7

CONFIG(int, InfoMessageTime).defaultValue(400);
CONFIG(std::string, InfoConsoleGeometry).defaultValue("0.26 0.96 0.41 0.205");

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

const size_t CInfoConsole::maxRawLines   = 1024;
const size_t CInfoConsole::maxLastMsgPos = 10;

CInfoConsole::CInfoConsole()
	: CEventClient("InfoConsole", 999, false)
	, fontScale(1.0f)
	, enabled(true)
	, lastMsgIter(lastMsgPositions.begin())
	, newLines(0)
	, rawId(0)
	, lastTime(0)
{
	data.clear();

	lifetime = configHandler->GetInt("InfoMessageTime");

	const std::string geo = configHandler->GetString("InfoConsoleGeometry");
	const int vars = sscanf(geo.c_str(), "%f %f %f %f",
	                        &xpos, &ypos, &width, &height);
	if (vars != 4) {
		xpos = 0.26f;
		ypos = 0.96f;
		width = 0.41f;
		height = 0.205f;
	}

	fontSize = fontScale * smallFont->GetSize();

	logSinkHandler.AddSink(this);
	eventHandler.AddClient(this);
}

CInfoConsole::~CInfoConsole()
{
	logSinkHandler.RemoveSink(this);
	eventHandler.RemoveClient(this);
}

void CInfoConsole::Draw()
{
	if (!enabled) return;
	if (!smallFont) return;

	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex); // XXX is this really needed?

	if(!data.empty() && (guihandler && !guihandler->GetOutlineFonts())){
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.2f, 0.2f, 0.2f, CInputReceiver::guiAlpha);

		glBegin(GL_TRIANGLE_STRIP);
			glVertex3f(xpos,         ypos,          0);
			glVertex3f(xpos + width, ypos,          0);
			glVertex3f(xpos,         ypos - height, 0);
			glVertex3f(xpos + width, ypos - height, 0);
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

void CInfoConsole::PushNewLinesToEventHandler()
{
	if (newLines == 0)
		return;

	std::deque<RawLine> newRawLines;

	{
		boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);

		const int count = (int)rawData.size();
		const int start = count - newLines;
		for (int i = start; i < count; i++) {
			const RawLine& rawLine = rawData[i];
			newRawLines.push_back(rawLine);
		}
		newLines = 0;
	}

	for (std::deque<RawLine>::iterator it = newRawLines.begin(); it != newRawLines.end(); ++it) {
		const RawLine& rawLine = (*it);
		eventHandler.AddConsoleLine(rawLine.text, rawLine.section, rawLine.level);
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


void CInfoConsole::RecordLogMessage(const std::string& section, int level,
			const std::string& text)
{
	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);

	RawLine rl(text, section, level, rawId);
	rawId++;
	rawData.push_back(rl);
	if (rawData.size() > maxRawLines) {
		rawData.pop_front();
	}
	if (newLines < maxRawLines) {
		newLines++;
	}

	if (!smallFont) {
		return;
	}

	const float maxWidth  = (width  * globalRendering->viewSizeX) - (border * 2);
	const float maxHeight = (height * globalRendering->viewSizeY) - (border * 2);
	const unsigned int maxLines = (smallFont->GetLineHeight() > 0)
			? math::floor(maxHeight / (fontSize * smallFont->GetLineHeight()))
			: 1; // this will likely be the case on HEADLESS only

	std::list<std::string> lines = smallFont->Wrap(text, fontSize, maxWidth);

	std::list<std::string>::iterator il;
	for (il = lines.begin(); il != lines.end(); ++il) {
		// add the line to the console
		InfoLine l;
		data.push_back(l);
		data.back().text = *il;
		data.back().time = lifetime - lastTime;
		lastTime = lifetime;
	}

	// if we have more lines then we can show, remove the oldest one,
	// and make sure the others are shown long enough
	for (size_t i = data.size(); i > maxLines; i--) {
		data[1].time += data[0].time;
		data.pop_front();
	}
}


void CInfoConsole::LastMessagePosition(const float3& pos)
{
	if (lastMsgPositions.size() < maxLastMsgPos) {
		lastMsgPositions.push_front(pos);
	} else {
		lastMsgPositions.push_front(pos);
		lastMsgPositions.pop_back();
	}

	// reset the iterator when a new msg comes in
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
