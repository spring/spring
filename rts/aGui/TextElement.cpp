/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TextElement.h"

#include "Gui.h"
#include "Rendering/Fonts/glFont.h"

namespace agui
{

TextElement::TextElement(const std::string& _text, GuiElement* parent) : GuiElement(parent), text(_text)
{
}

void TextElement::SetText(const std::string& str)
{
	text = str;
}

void TextElement::DrawSelf()
{
	const float opacity = Opacity();

	font->Begin();
	font->SetTextColor(1.0f, 1.0f, 1.0f, opacity);
	font->SetOutlineColor(0.0f, 0.0f, 0.0f, opacity);
	std::string mytext = text;

	CglFont* f = font;
	f->WrapInPlace(mytext, font->GetSize(), GlToPixelX(size[0]), GlToPixelY(size[1]));

	font->glPrint(pos[0] + size[0] * 0.5f, pos[1] + size[1] * 0.5f, 1.0f, FONT_CENTER | FONT_VCENTER | FONT_SHADOW | FONT_SCALE | FONT_NORM, mytext);
	gui->SetDrawMode(Gui::DrawMode::MASK);
	font->End();
}

}
