/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "InfoConsole.h"
#include "GuiHandler.h"
#include "Rendering/Fonts/glFont.h"
#include "System/EventHandler.h"
#include "System/SafeUtil.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/LogSinkHandler.h"

#define border 7

CONFIG(int, InfoMessageTime).defaultValue(10).description("Time until old messages disappear from the ingame console.");
CONFIG(std::string, InfoConsoleGeometry).defaultValue("0.26 0.96 0.41 0.205");

CInfoConsole* infoConsole = nullptr;

static uint8_t infoConsoleMem[sizeof(CInfoConsole)];




void CInfoConsole::InitStatic() {
	assert(infoConsole == nullptr);
	infoConsole = new (infoConsoleMem) CInfoConsole();
}

void CInfoConsole::KillStatic() {
	assert(infoConsole != nullptr);
	spring::SafeDestruct(infoConsole);
	std::memset(infoConsoleMem, 0, sizeof(infoConsoleMem));
}


void CInfoConsole::Init()
{
	maxLines = 1;
	newLines = 0;

	msgPosIndx = 0;
	numPosMsgs = 0;

	rawId = 0;
	lifetime = configHandler->GetInt("InfoMessageTime");

	xpos = 0.0f;
	ypos = 0.0f;
	width = 0.0f;
	height = 0.0f;

	fontScale = 1.0f;
	fontSize = fontScale * smallFont->GetSize();;

	const std::string geo = configHandler->GetString("InfoConsoleGeometry");

	if (sscanf(geo.c_str(), "%f %f %f %f", &xpos, &ypos, &width, &height) != 4) {
		xpos = 0.26f;
		ypos = 0.96f;
		width = 0.41f;
		height = 0.205f;
	}

	enabled = (width != 0.0f && height != 0.0f);
	inited = true;


	logSinkHandler.AddSink(this);
	eventHandler.AddClient(this);

	rawData.clear();
	data.clear();

	lastMsgPositions.fill(ZeroVector);
}

void CInfoConsole::Kill()
{
	logSinkHandler.RemoveSink(this);
	eventHandler.RemoveClient(this);

	inited = false;
}


void CInfoConsole::Draw()
{
	if (!enabled)
		return;
	if (smallFont == nullptr)
		return;
	if (data.empty())
		return;

	std::lock_guard<spring::recursive_mutex> scoped_lock(infoConsoleMutex);

	// infoConsole exists before guiHandler does, but is never drawn during that period
	assert(guihandler != nullptr);

	// always draw outlined text here, saves an ugly black background
	// (the default console is practically no longer used in any case)
	const uint32_t fontOptions = FONT_NORM | FONT_OUTLINE | FONT_BUFFERED;
	const float fontHeight = fontSize * smallFont->GetLineHeight() * globalRendering->pixelY;

	float curX = xpos + border * globalRendering->pixelX;
	float curY = ypos - border * globalRendering->pixelY;

	smallFont->SetTextColor(1.0f, 1.0f, 1.0f, 1.0f);
	smallFont->SetOutlineColor(0.0f, 0.0f, 0.0f, 1.0f);

	for (int i = 0; i < std::min(data.size(), maxLines); ++i) {
		smallFont->glPrint(curX, curY -= fontHeight, fontSize, fontOptions, data[i].text);
	}

	smallFont->DrawBufferedGL4();
	smallFont->SetColors(); // default
}


void CInfoConsole::Update()
{
	std::lock_guard<spring::recursive_mutex> scoped_lock(infoConsoleMutex);
	if (data.empty())
		return;

	// pop old messages after timeout
	if (data[0].timeout <= spring_gettime())
		data.pop_front();

	if (smallFont == nullptr)
		return;

	// if we have more lines then we can show, remove the
	// oldest and make sure the others are shown long enough
	const float  maxHeight = (height * globalRendering->viewSizeY) - (border * 2);
	const float fontHeight = smallFont->GetLineHeight();

	// height=0 will likely be the case on HEADLESS only
	maxLines = (fontHeight > 0.0f)? math::floor(maxHeight / (fontSize * fontHeight)): 1;

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
		std::lock_guard<spring::recursive_mutex> scoped_lock(infoConsoleMutex);

		const int count = (int)rawData.size();
		const int start = count - newLines;
		for (int i = start; i < count; i++) {
			newRawLines.push_back(rawData[i]);
		}
		newLines = 0;
	}

	for (const RawLine& rawLine: newRawLines) {
		eventHandler.AddConsoleLine(rawLine.text, rawLine.section, rawLine.level);
	}
}


int CInfoConsole::GetRawLines(std::deque<RawLine>& lines)
{
	std::lock_guard<spring::recursive_mutex> scoped_lock(infoConsoleMutex);
	lines = rawData;
	const int numNewLines = newLines;
	newLines = 0;
	return numNewLines;
}


void CInfoConsole::RecordLogMessage(int level, const std::string& section, const std::string& message)
{
	std::lock_guard<spring::recursive_mutex> scoped_lock(infoConsoleMutex);

	if (section == prvSection && message == prvMessage)
		return;

	newLines += (newLines < maxRawLines);

	if (rawData.size() > maxRawLines)
		rawData.pop_front();

	rawData.emplace_back(prvMessage = message, prvSection = section, level, rawId++);

	if (smallFont == nullptr)
		return;

	// NOTE
	//   do not remove elements from `data` here, ::Draw iterates over it
	//   and can call LOG() which will end up back in ::RecordLogMessage
	const std::string& wrappedText = smallFont->Wrap(message, fontSize, (width * globalRendering->viewSizeX) - (2 * border));
	const std::u8string& unicodeText = toustring(wrappedText);

	std::deque<std::string> lines = std::move(smallFont->SplitIntoLines(unicodeText));

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
	// reset index to head when a new msg comes in
	msgPosIndx  = numPosMsgs % lastMsgPositions.size();
	numPosMsgs += 1;

	lastMsgPositions[msgPosIndx] = pos;
}

const float3& CInfoConsole::GetMsgPos(const float3& defaultPos)
{
	if (numPosMsgs == 0)
		return defaultPos;

	const float3& p = lastMsgPositions[msgPosIndx];

	// cycle to previous position
	msgPosIndx += (std::min(numPosMsgs, lastMsgPositions.size()) - 1);
	msgPosIndx %= std::min(numPosMsgs, lastMsgPositions.size());

	return p;
}

