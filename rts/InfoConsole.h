// InfoConsole.h: interface for the CInfoConsole class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INFOCONSOLE_H__E9B2D6A1_80B3_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_INFOCONSOLE_H__E9B2D6A1_80B3_11D4_AD55_0080ADA84DE3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <deque>
#include <string>

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
};

extern CInfoConsole* info;

#endif // !defined(AFX_INFOCONSOLE_H__E9B2D6A1_80B3_11D4_AD55_0080ADA84DE3__INCLUDED_)
