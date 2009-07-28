#ifndef WINDOW_H
#define WINDOW_H

#include <string>

#include "GuiElement.h"

class Window : public GuiElement
{
public:
	Window(const std::string& title = "", GuiElement* parent = NULL);
	
private:
	virtual void DrawSelf();
	virtual bool HandleEventSelf(const SDL_Event& ev);

	bool dragging;
	std::string title;
	float titleHeight;
};

#endif
