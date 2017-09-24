/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LINE_EDIT_H
#define LINE_EDIT_H

#include <string>
#include <slimsig/slimsig.h>

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

	slimsig::signal<void (void)> DefaultAction;

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
