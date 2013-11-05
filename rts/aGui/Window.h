/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WINDOW_H
#define WINDOW_H

#include <string>
#include <boost/signals2/signal.hpp>

#include "GuiElement.h"

namespace agui {

class Window : public GuiElement
{
public:
	Window(const std::string& title = "", GuiElement* parent = NULL);
	
	virtual void AddChild(GuiElement* elem);
	boost::signals2::signal<void (void)> WantClose;

protected:
	std::string title;

private:
	virtual void DrawSelf();
	virtual bool HandleEventSelf(const SDL_Event& ev);

	virtual float Opacity() const;
	bool dragging;
	float dragPos[2];
	float titleHeight;
};

}

#endif
