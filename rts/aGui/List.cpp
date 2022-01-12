/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "List.h"

#include <SDL_mouse.h>

#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Game/GlobalUnsynced.h"
#include "Gui.h"
#include "System/StringUtil.h"

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
		clickedTime(spring_now()),
		place(0),
		activeMousePress(false),
		activeScrollbar(false),
		scrollbarGrabPos(0.0f),
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
	if (query != "") {
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
	float w = itemFontScale * font->GetSize() * font->GetTextWidth(name) * screensize[0] + 2 * itemSpacing;
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
	if(button & SDL_BUTTON_LEFT) {
		activeMousePress = false;
		activeScrollbar = false;
	}
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
	b.SetSize(size[0] - 2.0f * borderSpacing - ((scrollbar.GetSize()[0] < 0) ? 0 : (itemHeight + itemSpacing)), itemHeight);

	// Get list started up here
	std::vector<std::string>::iterator ii = filteredItems->begin();
	UpdateTopIndex();

	while (nCurIndex < topIndex) { ++ii; ++nCurIndex; }

	const int numDisplay = NumDisplay();

	float sbX = b.GetPos()[0];
	float sbY1 = b.GetPos()[1] + (itemHeight + itemSpacing);

	for (/*ii = items.begin()*/; ii != filteredItems->end() && nDrawOffset < numDisplay; ++ii)
	{
		if (b.MouseOver(mx, my))
		{
			if (nCurIndex == place && (clickedTime + spring_msecs(250)) > spring_now())
			{
				FinishSelection.emit();
			}
			clickedTime = spring_now();
			place = nCurIndex;
			return true;
		}

		// Up our index's
		nCurIndex++; nDrawOffset++;
		b.Move(0.0,  - (itemHeight + itemSpacing));
	}

	if(nDrawOffset < filteredItems->size()) {
		if (scrollbar.MouseOver(mx, my)) {
			activeScrollbar = true;
			scrollbarGrabPos = my - scrollbar.GetPos()[1];
		}
		else {
			float sbY2 = b.GetPos()[1] + (itemHeight + itemSpacing);
			float sbHeight = sbY1 - sbY2;

			b.SetPos(sbX + (size[0] - 2.0f * borderSpacing) - (itemHeight + itemSpacing), sbY2);
			b.SetSize(itemHeight + itemSpacing, sbHeight);
			if (b.MouseOver(mx, my)) {
				if(my > scrollbar.GetPos()[1] + scrollbar.GetSize()[1])
					topIndex = std::max(0, std::min(topIndex - NumDisplay(), (int)filteredItems->size() - NumDisplay()));
				else if(my < scrollbar.GetPos()[1])
					topIndex = std::max(0, std::min(topIndex + NumDisplay(), (int)filteredItems->size() - NumDisplay()));
			}
		}
	}

	return false;
}

void List::DrawSelf()
{
	gui->SetDrawMode(Gui::DrawMode::COLOR);

	const float opacity = Opacity();
	const float hf = font->GetSize() / ScaleFactor();

	font->SetTextColor(1.0f, 1.0f, 0.4f, opacity);

	int nCurIndex = 0; // The item we're on
	int nDrawOffset = 0; // The offset to the first draw item

	GuiElement b;
	b.SetPos(pos[0] + borderSpacing, pos[1] + size[1] - borderSpacing - itemHeight);
	b.SetSize(size[0] - 2.0f * borderSpacing - ((scrollbar.GetSize()[0] < 0) ? 0 : (itemHeight + itemSpacing)), itemHeight);

	// Get list started up here
	std::vector<std::string>::iterator ii = filteredItems->begin();
	// Skip to current selection - 3; ie: scroll
	UpdateTopIndex();

	ii += std::max(0, topIndex - nCurIndex);
	nCurIndex = topIndex;

	const int numDisplay = NumDisplay();

	font->SetTextColor(1.0f, 1.0f, 1.0f, opacity); //default
	font->SetOutlineColor(0.0f, 0.0f, 0.0f, opacity);
	font->SetTextDepth(depth);

	float sbX = b.GetPos()[0];
	float sbY1 = b.GetPos()[1] + (itemHeight + itemSpacing);

	for (/*ii = items.begin()*/; ii != filteredItems->end() && nDrawOffset < numDisplay; ++ii) {
		gui->SetColor(1.0f, 1.0f, 1.0f, opacity * 0.25f);
		b.DrawOutline();

		if (nCurIndex == place) {
			glAttribStatePtr->BlendFunc(GL_ONE, GL_ONE); // additive blending
			gui->SetColor(0.2f, 0.0f, 0.0f, opacity);
			b.DrawBox();
			glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			gui->SetColor(1.0f, 0.0f, 0.0f, opacity / 2.0f);
			b.DrawOutline();
		} else if (b.MouseOver(mx, my)) {
			glAttribStatePtr->BlendFunc(GL_ONE, GL_ONE); // additive blending
			gui->SetColor(0.0f, 0.0f, 0.2f, opacity);
			b.DrawBox();
			glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			gui->SetColor(1.0f, 1.0f, 1.0f, opacity / 2.0f);
			b.DrawOutline();
		}

		font->glPrint(pos[0] + borderSpacing + 0.002f, b.GetMidY() - hf * 0.15f, itemFontScale, FONT_BASELINE | FONT_SHADOW | FONT_SCALE | FONT_NORM | FONT_BUFFERED, *ii);

		nCurIndex++;
		nDrawOffset++;
		b.Move(0.0,  - (itemHeight + itemSpacing));
	}

	gui->SetDrawMode(Gui::DrawMode::FONT);
	font->DrawBufferedGL4(gui->GetShader());
	gui->SetDrawMode(Gui::DrawMode::COLOR);

	// scrollbar
	if (nDrawOffset < filteredItems->size()) {
		const float sbY2 = b.GetPos()[1] + (itemHeight + itemSpacing);
		const float sbHeight = sbY1 - sbY2;
		const float sbSize = (nDrawOffset * 1.0f / filteredItems->size()) * sbHeight;

		if (activeScrollbar) {
			const float dify = my - std::min(scrollbarGrabPos, sbSize);
			const float rely = ((sbY1 - sbSize) - dify) / sbHeight;

			const int minIndex = (filteredItems->size() * rely) + 0.5f;
			const int endIndex = filteredItems->size() - numDisplay;

			topIndex = std::min(minIndex, endIndex);
			topIndex = std::max(0, topIndex);
		}

		scrollbar.SetPos(
			sbX + (size[0] - 2.0f * borderSpacing) - (itemHeight + itemSpacing),
			sbY1 - sbSize - ((float)topIndex / (float)filteredItems->size()) * sbHeight
		);
		scrollbar.SetSize((itemHeight + itemSpacing) , sbSize);

		b.SetPos(scrollbar.GetPos()[0], sbY2);
		b.SetSize(itemHeight + itemSpacing, sbHeight);

		gui->SetColor(1.0f, 1.0f, 1.0f, opacity / 4.0f);
		b.DrawOutline();

		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		gui->SetColor(0.8f, 0.8f, 0.8f, opacity);
		scrollbar.DrawBox();
		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		gui->SetColor(1.0f, 1.0f, 1.0f, opacity / 2.0f);
		scrollbar.DrawOutline();
		return;
	}

	scrollbar.SetSize(-1, -1);
}

bool List::HandleEventSelf(const SDL_Event& ev)
{
	switch (ev.type) {
		case SDL_MOUSEBUTTONDOWN: {
			hasFocus = gui->MouseOverElement(GetRoot(), ev.button.x, ev.button.y);
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
			if (MouseOver(ev.button.x, ev.button.y) || activeScrollbar)
			{
				MouseRelease(ev.button.x, ev.button.y, ev.button.button);
				return true;
			}
			break;
		}
		case SDL_MOUSEWHEEL: {
			int mousex, mousey;
			SDL_GetMouseState(&mousex, &mousey);
			if(hasFocus && MouseOver(mousex, mousey)) {
				if (ev.wheel.y > 0) {
					ScrollUpOne();
				} else {
					ScrollDownOne();
				}
				return true;
			}
		} break;
		case SDL_MOUSEMOTION: {
			if (!hasFocus)
				break;
			if (MouseOver(ev.motion.x, ev.motion.y) || activeScrollbar)
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
	place = std::max(0, place - 1);
}

void List::DownOne()
{
	place = std::max(0, std::min(place + 1, int(filteredItems->size()) - 1));
}

void List::UpPage()
{
	place = std::max(0, place - NumDisplay());
}

void List::DownPage()
{
	place = std::max(0, std::min(place + NumDisplay(), int(filteredItems->size()) - 1));
}



std::string List::GetCurrentItem() const
{
	if (place < filteredItems->size()) {
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

bool List::KeyPressed(int k, bool isRepeat)
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
		FinishSelection.emit();
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
	std::string current = GetCurrentItem();
	std::vector<std::string>* destination = filteredItems == &temp1 ? &temp2 : &temp1;
	destination->clear();

	if (reset)
		filteredItems = &items; // reset filter

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
		place = std::max(0, std::min(place, int(filteredItems->size()) - 1));
		topIndex = 0;
		return true;
	}
}

}
