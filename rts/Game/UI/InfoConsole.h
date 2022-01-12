/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef INFO_CONSOLE_H
#define INFO_CONSOLE_H

#include "InputReceiver.h"
#include "System/float3.h"
#include "System/EventClient.h"
#include "System/Log/LogSinkHandler.h"
#include "System/Misc/SpringTime.h"
#include "System/Threading/SpringThreading.h"

#include <array>
#include <deque>
#include <string>

class CInfoConsole: public CInputReceiver, public CEventClient, public ILogSink
{
public:
	static void InitStatic();
	static void KillStatic();

	CInfoConsole(): CEventClient("InfoConsole", 999, false) { Init(); }
	~CInfoConsole() { Kill(); }

	void Init();
	void Kill();

	void Update() override;
	void Draw() override;
	void PushNewLinesToEventHandler();

	void RecordLogMessage(int level, const std::string& section, const std::string& message) override;

	bool WantsEvent(const std::string& eventName) override {
		return (eventName == "LastMessagePosition");
	}
	void LastMessagePosition(const float3& pos) override;
	const float3& GetMsgPos(const float3& defaultPos = ZeroVector);
	unsigned int GetMsgPosCount() const { return lastMsgPositions.size(); }

public:
	struct RawLine {
		RawLine(const std::string& text, const std::string& section, int level,
				int id)
			: text(text)
			, section(section)
			, level(level)
			, id(id)
			{}
		std::string text;
		std::string section;
		int level;
		int id;
	};

	size_t GetRawLines(std::vector<RawLine>& copy);

private:
	static constexpr size_t maxMsgCount = 10;
	static constexpr size_t maxRawLines = 1024;

	struct InfoLine {
		std::string text;
		spring_time timeout;
	};

	std::array<float3, maxMsgCount> lastMsgPositions;

	std::vector<RawLine> tmpLines;
	std::vector<InfoLine> tmpInfoLines;
	std::deque<RawLine> rawLines;
	std::deque<InfoLine> infoLines;

	std::string prvSection;
	std::string prvMessage;

	spring::recursive_mutex infoConsoleMutex;

	size_t maxLines = 1;
	size_t newLines = 0;

	size_t msgPosIndx = 0;
	size_t numPosMsgs = 0;

	int rawId = 0;
	int lifetime = 0;

	float xpos = 0.0f;
	float ypos = 0.0f;
	float width = 0.0f;
	float height = 0.0f;

	float fontScale = 1.0f;
	float fontSize = 0.0f;

public:
	bool enabled = true;
	bool inited = false;
};

extern CInfoConsole* infoConsole;

#endif /* INFO_CONSOLE_H */
