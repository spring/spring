#include "TextElement.h"

#include "Rendering/glFont.h"

TextElement::TextElement(const std::string& _text, GuiElement* parent) : GuiElement(parent), text(_text)
{
}

void TextElement::DrawSelf()
{
	font->SetTextColor(); //default
	font->SetOutlineColor(0.0f, 0.0f, 0.0f, 1.0f);
	font->glPrint(pos[0]+size[0]/2, pos[1]+size[1]/2, 1.0, FONT_CENTER | FONT_VCENTER | FONT_SHADOW | FONT_SCALE | FONT_NORM, text);
}
