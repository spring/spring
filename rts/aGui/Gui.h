#ifndef GUI_H
#define GUI_H

#include <list>
#include <boost/signals/connection.hpp>

class GuiElement;
union SDL_Event;

class Gui
{
public:
	Gui();
	
	void Draw();
	void AddElement(GuiElement*);
	void RmElement(GuiElement*);
	
	void UpdateScreenGeometry(int screenx, int screeny);
	
private:
	bool HandleEvent(const SDL_Event& ev);
	boost::signals::scoped_connection inputCon;
	
	typedef std::list<GuiElement*> ElList;
	ElList elements;
};

extern Gui* gui;
void InitGui();

#endif