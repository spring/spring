// GUItextField.cpp: implementation of the GUItextField class.
//
//////////////////////////////////////////////////////////////////////

#include "GUIconsole.h"
#include <string>
#include "GUIfont.h"
#include "System/Platform/ConfigHandler.h"

#include <time.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static long lifetime;

static long CurrentTime()
{
	time_t ltime;
	time(&ltime);
	return ltime;
}

GUIconsole::GUIconsole(const int x1, const int y1, const int w1, const int h1):GUIframe(x1,y1,0,0)
{
	numLines=(int)(h/guifont->GetHeight());

	lifetime=(long int)(configHandler.GetInt("InfoMessageTime",400)/100.0);
}

GUIconsole::~GUIconsole()
{

}

void GUIconsole::PrivateDraw()
{
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	GUIpane *pane=dynamic_cast<GUIpane*>(parent);
	if(pane)
	{
		if(h!=pane->h)
			SizeChanged();

		w=pane->w;
		h=pane->h;
	}

	glColor3f(1, 1, 1);

	long time=CurrentTime();

	if(!lines.empty())
	{
		long frontTime=lines.front().time;
		if(frontTime<time)
			lines.pop_front();
	}
	
	
	std::deque<ConsoleLine>::iterator i=lines.begin();
	std::deque<ConsoleLine>::iterator e=lines.end();
	int line=0;
	for(;i!=e; i++)
	{
		guifont->PrintColor(0, line*guifont->GetHeight(), i->text);
		line++;	
	}
	glPopAttrib();
}

void GUIconsole::AddText(const std::string& text)
{
	//CInfoConsole::AddText(text);
	curLine+=text;

	size_t lf;
	long time=CurrentTime()+lifetime;
	while((lf=curLine.find('\n'))!=string::npos)   
	{
		lines.push_back(ConsoleLine(curLine.substr(0, lf), time));
		curLine=curLine.substr(lf+1, string::npos);
	}

	lines.push_back(ConsoleLine(curLine, time));
	curLine="";

	while(lines.size()>(numLines-1))
	{
		lines.pop_front();
	}
}


void GUIconsole::SizeChanged()
{
	numLines=(int)(h/guifont->GetHeight());
	while(lines.size()>numLines)
		lines.pop_front();
}
