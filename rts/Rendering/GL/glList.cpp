#include "StdAfx.h"
// glList.cpp: implementation of the CglList class.
//
//////////////////////////////////////////////////////////////////////

#include "glList.h"
#include <SDL_keysym.h>
#include <SDL_mouse.h>
#include "mmgr.h"

#include "myGL.h"
#include "Game/UI/MouseHandler.h"
#include "Rendering/glFont.h"
#include "ConfigHandler.h"
#include "Util.h"

const float itemFontScale = 1.5f;
const float itemXMargin = 0.02f;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CglList::~CglList()
{
	if (place != cancelPlace) {
		char buf[50];
		sprintf(buf, "LastListChoice%i", id);
		configHandler->SetString(buf, (*filteredItems)[place]);
	}
}

CglList::CglList(const std::string& name, ListSelectCallback callback, int id) :
		place(0),
		name(name),
		callback(callback),
		cancelPlace(-1),
		tooltip("No tooltip defined"),
		activeMousePress(false),
		id(id),
		filteredItems(&items)
{
	char buf[50];
	sprintf(buf, "LastListChoice%i", id);
	lastChoosen=configHandler->GetString(buf, "");
	box.x1 = 0.3f;
	box.y1 = 0.1f;
	box.x2 = 0.7f;
	box.y2 = 0.9f;
}

void CglList::AddItem(const std::string& name,const std::string& description)
{
	if (lastChoosen == name)
		place = items.size();
	items.push_back(name);

	// calculate width of text and resize box if necessary
	float w = itemFontScale * font->GetSize() * font->GetTextWidth(name) * gu->pixelX + 2 * itemXMargin;
	if (w > (box.x2 - box.x1)) {
		box.x1 = 0.5f - 0.5f * w;
		box.x2 = 0.5f + 0.5f * w;
	}
}

bool CglList::MousePress(int x, int y, int button)
{
	if (button != SDL_BUTTON_LEFT || !IsAbove(x, y))
		return false;

	activeMousePress = true;
	MouseUpdate(x, y); // make sure place is up to date
	return true;
}

void CglList::MouseMove(int x, int y, int dx,int dy, int button)
{
	if (button != SDL_BUTTON_LEFT || !activeMousePress)
		return;

	MouseUpdate(x, y);
}

void CglList::MouseRelease(int x, int y, int button)
{
	if (button != SDL_BUTTON_LEFT || !activeMousePress)
		return;

	activeMousePress = false;
	if (!MouseUpdate(x, y) && cancelPlace >= 0) { // make sure place is up to date
		place = cancelPlace;
	}
	Select(); // watch out, the callback may delete this
}

bool CglList::MouseUpdate(int x, int y)
{
	float mx = MouseX(x);
	float my = MouseY(y);

	int nCurIndex = 0; // The item we're on
	int nDrawOffset = 0; // The offset to the first draw item
	ContainerBox b = box;

	// Get list started up here
	std::vector<std::string>::iterator ii = filteredItems->begin();
	// Skip to current selection - 3; ie: scroll
	while ((nCurIndex + 7) <= place && nCurIndex+13 <= filteredItems->size()) { ii++; nCurIndex++; }

	for (/*ii = items.begin()*/; ii != filteredItems->end() && nDrawOffset < 12; ii++)
	{
		b.y2 = box.y2 - 0.06f - (nDrawOffset * 0.06f);
		b.y1 = b.y2 - 0.05f;

		if (InBox(mx, my, b)) {
			place = nCurIndex;
			return true;
		}

		// Up our index's
		nCurIndex++; nDrawOffset++;
	}
	return false;
}

bool CglList::IsAbove(int x, int y)
{
// 	return InBox(MouseX(x), MouseY(y), box);
	// always take focus for now, otherwise weird behaviour results
	// in GroupAI choose menu
	return true;
}

void CglList::Draw()
{
	// dont call Mouse[XY] if we're being used in CPreGame (when gu or mouse is still NULL)
	float mx = (gu && mouse) ? MouseX(mouse->lastx) : 0;
	float my = (gu && mouse) ? MouseY(mouse->lasty) : 0;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glLoadIdentity();
	glColor4f(0.2f,0.2f,0.2f,guiAlpha);
	DrawBox(box);

	font->Begin();

	font->SetTextColor(1,1,0.4f,0.8f);
	font->glPrint(box.x1 + 0.01f, box.y2 - 0.05f, itemFontScale*0.7f, FONT_SCALE | FONT_NORM, name);

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
	const int iResX = gu->viewSizeX;
	const int iResY = gu->viewSizeY;

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
	ContainerBox b = box;

	b.x1 += 0.01f;
	b.x2 -= 0.01f;

	// Get list started up here
	std::vector<std::string>::iterator ii = filteredItems->begin();
	// Skip to current selection - 3; ie: scroll
	while ((nCurIndex + 7) <= place && nCurIndex+13 <= filteredItems->size()) { ii++; nCurIndex++; }

	font->SetTextColor(); //default
	font->SetOutlineColor(0.0f, 0.0f, 0.0f, 1.0f);
	for (/*ii = items.begin()*/; ii != filteredItems->end() && nDrawOffset < 12; ii++)
	{
		b.y2 = box.y2 - 0.06f - (nDrawOffset * 0.06f);
		b.y1 = b.y2 - 0.05f;

		// TODO lots of this should really be merged with GuiHandler.cpp
		// (as in, factor out the common code...)

		glColor4f(1,1,1,0.1f);
		DrawBox(b, GL_LINE_LOOP);

		if (nCurIndex == place) {
			glBlendFunc(GL_ONE, GL_ONE); // additive blending
			glColor4f(0.2f,0,0,1);
			DrawBox(b);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(1,0,0,0.5f);
			glLineWidth(1.49f);
			DrawBox(b, GL_LINE_LOOP);
			glLineWidth(1.0f);
		} else if (InBox(mx, my, b)) {
			glBlendFunc(GL_ONE, GL_ONE); // additive blending
			glColor4f(0,0,0.2f,1);
			DrawBox(b);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(1,1,1,0.5f);
			glLineWidth(1.49f);
			DrawBox(b, GL_LINE_LOOP);
			glLineWidth(1.0f);
		}

		const float xStart = box.x1 + itemXMargin;
		const float yStart = (b.y1 + b.y2) * 0.5f;

		font->glPrint(xStart, yStart, itemFontScale, FONT_VCENTER | FONT_SHADOW | FONT_SCALE | FONT_NORM, *ii);

		// Up our index's
		nCurIndex++; nDrawOffset++;
	}
	/**************
	* End insert *
	**************/

	font->End();
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
	if(place>=(int)filteredItems->size())
		place=filteredItems->size()-1;
	if(place<0)
		place=0;
}

void CglList::UpPage()
{
	place -= 12;
	if(place<0)
		place=0;
}

void CglList::DownPage()
{
	place += 12;
	if(place>=(int)filteredItems->size())
		place=filteredItems->size()-1;
	if(place<0)
		place=0;
}

void CglList::Select()
{
	if (!filteredItems->empty())
		callback((*filteredItems)[place]);
}

std::string CglList::GetCurrentItem() const
{
	if (!filteredItems->empty()) {
		return ((*filteredItems)[place]);
	}
	return "";
}

bool CglList::KeyPressed(unsigned short k, bool isRepeat)
{
	if (k == SDLK_ESCAPE) {
		if (cancelPlace >= 0) {
			place = cancelPlace;
			Select();
			return true;
		}
	} else if (k == SDLK_UP) {
		UpOne();
		return true;
	} else if (k == SDLK_DOWN) {
		DownOne();
		return true;
	} else if (k == SDLK_PAGEUP) {
		UpPage();
		return true;
	} else if (k == SDLK_PAGEDOWN) {
		DownPage();
		return true;
	} else if (k == SDLK_RETURN) {
		Select();
		return true;
	} else if (k == SDLK_BACKSPACE) {
		query = query.substr(0, query.length() - 1);
		return Filter(true);
	} else if ((k & ~0xFF) != 0) {
		// This prevents isalnum from asserting on msvc debug crt
		// We don't actually need to process the key tho;)
	} else if (isalnum(k)) {
		query += tolower(k);
		return Filter(false);
	}
	return false;
}

bool CglList::Filter(bool reset)
{
	std::string current = (*filteredItems)[place];
	std::vector<std::string>* destination = filteredItems == &temp1 ? &temp2 : &temp1;
	destination->clear();
	if (reset) filteredItems = &items; // reset filter
	for (std::vector<std::string>::const_iterator it = filteredItems->begin(); it != filteredItems->end(); ++it) {
		std::string lcitem(*it, 0, query.length());
		StringToLowerInPlace(lcitem);
		if (lcitem == query) {
			if (*it == current)
				place = destination->size();
			destination->push_back(*it);
		}
	}
	if (destination->empty()) {
		query = query.substr(0, query.length() - 1);
		return false;
	} else {
		filteredItems = destination;
		if(place>=(int)filteredItems->size())
			place=filteredItems->size()-1;
		if(place<0)
			place=0;
		return true;
	}
}
