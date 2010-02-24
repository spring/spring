/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <string>
#include <boost/signal.hpp>

#include "GuiElement.h"

namespace agui
{

class LineEdit : public GuiElement
{
public:
	LineEdit(GuiElement* parent = NULL);
	
	std::string GetContent() const
	{
		return content;
	};
	void SetContent(const std::string& line, bool moveCursor = true);
	
	void SetFocus(bool focus);
	void SetCrypt(bool focus);

	boost::signal<void (void)> DefaultAction;

private:
	virtual void DrawSelf();
	virtual bool HandleEventSelf(const SDL_Event& ev);

	bool hasFocus;
	bool crypt;
	std::string content;
	unsigned cursorPos;
};

}
#endif
