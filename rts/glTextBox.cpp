#include "StdAfx.h"
#include "glTextBox.h"
#include "myGL.h"
#include "glFont.h"
#include "MouseHandler.h"
//#include "mmgr.h"


CglTextBox::CglTextBox(std::string heading,std::string intext,int autoBreakAt)
:	heading(heading)
{
	int longestLine=0;

	if(autoBreakAt==0)
		autoBreakAt=1000;

	autoBreakAt*=60;

	std::string currentLine;
	int textLineSize=0;
	for(std::string::iterator ti=intext.begin();ti!=intext.end();++ti){
		if(textLineSize>=autoBreakAt || *ti=='\n'){
			if(textLineSize>longestLine)
				longestLine=textLineSize;
			textLineSize=0;
			text.push_back(currentLine);
			currentLine.clear();

		} else {
			currentLine+=*ti;
			textLineSize+=font->charWidths[*((unsigned char*)(&*ti))];
		}
	}
	if(textLineSize>longestLine)
		longestLine=textLineSize;
	if(!currentLine.empty())
		text.push_back(currentLine);

	float xsize=longestLine*(0.03/64)+0.04;
	float ysize=text.size()*0.03+0.15;

	box.x1=0.5-xsize*0.5;
	box.x2=0.5+xsize*0.5;
	box.y1=0.5-ysize*0.5;
	box.y2=0.5+ysize*0.5;

	okButton.x1=0.45;
	okButton.x2=0.55;
	okButton.y1=box.y1+0.02;
	okButton.y2=box.y1+0.07;
}

CglTextBox::~CglTextBox(void)
{
}

void CglTextBox::Draw(void)
{
	float mx=float(mouse->lastx)/gu->screenx;
	float my=(gu->screeny-float(mouse->lasty))/gu->screeny;

	glColor4f(0,0,0,0.5);
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
		glVertex2f(box.x1, box.y1);
		glVertex2f(box.x1, box.y2);
		glVertex2f(box.x2, box.y2);
		glVertex2f(box.x2, box.y1);
	glEnd();

	if(InBox(mx,my,okButton)){
		glColor4f(0.8,0.3,0.3,0.5);
		glBegin(GL_QUADS);
		glVertex2f(okButton.x1, okButton.y1);
		glVertex2f(okButton.x1, okButton.y2);
		glVertex2f(okButton.x2, okButton.y2);
		glVertex2f(okButton.x2, okButton.y1);
		glEnd();
	}	
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,0.8);
	font->glPrintAt(okButton.x1+0.04,okButton.y1+0.01,1,"Ok");

	font->glPrintAt(box.x1+0.02,box.y2-0.05,1.0f,heading.c_str());

	int line=0;
	for(std::vector<std::string>::iterator li=text.begin();li!=text.end();++li){
		font->glPrintAt(box.x1+0.02,box.y2-0.08-(line++)*0.03,0.7f,(*li).c_str());
	}
}

bool CglTextBox::MousePress(int x, int y, int button)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;

	if(InBox(mx,my,box))
		return true;
	return false;
}

void CglTextBox::MouseRelease(int x, int y,int button)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;

	if(InBox(mx,my,okButton)){
		delete this;
		return;
	}
}

bool CglTextBox::IsAbove(int x, int y)
{
	float mx=float(x)/gu->screenx;
	float my=(gu->screeny-float(y))/gu->screeny;

	if(InBox(mx,my,box))
		return true;
	return false;
};
