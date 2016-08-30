/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/myGL.h"

#include "InfoConsole.h"
#include "InputReceiver.h"
#include "GuiHandler.h"
#include "Rendering/Fonts/glFont.h"
#include "System/EventHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/LogSinkHandler.h"

#define border 7

CONFIG(int, InfoMessageTime).defaultValue(10).description("Timeout till old messages disappear from the ingame console.");
CONFIG(std::string, InfoConsoleGeometry).defaultValue("0.26 0.96 0.41 0.205");

CInfoConsole* infoConsole = nullptr;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

const size_t CInfoConsole::maxRawLines   = 1024;
const size_t CInfoConsole::maxLastMsgPos = 10;

CInfoConsole::CInfoConsole()
	: CEventClient("InfoConsole", 999, false)
	, enabled(true)
	, lastMsgIter(lastMsgPositions.begin())
	, newLines(0)
	, rawId(0)
	, fontScale(1.0f)
	, maxLines(1)
{
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

	if (width == 0.f || height == 0.f)
		enabled = false;

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
	if (data.empty()) return;

	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);

	if (guihandler && !guihandler->GetOutlineFonts()) {
		// draw a black background when not using outlined font
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

	for (int i = 0; i < std::min(data.size(), maxLines); ++i) {
		curY -= fontHeight;
		smallFont->glPrint(curX, curY, fontSize, fontOptions, data[i].text);
	}

	smallFont->End();
}


void CInfoConsole::Update()
{
	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);
	if (data.empty())
		return;

	// pop old messages after timeout
	if (data[0].timeout <= spring_gettime()) {
		data.pop_front();
	}

	if (!smallFont)
		return;

	// if we have more lines then we can show, remove the oldest one,
	// and make sure the others are shown long enough
	const float maxHeight = (height * globalRendering->viewSizeY) - (border * 2);
	maxLines = (smallFont->GetLineHeight() > 0)
			? math::floor(maxHeight / (fontSize * smallFont->GetLineHeight()))
			: 1; // this will likely be the case on HEADLESS only
	for (size_t i = data.size(); i > maxLines; i--) {
		data.pop_front();
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
	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);
	lines = rawData;
	int tmp = newLines;
	newLines = 0;
	return tmp;
}


void CInfoConsole::RecordLogMessage(const std::string& section, int level,
			const std::string& text)
{
	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);

	if (rawData.size() > maxRawLines) {
		rawData.pop_front();
	}
	if (newLines < maxRawLines) {
		++newLines;
	}
	rawData.emplace_back(text, section, level, rawId++);

	if (!smallFont) {
		return;
	}

	// !!! Warning !!!
	// We must not remove elements from `data` here
	// cause ::Draw() iterats that container, and it's
	// possible that it calls LOG(), which will end
	// in here. So if we would remove stuff here it's
	// possible that we delete a var that is used in
	// Draw() & co, and so we might invalidate references
	// (e.g. of std::strings) and cause SIGFAULTs!
	const float maxWidth = (width  * globalRendering->viewSizeX) - (2 * border);
	std::string wrappedText = smallFont->Wrap(text, fontSize, maxWidth);
	std::list<std::string> lines = smallFont->SplitIntoLines(toustring(wrappedText));

	for (auto& line: lines) {
		// add the line to the console
		data.emplace_back();
		InfoLine& l = data.back();
		l.text    = std::move(line);
		l.timeout = spring_gettime() + spring_secs(lifetime);
	}
}


void CInfoConsole::LastMessagePosition(const float3& pos)
{
	if (lastMsgPositions.size() >= maxLastMsgPos) {
		lastMsgPositions.pop_back();
	}
	lastMsgPositions.push_front(pos);

	// reset the iterator when a new msg comes in
	lastMsgIter = lastMsgPositions.begin();
}

const float3& CInfoConsole::GetMsgPos(const float3& defaultPos)
{
	if (lastMsgPositions.empty())
		return defaultPos;

	// advance the position
	const float3& p = *(lastMsgIter++);

	// wrap around
	if (lastMsgIter == lastMsgPositions.end()) {
		lastMsgIter = lastMsgPositions.begin();
	}

	return p;
}
