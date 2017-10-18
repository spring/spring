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
	TextElement(const std::string& str, GuiElement* parent = nullptr) : GuiElement(parent) { SetText(str); }

	void SetText(const std::string& str) { WrapText(wtext = text = str); }

private:
	void DrawSelf() override;
	void GeometryChangeSelf() override { WrapText(wtext = text); }
	void WrapText(std::string& str);

	std::string text;
	std::string wtext;
};
}
#endif // TEXTELEMENT_H
