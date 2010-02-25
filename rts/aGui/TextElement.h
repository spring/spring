/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TEXTELEMENT_H
#define TEXTELEMENT_H

#include <string>

#include "GuiElement.h"

namespace agui
{

class TextElement : public GuiElement
{
public:
	TextElement(const std::string& text, GuiElement* parent = NULL);
	void SetText(const std::string& str);

private:
	virtual void DrawSelf();

	std::string text;
};
}
#endif // TEXTELEMENT_H
