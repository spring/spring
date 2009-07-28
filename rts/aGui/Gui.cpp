#include "Gui.h"

#include <boost/bind.hpp>
#include <SDL_events.h>

#include "Rendering/GL/myGL.h"
#include "System/InputHandler.h"
#include "GuiElement.h"
#include "LogOutput.h"

namespace agui
{

Gui::Gui()
{
	inputCon = input.AddHandler(boost::bind(&Gui::HandleEvent, this, _1));
}

void Gui::Draw()
{
	for (ElList::iterator it = toBeRemoved.begin(); it != toBeRemoved.end(); ++it)
	{
		for (ElList::iterator elIt = elements.begin(); elIt != elements.end(); ++elIt)
		{
			if (*elIt == *it)
			{
				elements.erase(elIt);
				delete (*it);
				break;
			}
		}
	}
	toBeRemoved.clear();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glLoadIdentity();
	for (ElList::reverse_iterator it = elements.rbegin(); it != elements.rend(); ++it)
		(*it)->Draw();
}

void Gui::AddElement(GuiElement* elem, bool front)
{
	for (ElList::const_iterator it = elements.begin(); it != elements.end(); ++it)
	{
		if (*it == elem)
		{
			LogObject() << "Gui::AddElement: skipping duplicated object";
			return;
		}
	}
	if (front)
		elements.push_front(elem);
	else
		elements.push_back(elem);
}

void Gui::RmElement(GuiElement* elem)
{
	// has to be delayed, otherwise deleting a button during a callback would segfault
	for (ElList::iterator it = elements.begin(); it != elements.end(); ++it)
	{
		if (*it == elem)
		{
			toBeRemoved.push_back(elem);
			return;
		}
	}
}

void Gui::UpdateScreenGeometry(int screenx, int screeny)
{
	GuiElement::UpdateDisplayGeo(screenx, screeny);
}

bool Gui::MouseOverElement(const GuiElement* elem, int x, int y) const
{
	for (ElList::const_iterator it = elements.begin(); it != elements.end(); ++it)
	{
		if ((*it)->MouseOver(x, y))
		{
			if ((*it) == elem)
				return true;
			else
				return false;
		}
	}
	return false;
}

bool Gui::HandleEvent(const SDL_Event& ev)
{
	bool mouseEvent = false;
	if  (ev.type == SDL_MOUSEMOTION || ev.type == SDL_MOUSEBUTTONDOWN || ev.type == SDL_MOUSEBUTTONUP)
		mouseEvent = true;
	for (ElList::iterator it = elements.begin(); it != elements.end(); ++it)
	{
		(*it)->HandleEvent(ev);
	}
	return false;
}

Gui* gui = NULL;
void InitGui()
{
	if (!gui)
		gui = new Gui();
};

}
