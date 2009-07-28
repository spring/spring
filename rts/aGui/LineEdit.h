#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <string>

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
private:
	virtual void DrawSelf();
	virtual bool HandleEventSelf(const SDL_Event& ev);

	bool hasFocus;
	std::string content;
	unsigned cursorPos;
};

}
#endif
