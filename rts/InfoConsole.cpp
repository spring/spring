#include "StdAfx.h"
// InfoConsole.cpp: implementation of the CInfoConsole class.
//
//////////////////////////////////////////////////////////////////////

#include "InfoConsole.h"
#include "myGL.h"
#include <iostream>
#include <fstream>
#include "glFont.h"

#include "SyncTracer.h"
#include "RegHandler.h"
#include "InfoConsole.h"
#include <stdarg.h>

//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CInfoConsole* info=0;
static ofstream* filelog;
static HANDLE infoConsoleMutex;

CInfoConsole::CInfoConsole()
: lastMsgPos(0,0,0)
{
	lastTime=0;
	lifetime=400;

	lifetime=regHandler.GetInt("InfoMessageTime",400);
	xpos=0.26f;
	ypos=0.946f;
	width=0.41f;
	height=0.2f;
	numLines = 7;

	filelog=new ofstream("infolog.txt", ios::out);
#ifndef NO_MUTEXTHREADS
	infoConsoleMutex=CreateMutex(0,false,"SpringInfoConsoleMutex");
#endif
}

CInfoConsole::~CInfoConsole()
{
	delete filelog;
#ifndef NO_IO
	CloseHandle(infoConsoleMutex);
#endif
}

void CInfoConsole::AddLine(const char *fmt, ...)
{
	PUSH_CODE_MODE;
	ENTER_MIXED;
#ifndef NO_MUTEXTHREADS
	WaitForSingleObject(infoConsoleMutex,INFINITE);
#endif
	char text[500];
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing

	va_start(ap, fmt);									// Parses The String For Variables
	    vsprintf(text, fmt, ap);						// And Converts Symbols To Actual Numbers
	va_end(ap);											// Results Are Stored In Text

	char text2[50];
	if(strlen(text)>42){
		memcpy(text2,text,42);
		text2[42]=0;
	} else
		strcpy(text2,text);
	(*filelog) << text2  << "\n";
	filelog->flush();
	InfoLine l;
	if((int)data.size()>(numLines-1)){
		data[1].time+=data[0].time;
		data.pop_front();
	}
	data.push_back(l);
	data.back().text=text2;
	data.back().time=lifetime-lastTime;
	lastTime=lifetime;
/*#ifdef TRACE_SYNC
		tracefile << "Info line: ";
		tracefile << text << "\n";
#endif*/
	if(strlen(text)>42){
		AddLine("%s",&text[42]);
	}
#ifndef NO_MUTEXTHREADS
	ReleaseMutex(infoConsoleMutex);
#endif
	POP_CODE_MODE;
}

void CInfoConsole::AddLine(std::string text)
{
	AddLine("%s",text.c_str());
}

void CInfoConsole::Draw()
{
#ifndef NO_MUTEXTHREADS
	WaitForSingleObject(infoConsoleMutex,INFINITE);
#endif
	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glColor4f(0,0,0.5f,0.5f);
	if(!data.empty()){
		glBegin(GL_TRIANGLE_STRIP);
			glVertex3f(xpos,ypos,0);
			glVertex3f(xpos+width,ypos,0);
			glVertex3f(xpos,ypos-height,0);
			glVertex3f(xpos+width,ypos-height,0);
		glEnd();
	}
	glTranslatef(xpos+0.01,ypos-0.026f,0);
	glScalef(0.015f,0.02f,0.02f);
	glColor4f(1,1,1,1);

	glEnable(GL_TEXTURE_2D);

	std::deque<InfoLine>::iterator ili;
	for(ili=data.begin();ili!=data.end();ili++){
		font->glPrint("%s",ili->text.c_str());
		glTranslatef(0,-1.2f,0);
	}
	glPopMatrix();
#ifndef NO_MUTEXTHREADS
	ReleaseMutex(infoConsoleMutex);
#endif
}

void CInfoConsole::Update()
{
#ifndef NO_MUTEXTHREADS
	WaitForSingleObject(infoConsoleMutex,INFINITE);
#endif
	if(lastTime>0)
		lastTime--;
	if(!data.empty()){
		data.begin()->time--;
		if(data[0].time<=0)
			data.pop_front();
	}
#ifndef NO_MUTEXTHREADS
	ReleaseMutex(infoConsoleMutex);
#endif
}

CInfoConsole& CInfoConsole::operator<< (int i)
{
#ifndef NO_MUTEXTHREADS
	WaitForSingleObject(infoConsoleMutex,INFINITE);
#endif
	char t[50];
	sprintf(t,"%d ",i);
	tempstring+=t;
#ifndef NO_MUTEXTHREADS
	ReleaseMutex(infoConsoleMutex);
#endif
	return *this;
}

CInfoConsole& CInfoConsole::operator<< (float f)
{
#ifndef NO_MUTEXTHREADS
	WaitForSingleObject(infoConsoleMutex,INFINITE);
#endif
	char t[50];
	sprintf(t,"%f ",f);
	tempstring+=t;
#ifndef NO_MUTEXTHREADS
	ReleaseMutex(infoConsoleMutex);
#endif
	return *this;
}

CInfoConsole& CInfoConsole::operator<< (const char* c)
{
#ifndef NO_MUTEXTHREADS
	WaitForSingleObject(infoConsoleMutex,INFINITE);
#endif
	for(unsigned int a=0;a<strlen(c);a++){
		if(c[a]!='\n'){
			tempstring+=c[a];
		} else {
			AddLine(tempstring);
			tempstring="";
			break;
		}
	}
#ifndef NO_MUTEXTHREADS
	ReleaseMutex(infoConsoleMutex);
#endif
	return *this;
}

void CInfoConsole::SetLastMsgPos(float3 pos)
{
	lastMsgPos=pos;
}
