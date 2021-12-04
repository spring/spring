/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Picture.h"

#include "Gui.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Log/ILog.h"

namespace agui
{

Picture::Picture(GuiElement* parent)
	: GuiElement(parent)
	, texture(0)
{
	GuiElement::GeometryChangeSelf();
}

Picture::~Picture()
{
	if (texture == 0)
		return;

	glDeleteTextures(1, &texture);
}

void Picture::Load(const std::string& _file)
{
	CBitmap bmp;

	if (bmp.Load(file = _file)) {
		texture = bmp.CreateTexture();
		return;
	}

	LOG_L(L_WARNING, "[aGui::Picture] failed to load \"%s\"", file.c_str());
}

void Picture::DrawSelf()
{
	if (texture == 0)
		return;

	gui->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	gui->SetDrawMode(Gui::DrawMode::TEXTURE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	DrawBox();

	glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace agui
