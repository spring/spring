/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Picture.h"

#include "Rendering/Textures/Bitmap.h"
#include "LogOutput.h"

namespace agui
{

Picture::Picture(GuiElement* parent) : GuiElement(parent)
{
}

Picture::~Picture()
{
	if (texture)
		glDeleteTextures(1, &texture);
}

void Picture::Load(const std::string& _file)
{
	file = _file;
	
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

void Picture::DrawSelf()
{
	if (texture)
	{
		glColor3f(1,1,1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture);
		glBegin(GL_QUADS);
			glTexCoord2f(0,1);glVertex2f(pos[0],pos[1]);
			glTexCoord2f(0,0);glVertex2f(pos[0],pos[1]+size[1]);
			glTexCoord2f(1,0);glVertex2f(pos[0]+size[0],pos[1]+size[1]);
			glTexCoord2f(1,1);glVertex2f(pos[0]+size[0],pos[1]);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}
}

}
