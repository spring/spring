#ifndef BUTTON_H
#define BUTTON_H

#include <string>
#include <boost/signals.hpp>

#include "GuiElement.h"

namespace agui
{

class Button : public GuiElement
{
	typedef boost::signal<void (void)> SignalType;
public:
	Button(const std::string& label = "", GuiElement* parent = NULL);
	
	void Label(const std::string& label);
	
	boost::signals::connection ClickHandler(SignalType::slot_function_type);

private:
	virtual void DrawSelf();
	virtual bool HandleEventSelf(const SDL_Event& ev);

	bool hovered;
	bool clicked;

	std::string label;

	SignalType sig;
};

}

#endif