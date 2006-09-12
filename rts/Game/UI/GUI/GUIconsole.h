// GUItextField.h: interface for the GUItextField class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(GUI_CONSOLE_H)
#define GUI_CONSOLE_H

#include "GUIpane.h"
#include <string>
#include <deque>
#include "LogOutput.h"

class GUIconsole : public GUIframe
{
public:
	GUIconsole(const int x1, const int y1, const int w1, const int h1);
	virtual ~GUIconsole();

	void AddText(const std::string& text);

protected:
	void PrivateDraw();
	void SizeChanged();

	int numLines;

	struct ConsoleLine
	{
		ConsoleLine(std::string text1, int time1): time(time1), text(text1)
		{}
		int time;
		std::string text;
	};	
	std::deque<ConsoleLine> lines;
	
	std::string curLine;
};

#endif // !defined(GUI_CONSOLE_H)
