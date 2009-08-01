#include "glList.h"

#include <SDL_keysym.h>
#include <SDL_mouse.h>

#include "Rendering/GL/myGL.h"
#include "Game/UI/MouseHandler.h"
#include "Gui.h"
#include "Rendering/glFont.h"
#include "Util.h"

namespace agui
{

const float itemFontScale = 1.0f;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CglList::CglList(GuiElement* parent) :
		GuiElement(parent),
		place(0),
		cancelPlace(-1),
		tooltip("No tooltip defined"),
		activeMousePress(false),
		filteredItems(&items)
{
	borderSpacing = 0.005f;
	itemSpacing = 0.003f;
	itemHeight = 0.04f;
}

CglList::~CglList()
{
}

void CglList::AddItem(const std::string& name, const std::string& description)
{
	if (lastChoosen == name)
		place = items.size();
	items.push_back(name);

	// calculate width of text and resize box if necessary
	const float w = itemFontScale * font->GetSize() * font->GetTextWidth(name) * screensize[0] + 2 * itemSpacing;
	if (w > (size[0]))
	{
		//box.x1 = 0.5f - 0.5f * w;
		//box.x2 = 0.5f + 0.5f * w;
	}
}

bool CglList::MousePress(int x, int y, int button)
{
	switch (button)
	{
		case SDL_BUTTON_LEFT:
		{
			activeMousePress = true;
			MouseUpdate(x, y); // make sure place is up to date
			break;
		}
		case SDL_BUTTON_WHEELDOWN:
			DownOne();
			break;
			
		case SDL_BUTTON_WHEELUP:
			UpOne();
			break;
	}
	return false;
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
}

bool CglList::MouseUpdate(int x, int y)
{
	mx = PixelToGlX(x);
	my = PixelToGlY(y);

	int nCurIndex = 0; // The item we're on
	int nDrawOffset = 0; // The offset to the first draw item

	GuiElement b;
	b.SetPos(pos[0] + borderSpacing, pos[1] + size[1] - borderSpacing - itemHeight);
	b.SetSize(size[0] - 2.0f * borderSpacing, itemHeight);
	
	// Get list started up here
	std::vector<std::string>::iterator ii = filteredItems->begin();
	const int numDisplay = (size[1]-2.0f*borderSpacing)/(itemHeight+itemSpacing);
	assert(numDisplay >= 0);
	while ((nCurIndex + numDisplay/2) <= place && nCurIndex+numDisplay <= filteredItems->size()) { ii++; nCurIndex++; }
	
	for (/*ii = items.begin()*/; ii != filteredItems->end() && nDrawOffset < numDisplay; ii++)
	{
		if (b.MouseOver(mx, my))
		{
			place = nCurIndex;
			return true;
		}

		// Up our index's
		nCurIndex++; nDrawOffset++;
		b.Move(0.0,  - (itemHeight + itemSpacing));
	}
	return false;
}

void CglList::DrawSelf()
{
	glLoadIdentity();
	glColor4f(0.2f,0.2f,0.2f,1.0f);
	DrawBox(GL_QUADS);

	font->Begin();

	font->SetTextColor(1,1,0.4f,0.8f);

	int nCurIndex = 0; // The item we're on
	int nDrawOffset = 0; // The offset to the first draw item

	GuiElement b;
	b.SetPos(pos[0] + borderSpacing, pos[1] + size[1] - borderSpacing - itemHeight);
	b.SetSize(size[0] - 2.0f * borderSpacing, itemHeight);

	// Get list started up here
	std::vector<std::string>::iterator ii = filteredItems->begin();
	// Skip to current selection - 3; ie: scroll
	const int numDisplay = (size[1]-2.0f*borderSpacing)/(itemHeight+itemSpacing);
	assert(numDisplay >= 0);
	while ((nCurIndex + numDisplay/2) <= place && nCurIndex+numDisplay <= filteredItems->size()) { ii++; nCurIndex++; }

	font->SetTextColor(); //default
	font->SetOutlineColor(0.0f, 0.0f, 0.0f, 1.0f);
	for (/*ii = items.begin()*/; ii != filteredItems->end() && nDrawOffset < numDisplay; ii++)
	{
		glColor4f(1,1,1,0.1f);
		b.DrawBox(GL_LINE_LOOP);

		if (nCurIndex == place) {
			glBlendFunc(GL_ONE, GL_ONE); // additive blending
			glColor4f(0.2f,0,0,1);
			b.DrawBox(GL_QUADS);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(1,0,0,0.5f);
			glLineWidth(1.49f);
			b.DrawBox(GL_LINE_LOOP);
			glLineWidth(1.0f);
		} else if (b.MouseOver(mx, my)) {
			glBlendFunc(GL_ONE, GL_ONE); // additive blending
			glColor4f(0,0,0.2f,1);
			b.DrawBox(GL_QUADS);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(1,1,1,0.5f);
			glLineWidth(1.49f);
			b.DrawBox(GL_LINE_LOOP);
			glLineWidth(1.0f);
		}

		font->glPrint(pos[0]+borderSpacing+0.001f, b.GetMidY(), itemFontScale, FONT_VCENTER | FONT_SHADOW | FONT_SCALE | FONT_NORM, *ii);

		// Up our index's
		nCurIndex++; nDrawOffset++;
		b.Move(0.0,  - (itemHeight + itemSpacing));
	}
	/**************
	* End insert *
	**************/

	font->End();
}

bool CglList::HandleEventSelf(const SDL_Event& ev)
{
	if (MouseOver(ev.button.x, ev.button.y))
	{
		switch (ev.type) {
			case SDL_MOUSEBUTTONDOWN: {
				MousePress(ev.button.x, ev.button.y, ev.button.button);
				break;
			}
			case SDL_MOUSEBUTTONUP: {
				MouseRelease(ev.button.x, ev.button.y, ev.button.button);
				break;
			}
			case SDL_MOUSEMOTION: {
				MouseMove(ev.motion.x, ev.motion.y, ev.motion.xrel, ev.motion.yrel, ev.motion.state);
				break;
			}
		}
		return true;
	}
	else
	{
		return false;
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

}
