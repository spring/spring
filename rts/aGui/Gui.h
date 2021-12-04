/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GUI_H
#define GUI_H

#include <list>
#include <slimsig/connection.h>
#include "System/Input/InputHandler.h"

union SDL_Event;

namespace agui
{

class GuiElement;

class Gui
{
public:
	Gui();
	virtual ~Gui();

	void Clean();
	void Draw();
	void AddElement(GuiElement*, bool asBackground = false);
	/// deletes the element on the next draw
	void RmElement(GuiElement*);

	void UpdateScreenGeometry(int screenx, int screeny, int screenOffsetX, int screenOffsetY);

	bool MouseOverElement(const GuiElement*, int x, int y) const;

private:
	bool HandleEvent(const SDL_Event& ev);
	InputHandler::SignalType::connection_type inputCon;

	struct GuiItem
	{
		GuiItem(GuiElement* el, bool back) : element(el), asBackground(back) {};
		GuiElement* element;
		bool asBackground;
	};
	typedef std::list<GuiItem> ElList;
	ElList elements;
	ElList toBeRemoved;
	ElList toBeAdded;
};

extern Gui* gui;
void InitGui();
void FreeGui();

}

#endif
