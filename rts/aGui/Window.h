#ifndef WINDOW_H
#define WINDOW_H

#include <string>
#include <boost/signal.hpp>

#include "GuiElement.h"

namespace agui {

class Window : public GuiElement
{
public:
	Window(const std::string& title = "", GuiElement* parent = NULL);
	
	virtual void AddChild(GuiElement* elem);
	boost::signal<void (void)> WantClose;

private:
	virtual void DrawSelf();
	virtual bool HandleEventSelf(const SDL_Event& ev);

	bool dragging;
	float dragPos[2];
	std::string title;
	float titleHeight;
};

}

#endif
