#include "StdAfx.h"
// glList.cpp: implementation of the CglList class.
//
//////////////////////////////////////////////////////////////////////

#include "glList.h"
#include "myGL.h"

#include "glFont.h"
#include "ConfigHandler.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CglList::~CglList()
{
	configHandler.SetString("LastListChoice",items[place]);
}

CglList::CglList(const char *name,ListSelectCallback callback)
{
	this->name=name;
	this->callback=callback;
	place=0;
	lastChoosen="";
	lastChoosen=configHandler.GetString("LastListChoice","");
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

	/****************************************
	* Insert Robert Diamond's section here *
	* <deadram@gmail.com> *
	****************************************/

	/****************************************
	* TODO: *
	* *
	* This part of the code will need some *
	* math, and it's been a while since I *
	* did any 3D coding... the idea is to *
	* keep the selected item in the middle *
	* 60% of the screen, but don't scroll *
	* till we reach one end, or the other *
	* of that boundary. We'd also need the *
	* "oldPlace" variable to do this *
	* properly. *
	* *
	****************************************
	// Get screen res, so that the selected item is always within the middle 60% of screen
	int iResX = configHandler.GetInt("XResolution", 1024);
	int iResY = configHandler.GetInt("YResolution", 768);

	// Keep tabs on the last place. change this ONLY AFTER a scroll
	static int siOldPlace = place;

	if (we're scrolling up) siOldPlace = place;
	if (we're scrolling down) siOldPlace = place;
	if (we're not scrolling) siOldPlace = siOldPlace;

	**************************************
	* Hey remove me when TODO is done XD *
	**************************************/

	int nCurIndex = 0; // The item we're on
	int nDrawOffset = 0; // The offset to the first draw item

	// Get list started up here
	ii = items.begin();
	// Skip to current selection - 3; ie: scroll
	while ((nCurIndex + 7) <= place && nCurIndex+14 <= items.size()) { ii++; nCurIndex++; }

	for (/*ii = items.begin()*/; ii != items.end(); ii++)
	{
		if (nCurIndex == place) {
			glColor4f(1,1,1.0f,1.0f);
		} else {
			glColor4f(1,1,0.0f,0.3f);
		}

		glLoadIdentity();
		glTranslatef(0.31f, 0.79f - (nDrawOffset * 0.06f), 0.0f);
		glScalef(0.035f,0.05f,0.1f);
		font->glPrint(ii->c_str());

		// Up our index's
		nCurIndex++; nDrawOffset++;
	}	
	/**************
	* End insert *
	**************/
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
