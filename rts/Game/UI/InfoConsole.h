/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef INFOCONSOLE_H
#define INFOCONSOLE_H

#include <deque>
#include <vector>
#include <string>
#include <list>
#include <boost/thread/recursive_mutex.hpp>
#include "float3.h"
#include "InputReceiver.h"
#include "LogOutput.h"

class CInfoConsole: public CInputReceiver, public ILogSubscriber
{
public:
	CInfoConsole();
	virtual ~CInfoConsole();

	void Update();
	void Draw();

	// ILogSubscriber interface implementation
	void NotifyLogMsg(const CLogSubsystem& subsystem, const std::string& txt);


	void SetLastMsgPos(const float3& pos);
	const float3& GetMsgPos();
	int GetMsgPosCount() const{
		return lastMsgPositions.size();
	}


	int lifetime;
	float xpos;
	float ypos;
	float width;
	float height;
	float fontScale;
	float fontSize;
	bool disabled;

public:
	static const size_t maxLastMsgPos;

	static const size_t maxRawLines;
	struct RawLine {
		RawLine(const std::string& text, const CLogSubsystem* subsystem, int id)
		: text(text), subsystem(subsystem), id(id), time(0) {}
		std::string text;
		const CLogSubsystem* subsystem;
		int id;
		boost::uint32_t time;
	};
	int  GetRawLines(std::deque<RawLine>& copy);
	void GetNewRawLines(std::vector<RawLine>& copy);

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

#endif /* INFOCONSOLE_H */
