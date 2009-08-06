#include "Picture.h"

#include "Rendering/Textures/Bitmap.h"
#include "LogOutput.h"

namespace agui
{

Picture::~Picture()
{
	if (texture)
		glDeleteTextures(1, &texture);
}

void Picture::Load(const std::string& _file)
{
	file = _file;
	texture = 0;
	GeometryChangeSelf();
}

void Picture::DrawSelf()
{
	if (texture)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture);
		glBegin(GL_QUADS);
			glTexCoord2f(0,1);glVertex2f(0,0);
			glTexCoord2f(0,0);glVertex2f(0,1);
			glTexCoord2f(1,0);glVertex2f(1,1);
			glTexCoord2f(1,1);glVertex2f(1,0);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}
}

void Picture::GeometryChangeSelf()
{
	CBitmap bmp;
	if (bmp.Load(file))
	{
		texture = bmp.CreateTexture(false);
	}
	else
	{
		LogObject() << "Failed to load: " << file;
		texture = 0;
	}
}

}
