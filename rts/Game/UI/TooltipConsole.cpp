#include "StdAfx.h"
#include "TooltipConsole.h"
#include "MouseHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/glFont.h"
#include "mmgr.h"


CTooltipConsole* tooltip = 0;


CTooltipConsole::CTooltipConsole(void)
{
}

CTooltipConsole::~CTooltipConsole(void)
{
}

void CTooltipConsole::Draw(void)
{
	std::string s=mouse->GetCurrentTooltip();

	glPushMatrix();
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.2f, 0.2f, 0.2f, CInputReceiver::guiAlpha);

	glBegin(GL_TRIANGLE_STRIP);
		glVertex3f(0.0f,  0.0f, 0.0f);
		glVertex3f(0.41f, 0.0f, 0.0f);
		glVertex3f(0.0f,  0.1f, 0.0f);
		glVertex3f(0.41f, 0.1f, 0.0f);
	glEnd();

	const float yScale = 0.015f;
	const float xScale = (yScale / gu->aspectRatio) * 1.2f;
	
	glTranslatef(0.01f, 0.08f, 0.0f);
	glScalef(xScale, yScale, 1.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 0.8f);

	glEnable(GL_TEXTURE_2D);

	unsigned int p=0;
	while(p<s.size()){
		std::string s2;
		int pos=0;
		for(int a=0;a<420;++a){
			s2+=s[p];
			if(s[p++]=='\n' || p>=s.size())
				break;
		}
		font->glPrintColor("%s",s2.c_str());
		glTranslatef(0, -1.2f, 0);
	}
	glPopMatrix();
}


bool CTooltipConsole::IsAbove(int x,int y)
{
	float mx=MouseX(x);
	float my=MouseY(y);

	return (mx>0.01f && mx<0.41f && my>0.01f && my<0.1f);
}
