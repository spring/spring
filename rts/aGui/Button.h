#ifndef BUTTON_H
#define BUTTON_H

#include <string>
#include <boost/function.hpp>

#include "GuiElement.h"

class Button : public GuiElement
{
	typedef boost::function<void(void)> CallbackType;
public:
	Button(const std::string& label = "", GuiElement* parent = NULL);
	
	void Label(const std::string& label);
	
	void SetCallback(CallbackType cb)
	{
		click = cb;
	};

private:
	virtual void DrawSelf();
	virtual bool HandleEventSelf(const SDL_Event& ev);
	
	bool hovered;
	bool clicked;

	std::string label;
	
	CallbackType click;
};

#endif