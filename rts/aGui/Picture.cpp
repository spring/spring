/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Picture.h"

#include "Gui.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/VertexArrayTypes.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Log/ILog.h"

namespace agui
{

Picture::Picture(GuiElement* parent)
	: GuiElement(parent)
	, texture(0)
{
	VA_TYPE_2dT vaElems[4];

	vaElems[0].x = pos[0]          ; vaElems[0].y = pos[1]          ;
	vaElems[1].x = pos[0]          ; vaElems[1].y = pos[1] + size[1];
	vaElems[2].x = pos[0] + size[0]; vaElems[2].y = pos[1] + size[1];
	vaElems[3].x = pos[0] + size[0]; vaElems[3].y = pos[1]          ;

	vaElems[0].s = 0.0f; vaElems[0].t = 1.0f;
	vaElems[1].s = 0.0f; vaElems[1].t = 0.0f;
	vaElems[2].s = 1.0f; vaElems[2].t = 0.0f;
	vaElems[3].s = 1.0f; vaElems[3].t = 1.0f;

	VBO* vbo = GetVBO(0);

	vbo->Bind();
	vbo->New(sizeof(vaElems), GL_DYNAMIC_DRAW, vaElems);
	vbo->Unbind();
}

Picture::~Picture()
{
	if (texture == 0)
		return;

	glDeleteTextures(1, &texture);
	// GuiElement destructor cleans up VBO
}

void Picture::Load(const std::string& _file)
{
	CBitmap bmp;

	if (bmp.Load(file = _file)) {
		bmp.ReverseYAxis();
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

	DrawBox(GL_QUADS);

	glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace agui
