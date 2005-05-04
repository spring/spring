#include "stdafx.h"
// InfoConsole.cpp: implementation of the CInfoConsole class.
//
//////////////////////////////////////////////////////////////////////

#include "InfoConsole.h"
#include "mygl.h"
#include <iostream>
#include <fstream>
#include "glFont.h"

#include "SyncTracer.h"
#include "reghandler.h"
#include ".\infoconsole.h"
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
	infoConsoleMutex=CreateMutex(0,false,"SpringInfoConsoleMutex");

}

CInfoConsole::~CInfoConsole()
{
	delete filelog;
	CloseHandle(infoConsoleMutex);
}

void CInfoConsole::AddLine(const char *fmt, ...)
{
	PUSH_CODE_MODE;
	ENTER_MIXED;
	WaitForSingleObject(infoConsoleMutex,INFINITE);
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
	ReleaseMutex(infoConsoleMutex);
	POP_CODE_MODE;
}

void CInfoConsole::AddLine(std::string text)
{
	AddLine("%s",text.c_str());
}

void CInfoConsole::Draw()
{
	WaitForSingleObject(infoConsoleMutex,INFINITE);
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
	ReleaseMutex(infoConsoleMutex);
}

void CInfoConsole::Update()
{
	WaitForSingleObject(infoConsoleMutex,INFINITE);
	if(lastTime>0)
		lastTime--;
	if(!data.empty()){
		data.begin()->time--;
		if(data[0].time<=0)
			data.pop_front();
	}
	ReleaseMutex(infoConsoleMutex);
}

CInfoConsole& CInfoConsole::operator<< (int i)
{
	WaitForSingleObject(infoConsoleMutex,INFINITE);
	char t[50];
	sprintf(t,"%d ",i);
	tempstring+=t;
	ReleaseMutex(infoConsoleMutex);
	return *this;
}

CInfoConsole& CInfoConsole::operator<< (float f)
{
	WaitForSingleObject(infoConsoleMutex,INFINITE);
	char t[50];
	sprintf(t,"%f ",f);
	tempstring+=t;
	ReleaseMutex(infoConsoleMutex);
	return *this;
}

CInfoConsole& CInfoConsole::operator<< (const char* c)
{
	WaitForSingleObject(infoConsoleMutex,INFINITE);
	for(unsigned int a=0;a<strlen(c);a++){
		if(c[a]!='\n'){
			tempstring+=c[a];
		} else {
			AddLine(tempstring);
			tempstring="";
			break;
		}
	}
	ReleaseMutex(infoConsoleMutex);
	return *this;
}

void CInfoConsole::SetLastMsgPos(float3 pos)
{
	lastMsgPos=pos;
}
