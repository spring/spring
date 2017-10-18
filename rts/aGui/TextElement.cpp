/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TextElement.h"

#include "Gui.h"
#include "Rendering/Fonts/glFont.h"

namespace agui
{

void TextElement::WrapText(std::string& str)
{
	font->WrapInPlace(str, font->GetSize(), GlToPixelX(size[0]), GlToPixelY(size[1]));
}

void TextElement::DrawSelf()
{
	const float opacity = Opacity();

	font->SetTextColor(1.0f, 1.0f, 1.0f, opacity);
	font->SetOutlineColor(0.0f, 0.0f, 0.0f, opacity);

	gui->SetDrawMode(Gui::DrawMode::FONT);
	font->SetTextDepth(depth);
	font->glPrint(pos[0] + size[0] * 0.5f, pos[1] + size[1] * 0.5f, 1.0f, FONT_CENTER | FONT_VCENTER | FONT_SHADOW | FONT_SCALE | FONT_NORM | FONT_BUFFERED, wtext);
	font->DrawBufferedGL4(gui->GetShader());
}

}
