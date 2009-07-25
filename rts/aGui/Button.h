#ifndef BUTTON_H
#define BUTTON_H

#include <string>

#include "GuiElement.h"

class Button : public GuiElement
{
public:
	Button(const std::string& label = "", GuiElement* parent = NULL);
	
	void Label(const std::string& label);

private:
	virtual void DrawSelf();
	virtual bool HandleEventSelf(const SDL_Event& ev);
	
	std::string label;
};

#endif