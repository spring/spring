#ifndef GUI_H
#define GUI_H

#include <list>
#include <boost/signals/connection.hpp>

union SDL_Event;

namespace agui
{

class GuiElement;

class Gui
{
public:
	Gui();
	
	void Draw();
	void AddElement(GuiElement*, bool front = true);
	/// deletes the element on the next draw
	void RmElement(GuiElement*);
	
	void UpdateScreenGeometry(int screenx, int screeny);

	bool MouseOverElement(const GuiElement*, int x, int y) const;

private:
	bool HandleEvent(const SDL_Event& ev);
	boost::signals::scoped_connection inputCon;
	
	typedef std::list<GuiElement*> ElList;
	ElList elements;
	ElList toBeRemoved;
	ElList toBeAdded;
};

extern Gui* gui;
void InitGui();

}

#endif