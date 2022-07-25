/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TextElement.h"

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

#ifdef HEADLESS
void TextElement::DrawSelf() {}
#else
void TextElement::DrawSelf()
{
	const float opacity = Opacity();
	font->Begin();
	font->SetTextColor(1.0f, 1.0f, 1.0f, opacity);
	font->SetOutlineColor(0.0f, 0.0f, 0.0f, opacity);
	std::string mytext = text;

	font->WrapInPlace(mytext, font->GetSize(), GlToPixelX(size[0]), GlToPixelY(size[1]));
	font->glPrint(pos[0]+size[0]/2, pos[1]+size[1]/2, 1.f, FONT_CENTER | FONT_VCENTER | FONT_SHADOW | FONT_SCALE | FONT_NORM, mytext);
	font->End();
}
#endif

}
