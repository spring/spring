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

	void AddLine(const char* text, ...);
	void AddLine(std::string text);
	void SetLastMsgPos(float3 pos);

	CInfoConsole& operator<< (int i);
	CInfoConsole& operator<< (float f);
	CInfoConsole& operator<< (const char* c);

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
	mutable boost::recursive_mutex infoConsoleMutex;
};

extern CInfoConsole* info;

#endif /* INFOCONSOLE_H */
