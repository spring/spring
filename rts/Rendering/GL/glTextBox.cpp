#include "StdAfx.h"
#include "mmgr.h"

#include "glTextBox.h"
#include "myGL.h"
#include "Rendering/glFont.h"
#include "Game/UI/MouseHandler.h"


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
			textLineSize+=(int)ceil(font->CalcCharWidth(*ti));
		}
	}
	if(textLineSize>longestLine)
		longestLine=textLineSize;
	if(!currentLine.empty())
		text.push_back(currentLine);

	float xsize=longestLine*(0.03f/64)+0.04f;
	float ysize=text.size()*0.03f+0.15f;

	box.x1=0.5f-xsize*0.5f;
	box.x2=0.5f+xsize*0.5f;
	box.y1=0.5f-ysize*0.5f;
	box.y2=0.5f+ysize*0.5f;

	okButton.x1=0.45f;
	okButton.x2=0.55f;
	okButton.y1=box.y1+0.02f;
	okButton.y2=box.y1+0.07f;
}

CglTextBox::~CglTextBox(void)
{
}

void CglTextBox::Draw(void)
{
	float mx=float(mouse->lastx)/gu->viewSizeX;
	float my=(gu->viewSizeY-float(mouse->lasty))/gu->viewSizeY;

	glColor4f(0,0,0,0.5f);
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);
		glVertex2f(box.x1, box.y1);
		glVertex2f(box.x1, box.y2);
		glVertex2f(box.x2, box.y2);
		glVertex2f(box.x2, box.y1);
	glEnd();

	if(InBox(mx,my,okButton)){
		glColor4f(0.8f,0.3f,0.3f,0.5f);
		glBegin(GL_QUADS);
		glVertex2f(okButton.x1, okButton.y1);
		glVertex2f(okButton.x1, okButton.y2);
		glVertex2f(okButton.x2, okButton.y2);
		glVertex2f(okButton.x2, okButton.y1);
		glEnd();
	}	
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,0.8f);
	font->glPrintAt(okButton.x1+0.04f,okButton.y1+0.01f,1,"Ok");

	font->glPrintAt(box.x1+0.02f,box.y2-0.05f,1.0f,heading.c_str());

	int line=0;
	for(std::vector<std::string>::iterator li=text.begin();li!=text.end();++li){
		font->glPrintAt(box.x1+0.02f,box.y2-0.08f-(line++)*0.03f,0.7f,(*li).c_str());
	}
}

bool CglTextBox::MousePress(int x, int y, int button)
{
	float mx=float(x)/gu->viewSizeX;
	float my=(gu->viewSizeY-float(y))/gu->viewSizeY;

	if(InBox(mx,my,box))
		return true;
	return false;
}

void CglTextBox::MouseRelease(int x, int y,int button)
{
	float mx=float(x)/gu->viewSizeX;
	float my=(gu->viewSizeY-float(y))/gu->viewSizeY;

	if(InBox(mx,my,okButton)){
		delete this;
		return;
	}
}

bool CglTextBox::IsAbove(int x, int y)
{
	float mx=float(x)/gu->viewSizeX;
	float my=(gu->viewSizeY-float(y))/gu->viewSizeY;

	if(InBox(mx,my,box))
		return true;
	return false;
};
