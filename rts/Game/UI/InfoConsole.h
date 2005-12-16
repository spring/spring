#ifndef INFOCONSOLE_H
#define INFOCONSOLE_H
// InfoConsole.h: interface for the CInfoConsole class.
//
//////////////////////////////////////////////////////////////////////

#include <deque>
#include <string>
#include <boost/thread/recursive_mutex.hpp>

class CInfoConsole  
{
public:
	CInfoConsole();
	virtual ~CInfoConsole();

	void Update();
	void Draw();

	void AddLine(int priority,const char *fmt, ...);
	void AddLine(int priority, const std::string& text);

	// Everything without a priority will have priority 0 (the highest)
	// Messages will be shown if messagePriority <= verboseLevel
	CInfoConsole& operator<< (int i);
	CInfoConsole& operator<< (float f);
	CInfoConsole& operator<< (const char* c);
	void AddLine(const char* text, ...);
	void AddLine(const std::string& text);

	void SetLastMsgPos(float3 pos);

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

extern CInfoConsole* info;

#endif /* INFOCONSOLE_H */
