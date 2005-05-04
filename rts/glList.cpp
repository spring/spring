#include "stdafx.h"
// glList.cpp: implementation of the CglList class.
//
//////////////////////////////////////////////////////////////////////

#include "glList.h"
#include "mygl.h"
#include <gl\glaux.h>		// Header File For The Glaux Library

#include "glfont.h"
#include "reghandler.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CglList::~CglList()
{
	regHandler.SetString("LastListChoice",items[place]);
}

CglList::CglList(const char *name,ListSelectCallback callback)
{
	this->name=name;
	this->callback=callback;
	place=0;
	lastChoosen="";
	lastChoosen=regHandler.GetString("LastListChoice","");
}

void CglList::AddItem(const char *name,const char *description)
{
	items.push_back(name);
	if(lastChoosen.compare(name)==0)
		place=items.size()-1;
}

void CglList::Draw()
{
	glDisable(GL_TEXTURE_2D);
	glLoadIdentity();
	glColor4f(0,0,0,0.5f);
	glBegin(GL_QUADS);
		glVertex3f(0.3f,0.1f,0);
		glVertex3f(0.7f,0.1f,0);
		glVertex3f(0.7f,0.9f,0);
		glVertex3f(0.3f,0.9f,0);
	glEnd();

	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,0.5f,0.8f);
	glLoadIdentity();
	glTranslatef(0.31f,0.85f,0.0f);
	glScalef(0.035f,0.05f,0.1f);
	font->glPrint(name.c_str());

	std::vector<std::string>::iterator ii;
	int a=0;
	for(ii=items.begin();ii!=items.end();ii++){
		if(a==place)
			glColor4f(1,1,1.0f,1.0f);
		else
			glColor4f(1,1,0.0f,0.3f);

		glLoadIdentity();
		glTranslatef(0.31f,0.79f-a*0.06f,0.0f);
		glScalef(0.035f,0.05f,0.1f);
		font->glPrint(ii->c_str());
		a++;
	}	
}

void CglList::UpOne()
{
	place--;
	if(place<0)
		place=0;
}

void CglList::DownOne()
{
	place++;
	if(place>=(int)items.size())
		place=items.size()-1;
	if(place<0)
		place=0;
}

void CglList::Select()
{
	callback(items[place]);
}
