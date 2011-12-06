/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef INFO_CONSOLE_H
#define INFO_CONSOLE_H

#include "InputReceiver.h"
#include "System/float3.h"
#include "System/EventClient.h"
#include "System/Log/LogSinkHandler.h"

#include <deque>
#include <vector>
#include <string>
#include <list>
#include <boost/thread/recursive_mutex.hpp>

class CInfoConsole: public CInputReceiver, public CEventClient, public ILogSink
{
public:
	CInfoConsole();
	virtual ~CInfoConsole();

	void Update();
	void Draw();
	void PushNewLinesToEventHandler();

	void RecordLogMessage(const std::string& section, int level,
			const std::string& text);


	bool WantsEvent(const std::string& eventName) {
		return (eventName == "LastMessagePosition");
	}
	void LastMessagePosition(const float3& pos);
	const float3& GetMsgPos();
	int GetMsgPosCount() const {
		return lastMsgPositions.size();
	}


	int lifetime;
	float xpos;
	float ypos;
	float width;
	float height;
	float fontScale;
	float fontSize;
	bool enabled;

public:
	static const size_t maxLastMsgPos;
	static const size_t maxRawLines;

	struct RawLine {
		RawLine(const std::string& text, const std::string& section, int level,
				int id)
			: text(text)
			, section(section)
			, level(level)
			, id(id)
			, time(0)
			{}
		std::string text;
		std::string section;
		int level;
		int id;
		boost::uint32_t time;
	};

	int  GetRawLines(std::deque<RawLine>& copy);

private:
	std::list<float3> lastMsgPositions;
	std::list<float3>::iterator lastMsgIter;

	std::deque<RawLine> rawData;
	size_t newLines;
	int rawId;

	struct InfoLine {
		std::string text;
		int time;
	};
	int lastTime;
	std::deque<InfoLine> data;

	mutable boost::recursive_mutex infoConsoleMutex;
};

#endif /* INFO_CONSOLE_H */
