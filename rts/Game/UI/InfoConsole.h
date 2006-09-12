#ifndef INFOCONSOLE_H
#define INFOCONSOLE_H
// InfoConsole.h: interface for the CInfoConsole class.
//
//////////////////////////////////////////////////////////////////////

#include <deque>
#include <string>
#include <boost/thread/recursive_mutex.hpp>
#include "float3.h"
#include "LogOutput.h"

class CInfoConsole : public ILogSubscriber
{
public:
	CInfoConsole();
	virtual ~CInfoConsole();

	void Update();
	void Draw();

	// ILogSubscriber interface implementation
	void NotifyLogMsg(int priority, const char* txt);
	void SetLastMsgPos(const float3& pos);

	int lifetime;
	float xpos;
	float ypos;
	float width;
	float height;
	int numLines;

	float3 lastMsgPos;

private:
	struct InfoLine {
		std::string text;
		int time;
	};
	int lastTime;
	std::deque<InfoLine> data;
	std::string tempstring;
	int verboseLevel;

	void AddLineHelper (int priority, const char *text);
	mutable boost::recursive_mutex infoConsoleMutex;
};

#endif /* INFOCONSOLE_H */
