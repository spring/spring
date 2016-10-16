/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BUTTON_H
#define BUTTON_H

#include <string>
#include <slimsig/slimsig.h>

#include "GuiElement.h"

namespace agui
{

class Button : public GuiElement
{
public:
	Button(const std::string& label = "", GuiElement* parent = NULL);

	void Label(const std::string& label);

	slimsig::signal<void (void)> Clicked;

private:
	virtual void DrawSelf();
	virtual bool HandleEventSelf(const SDL_Event& ev);

	bool hovered;
	bool clicked;

	std::string label;
};

}

#endif
