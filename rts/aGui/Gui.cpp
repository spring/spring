/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Input/InputHandler.h"
#include "Gui.h"

#include <boost/bind.hpp>
#include <SDL_events.h>

#include "GuiElement.h"
#include "Rendering/GL/myGL.h"
#include "System/Log/ILog.h"


namespace agui
{

Gui::Gui():
	inputConBlock(inputCon)
{
	inputCon = input.AddHandler(boost::bind(&Gui::HandleEvent, this, _1));
}

void Gui::Draw()
{
	Clean();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 1, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	for (ElList::reverse_iterator it = elements.rbegin(); it != elements.rend(); ++it) {
		(*it).element->Draw();
	}
}

void Gui::Clean() {
	for (ElList::iterator it = toBeAdded.begin(); it != toBeAdded.end(); ++it)
	{
		bool duplicate = false;
		for (ElList::iterator elIt = elements.begin(); elIt != elements.end(); ++elIt)
		{
			if (it->element == elIt->element)
			{
				LOG_L(L_DEBUG, "Gui::AddElement: skipping duplicated object");
				duplicate = true;
				break;
			}
		}
		if (!duplicate)
		{
			if (it->asBackground)
				elements.push_back(*it);
			else
				elements.push_front(*it);
		}
	}
	toBeAdded.clear();

	for (ElList::iterator it = toBeRemoved.begin(); it != toBeRemoved.end(); ++it)
	{
		for (ElList::iterator elIt = elements.begin(); elIt != elements.end(); ++elIt)
		{
			if (it->element == elIt->element)
			{
				delete (elIt->element);
				elements.erase(elIt);
				break;
			}
		}
	}
	toBeRemoved.clear();
}

Gui::~Gui() {
	Clean();
}

void Gui::AddElement(GuiElement* elem, bool asBackground)
{
	if (elements.empty()) {
		inputConBlock.unblock();
	}
	toBeAdded.push_back(GuiItem(elem,asBackground));
}

void Gui::RmElement(GuiElement* elem)
{
	// has to be delayed, otherwise deleting a button during a callback would segfault
	for (ElList::iterator it = elements.begin(); it != elements.end(); ++it) {
		if ((*it).element == elem) {
			toBeRemoved.push_back(GuiItem(elem,true));
			break;
		}
	}

	if (elements.empty()) {
		inputConBlock.block();
	}
}

void Gui::UpdateScreenGeometry(int screenx, int screeny, int screenOffsetX, int screenOffsetY)
{
	GuiElement::UpdateDisplayGeo(screenx, screeny, screenOffsetX, screenOffsetY);
}

bool Gui::MouseOverElement(const GuiElement* elem, int x, int y) const
{
	for (ElList::const_iterator it = elements.begin(); it != elements.end(); ++it)
	{
		if (it->element->MouseOver(x, y))
		{
			if (it->element == elem)
				return true;
			else
				return false;
		}
	}
	return false;
}

bool Gui::HandleEvent(const SDL_Event& ev)
{
	ElList::iterator handler = elements.end();
	for (ElList::iterator it = elements.begin(); it != elements.end(); ++it)
	{
		if (it->element->HandleEvent(ev))
		{
			handler = it;
			break;
		}
	}
	if (handler != elements.end() && !handler->asBackground)
	{
		elements.push_front(*handler);
		elements.erase(handler);
	}
	return false;
}

Gui* gui = NULL;
void InitGui()
{
	if (gui == NULL)
		gui = new Gui();
}

void FreeGui()
{
	if (gui != NULL) {
		delete gui;
		gui = NULL;
	}
}

}
