/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "List.h"

#include <SDL_keysym.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>

#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Game/UI/MouseHandler.h"
#include "Gui.h"
#include "Rendering/glFont.h"
#include "Util.h"
#include "GlobalUnsynced.h"

namespace agui
{

const float itemFontScale = 1.0f;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

List::List(GuiElement* parent) :
		GuiElement(parent),
		cancelPlace(-1),
		tooltip("No tooltip defined"),
		clickedTime(SDL_GetTicks()),
		place(0),
		activeMousePress(false),
		filteredItems(&items)
{
	borderSpacing = 0.005f;
	itemSpacing = 0.000f;
	itemHeight = 0.04f;
	hasFocus = true;
	topIndex = 0;
	mx = my = 0;
}

List::~List()
{
}

void List::SetFocus(bool focus)
{
	hasFocus = focus;
}

void List::RemoveAllItems() {
	items.clear();
}

void List::RefreshQuery() {
	if(query != "") {
		int t = topIndex;
		std::string q = query;
		Filter(true);
		query = q;
		Filter(false);
		topIndex = t;
	}
}

void List::AddItem(const std::string& name, const std::string& description)
{
	items.push_back(name);

	// calculate width of text and resize box if necessary
	const float w = itemFontScale * font->GetSize() * font->GetTextWidth(name) * screensize[0] + 2 * itemSpacing;
	if (w > (size[0]))
	{
		//box.x1 = 0.5f - 0.5f * w;
		//box.x2 = 0.5f + 0.5f * w;
	}
}

bool List::MousePress(int x, int y, int button)
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
			ScrollDownOne();
			break;
			
		case SDL_BUTTON_WHEELUP:
			ScrollUpOne();
			break;
	}
	return false;
}

int List::NumDisplay()
{
	return std::max(1.0f, (size[1] - 2.0f * borderSpacing) / (itemHeight + itemSpacing));
}

void List::ScrollUpOne()
{
	if(topIndex > 0)
		--topIndex;
}

void List::ScrollDownOne()
{
	if(topIndex + NumDisplay() < filteredItems->size())
		++topIndex;
}

void List::MouseMove(int x, int y, int dx,int dy, int button)
{
	mx = PixelToGlX(x);
	my = PixelToGlY(y);
}

void List::MouseRelease(int x, int y, int button)
{
	activeMousePress = false;
}

float List::ScaleFactor() {
	return (float)std::max(1, globalRendering->winSizeY) * (size[1] - 2.0f * borderSpacing);
}

void List::UpdateTopIndex()
{
	itemHeight = 18.0f / ScaleFactor();
	const int numDisplay = NumDisplay();
	if(topIndex + numDisplay > filteredItems->size())
		topIndex = std::max(0, (int)filteredItems->size() - numDisplay);
}

bool List::MouseUpdate(int x, int y)
{
	int nCurIndex = 0; // The item we're on
	int nDrawOffset = 0; // The offset to the first draw item

	GuiElement b;
	b.SetPos(pos[0] + borderSpacing, pos[1] + size[1] - borderSpacing - itemHeight);
	b.SetSize(size[0] - 2.0f * borderSpacing, itemHeight);
	
	// Get list started up here
	std::vector<std::string>::iterator ii = filteredItems->begin();
	UpdateTopIndex();

	while (nCurIndex < topIndex) { ii++; nCurIndex++; }

	const int numDisplay = NumDisplay();

	for (/*ii = items.begin()*/; ii != filteredItems->end() && nDrawOffset < numDisplay; ii++)
	{
		if (b.MouseOver(mx, my))
		{
			if (nCurIndex == place && clickedTime + 250 > SDL_GetTicks())
			{
				FinishSelection();
			}
			clickedTime = SDL_GetTicks();
			place = nCurIndex;
			return true;
		}

		// Up our index's
		nCurIndex++; nDrawOffset++;
		b.Move(0.0,  - (itemHeight + itemSpacing));
	}
	return false;
}

void List::DrawSelf()
{
	const float opacity = Opacity();
	font->Begin();
	float hf = font->GetSize() / ScaleFactor();

	font->SetTextColor(1,1,0.4f,opacity);

	int nCurIndex = 0; // The item we're on
	int nDrawOffset = 0; // The offset to the first draw item

	GuiElement b;
	b.SetPos(pos[0] + borderSpacing, pos[1] + size[1] - borderSpacing - itemHeight);
	b.SetSize(size[0] - 2.0f * borderSpacing, itemHeight);

	// Get list started up here
	std::vector<std::string>::iterator ii = filteredItems->begin();
	// Skip to current selection - 3; ie: scroll
	UpdateTopIndex();

	while (nCurIndex < topIndex) { ii++; nCurIndex++; }

	const int numDisplay = NumDisplay();

	font->SetTextColor(1.0f, 1.0f, 1.0f, opacity); //default
	font->SetOutlineColor(0.0f, 0.0f, 0.0f, opacity);
	glLineWidth(1.0f);

	for (/*ii = items.begin()*/; ii != filteredItems->end() && nDrawOffset < numDisplay; ii++)
	{
		glColor4f(1,1,1,opacity/4.f);
		b.DrawBox(GL_LINE_LOOP);

		if (nCurIndex == place) {
			glBlendFunc(GL_ONE, GL_ONE); // additive blending
			glColor4f(0.2f,0,0,opacity);
			b.DrawBox(GL_QUADS);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(1,0,0,opacity/2.f);
			glLineWidth(1.49f);
			b.DrawBox(GL_LINE_LOOP);
			glLineWidth(1.0f);
		} else if (b.MouseOver(mx, my)) {
			glBlendFunc(GL_ONE, GL_ONE); // additive blending
			glColor4f(0,0,0.2f,opacity);
			b.DrawBox(GL_QUADS);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(1,1,1,opacity/2.f);
			glLineWidth(1.49f);
			b.DrawBox(GL_LINE_LOOP);
			glLineWidth(1.0f);
		}

		font->glPrint(pos[0]+borderSpacing + 0.002f, b.GetMidY() - hf * 0.15f, itemFontScale, FONT_BASELINE | FONT_SHADOW | FONT_SCALE | FONT_NORM, *ii);

		// Up our index's
		nCurIndex++; nDrawOffset++;
		b.Move(0.0,  - (itemHeight + itemSpacing));
	}
	/**************
	* End insert *
	**************/

	font->End();
}

bool List::HandleEventSelf(const SDL_Event& ev)
{
	switch (ev.type) {
		case SDL_MOUSEBUTTONDOWN: {
			if (gui->MouseOverElement(GetRoot(), ev.motion.x, ev.motion.y))
			{
				if(!hasFocus) {
					hasFocus = true;
					MouseMove(ev.motion.x, ev.motion.y, ev.motion.xrel, ev.motion.yrel, ev.motion.state);
				}
			}
			else {
				hasFocus = false;
			}
			if(MouseOver(ev.button.x, ev.button.y)) {
				if(hasFocus) {
					MousePress(ev.button.x, ev.button.y, ev.button.button);
					return true;
				}
			}
			break;
		}
		case SDL_MOUSEBUTTONUP: {
			if (!hasFocus)
				break;
			if (MouseOver(ev.button.x, ev.button.y))
			{
				MouseRelease(ev.button.x, ev.button.y, ev.button.button);
				return true;
			}
			break;
		}
		case SDL_MOUSEMOTION: {
			if (!hasFocus)
				break;
			if (MouseOver(ev.button.x, ev.button.y))
			{
				MouseMove(ev.motion.x, ev.motion.y, ev.motion.xrel, ev.motion.yrel, ev.motion.state);
				return true;
			}
			break;
		}
		case SDL_KEYDOWN: {
			if (!hasFocus)
				break;
			if(ev.key.keysym.sym == SDLK_ESCAPE)
			{
				hasFocus = false;
				break;
			}
			return KeyPressed(ev.key.keysym.sym, false);
		}
	}
	return false;
}

void List::UpOne()
{
	place--;
	if(place<0)
		place=0;
}

void List::DownOne()
{
	place++;
	if(place>=(int)filteredItems->size())
		place=filteredItems->size()-1;
	if(place<0)
		place=0;
}

void List::UpPage()
{
	place -= NumDisplay();
	if(place<0)
		place=0;
}

void List::DownPage()
{
	place += NumDisplay();
	if(place>=(int)filteredItems->size())
		place=filteredItems->size()-1;
	if(place<0)
		place=0;
}

std::string List::GetCurrentItem() const
{
	if (!filteredItems->empty()) {
		return ((*filteredItems)[place]);
	}
	return "";
}

bool List::SetCurrentItem(const std::string& newCurrent)
{
	for (unsigned i = 0; i < items.size(); ++i)
	{
		if (newCurrent == items[i])
		{
			place = i;
			CenterSelected();
			return true;
		}
	}
	return false;
}

void List::CenterSelected()
{
	topIndex = std::max(0, place - NumDisplay()/2);
}

bool List::KeyPressed(unsigned short k, bool isRepeat)
{
	if (k == SDLK_ESCAPE) {
		if (cancelPlace >= 0) {
			place = cancelPlace;
			return true;
		}
	} else if (k == SDLK_UP) {
		UpOne();
		CenterSelected();
		return true;
	} else if (k == SDLK_DOWN) {
		DownOne();
		CenterSelected();
		return true;
	} else if (k == SDLK_PAGEUP) {
		UpPage();
		CenterSelected();
		return true;
	} else if (k == SDLK_PAGEDOWN) {
		DownPage();
		CenterSelected();
		return true;
	} else if (k == SDLK_BACKSPACE) {
		query = query.substr(0, query.length() - 1);
		return Filter(true);
	} else if (k == SDLK_RETURN) {
		FinishSelection();
		return true;
	} else if ((k & ~0xFF) != 0) {
		// This prevents isalnum from asserting on msvc debug crt
		// We don't actually need to process the key tho;)
	} else if (isalnum(k)) {
		query += tolower(k);
		return Filter(false);
	}
	return false;
}

bool List::Filter(bool reset)
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
		if(place >= (int)filteredItems->size())
			place = filteredItems->size() - 1;
		if(place < 0)
			place = 0;
		topIndex = 0;
		return true;
	}
}

}
