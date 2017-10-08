/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WINDOW_H
#define WINDOW_H

#include <string>
#include <slimsig/slimsig.h>

#include "GuiElement.h"

namespace agui {

class Window : public GuiElement
{
public:
	Window(const std::string& title = "", GuiElement* parent = nullptr);

	virtual void AddChild(GuiElement* elem);
	slimsig::signal<void (void)> WantClose;

protected:
	std::string title;

private:
	void DrawSelf() override;
	void GeometryChangeSelf() override;
	bool HandleEventSelf(const SDL_Event& ev) override;

	float Opacity() const override;

	bool dragging;
	float dragPos[2];
	float titleHeight;
};

}

#endif
