#include "StdAfx.h"
// InfoConsole.cpp: implementation of the CInfoConsole class.
//
//////////////////////////////////////////////////////////////////////

#include "InfoConsole.h"
#include "Rendering/GL/myGL.h"
#include <fstream>
#include "Rendering/glFont.h"
#include "NewGuiDefine.h"
#ifdef NEW_GUI
	#include "GUI/GUIcontroller.h"
#endif

#ifdef WIN32
#include "Platform/Win/win32.h"
#endif
 
#include "SyncTracer.h"
#include "Platform/ConfigHandler.h"

#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CInfoConsole* info=0;

CInfoConsole::CInfoConsole()
: lastMsgPos(0,0,0)
{
	lastTime=0;
	lifetime=400;

	lifetime=configHandler.GetInt("InfoMessageTime",400);
	xpos=0.26f;
	verboseLevel=configHandler.GetInt("VerboseLevel",0);
	ypos=0.946f;
	width=0.41f;
	height=0.2f;
	numLines = 7;

	logOutput.AddSubscriber(this);
}

CInfoConsole::~CInfoConsole()
{
	logOutput.RemoveSubscriber(this);
}

void CInfoConsole::Draw()
{
	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);
	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glColor4f(0.5f,0.5f,0.5f,0.4f);
	if(!data.empty()){
		glBegin(GL_TRIANGLE_STRIP);
			glVertex3f(xpos,ypos,0);
			glVertex3f(xpos+width,ypos,0);
			glVertex3f(xpos,ypos-height,0);
			glVertex3f(xpos+width,ypos-height,0);
		glEnd();
	}
	glTranslatef(xpos+0.01f,ypos-0.026f,0);
	glScalef(0.015f,0.02f,0.02f);
	glColor4f(1,1,1,1);

	glEnable(GL_TEXTURE_2D);

	std::deque<InfoLine>::iterator ili;
	for(ili=data.begin();ili!=data.end();ili++){
		font->glPrint("%s",ili->text.c_str());
		glTranslatef(0,-1.2f,0);
	}
	glPopMatrix();
}

void CInfoConsole::Update()
{
	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);
	if(lastTime>0)
		lastTime--;
	if(!data.empty()){
		data.begin()->time--;
		if(data[0].time<=0)
			data.pop_front();
	}
}


void CInfoConsole::NotifyLogMsg(int priority, const char *text)
{
	if (priority > verboseLevel)
		return;

	PUSH_CODE_MODE;
	ENTER_MIXED;
	boost::recursive_mutex::scoped_lock scoped_lock(infoConsoleMutex);

	float maxWidth = 25.0f;
	int pos=0, line_start=0;

	while (text[pos]) {
		// iterate through text until maxWidth width is reached
		char temp[120];
		float w = 0.0f;
		for (;text[pos] && pos-line_start < sizeof(temp) - 1 && w <= maxWidth;pos++) {
			w += font->CalcCharWidth (text[pos]);
			temp[pos-line_start] = text[pos];
		}

		// if needed, find a breaking position
		if (w > maxWidth) {
			int break_pos = pos-line_start;
			while (break_pos >= 0 && temp[break_pos] != ' ')
				break_pos --;

			if (break_pos <= 0) break_pos = pos-line_start;
			line_start += break_pos + (temp[break_pos] == ' ' ? 1 : 0);
			pos = line_start;
			temp[break_pos] = 0;
		} else {
			temp[pos-line_start] = 0;
			line_start = pos;
		}

		// add the line to the console
		InfoLine l;
		if((int)data.size()>(numLines-1)){
			data[1].time+=data[0].time;
			data.pop_front();
		}
		data.push_back(l);
		data.back().text=temp;
		data.back().time=lifetime-lastTime;
		lastTime=lifetime;
	}
	POP_CODE_MODE;
}


void CInfoConsole::SetLastMsgPos(const float3& pos)
{
	lastMsgPos=pos;
}
