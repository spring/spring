#ifndef TEXTELEMENT_H
#define TEXTELEMENT_H

#include <string>

#include "GuiElement.h"

class TextElement : public GuiElement
{
public:
	TextElement(const std::string& text, GuiElement* parent = NULL);

private:
	virtual void DrawSelf();

	std::string text;
};

#endif // TEXTELEMENT_H
