#include "Gui.h"

#include <boost/bind.hpp>
#include <SDL_events.h>

#include "Rendering/GL/myGL.h"
#include "System/InputHandler.h"
#include "GuiElement.h"

Gui::Gui()
{
	inputCon = input.AddHandler(boost::bind(&Gui::HandleEvent, this, _1));
}

void Gui::Draw()
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glLoadIdentity();
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
