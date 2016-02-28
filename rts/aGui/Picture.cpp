/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Picture.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Log/ILog.h"

namespace agui
{

Picture::Picture(GuiElement* parent)
	: GuiElement(parent)
	, texture(0)
{
}

Picture::~Picture()
{
	if (texture) {
		glDeleteTextures(1, &texture);
	}
}

void Picture::Load(const std::string& _file)
{
	file = _file;
	
	CBitmap bmp;
	if (bmp.Load(file)) {
		texture = bmp.CreateTexture();
	} else {
		LOG_L(L_WARNING, "Failed to load: %s", file.c_str());
		texture = 0;
	}
}

void Picture::DrawSelf()
{
	if (texture) {
		glColor3f(1.0f, 1.0f, 1.0f);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(pos[0],           pos[1]);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(pos[0],           pos[1] + size[1]);
			glTexCoord2f(1.0f, 0.0f); glVertex2f(pos[0] + size[0], pos[1] + size[1]);
			glTexCoord2f(1.0f, 1.0f); glVertex2f(pos[0] + size[0], pos[1]);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}
}

} // namespace agui
