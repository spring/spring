#ifndef INFOCONSOLE_H
#define INFOCONSOLE_H
// InfoConsole.h: interface for the CInfoConsole class.
//
//////////////////////////////////////////////////////////////////////

#include <deque>
#include <vector>
#include <string>
#include <boost/thread/recursive_mutex.hpp>
#include <SDL_types.h>
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
	bool disabled;

	float3 lastMsgPos;
	
public:
	static const int maxRawLines;
	struct RawLine {
		std::string text;
		int priority;
		int id;
		Uint32 time;
	};
	int  GetRawLines(std::deque<RawLine>& copy);
	void GetNewRawLines(std::vector<RawLine>& copy);

private:
	std::deque<RawLine> rawData;
	int newLines;
	int rawId;

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
