/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef INFO_CONSOLE_H
#define INFO_CONSOLE_H

#include "InputReceiver.h"
#include "System/float3.h"
#include "System/EventClient.h"
#include "System/Log/LogSinkHandler.h"
#include "System/Misc/SpringTime.h"
#include "System/Threading/SpringThreading.h"

#include <deque>
#include <string>
#include <list>

class CInfoConsole: public CInputReceiver, public CEventClient, public ILogSink
{
public:
	CInfoConsole();
	virtual ~CInfoConsole();

	void Update() override;
	void Draw() override;
	void PushNewLinesToEventHandler();

	void RecordLogMessage(int level, const std::string& section, const std::string& text) override;

	bool WantsEvent(const std::string& eventName) override {
		return (eventName == "LastMessagePosition");
	}
	void LastMessagePosition(const float3& pos) override;
	const float3& GetMsgPos(const float3& defaultPos = ZeroVector);
	unsigned int GetMsgPosCount() const { return lastMsgPositions.size(); }

	bool enabled;

public:
	static constexpr size_t maxLastMsgPos = 10;
	static constexpr size_t maxRawLines = 1024;

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

	int GetRawLines(std::deque<RawLine>& copy);

private:
	struct InfoLine {
		std::string text;
		spring_time timeout;
	};

	std::list<float3> lastMsgPositions;
	std::list<float3>::iterator lastMsgIter;

	std::deque<RawLine> rawData;
	std::deque<InfoLine> data;

	spring::recursive_mutex infoConsoleMutex;

	size_t maxLines = 1;
	size_t newLines = 0;

	int rawId = 0;
	int lifetime = 0;

	float xpos = 0.0f;
	float ypos = 0.0f;
	float width = 0.0f;
	float height = 0.0f;
	float fontScale = 1.0f;
	float fontSize = 0.0f;
};

extern CInfoConsole* infoConsole;

#endif /* INFO_CONSOLE_H */
