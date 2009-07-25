#include "Gui.h"

#include <boost/bind.hpp>
#include <SDL_events.h>

#include "System/InputHandler.h"
#include "GuiElement.h"

Gui::Gui()
{
	inputCon = input.AddHandler(boost::bind(&Gui::HandleEvent, this, _1));
}

void Gui::Draw()
{
	for (ElList::iterator it = elements.begin(); it != elements.end(); ++it)
		(*it)->Draw();
}

void Gui::AddElement(GuiElement* elem)
{
	for (ElList::const_iterator it = elements.begin(); it != elements.end(); ++it)
	{
		if (*it == elem)
			return;
	}
	elements.push_back(elem);
}

void Gui::RmElement(GuiElement* elem)
{
	for (ElList::iterator it = elements.begin(); it != elements.end(); ++it)
	{
		if (*it == elem)
		{
			elements.erase(it);
			return;
		}
	}
}

void Gui::UpdateScreenGeometry(int screenx, int screeny)
{
	GuiElement::UpdateDisplayGeo(screenx, screeny);
}

bool Gui::HandleEvent(const SDL_Event& ev)
{
	for (ElList::iterator it = elements.begin(); it != elements.end(); ++it)
		(*it)->HandleEvent(ev);
	return false;
}

Gui* gui = NULL;
void InitGui()
{
	if (!gui)
		gui = new Gui();
};
